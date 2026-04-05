# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure (only needed once or after CMakeLists changes)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build everything (daemon + plugins + tests)
cmake --build build -j$(nproc)

# Build outputs:
#   build/bin/blueduck                          - daemon binary
#   build/plugins/cve_source/*.so               - CVE source plugins
#   build/plugins/dependency_analyzer/*.so      - dependency analyzer plugins
#   build/bin/blueduck_unit_tests               - unit tests
#   build/bin/blueduck_integration_tests        - integration tests

# Run unit tests (no database required)
./build/bin/blueduck_unit_tests

# Run a single unit test by filter
./build/bin/blueduck_unit_tests --gtest_filter="VersionCompareTest.ExactMatch"

# Run integration tests (requires BD_TEST_DB_URL)
export BD_TEST_DB_URL="postgresql://blueduck:changeme@localhost:5432/blueduck_test"
./build/bin/blueduck_integration_tests

# Frontend (from frontend/ directory)
npm install
npm start           # dev server on :4200, proxies /api to :8080
npm run build       # production build to dist/blueduck-frontend/
npm test            # jest unit tests
```

## Architecture

blueDuck is a CVE vulnerability scanner daemon: a single C++23 process serving both a REST API and an Angular SPA, backed by PostgreSQL.

### Plugin System (dlopen)

The daemon loads `.so` plugins at startup from two subdirectories under `plugin_dir`:
- `plugin_dir/cve_source/` — CVE data importers (NVD, CVE.org)
- `plugin_dir/dependency_analyzer/` — per-ecosystem dependency extractors (cmake, npm, cargo, python, nuget)

Each plugin exports three C symbols (`blueduck_plugin_meta`, `blueduck_create_plugin`, `blueduck_destroy_plugin`). The `BLUEDUCK_REGISTER_PLUGIN` macro in `interfaces/PluginMeta.h` generates these. Plugin interfaces are in `interfaces/` — `ICveSource.h` and `IDependencyAnalyzer.h`.

Plugins do **not** link Drogon. CVE source plugins use libpq directly (`plugins/common/CveDbWriter`) and libcurl (`plugins/common/CveHttpClient`) for HTTP and database access.

When adding a new plugin, use `add_blueduck_plugin()` from `cmake/PluginHelpers.cmake` with `TYPE cve_source` or `TYPE dependency_analyzer` to place the `.so` in the correct subdirectory.

### Core Daemon (src/)

- **`core/Application`** — Singleton. Orchestrates init: loads config, runs migrations, loads plugins, configures Drogon. Entry point for `syncCveSource()` and `analyzeVersion()`.
- **`core/PluginLoader`** — dlopen/dlsym wrapper. Registers plugins by type, exposes them by name/ecosystem.
- **`core/AnalysisService`** — Checks out a git ref, runs all applicable analyzer plugins, stores dependencies in a single transaction.
- **`core/Config`** — Loads `config.json` with environment variable overrides (`BD_DB_*`, `BD_NVD_API_KEY`).
- **`db/Database`** — Static wrapper around Drogon's DB client pool. Two named clients: `"default"` (async, for request handlers) and `"blocking"` (for migrations/analysis).
- **`db/FieldHelpers.h`** — `fieldOr(field, default)` and `fieldOrEmpty(field)` helpers for null-safe Drogon ORM field access (this Drogon version's `Field::as<T>()` takes no default argument).
- **`git/CredentialProvider`** — AES-256-GCM encryption for stored PATs using `BLUEDUCK_SECRET_KEY` env var. Also implements the libgit2 credential callback.
- **`git/GitRepository`** — Clone/fetch/checkout operations via libgit2.
- **`report/ReportEngine`** — Matches stored dependencies against CVE records using version range comparison and CPE mapping. Generates report with severity counts.
- **`api/controllers/`** — Drogon HTTP controllers. Async operations (sync, analyze, report generation) use `drogon::async_run` to return 202 immediately.

### Database

Schema is in `db/migrations/001_initial_schema.sql`. Migrations run automatically at startup via `db/Migrator`. Key tables: `projects`, `project_versions`, `dependencies`, `cve_records`, `cve_affected_products`, `ecosystem_cpe_mapping`, `reports`, `report_findings`.

### Frontend (Angular 17)

Standalone components with Angular Material. All API calls go through `core/services/project.service.ts`. Uses Jest for testing (not Karma). The dev server proxies `/api` to `:8080` via `proxy.conf.json`.

### Key Patterns

- **Drogon ORM callbacks**: All DB queries use the `>> [success callback] >> [error callback]` pattern. Use `fieldOr()`/`fieldOrEmpty()` from `db/FieldHelpers.h` for nullable columns — never pass default arguments to `Field::as<T>()`.
- **Async fire-and-forget**: Long operations (git sync, analysis, report generation, CVE sync) run via `drogon::async_run` and return 202 Accepted. Results are logged to stdout/stderr.
- **Plugin config**: All plugins receive the same `std::unordered_map<string,string>` config. The `db_connstr` key is injected by `Config.cpp` so plugins can connect to PostgreSQL independently of Drogon.

## Environment Variables

| Variable | Purpose |
|---|---|
| `BLUEDUCK_SECRET_KEY` | Required. 64 hex chars (32-byte AES key) for credential encryption |
| `BD_DB_HOST/PORT/NAME/USER/PASSWORD` | Override database config from config.json |
| `BD_NVD_API_KEY` | Override NVD API key (increases rate limit) |
| `BD_TEST_DB_URL` | Connection string for integration tests |
