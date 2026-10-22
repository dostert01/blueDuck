import { Component, inject } from '@angular/core';
import { MAT_DIALOG_DATA, MatDialogModule } from '@angular/material/dialog';
import { MatButtonModule } from '@angular/material/button';
import { MatChipsModule } from '@angular/material/chips';
import { MatIconModule } from '@angular/material/icon';
import { NgClass, DatePipe, DecimalPipe } from '@angular/common';
import { ReportFinding } from '../../../core/models/project.model';

@Component({
  selector: 'bd-finding-detail-dialog',
  standalone: true,
  imports: [
    MatDialogModule, MatButtonModule, MatChipsModule, MatIconModule,
    NgClass, DatePipe, DecimalPipe,
  ],
  template: `
    <h2 mat-dialog-title>
      <code>{{ data.cve_id }}</code>
      <mat-chip [ngClass]="'severity-' + data.severity.toLowerCase()">
        {{ data.severity }}
      </mat-chip>
    </h2>

    <mat-dialog-content>
      <div class="meta-grid">
        <div class="meta-item">
          <div class="meta-label">Package</div>
          <div class="meta-value">
            {{ data.package_name }}
            @if (data.package_version) {
              <span class="version">{{'@'}}{{ data.package_version }}</span>
            }
          </div>
        </div>
        <div class="meta-item">
          <div class="meta-label">Ecosystem</div>
          <div class="meta-value">{{ data.ecosystem }}</div>
        </div>
        <div class="meta-item">
          <div class="meta-label">CVSS Score</div>
          <div class="meta-value">
            {{ data.cvss_score | number:'1.1-1' }}
            @if (data.cvss_v3_score) {
              <span class="sub">(v3: {{ data.cvss_v3_score | number:'1.1-1' }})</span>
            }
            @if (data.cvss_v2_score) {
              <span class="sub">(v2: {{ data.cvss_v2_score | number:'1.1-1' }})</span>
            }
          </div>
        </div>
        @if (data.published_at) {
          <div class="meta-item">
            <div class="meta-label">Published</div>
            <div class="meta-value">{{ data.published_at | date:'mediumDate' }}</div>
          </div>
        }
        @if (data.modified_at) {
          <div class="meta-item">
            <div class="meta-label">Last Modified</div>
            <div class="meta-value">{{ data.modified_at | date:'mediumDate' }}</div>
          </div>
        }
      </div>

      @if (data.description) {
        <h3>Description</h3>
        <p class="description">{{ data.description }}</p>
      }

      <h3>External References</h3>
      <div class="links">
        <a [href]="nvdUrl" target="_blank" rel="noopener">
          <mat-icon>open_in_new</mat-icon> View on NVD
        </a>
        <a [href]="cveOrgUrl" target="_blank" rel="noopener">
          <mat-icon>open_in_new</mat-icon> View on CVE.org
        </a>
      </div>
    </mat-dialog-content>

    <mat-dialog-actions align="end">
      <button mat-button mat-dialog-close>Close</button>
    </mat-dialog-actions>
  `,
  styles: [`
    h2[mat-dialog-title] { display: flex; align-items: center; gap: 12px; }
    code { font-size: 1.1em; }
    .meta-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px 24px; margin-bottom: 16px; }
    .meta-label { font-size: 0.8em; color: #666; text-transform: uppercase; }
    .meta-value { font-size: 1em; font-weight: 500; }
    .version { color: #666; margin-left: 2px; }
    .sub { color: #666; font-weight: 400; font-size: 0.9em; }
    .description { line-height: 1.6; color: #333; }
    h3 { margin: 16px 0 8px; }
    .links { display: flex; flex-direction: column; gap: 8px; }
    .links a { display: flex; align-items: center; gap: 6px; color: #1565c0;
               text-decoration: none; font-weight: 500; }
    .links a:hover { text-decoration: underline; }
    mat-chip.severity-critical { --mdc-chip-label-text-color: white; background: #d32f2f; }
    mat-chip.severity-high     { --mdc-chip-label-text-color: white; background: #f57c00; }
    mat-chip.severity-medium   { --mdc-chip-label-text-color: black; background: #fbc02d; }
    mat-chip.severity-low      { --mdc-chip-label-text-color: white; background: #388e3c; }
    mat-chip.severity-none     { --mdc-chip-label-text-color: #333; background: #e0e0e0; }
  `]
})
export class FindingDetailDialogComponent {
  data = inject<ReportFinding>(MAT_DIALOG_DATA);

  get nvdUrl(): string {
    return `https://nvd.nist.gov/vuln/detail/${this.data.cve_id}`;
  }

  get cveOrgUrl(): string {
    return `https://www.cve.org/CVERecord?id=${this.data.cve_id}`;
  }
}
