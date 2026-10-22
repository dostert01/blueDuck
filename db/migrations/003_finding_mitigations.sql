CREATE TABLE report_findings_mitigations (
    id            SERIAL PRIMARY KEY,
    finding_id    INTEGER NOT NULL REFERENCES report_findings(id) ON DELETE CASCADE,
    type          TEXT NOT NULL CHECK (type IN ('NOT_APPLICABLE', 'FALSE_FINDING', 'MITIGATED_BY_PROJECT')),
    description   TEXT NOT NULL DEFAULT '',
    mitigated_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_mitigations_finding ON report_findings_mitigations(finding_id);

ALTER TABLE report_findings ADD COLUMN mitigation_id INTEGER REFERENCES report_findings_mitigations(id) ON DELETE SET NULL;
