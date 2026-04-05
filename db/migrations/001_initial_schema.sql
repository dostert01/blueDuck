-- blueDuck initial schema
-- Applied by Migrator on first startup.

-- ---------------------------------------------------------------
-- Projects
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS projects (
    id          SERIAL PRIMARY KEY,
    name        TEXT        NOT NULL UNIQUE,
    git_url     TEXT        NOT NULL,
    description TEXT        NOT NULL DEFAULT '',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_synced_at TIMESTAMPTZ
);

-- ---------------------------------------------------------------
-- Per-project Git credentials (one row per project)
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS project_credentials (
    project_id          INT  PRIMARY KEY REFERENCES projects(id) ON DELETE CASCADE,
    auth_method         TEXT NOT NULL DEFAULT 'none',   -- none | pat | ssh_key
    pat_enc             TEXT,                           -- AES-256-GCM ciphertext (hex)
    ssh_private_key_path TEXT,
    ssh_public_key_path  TEXT,
    ssh_passphrase_enc   TEXT
);

-- ---------------------------------------------------------------
-- Tracked branches/tags per project
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS project_versions (
    id                 SERIAL PRIMARY KEY,
    project_id         INT  NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    ref_name           TEXT NOT NULL,                   -- e.g. "main", "v1.2.3"
    ref_type           TEXT NOT NULL DEFAULT 'branch',  -- branch | tag
    commit_hash        TEXT,
    analyzed_at        TIMESTAMPTZ,
    UNIQUE (project_id, ref_name)
);

-- ---------------------------------------------------------------
-- Dependencies extracted from a project version
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS dependencies (
    id                 SERIAL PRIMARY KEY,
    project_version_id INT  NOT NULL REFERENCES project_versions(id) ON DELETE CASCADE,
    ecosystem          TEXT NOT NULL,   -- cmake | npm | cargo | pypi | nuget
    package_name       TEXT NOT NULL,
    package_version    TEXT,
    UNIQUE (project_version_id, ecosystem, package_name)
);

-- ---------------------------------------------------------------
-- CVE records (normalised from NVD / CVE.org)
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS cve_records (
    id           SERIAL PRIMARY KEY,
    cve_id       TEXT    NOT NULL UNIQUE,  -- e.g. "CVE-2023-12345"
    description  TEXT    NOT NULL DEFAULT '',
    severity     TEXT    NOT NULL DEFAULT 'NONE',  -- CRITICAL | HIGH | MEDIUM | LOW | NONE
    cvss_v3_score REAL,
    cvss_v2_score REAL,
    published_at TIMESTAMPTZ,
    modified_at  TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_cve_records_severity ON cve_records(severity);

-- ---------------------------------------------------------------
-- Affected products / CPE entries linked to CVE records
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS cve_affected_products (
    id                      SERIAL PRIMARY KEY,
    cve_record_id           INT  NOT NULL REFERENCES cve_records(id) ON DELETE CASCADE,
    ecosystem               TEXT NOT NULL DEFAULT '',   -- e.g. "npm", "cargo" (empty for CPE-only)
    package_name            TEXT NOT NULL DEFAULT '',
    cpe_uri                 TEXT,                       -- e.g. "cpe:2.3:a:vendor:product:*:..."
    version_exact           TEXT,
    version_start_including TEXT,
    version_start_excluding TEXT,
    version_end_including   TEXT,
    version_end_excluding   TEXT
);

CREATE INDEX IF NOT EXISTS idx_cap_ecosystem_pkg
    ON cve_affected_products(ecosystem, LOWER(package_name));
CREATE INDEX IF NOT EXISTS idx_cap_cpe_uri
    ON cve_affected_products(LOWER(cpe_uri));

-- ---------------------------------------------------------------
-- CPE mapping table: bridges package manager names to NVD CPE strings
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS ecosystem_cpe_mapping (
    id              SERIAL PRIMARY KEY,
    ecosystem       TEXT NOT NULL,    -- cmake | npm | cargo | pypi | nuget
    package_name    TEXT NOT NULL,    -- as seen in the package manager
    cpe_vendor      TEXT NOT NULL,
    cpe_product     TEXT NOT NULL,
    git_url_pattern TEXT,             -- optional: match via git URL substring
    UNIQUE (ecosystem, package_name)
);

-- Seed a handful of well-known CMake packages
INSERT INTO ecosystem_cpe_mapping (ecosystem, package_name, cpe_vendor, cpe_product)
VALUES
    ('cmake', 'ZLIB',       'zlib',      'zlib'),
    ('cmake', 'OpenSSL',    'openssl',   'openssl'),
    ('cmake', 'CURL',       'haxx',      'curl'),
    ('cmake', 'Boost',      'boost',     'boost'),
    ('cmake', 'libzip',     'libzip',    'libzip'),
    ('cmake', 'PostgreSQL', 'postgresql','postgresql'),
    ('cmake', 'LibGit2',    'libgit2',   'libgit2'),
    ('cmake', 'pugixml',    'pugixml',   'pugixml')
ON CONFLICT DO NOTHING;

-- ---------------------------------------------------------------
-- Vulnerability reports
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS reports (
    id                 SERIAL PRIMARY KEY,
    project_version_id INT  NOT NULL REFERENCES project_versions(id) ON DELETE CASCADE,
    created_at         TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    total_dependencies INT  NOT NULL DEFAULT 0,
    vulnerable_count   INT  NOT NULL DEFAULT 0,
    critical_count     INT  NOT NULL DEFAULT 0,
    high_count         INT  NOT NULL DEFAULT 0,
    medium_count       INT  NOT NULL DEFAULT 0,
    low_count          INT  NOT NULL DEFAULT 0
);

-- ---------------------------------------------------------------
-- Individual findings within a report
-- ---------------------------------------------------------------
CREATE TABLE IF NOT EXISTS report_findings (
    id             SERIAL PRIMARY KEY,
    report_id      INT  NOT NULL REFERENCES reports(id) ON DELETE CASCADE,
    dependency_id  INT  NOT NULL REFERENCES dependencies(id) ON DELETE CASCADE,
    cve_record_id  INT  NOT NULL REFERENCES cve_records(id) ON DELETE CASCADE,
    cvss_score     REAL NOT NULL DEFAULT 0.0,
    severity       TEXT NOT NULL DEFAULT 'NONE',
    UNIQUE (report_id, dependency_id, cve_record_id)
);

CREATE INDEX IF NOT EXISTS idx_findings_report ON report_findings(report_id);
CREATE INDEX IF NOT EXISTS idx_findings_severity ON report_findings(severity);
