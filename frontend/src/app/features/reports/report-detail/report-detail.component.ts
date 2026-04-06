import { Component, inject, signal, computed, OnInit } from '@angular/core';
import { ActivatedRoute, RouterLink } from '@angular/router';
import { MatTableModule } from '@angular/material/table';
import { MatChipsModule } from '@angular/material/chips';
import { MatCardModule } from '@angular/material/card';
import { MatIconModule } from '@angular/material/icon';
import { MatButtonModule } from '@angular/material/button';
import { MatSortModule, Sort } from '@angular/material/sort';
import { MatDialogModule, MatDialog } from '@angular/material/dialog';
import { MatTabsModule } from '@angular/material/tabs';
import { MatCheckboxModule } from '@angular/material/checkbox';
import { NgClass, DatePipe, DecimalPipe } from '@angular/common';
import { ProjectService } from '../../../core/services/project.service';
import { Report, ReportFinding, ReportDependency } from '../../../core/models/project.model';
import { MatTooltipModule } from '@angular/material/tooltip';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { FormsModule } from '@angular/forms';
import { FindingDetailDialogComponent } from '../finding-detail-dialog/finding-detail-dialog.component';
import { MitigationDialogComponent, MitigationDialogData } from '../mitigation-dialog/mitigation-dialog.component';
import { MitigationLabelPipe } from '../../../core/pipes/mitigation-label.pipe';

type SeverityFilter = 'CRITICAL' | 'HIGH' | 'MEDIUM' | 'LOW' | null;

@Component({
  selector: 'bd-report-detail',
  standalone: true,
  imports: [
    RouterLink, MatTableModule, MatChipsModule, MatCardModule,
    MatIconModule, MatButtonModule, MatSortModule, MatDialogModule,
    MatTabsModule, MatTooltipModule, MatSlideToggleModule, MatCheckboxModule,
    FormsModule, NgClass, DatePipe, DecimalPipe, MitigationLabelPipe,
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

      <mat-tab-group>
        <mat-tab label="Findings ({{ displayedFindings().length }})">
          <div class="tab-content">
            <div class="findings-header">
              <mat-slide-toggle [(ngModel)]="showMitigated"
                                (ngModelChange)="showMitigatedChanged()">
                Show mitigated
              </mat-slide-toggle>
              @if (selectedIds().size > 0) {
                <button mat-raised-button color="primary"
                        (click)="mitigateSelected()">
                  <mat-icon>verified_user</mat-icon>
                  Mitigate Selected ({{ selectedIds().size }})
                </button>
              }
            </div>
            <table mat-table [dataSource]="displayedFindings()" matSort (matSortChange)="onSort($event)"
                   class="mat-elevation-z2 full-width">

              <ng-container matColumnDef="select">
                <th mat-header-cell *matHeaderCellDef>
                  <mat-checkbox [checked]="allVisibleSelected()"
                                [indeterminate]="someVisibleSelected()"
                                (change)="toggleSelectAll($event.checked)">
                  </mat-checkbox>
                </th>
                <td mat-cell *matCellDef="let f">
                  @if (!f.mitigation_id) {
                    <mat-checkbox [checked]="selectedIds().has(f.id)"
                                  (change)="toggleSelect(f.id, $event.checked)"
                                  (click)="$event.stopPropagation()">
                    </mat-checkbox>
                  }
                </td>
              </ng-container>

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

              <ng-container matColumnDef="mitigation">
                <th mat-header-cell *matHeaderCellDef>Mitigation</th>
                <td mat-cell *matCellDef="let f">
                  @if (f.mitigation_id) {
                    <mat-chip class="mitigated" [matTooltip]="f.mitigation_description">
                      {{ f.mitigation_type | mitigationLabel }}
                    </mat-chip>
                  }
                </td>
              </ng-container>

              <ng-container matColumnDef="actions">
                <th mat-header-cell *matHeaderCellDef></th>
                <td mat-cell *matCellDef="let f">
                  @if (!f.mitigation_id) {
                    <button mat-icon-button matTooltip="Mitigate"
                            (click)="openMitigation(f); $event.stopPropagation()">
                      <mat-icon>verified_user</mat-icon>
                    </button>
                  } @else {
                    <button mat-icon-button matTooltip="Remove mitigation" color="warn"
                            (click)="removeMitigation(f); $event.stopPropagation()">
                      <mat-icon>remove_circle_outline</mat-icon>
                    </button>
                  }
                </td>
              </ng-container>

              <tr mat-header-row *matHeaderRowDef="findingCols"></tr>
              <tr mat-row *matRowDef="let row; columns: findingCols;"
                  class="clickable-row" (click)="openFinding(row)"
                  [class.mitigated-row]="row.mitigation_id"></tr>
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
          </div>
        </mat-tab>

        <mat-tab label="All Dependencies ({{ dependencies().length }})">
          <div class="tab-content">
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
          </div>
        </mat-tab>
      </mat-tab-group>
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
    .tab-content { padding-top: 16px; }
    .findings-header { display: flex; align-items: center; gap: 16px; margin-bottom: 12px; }
    .filter-info { margin-bottom: 16px; display: flex; align-items: center; gap: 8px; }
    mat-chip.severity-critical { --mdc-chip-label-text-color: white; background: #d32f2f; }
    mat-chip.severity-high     { --mdc-chip-label-text-color: white; background: #f57c00; }
    mat-chip.severity-medium   { --mdc-chip-label-text-color: black; background: #fbc02d; }
    mat-chip.severity-low      { --mdc-chip-label-text-color: white; background: #388e3c; }
    mat-chip.severity-none     { --mdc-chip-label-text-color: #333; background: #e0e0e0; }
    mat-chip.mitigated         { --mdc-chip-label-text-color: white; background: #1565c0; }
    .mitigated-row             { opacity: 0.6; }
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
  showMitigatedSignal = signal(false);
  showMitigated = false;
  selectedIds  = signal<Set<number>>(new Set());
  private currentSort = signal<Sort>({ active: '', direction: '' });

  displayedFindings = computed(() => {
    let data = [...this.findings()];
    if (!this.showMitigatedSignal()) {
      data = data.filter(f => !f.mitigation_id);
    }
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

  allVisibleSelected = computed(() => {
    const unmitigated = this.displayedFindings().filter(f => !f.mitigation_id);
    if (unmitigated.length === 0) return false;
    const sel = this.selectedIds();
    return unmitigated.every(f => sel.has(f.id));
  });

  someVisibleSelected = computed(() => {
    const unmitigated = this.displayedFindings().filter(f => !f.mitigation_id);
    const sel = this.selectedIds();
    const count = unmitigated.filter(f => sel.has(f.id)).length;
    return count > 0 && count < unmitigated.length;
  });

  findingCols = ['select', 'severity', 'cvss_score', 'cve_id', 'package_name', 'ecosystem', 'mitigation', 'actions'];
  depCols     = ['ecosystem', 'package_name', 'package_version'];

  ngOnInit() {
    this.projectId = Number(this.route.snapshot.paramMap.get('projectId'));
    const reportId = Number(this.route.snapshot.paramMap.get('reportId'));

    this.svc.getReport(reportId).subscribe(r => this.report.set(r));
    this.svc.getFindings(reportId).subscribe(fs => this.findings.set(fs));
    this.svc.getDependencies(reportId).subscribe(ds => this.dependencies.set(ds));
  }

  showMitigatedChanged() {
    this.showMitigatedSignal.set(this.showMitigated);
  }

  toggleFilter(severity: SeverityFilter) {
    this.activeFilter.set(this.activeFilter() === severity ? null : severity);
  }

  onSort(sort: Sort) {
    this.currentSort.set(sort);
  }

  toggleSelect(id: number, checked: boolean) {
    const next = new Set(this.selectedIds());
    if (checked) next.add(id); else next.delete(id);
    this.selectedIds.set(next);
  }

  toggleSelectAll(checked: boolean) {
    const next = new Set(this.selectedIds());
    for (const f of this.displayedFindings()) {
      if (f.mitigation_id) continue;
      if (checked) next.add(f.id); else next.delete(f.id);
    }
    this.selectedIds.set(next);
  }

  openFinding(finding: ReportFinding) {
    this.dialog.open(FindingDetailDialogComponent, {
      data: finding,
      width: '600px',
    });
  }

  openMitigation(finding: ReportFinding) {
    this.dialog.open(MitigationDialogComponent, {
      data: { findings: [finding] } as MitigationDialogData,
      width: '500px',
    }).afterClosed().subscribe(result => {
      if (result) {
        this.selectedIds.set(new Set());
        this.reloadFindings();
      }
    });
  }

  mitigateSelected() {
    const sel = this.selectedIds();
    const selected = this.findings().filter(f => sel.has(f.id));
    if (selected.length === 0) return;

    this.dialog.open(MitigationDialogComponent, {
      data: { findings: selected } as MitigationDialogData,
      width: '500px',
    }).afterClosed().subscribe(result => {
      if (result) {
        this.selectedIds.set(new Set());
        this.reloadFindings();
      }
    });
  }

  removeMitigation(finding: ReportFinding) {
    this.svc.deleteMitigation(finding.id).subscribe(() => this.reloadFindings());
  }

  private reloadFindings() {
    const reportId = Number(this.route.snapshot.paramMap.get('reportId'));
    this.svc.getFindings(reportId).subscribe(fs => this.findings.set(fs));
  }
}

function compare(a: number | string, b: number | string, isAsc: boolean) {
  return (a < b ? -1 : 1) * (isAsc ? 1 : -1);
}
