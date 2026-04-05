import { Component, inject, signal, computed, OnInit } from '@angular/core';
import { ActivatedRoute, RouterLink } from '@angular/router';
import { MatTableModule } from '@angular/material/table';
import { MatChipsModule } from '@angular/material/chips';
import { MatCardModule } from '@angular/material/card';
import { MatIconModule } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { MatSortModule, Sort } from '@angular/material/sort';
import { MatDialogModule, MatDialog } from '@angular/material/dialog';
import { NgClass, DatePipe, DecimalPipe } from '@angular/common';
import { ProjectService } from '../../../core/services/project.service';
import { Report, ReportFinding, ReportDependency } from '../../../core/models/project.model';
import { FindingDetailDialogComponent } from '../finding-detail-dialog/finding-detail-dialog.component';

type SeverityFilter = 'CRITICAL' | 'HIGH' | 'MEDIUM' | 'LOW' | null;

@Component({
  selector: 'bd-report-detail',
  standalone: true,
  imports: [
    RouterLink, MatTableModule, MatChipsModule, MatCardModule,
    MatIconModule, MatButtonModule, MatSortModule, MatDialogModule,
    NgClass, DatePipe, DecimalPipe,
  ],
  template: `
    @if (report()) {
      <div class="page-header">
        <a [routerLink]="['/projects', projectId]" class="back-link">Back to Project</a>
        <h1>Report #{{ report()!.id }}</h1>
        <span class="date">{{ report()!.created_at | date:'medium' }}</span>
      </div>

      <!-- Summary cards — clickable as severity filters -->
      <div class="summary-cards">
        <mat-card class="summary-card">
          <mat-card-content>
            <div class="stat">{{ report()!.total_dependencies }}</div>
            <div class="label">Dependencies</div>
          </mat-card-content>
        </mat-card>
        <mat-card class="summary-card severity-critical filter-btn"
                  [class.active]="activeFilter() === 'CRITICAL'"
                  (click)="toggleFilter('CRITICAL')">
          <mat-card-content>
            <div class="stat">{{ report()!.critical_count }}</div>
            <div class="label">Critical</div>
          </mat-card-content>
        </mat-card>
        <mat-card class="summary-card severity-high filter-btn"
                  [class.active]="activeFilter() === 'HIGH'"
                  (click)="toggleFilter('HIGH')">
          <mat-card-content>
            <div class="stat">{{ report()!.high_count }}</div>
            <div class="label">High</div>
          </mat-card-content>
        </mat-card>
        <mat-card class="summary-card severity-medium filter-btn"
                  [class.active]="activeFilter() === 'MEDIUM'"
                  (click)="toggleFilter('MEDIUM')">
          <mat-card-content>
            <div class="stat">{{ report()!.medium_count }}</div>
            <div class="label">Medium</div>
          </mat-card-content>
        </mat-card>
        <mat-card class="summary-card severity-low filter-btn"
                  [class.active]="activeFilter() === 'LOW'"
                  (click)="toggleFilter('LOW')">
          <mat-card-content>
            <div class="stat">{{ report()!.low_count }}</div>
            <div class="label">Low</div>
          </mat-card-content>
        </mat-card>
      </div>

      @if (activeFilter()) {
        <div class="filter-info">
          Showing <strong>{{ activeFilter() }}</strong> findings
          <button mat-button (click)="toggleFilter(null)">Clear filter</button>
        </div>
      }

      <!-- Findings table -->
      <h2>Findings</h2>
      <table mat-table [dataSource]="displayedFindings()" matSort (matSortChange)="onSort($event)"
             class="mat-elevation-z2 full-width">

        <ng-container matColumnDef="severity">
          <th mat-header-cell *matHeaderCellDef mat-sort-header>Severity</th>
          <td mat-cell *matCellDef="let f">
            <mat-chip [ngClass]="'severity-' + f.severity.toLowerCase()">
              {{ f.severity }}
            </mat-chip>
          </td>
        </ng-container>

        <ng-container matColumnDef="cvss_score">
          <th mat-header-cell *matHeaderCellDef mat-sort-header>CVSS</th>
          <td mat-cell *matCellDef="let f">{{ f.cvss_score | number:'1.1-1' }}</td>
        </ng-container>

        <ng-container matColumnDef="cve_id">
          <th mat-header-cell *matHeaderCellDef mat-sort-header>CVE</th>
          <td mat-cell *matCellDef="let f"><code>{{ f.cve_id }}</code></td>
        </ng-container>

        <ng-container matColumnDef="package_name">
          <th mat-header-cell *matHeaderCellDef mat-sort-header>Package</th>
          <td mat-cell *matCellDef="let f">
            <strong>{{ f.package_name }}</strong>
            @if (f.package_version) { <span class="version">{{'@'}}{{ f.package_version }}</span> }
          </td>
        </ng-container>

        <ng-container matColumnDef="ecosystem">
          <th mat-header-cell *matHeaderCellDef>Ecosystem</th>
          <td mat-cell *matCellDef="let f">{{ f.ecosystem }}</td>
        </ng-container>

        <tr mat-header-row *matHeaderRowDef="findingCols"></tr>
        <tr mat-row *matRowDef="let row; columns: findingCols;"
            class="clickable-row" (click)="openFinding(row)"></tr>
      </table>

      @if (displayedFindings().length === 0) {
        <p class="no-findings">
          @if (activeFilter()) {
            No {{ activeFilter()!.toLowerCase() }} severity findings.
          } @else {
            No vulnerabilities found.
          }
        </p>
      }

      <!-- All dependencies -->
      <h2>All Dependencies</h2>
      <table mat-table [dataSource]="dependencies()" class="mat-elevation-z2 full-width">

        <ng-container matColumnDef="ecosystem">
          <th mat-header-cell *matHeaderCellDef>Ecosystem</th>
          <td mat-cell *matCellDef="let d">{{ d.ecosystem }}</td>
        </ng-container>

        <ng-container matColumnDef="package_name">
          <th mat-header-cell *matHeaderCellDef>Package</th>
          <td mat-cell *matCellDef="let d"><strong>{{ d.package_name }}</strong></td>
        </ng-container>

        <ng-container matColumnDef="package_version">
          <th mat-header-cell *matHeaderCellDef>Version</th>
          <td mat-cell *matCellDef="let d">{{ d.package_version || '—' }}</td>
        </ng-container>

        <tr mat-header-row *matHeaderRowDef="depCols"></tr>
        <tr mat-row *matRowDef="let row; columns: depCols;"></tr>
      </table>

      @if (dependencies().length === 0) {
        <p class="no-findings">No dependencies found.</p>
      }
    }
  `,
  styles: [`
    .page-header { display: flex; align-items: center; gap: 12px; margin-bottom: 16px; }
    h1 { margin: 0; }
    .back-link { cursor: pointer; }
    .date { color: #666; font-size: 0.9em; }
    .summary-cards { display: flex; gap: 16px; flex-wrap: wrap; margin-bottom: 24px; }
    .summary-card { text-align: center; min-width: 120px; }
    .stat { font-size: 2em; font-weight: 700; }
    .label { font-size: 0.9em; color: #666; }
    .full-width { width: 100%; }
    .version { color: #666; margin-left: 4px; }
    .no-findings { color: #388e3c; font-style: italic; }
    .clickable-row { cursor: pointer; }
    .clickable-row:hover { background: rgba(0,0,0,.04); }
    .filter-btn { cursor: pointer; transition: transform 0.15s, box-shadow 0.15s; }
    .filter-btn:hover { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.15); }
    .filter-btn.active { outline: 3px solid #1976d2; outline-offset: 2px; }
    .filter-info { margin-bottom: 16px; display: flex; align-items: center; gap: 8px; }
    mat-chip.severity-critical { --mdc-chip-label-text-color: white; background: #d32f2f; }
    mat-chip.severity-high     { --mdc-chip-label-text-color: white; background: #f57c00; }
    mat-chip.severity-medium   { --mdc-chip-label-text-color: black; background: #fbc02d; }
    mat-chip.severity-low      { --mdc-chip-label-text-color: white; background: #388e3c; }
    mat-chip.severity-none     { --mdc-chip-label-text-color: #333; background: #e0e0e0; }
  `]
})
export class ReportDetailComponent implements OnInit {
  private route  = inject(ActivatedRoute);
  private svc    = inject(ProjectService);
  private dialog = inject(MatDialog);

  projectId = 0;
  report       = signal<Report | null>(null);
  findings     = signal<ReportFinding[]>([]);
  dependencies = signal<ReportDependency[]>([]);
  activeFilter = signal<SeverityFilter>(null);
  private currentSort = signal<Sort>({ active: '', direction: '' });

  displayedFindings = computed(() => {
    let data = [...this.findings()];
    const filter = this.activeFilter();
    if (filter) {
      data = data.filter(f => f.severity === filter);
    }
    const sort = this.currentSort();
    if (sort.active && sort.direction !== '') {
      data.sort((a, b) => {
        const isAsc = sort.direction === 'asc';
        switch (sort.active) {
          case 'cvss_score':   return compare(a.cvss_score, b.cvss_score, isAsc);
          case 'severity':     return compare(a.severity, b.severity, isAsc);
          case 'cve_id':       return compare(a.cve_id, b.cve_id, isAsc);
          case 'package_name': return compare(a.package_name, b.package_name, isAsc);
          default: return 0;
        }
      });
    }
    return data;
  });

  findingCols = ['severity', 'cvss_score', 'cve_id', 'package_name', 'ecosystem'];
  depCols     = ['ecosystem', 'package_name', 'package_version'];

  ngOnInit() {
    this.projectId = Number(this.route.snapshot.paramMap.get('projectId'));
    const reportId = Number(this.route.snapshot.paramMap.get('reportId'));

    this.svc.getReport(reportId).subscribe(r => this.report.set(r));
    this.svc.getFindings(reportId).subscribe(fs => this.findings.set(fs));
    this.svc.getDependencies(reportId).subscribe(ds => this.dependencies.set(ds));
  }

  toggleFilter(severity: SeverityFilter) {
    this.activeFilter.set(this.activeFilter() === severity ? null : severity);
  }

  onSort(sort: Sort) {
    this.currentSort.set(sort);
  }

  openFinding(finding: ReportFinding) {
    this.dialog.open(FindingDetailDialogComponent, {
      data: finding,
      width: '600px',
    });
  }
}

function compare(a: number | string, b: number | string, isAsc: boolean) {
  return (a < b ? -1 : 1) * (isAsc ? 1 : -1);
}
