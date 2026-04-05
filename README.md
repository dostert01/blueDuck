# blueDuck

A CVE vulnerability scanner daemon. blueDuck monitors Git repositories, extracts
dependency information from multiple ecosystems (C++/CMake, .NET/NuGet, npm,
Rust/Cargo, Python), and cross-references them against the NVD and CVE.org
vulnerability databases to produce actionable security reports.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Directory Structure](#directory-structure)
3. [Data Model](#data-model)
4. [Plugin System](#plugin-system)
5. [CVE Matching](#cve-matching)
6. [Git Authentication](#git-authentication)
7. [API Reference](#api-reference)
8. [Frontend](#frontend)
9. [Configuration Reference](#configuration-reference)
10. [Docker — Quick Start](#docker--quick-start)
11. [Build — Development](#build--development)
12. [Build — Production](#build--production)
13. [Running](#running)
14. [Testing](#testing)
15. [Dependencies](#dependencies)

---

## Architecture Overview

```
┌───────────────────────────────────────────────────────────┐
│                       blueduck daemon                     │
│                                                           │
│  ┌──────────┐   ┌──────────────┐   ┌────────────────┐     │
│  │  Drogon  │   │ PluginLoader │   │ AnalysisService│     │
│  │HTTP/8080 │   │  (dlopen)    │   │  (git+analyze) │     │
│  └────┬─────┘   └──────┬───────┘   └───────┬────────┘     │
│       │                │                   │              │
│  ┌────▼──────────────┐ │ ┌─────────────────▼──────────┐   │
│  │  REST API         │ │ │  CVE Source Plugins (.so)  │   │
│  │  /api/projects    │ │ │  - nvd  (NVD API 2.0)      │   │
│  │  /api/reports     │ │ │  - cveorg (ZIP bulk)       │   │
│  │  /api/cve-sources │ │ └────────────────────────────┘   │
│  └───────────────────┘ │ ┌─────────────────────────────┐  │
│                        │ │  Analyzer Plugins (.so)     │  │
│  ┌─────────────────┐   │ │  - cmake_cpp  - cargo       │  │
│  │  Angular SPA    │   │ │  - npm        - python      │  │
│  │  (same port)    │   │ │  - nuget                    │  │
│  └─────────────────┘   └─┴─────────────────────────────┘  │
│                                                           │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              PostgreSQL                             │  │
│  │  projects · project_versions · dependencies         │  │
│  │  cve_records · cve_affected_products                │  │
│  │  reports · report_findings · ecosystem_cpe_mapping  │  │
│  └─────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────┘
```

The daemon is a single process:

- **Drogon** (C++ async HTTP framework) handles all HTTP traffic on one port.
  The Angular SPA is served as static files from the same port; API routes
  (`/api/*`) take priority; a catch-all handler serves `index.html` for Angular
  client-side routing.
- **Plugin loader** (`dlopen`/`dlsym`) discovers and loads `.so` files at
  startup; plugins are versioned via the `api_version` field in `PluginMeta`.
- **AnalysisService** checks out a git ref, runs every applicable analyzer
  plugin against the working tree, and stores the resulting dependency list in
  PostgreSQL.
- **ReportEngine** matches stored dependencies against CVE records already
  imported into the database, then writes a report with severity counts.
- CVE data is populated by CVE source plugins (NVD, CVE.org) and lives entirely
  in the database — the daemon itself does not query external services at
  report-generation time.

---

## Directory Structure

```
blueDuck/
├── CMakeLists.txt              root build
├── config.json                 runtime config template
├── cmake/
│   ├── CompilerOptions.cmake
│   ├── FindLibGit2.cmake
│   └── PluginHelpers.cmake     add_blueduck_plugin() function
├── interfaces/                 shared C++ plugin ABIs
│   ├── PluginMeta.h            PluginMeta struct + BLUEDUCK_PLUGIN_API_VERSION
│   ├── ICveSource.h            CVE source plugin interface
│   └── IDependencyAnalyzer.h   dependency analyzer plugin interface
├── src/
│   ├── main.cpp
│   ├── core/                   Application, Config, PluginLoader, AnalysisService
│   ├── db/                     Database (Drogon ORM pool), Migrator
│   ├── git/                    GitRepository (libgit2), CredentialProvider (AES-256-GCM)
│   ├── report/                 ReportEngine (version range matching, findings)
│   └── api/
│       ├── ApiHelpers.h        inline response builders
│       └── controllers/        Drogon HTTP controllers
├── plugins/
│   ├── common/                 CveHttpClient (libcurl), CveDbWriter (libpq)
│   ├── cve_sources/
│   │   ├── nvd/                NVD API 2.0 source
│   │   └── cveorg/             CVE.org bulk ZIP source
│   └── dependency_analyzers/
│       ├── cmake_cpp/          find_package + FetchContent_Declare
│       ├── npm/                package.json (all workspaces)
│       ├── cargo/              Cargo.toml (workspace members)
│       ├── python/             requirements.txt, pyproject.toml, setup.cfg
│       └── nuget/              *.csproj, *.fsproj, packages.config
├── db/
│   └── migrations/
│       └── 001_initial_schema.sql
├── tests/
│   ├── unit/                   GTest unit tests (no DB required)
│   └── integration/            GTest integration tests (require BD_TEST_DB_URL)
└── frontend/                   Angular 17 SPA
    ├── src/app/
    │   ├── core/               models + ProjectService
    │   └── features/
    │       ├── projects/       list, detail, new-project dialog
    │       └── reports/        report detail with sortable findings
    └── ...
```

---

## Data Model

```
projects
  id · name · git_url · description · created_at · last_synced_at

project_credentials          (one row per project)
  project_id → projects
  auth_method                none | pat | ssh_key
  pat_enc                    AES-256-GCM ciphertext (hex)
  ssh_private_key_path
  ssh_public_key_path
  ssh_passphrase_enc

project_versions             (tracked branches / tags)
  id · project_id → projects
  ref_name · ref_type        branch | tag
  commit_hash · analyzed_at

dependencies                 (extracted per version)
  id · project_version_id → project_versions
  ecosystem                  cmake | npm | cargo | pypi | nuget
  package_name · package_version

cve_records                  (imported from NVD / CVE.org)
  id · cve_id (UNIQUE)
  description · severity     CRITICAL | HIGH | MEDIUM | LOW | NONE
  cvss_v3_score · cvss_v2_score
  published_at · modified_at

cve_affected_products        (version ranges per CVE)
  id · cve_record_id → cve_records
  ecosystem · package_name · cpe_uri
  version_exact
  version_start_including · version_start_excluding
  version_end_including   · version_end_excluding

ecosystem_cpe_mapping        (bridges package names → NVD CPE vendor:product)
  ecosystem · package_name
  cpe_vendor · cpe_product
  git_url_pattern            optional URL-based fallback match

reports
  id · project_version_id → project_versions · created_at
  total_dependencies · vulnerable_count
  critical_count · high_count · medium_count · low_count

report_findings
  id · report_id → reports
  dependency_id → dependencies · cve_record_id → cve_records
  cvss_score · severity
```

Schema migrations run automatically at daemon startup from `migrations_dir`.
The `schema_migrations` table tracks applied files by filename.

---

## Plugin System

Plugins are standard shared libraries (`.so`) discovered at startup from the
`plugin_dir` directory. Each plugin exports three C symbols:

```cpp
// Returns a pointer to a static PluginMeta struct
const blueduck::PluginMeta* blueduck_plugin_meta();

// Allocates and returns a new plugin instance
void* blueduck_create_plugin();

// Frees a plugin instance
void  blueduck_destroy_plugin(void* instance);
```

`PluginMeta` carries:

| Field            | Description                              |
|------------------|------------------------------------------|
| `api_version`    | Must equal `BLUEDUCK_PLUGIN_API_VERSION` |
| `plugin_type`    | `"cve_source"` or `"dependency_analyzer"` |
| `plugin_name`    | Unique id, e.g. `"nvd"`, `"npm"`        |
| `plugin_version` | Semver string                            |

The `BLUEDUCK_REGISTER_PLUGIN` macro in `interfaces/PluginMeta.h` provides a
convenient shorthand for writing these entry points.

Plugins are loaded with `RTLD_NOW | RTLD_LOCAL` to catch missing symbols
immediately and to isolate symbol namespaces between plugins.

The `add_blueduck_plugin()` CMake function in `cmake/PluginHelpers.cmake`
sets up the correct compile flags, strips the `lib` prefix, and places the
`.so` in `build/plugins/`.

---

## CVE Matching

When a report is generated (`ReportEngine::generateReport`):

1. All `dependencies` for the version are loaded from the database.
2. For each dependency, `ecosystem_cpe_mapping` is queried to find a
   `(cpe_vendor, cpe_product)` pair — first by exact package name, then by
   `git_url_pattern` (ILIKE match).
3. `cve_affected_products` is queried joining on **either**:
   - `(ecosystem, LOWER(package_name))` — direct ecosystem match, or
   - CPE URI pattern `'%:vendor:product:%'` — for packages whose ecosystem name
     differs from their CPE identity (common for C/C++ libraries).
4. Each candidate CVE is filtered through `versionInRange`, which handles:
   - Exact matches (`version_exact`)
   - Half-open and closed ranges (`start_including/excluding`, `end_including/excluding`)
   - Pre-release ordering (`1.0-alpha < 1.0`)
   - Leading `v`/`V` stripping
   - Short-version padding (`1.0 == 1.0.0`)
5. Matched CVEs are deduplicated and inserted into `report_findings`.

The `ecosystem_cpe_mapping` table ships with seed data for common C++ CMake
packages (OpenSSL, ZLIB, CURL, Boost, libgit2, etc.) and can be extended
without recompiling.

---

## Git Authentication

Three authentication methods are supported, stored encrypted per project:

| Method    | Storage                                           |
|-----------|---------------------------------------------------|
| `none`    | No credentials (public repos)                     |
| `pat`     | Personal Access Token, AES-256-GCM encrypted      |
| `ssh_key` | Paths to private/public key files on the server   |

The encryption key is loaded from the `BLUEDUCK_SECRET_KEY` environment
variable (32-byte hex string, 64 hex chars). Generate one with:

```bash
openssl rand -hex 32
```

Encryption uses OpenSSL's EVP API (AES-256-GCM). The ciphertext format is:
`[12-byte IV][N-byte ciphertext][16-byte auth tag]`, stored as a binary string
in PostgreSQL.

Credentials are set via the REST API and never returned in responses.

---

## API Reference

All endpoints are under `/api`. Responses are JSON.

### Projects

| Method | Path                              | Description                  |
|--------|-----------------------------------|------------------------------|
| GET    | `/api/projects`                   | List all projects             |
| POST   | `/api/projects`                   | Create project                |
| GET    | `/api/projects/:id`               | Get project                  |
| DELETE | `/api/projects/:id`               | Delete project                |
| POST   | `/api/projects/:id/credentials`   | Set Git credentials           |
| POST   | `/api/projects/:id/sync`          | Clone/fetch repository (async)|

**POST /api/projects** body:
```json
{ "name": "my-service", "git_url": "https://github.com/org/repo.git", "description": "" }
```

**POST /api/projects/:id/credentials** body examples:
```json
{ "auth_method": "none" }
{ "auth_method": "pat", "pat": "ghp_..." }
{ "auth_method": "ssh_key", "ssh_private_key_path": "/home/user/.ssh/id_ed25519",
                             "ssh_public_key_path":  "/home/user/.ssh/id_ed25519.pub" }
```

### Versions

| Method | Path                                          | Description              |
|--------|-----------------------------------------------|--------------------------|
| GET    | `/api/projects/:pid/versions`                 | List versions            |
| GET    | `/api/projects/:pid/versions/:id`             | Get version              |
| DELETE | `/api/projects/:pid/versions/:id`             | Delete version           |
| POST   | `/api/projects/:pid/versions/:id/analyze`     | Run dependency analysis (async) |

Versions are created automatically by the sync process (one row per tracked
branch/tag found in the repository).

### Reports

| Method | Path                                              | Description               |
|--------|---------------------------------------------------|---------------------------|
| GET    | `/api/projects/:pid/versions/:vid/reports`        | List reports for version  |
| POST   | `/api/projects/:pid/versions/:vid/reports`        | Generate report (async)   |
| GET    | `/api/reports/:id`                                | Get report summary        |
| GET    | `/api/reports/:id/findings`                       | Get findings (sorted by CVSS desc) |

### CVE Sources

| Method | Path                          | Description               |
|--------|-------------------------------|---------------------------|
| GET    | `/api/cve-sources`            | List loaded CVE plugins   |
| POST   | `/api/cve-sources/:name/sync` | Trigger CVE sync (async)  |

---

## Frontend

The Angular 17 SPA is served by Drogon at the same port as the API.

**Development** (with hot-reload, proxied to `:8080`):
```bash
cd frontend
npm install
npm start          # listens on :4200, proxies /api to :8080
```

**Production build** (outputs to `frontend/dist/blueduck-frontend/`):
```bash
cd frontend
npm run build
```

Copy the built assets into the path set by `server.document_root` in
`config.json`. Drogon will serve them automatically and fall back to
`index.html` for any path not matching a real file (enabling Angular routing).

Key UI flows:
- **Projects list** → sync a repo, delete a project
- **Project detail** → tabs for Versions and Reports; trigger analysis or
  report generation per version
- **Report detail** → severity summary cards + sortable findings table
- **Sidebar** → list CVE sources with one-click sync buttons

---

## Configuration Reference

`config.json` (path passed via `--config` CLI argument):

```json
{
  "server": {
    "port": 8080,
    "threads": 4,
    "document_root": "/var/lib/blueduck/www"
  },
  "database": {
    "host": "localhost",
    "port": 5432,
    "name": "blueduck",
    "user": "blueduck",
    "password": "changeme",
    "pool_size": 10
  },
  "plugin_dir":     "/usr/lib/blueduck/plugins",
  "repos_dir":      "/var/lib/blueduck/repos",
  "migrations_dir": "db/migrations",
  "plugin_config": {
    "nvd_api_key": "",
    "cache_dir":   "/tmp/blueduck"
  }
}
```

| Key                       | Description                                              |
|---------------------------|----------------------------------------------------------|
| `server.threads`          | I/O threads (0 = hardware_concurrency)                   |
| `server.document_root`    | Angular dist directory; omit to disable frontend serving |
| `plugin_dir`              | Directory scanned for `.so` plugin files                 |
| `repos_dir`               | Where cloned git repositories are stored                 |
| `migrations_dir`          | Directory containing `NNN_*.sql` migration files         |
| `plugin_config.nvd_api_key` | NVD API key (increases rate limit from 5 to 50 req/30s)|
| `plugin_config.cache_dir` | Scratch space for CVE.org ZIP downloads                  |

**Environment variable overrides** (take precedence over `config.json`):

| Variable              | Overrides                  |
|-----------------------|----------------------------|
| `BLUEDUCK_SECRET_KEY` | AES-256-GCM encryption key (required) |
| `BD_DB_HOST`          | `database.host`            |
| `BD_DB_PORT`          | `database.port`            |
| `BD_DB_NAME`          | `database.name`            |
| `BD_DB_USER`          | `database.user`            |
| `BD_DB_PASSWORD`      | `database.password`        |
| `BD_NVD_API_KEY`      | `plugin_config.nvd_api_key`|

---

## Docker — Quick Start

The fastest way to run blueDuck is with Docker Compose. No local C++ or Node
toolchain is required — everything is built inside the containers.

### Requirements

- Docker Engine ≥ 24
- Docker Compose v2 (`docker compose` — note: no hyphen)

### 1. Create the environment file

```bash
cp .env.example .env
```

Open `.env` and fill in the two required values:

```dotenv
# Mandatory — 32-byte AES key for encrypting Git credentials in the DB
BLUEDUCK_SECRET_KEY=$(openssl rand -hex 32)   # paste the output here

# Postgres password (default "changeme" is fine for local dev)
BD_DB_PASSWORD=changeme
```

### 2. Build and start

```bash
docker compose up --build
```

The first build compiles the full C++ backend and Angular frontend, which takes
several minutes. Subsequent starts reuse the cached image unless source files
change.

| Service     | Published port    | Description                           |
|-------------|-------------------|---------------------------------------|
| `blueduck`  | `localhost:8080`  | Web UI + REST API                     |
| `db`        | not exposed       | PostgreSQL (internal network only)    |

The web interface is available at **http://localhost:8080** once the daemon
prints `[blueDuck] Running`.

### 3. Load CVE data

On first run the database has no CVE records. Trigger a sync from the sidebar
in the UI or via the API:

```bash
# NVD — full download, all CVEs (~250 000 records)
curl -X POST http://localhost:8080/api/cve-sources/nvd/sync

# CVE.org — downloads the latest delta ZIP
curl -X POST http://localhost:8080/api/cve-sources/cveorg/sync
```

The sync runs asynchronously in the background; the daemon logs its progress.

### Stopping and data persistence

```bash
docker compose down          # stop containers, keep volumes
docker compose down -v       # stop containers AND delete all data
```

Named volumes used:

| Volume          | Contents                                    |
|-----------------|---------------------------------------------|
| `postgres_data` | All database tables and CVE data            |
| `repos`         | Cloned Git repositories                     |
| `cache`         | CVE.org ZIP download cache                  |

### Changing the published port

Set `BLUEDUCK_PORT` in `.env` if port 8080 is already in use:

```dotenv
BLUEDUCK_PORT=9090
```

### Image layers (multi-stage build)

```
drogonframework/drogon ──▶ cpp-builder   (compiles daemon + plugins)
node:20-alpine         ──▶ frontend-builder  (builds Angular SPA)
drogonframework/drogon ──▶ runtime          (minimal: only runtime libs)
```

The final runtime image contains only the compiled binaries, Angular assets,
migration SQL files, and the shared libraries needed at runtime. No compiler,
no Node, no source code.

---

## Build — Development

### Prerequisites

| Tool / Library     | Version   | Purpose                            |
|--------------------|-----------|------------------------------------|
| GCC or Clang       | C++23     | Compiler                           |
| CMake              | ≥ 3.25    | Build system                       |
| Drogon             | ≥ 1.9     | HTTP framework + async ORM         |
| libgit2            | ≥ 1.7     | Git operations                     |
| OpenSSL            | ≥ 3.0     | AES-256-GCM encryption             |
| libpq / PostgreSQL | ≥ 15      | Database (Drogon ORM + CVE plugins)|
| libcurl            | ≥ 7.88    | HTTP client in CVE plugins         |
| nlohmann/json      | ≥ 3.11    | JSON parsing in CVE/npm plugins    |
| pugixml            | ≥ 1.13    | XML parsing in NuGet plugin        |
| libzip             | ≥ 1.9     | ZIP extraction in CVE.org plugin   |
| GTest              | ≥ 1.14    | Unit and integration tests         |
| Node.js            | ≥ 20      | Angular frontend build             |
| Angular CLI        | 17        | Frontend toolchain                 |

On Ubuntu 24.04:
```bash
sudo apt install cmake ninja-build libgit2-dev libssl-dev libpq-dev \
                 libcurl4-openssl-dev nlohmann-json3-dev libpugixml-dev \
                 libzip-dev libgtest-dev
# Install Drogon from source — see https://github.com/drogonframework/drogon
```

### Configure and build

```bash
git clone <repo-url> blueDuck
cd blueDuck

cmake -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_TESTS=ON

cmake --build build -j$(nproc)
```

Build outputs:
```
build/bin/blueduck                          daemon binary
build/plugins/cve_source/*.so               CVE source plugins
build/plugins/dependency_analyzer/*.so      dependency analyzer plugins
```

### PostgreSQL setup (dev)

The easiest way is to use the included dev compose file:

```bash
docker compose -f docker-compose.dev.yml up -d
```

This starts a PostgreSQL 16 container on `localhost:5432` with user `blueduck`,
password `changeme`, and database `blueduck` — matching the defaults in
`config.json`. Data is persisted in a named volume (`pgdata`).

```bash
docker compose -f docker-compose.dev.yml down      # stop, keep data
docker compose -f docker-compose.dev.yml down -v    # stop and delete data
```

Alternatively, set up a local PostgreSQL instance manually:

```bash
sudo -u postgres psql <<'SQL'
CREATE USER blueduck WITH PASSWORD 'changeme';
CREATE DATABASE blueduck OWNER blueduck;
SQL
```

### Generate the encryption key

```bash
export BLUEDUCK_SECRET_KEY=$(openssl rand -hex 32)
echo "BLUEDUCK_SECRET_KEY=$BLUEDUCK_SECRET_KEY"   # save this!
```

### Edit the development config

Copy and adjust `config.json`:
```bash
cp config.json config.dev.json
# Set plugin_dir to build/plugins, repos_dir to a local temp path,
# migrations_dir to db/migrations, and omit document_root for API-only mode.
```

Minimal `config.dev.json`:
```json
{
  "server":   { "port": 8080, "threads": 2 },
  "database": { "host": "localhost", "port": 5432,
                "name": "blueduck", "user": "blueduck", "password": "changeme" },
  "plugin_dir":     "build/plugins",
  "repos_dir":      "/tmp/blueduck/repos",
  "migrations_dir": "db/migrations",
  "plugin_config":  { "cache_dir": "/tmp/blueduck" }
}
```

### Run the daemon (dev)

```bash
export BLUEDUCK_SECRET_KEY=<your-key>
./build/bin/blueduck --config config.dev.json
```

### Run the Angular dev server

In a second terminal:
```bash
cd frontend
npm install
npm start         # http://localhost:4200 — proxies /api → :8080
```

---

## Build — Production

### 1. Build the backend (Release)

```bash
cmake -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=/usr

cmake --build build -j$(nproc)
```

### 2. Build the frontend

```bash
cd frontend
npm ci
npm run build     # output: frontend/dist/blueduck-frontend/browser/
```

### 3. Install

```bash
# Binary
sudo install -m 755 build/bin/blueduck /usr/bin/blueduck

# Plugins (preserving cve_source/ and dependency_analyzer/ subdirectories)
sudo mkdir -p /usr/lib/blueduck/plugins/cve_source
sudo mkdir -p /usr/lib/blueduck/plugins/dependency_analyzer
sudo install -m 755 build/plugins/cve_source/*.so /usr/lib/blueduck/plugins/cve_source/
sudo install -m 755 build/plugins/dependency_analyzer/*.so /usr/lib/blueduck/plugins/dependency_analyzer/

# Migrations
sudo mkdir -p /usr/share/blueduck/migrations
sudo install -m 644 db/migrations/*.sql /usr/share/blueduck/migrations/

# Frontend assets
sudo mkdir -p /var/lib/blueduck/www
sudo cp -r frontend/dist/blueduck-frontend/browser/. /var/lib/blueduck/www/

# Config
sudo install -m 640 config.json /etc/blueduck/config.json
sudo chown blueduck:blueduck /etc/blueduck/config.json
```

### 4. systemd unit

Create `/etc/systemd/system/blueduck.service`:

```ini
[Unit]
Description=blueDuck CVE scanner daemon
After=network.target postgresql.service

[Service]
Type=simple
User=blueduck
Group=blueduck
ExecStart=/usr/bin/blueduck --config /etc/blueduck/config.json
Restart=on-failure
RestartSec=5

# Encryption key — store in a credentials file or a secrets manager
EnvironmentFile=/etc/blueduck/environment

# Filesystem isolation
PrivateTmp=true
NoNewPrivileges=true
ProtectSystem=strict
ReadWritePaths=/var/lib/blueduck

[Install]
WantedBy=multi-user.target
```

`/etc/blueduck/environment` (mode 640, owned root:blueduck):
```
BLUEDUCK_SECRET_KEY=<your-32-byte-hex-key>
BD_DB_PASSWORD=<production-password>
BD_NVD_API_KEY=<nvd-api-key>
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now blueduck
```

### 5. Production config

```json
{
  "server": {
    "port": 8080,
    "threads": 0,
    "document_root": "/var/lib/blueduck/www"
  },
  "database": {
    "host": "localhost",
    "port": 5432,
    "name": "blueduck",
    "user": "blueduck",
    "password": "",
    "pool_size": 20
  },
  "plugin_dir":     "/usr/lib/blueduck/plugins",
  "repos_dir":      "/var/lib/blueduck/repos",
  "migrations_dir": "/usr/share/blueduck/migrations",
  "plugin_config": {
    "nvd_api_key": "",
    "cache_dir":   "/var/lib/blueduck/cache"
  }
}
```

> **Note:** Set `BD_DB_PASSWORD` and `BLUEDUCK_SECRET_KEY` via environment
> variables so credentials never appear in the config file.

---

## Running

```
blueduck --config <path/to/config.json>
```

On startup, the daemon:
1. Loads the config file
2. Reads `BLUEDUCK_SECRET_KEY` from the environment
3. Connects to PostgreSQL and runs pending migrations
4. Loads all `.so` files from `plugin_dir` and calls `configure()` on each
5. Starts the Drogon HTTP server

Graceful shutdown on `SIGTERM` or `SIGINT`.

### Initial CVE data load

After the daemon is running, trigger a full CVE sync via the API (or the
frontend sidebar):

```bash
# NVD — full download (~250 000 CVEs, takes several minutes without an API key)
curl -X POST http://localhost:8080/api/cve-sources/nvd/sync

# CVE.org — delta ZIP (fast) or full ZIP (first run)
curl -X POST http://localhost:8080/api/cve-sources/cveorg/sync
```

Get an NVD API key for free at https://nvd.nist.gov/developers/request-an-api-key
to raise the rate limit from 5 to 50 requests per 30 seconds.

---

## Testing

### Unit tests (no database required)

```bash
cd build
ctest -R UnitTests -V
# or directly:
./build/bin/blueduck_unit_tests
```

Covers: version range comparison, CMake analyzer, Python analyzer, NVD parser.

### Integration tests (PostgreSQL required)

```bash
export BD_TEST_DB_URL="postgresql://blueduck:changeme@localhost:5432/blueduck_test"
ctest -R IntegrationTests -V
```

The integration test runs migrations against the test database and verifies
idempotency. Tests are skipped automatically when `BD_TEST_DB_URL` is unset.

### Angular tests

```bash
cd frontend
npm test             # jest — runs *.spec.ts files
npm test -- --coverage
```

---

## Dependencies

| Library       | License       | Used by              |
|---------------|---------------|----------------------|
| Drogon        | MIT           | core daemon          |
| libgit2       | GPL-2 + linking exception | git/ |
| OpenSSL       | Apache-2.0    | git/ (AES-256-GCM)   |
| libpq         | PostgreSQL    | CVE plugins          |
| libcurl       | MIT           | CVE plugins          |
| nlohmann/json | MIT           | nvd, cveorg, npm plugins |
| pugixml       | MIT           | nuget plugin         |
| libzip        | BSD-3         | cveorg plugin        |
| GTest         | BSD-3         | tests only           |
| Angular       | MIT           | frontend             |
| Angular Material | MIT        | frontend             |
