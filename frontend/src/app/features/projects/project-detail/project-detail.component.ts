import { Component, inject, signal, OnInit } from '@angular/core';
import { ActivatedRoute, Router, RouterLink } from '@angular/router';
import { MatTabsModule } from '@angular/material/tabs';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatTableModule } from '@angular/material/table';
import { MatChipsModule } from '@angular/material/chips';
import { MatTooltipModule } from '@angular/material/tooltip';
import { MatSnackBar, MatSnackBarModule } from '@angular/material/snack-bar';
import { MatBadgeModule } from '@angular/material/badge';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { FormsModule } from '@angular/forms';
import { DatePipe, NgClass } from '@angular/common';
import { ProjectService } from '../../../core/services/project.service';
import { Project, ProjectVersion, Report } from '../../../core/models/project.model';

@Component({
  selector: 'bd-project-detail',
  standalone: true,
  imports: [
    RouterLink, MatTabsModule, MatButtonModule, MatIconModule,
    MatTableModule, MatChipsModule, MatTooltipModule, MatSnackBarModule,
    MatBadgeModule, MatFormFieldModule, MatInputModule, MatSelectModule,
    FormsModule, DatePipe, NgClass,
  ],
  template: `
    @if (project()) {
      <div class="page-header">
        <a routerLink="/projects" class="back-link">Projects</a>
        <span> / </span>
        <h1>{{ project()!.name }}</h1>
        <span class="bd-spacer"></span>
        <button mat-icon-button matTooltip="Sync repository" (click)="sync()">
          <mat-icon>sync</mat-icon>
        </button>
      </div>

      <p class="git-url">{{ project()!.git_url }}</p>

      <mat-tab-group>
        <!-- Versions tab -->
        <mat-tab label="Versions ({{ versions().length }})">
          <div class="tab-content">
            <table mat-table [dataSource]="versions()" class="mat-elevation-z2 full-width">
              <ng-container matColumnDef="ref_name">
                <th mat-header-cell *matHeaderCellDef>Ref</th>
                <td mat-cell *matCellDef="let v">
                  <mat-chip [ngClass]="v.ref_type">{{ v.ref_name }}</mat-chip>
                </td>
              </ng-container>

              <ng-container matColumnDef="commit_hash">
                <th mat-header-cell *matHeaderCellDef>Commit</th>
                <td mat-cell *matCellDef="let v">
                  <code>{{ v.commit_hash ? v.commit_hash.substring(0, 8) : '—' }}</code>
                </td>
              </ng-container>

              <ng-container matColumnDef="dep_count">
                <th mat-header-cell *matHeaderCellDef>Deps</th>
                <td mat-cell *matCellDef="let v">{{ v.dep_count }}</td>
              </ng-container>

              <ng-container matColumnDef="analyzed_at">
                <th mat-header-cell *matHeaderCellDef>Analyzed</th>
                <td mat-cell *matCellDef="let v">
                  {{ v.analyzed_at ? (v.analyzed_at | date:'short') : '—' }}
                </td>
              </ng-container>

              <ng-container matColumnDef="actions">
                <th mat-header-cell *matHeaderCellDef></th>
                <td mat-cell *matCellDef="let v">
                  <button mat-icon-button matTooltip="Analyze" (click)="analyze(v)">
                    <mat-icon>search</mat-icon>
                  </button>
                  <button mat-icon-button matTooltip="Generate report" (click)="generateReport(v)">
                    <mat-icon>assessment</mat-icon>
                  </button>
                  <button mat-icon-button matTooltip="View reports" (click)="viewReports(v)">
                    <mat-icon>list_alt</mat-icon>
                  </button>
                </td>
              </ng-container>

              <tr mat-header-row *matHeaderRowDef="versionCols"></tr>
              <tr mat-row *matRowDef="let row; columns: versionCols;"></tr>
            </table>
          </div>
        </mat-tab>

        <!-- Reports tab -->
        <mat-tab label="Reports ({{ reports().length }})">
          <div class="tab-content">
            <table mat-table [dataSource]="reports()" class="mat-elevation-z2 full-width">
              <ng-container matColumnDef="created_at">
                <th mat-header-cell *matHeaderCellDef>Created</th>
                <td mat-cell *matCellDef="let r">{{ r.created_at | date:'short' }}</td>
              </ng-container>

              <ng-container matColumnDef="vulnerable_count">
                <th mat-header-cell *matHeaderCellDef>Vulnerabilities</th>
                <td mat-cell *matCellDef="let r">
                  @if (r.vulnerable_count > 0) {
                    <span class="severity-critical">C:{{ r.critical_count }}</span>
                    <span class="severity-high"> H:{{ r.high_count }}</span>
                    <span class="severity-medium"> M:{{ r.medium_count }}</span>
                    <span class="severity-low"> L:{{ r.low_count }}</span>
                  } @else {
                    <span class="severity-none">None</span>
                  }
                </td>
              </ng-container>

              <ng-container matColumnDef="actions">
                <th mat-header-cell *matHeaderCellDef></th>
                <td mat-cell *matCellDef="let r">
                  <button mat-icon-button matTooltip="View findings"
                          (click)="openReport(r)">
                    <mat-icon>open_in_new</mat-icon>
                  </button>
                </td>
              </ng-container>

              <tr mat-header-row *matHeaderRowDef="reportCols"></tr>
              <tr mat-row *matRowDef="let row; columns: reportCols;"></tr>
            </table>
          </div>
        </mat-tab>

        <!-- Settings tab -->
        <mat-tab label="Settings">
          <div class="tab-content settings-form">
            <h3>Git Credentials</h3>
            <mat-form-field appearance="outline" class="full-width">
              <mat-label>Authentication Method</mat-label>
              <mat-select [(ngModel)]="credMethod">
                <mat-option value="none">None (public repo)</mat-option>
                <mat-option value="pat">Personal Access Token</mat-option>
                <mat-option value="ssh_key">SSH Key</mat-option>
              </mat-select>
            </mat-form-field>

            @if (credMethod === 'pat') {
              <mat-form-field appearance="outline" class="full-width">
                <mat-label>Personal Access Token</mat-label>
                <input matInput [(ngModel)]="credPat" type="password"
                       placeholder="ghp_... or glpat-...">
              </mat-form-field>
            }

            @if (credMethod === 'ssh_key') {
              <mat-form-field appearance="outline" class="full-width">
                <mat-label>SSH Private Key</mat-label>
                <textarea matInput [(ngModel)]="credSshPrivate" rows="8"
                          class="mono"
                          placeholder="-----BEGIN OPENSSH PRIVATE KEY-----"></textarea>
              </mat-form-field>
              <mat-form-field appearance="outline" class="full-width">
                <mat-label>SSH Public Key</mat-label>
                <textarea matInput [(ngModel)]="credSshPublic" rows="3"
                          class="mono"
                          placeholder="ssh-ed25519 AAAA..."></textarea>
              </mat-form-field>
            }

            <button mat-raised-button color="primary" (click)="saveCredentials()">
              Save Credentials
            </button>
          </div>
        </mat-tab>
      </mat-tab-group>
    }
  `,
  styles: [`
    .page-header { display: flex; align-items: center; gap: 8px; margin-bottom: 8px; }
    h1 { margin: 0; }
    .back-link { cursor: pointer; }
    .git-url { color: #666; font-size: 0.9em; margin: 0 0 16px; }
    .tab-content { padding-top: 16px; }
    .full-width { width: 100%; }
    mat-chip.branch { --mdc-chip-label-text-color: #1565c0; }
    mat-chip.tag { --mdc-chip-label-text-color: #2e7d32; }
    .settings-form { max-width: 500px; }
    .settings-form h3 { margin-top: 0; }
    .mono { font-family: monospace; font-size: 0.85em; }
  `]
})
export class ProjectDetailComponent implements OnInit {
  private route  = inject(ActivatedRoute);
  private router = inject(Router);
  private svc    = inject(ProjectService);
  private snack  = inject(MatSnackBar);

  projectId = 0;
  project   = signal<Project | null>(null);
  versions  = signal<ProjectVersion[]>([]);
  reports   = signal<Report[]>([]);

  versionCols = ['ref_name', 'commit_hash', 'dep_count', 'analyzed_at', 'actions'];
  reportCols  = ['created_at', 'vulnerable_count', 'actions'];

  credMethod     = 'none';
  credPat        = '';
  credSshPrivate = '';
  credSshPublic  = '';

  ngOnInit() {
    this.projectId = Number(this.route.snapshot.paramMap.get('projectId'));
    this.svc.getProject(this.projectId).subscribe(p => this.project.set(p));
    this.loadVersionsAndReports();
  }

  loadVersionsAndReports() {
    this.svc.getVersions(this.projectId).subscribe(vs => {
      this.versions.set(vs);

      // Collect all reports across all versions
      const all: Report[] = [];
      let remaining = vs.length;
      if (!remaining) { this.reports.set([]); return; }
      for (const v of vs) {
        this.svc.getReports(this.projectId, v.id).subscribe(rs => {
          all.push(...rs);
          if (--remaining === 0) {
            all.sort((a, b) => b.created_at.localeCompare(a.created_at));
            this.reports.set(all);
          }
        });
      }
    });
  }

  sync() {
    this.svc.syncProject(this.projectId).subscribe({
      next: () => this.snack.open('Sync started', 'OK', { duration: 3000 }),
      error: () => this.snack.open('Sync failed', 'OK', { duration: 3000 }),
    });
  }

  analyze(v: ProjectVersion) {
    this.svc.analyzeVersion(this.projectId, v.id).subscribe({
      next: () => this.snack.open('Analysis started', 'OK', { duration: 3000 }),
      error: () => this.snack.open('Analysis failed', 'OK', { duration: 3000 }),
    });
  }

  generateReport(v: ProjectVersion) {
    this.svc.generateReport(this.projectId, v.id).subscribe({
      next: () => this.snack.open('Report generation started', 'OK', { duration: 3000 }),
      error: () => this.snack.open('Failed', 'OK', { duration: 3000 }),
    });
  }

  viewReports(v: ProjectVersion) {
    // Load latest report for this version and navigate
    this.svc.getReports(this.projectId, v.id).subscribe(rs => {
      if (!rs.length) { this.snack.open('No reports yet', 'OK', { duration: 2000 }); return; }
      this.openReport(rs[0]);
    });
  }

  openReport(r: Report) {
    const versionId = r.project_version_id;
    this.router.navigate(['/projects', this.projectId, 'versions', versionId, 'reports', r.id]);
  }

  saveCredentials() {
    const creds: Record<string, string> = { auth_method: this.credMethod };
    if (this.credMethod === 'pat') {
      if (!this.credPat) {
        this.snack.open('Please enter a personal access token', 'OK', { duration: 3000 });
        return;
      }
      creds['pat'] = this.credPat;
    } else if (this.credMethod === 'ssh_key') {
      if (!this.credSshPrivate || !this.credSshPublic) {
        this.snack.open('Both SSH keys are required', 'OK', { duration: 3000 });
        return;
      }
      creds['ssh_private_key'] = this.credSshPrivate;
      creds['ssh_public_key'] = this.credSshPublic;
    }
    this.svc.setCredentials(this.projectId, creds).subscribe({
      next: () => {
        this.snack.open('Credentials saved', 'OK', { duration: 3000 });
        this.credPat = '';
        this.credSshPrivate = '';
        this.credSshPublic = '';
      },
      error: () => this.snack.open('Failed to save credentials', 'OK', { duration: 3000 }),
    });
  }
}
