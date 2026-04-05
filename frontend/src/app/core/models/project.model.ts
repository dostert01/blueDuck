export interface Project {
  id: number;
  name: string;
  git_url: string;
  description: string;
  created_at: string;
  last_synced_at: string | null;
}

export interface ProjectVersion {
  id: number;
  project_id: number;
  ref_name: string;
  ref_type: 'branch' | 'tag';
  commit_hash: string;
  analyzed_at: string | null;
  dep_count: number;
}

export interface Report {
  id: number;
  project_version_id: number;
  created_at: string;
  total_dependencies: number;
  vulnerable_count: number;
  critical_count: number;
  high_count: number;
  medium_count: number;
  low_count: number;
}

export interface ReportFinding {
  id: number;
  dependency_id: number;
  ecosystem: string;
  package_name: string;
  package_version: string;
  cve_id: string;
  severity: 'CRITICAL' | 'HIGH' | 'MEDIUM' | 'LOW' | 'NONE';
  cvss_score: number;
  description: string;
  cvss_v3_score: number;
  cvss_v2_score: number;
  published_at: string;
  modified_at: string;
}

export interface ReportDependency {
  id: number;
  ecosystem: string;
  package_name: string;
  package_version: string;
}

export interface CpeMapping {
  id: number;
  ecosystem: string;
  package_name: string;
  cpe_vendor: string;
  cpe_product: string;
  git_url_pattern: string;
}

export interface CveSource {
  name: string;
}
