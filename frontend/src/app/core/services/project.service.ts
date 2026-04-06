import { Injectable, inject } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { Project, ProjectVersion, Report, ReportFinding, ReportDependency, CpeMapping, Mitigation, MitigationType, CveSource } from '../models/project.model';

@Injectable({ providedIn: 'root' })
export class ProjectService {
  private http = inject(HttpClient);

  // Projects
  getProjects(): Observable<Project[]> {
    return this.http.get<Project[]>('/api/projects');
  }

  getProject(id: number): Observable<Project> {
    return this.http.get<Project>(`/api/projects/${id}`);
  }

  createProject(data: Pick<Project, 'name' | 'git_url' | 'description'>): Observable<Project> {
    return this.http.post<Project>('/api/projects', data);
  }

  deleteProject(id: number): Observable<void> {
    return this.http.delete<void>(`/api/projects/${id}`);
  }

  setCredentials(projectId: number, creds: Record<string, string>): Observable<void> {
    return this.http.post<void>(`/api/projects/${projectId}/credentials`, creds);
  }

  syncProject(projectId: number): Observable<{ status: string }> {
    return this.http.post<{ status: string }>(`/api/projects/${projectId}/sync`, {});
  }

  testConnection(projectId: number): Observable<{ success: boolean; message: string }> {
    return this.http.post<{ success: boolean; message: string }>(
      `/api/projects/${projectId}/test-connection`, {});
  }

  // Versions
  getVersions(projectId: number): Observable<ProjectVersion[]> {
    return this.http.get<ProjectVersion[]>(`/api/projects/${projectId}/versions`);
  }

  deleteVersion(projectId: number, versionId: number): Observable<void> {
    return this.http.delete<void>(`/api/projects/${projectId}/versions/${versionId}`);
  }

  analyzeVersion(projectId: number, versionId: number): Observable<{ status: string }> {
    return this.http.post<{ status: string }>(
      `/api/projects/${projectId}/versions/${versionId}/analyze`, {});
  }

  // Reports
  getReports(projectId: number, versionId: number): Observable<Report[]> {
    return this.http.get<Report[]>(
      `/api/projects/${projectId}/versions/${versionId}/reports`);
  }

  getReport(reportId: number): Observable<Report> {
    return this.http.get<Report>(`/api/reports/${reportId}`);
  }

  getFindings(reportId: number): Observable<ReportFinding[]> {
    return this.http.get<ReportFinding[]>(`/api/reports/${reportId}/findings`);
  }

  getDependencies(reportId: number): Observable<ReportDependency[]> {
    return this.http.get<ReportDependency[]>(`/api/reports/${reportId}/dependencies`);
  }

  generateReport(projectId: number, versionId: number): Observable<{ status: string }> {
    return this.http.post<{ status: string }>(
      `/api/projects/${projectId}/versions/${versionId}/reports`, {});
  }

  // Mitigations
  createMitigation(findingId: number, data: { type: MitigationType; description: string }): Observable<Mitigation> {
    return this.http.post<Mitigation>(`/api/findings/${findingId}/mitigation`, data);
  }

  createBulkMitigation(findingIds: number[], data: { type: MitigationType; description: string }): Observable<Mitigation> {
    return this.http.post<Mitigation>('/api/findings/mitigate-bulk', { ...data, finding_ids: findingIds });
  }

  deleteMitigation(findingId: number): Observable<void> {
    return this.http.delete<void>(`/api/findings/${findingId}/mitigation`);
  }

  // CPE Mappings
  getCpeMappings(): Observable<CpeMapping[]> {
    return this.http.get<CpeMapping[]>('/api/cpe-mappings');
  }

  createCpeMapping(data: Omit<CpeMapping, 'id'>): Observable<CpeMapping> {
    return this.http.post<CpeMapping>('/api/cpe-mappings', data);
  }

  updateCpeMapping(id: number, data: Omit<CpeMapping, 'id'>): Observable<CpeMapping> {
    return this.http.put<CpeMapping>(`/api/cpe-mappings/${id}`, data);
  }

  deleteCpeMapping(id: number): Observable<void> {
    return this.http.delete<void>(`/api/cpe-mappings/${id}`);
  }

  // CVE Sources
  getCveSources(): Observable<CveSource[]> {
    return this.http.get<CveSource[]>('/api/cve-sources');
  }

  syncCveSource(name: string): Observable<{ status: string }> {
    return this.http.post<{ status: string }>(`/api/cve-sources/${name}/sync`, {});
  }
}
