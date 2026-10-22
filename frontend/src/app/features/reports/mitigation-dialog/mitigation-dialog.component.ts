import { Component, inject } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MAT_DIALOG_DATA, MatDialogRef, MatDialogModule } from '@angular/material/dialog';
import { MatButtonModule } from '@angular/material/button';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';
import { MatSnackBar, MatSnackBarModule } from '@angular/material/snack-bar';
import { ProjectService } from '../../../core/services/project.service';
import { ReportFinding, MitigationType } from '../../../core/models/project.model';

export interface MitigationDialogData {
  findings: ReportFinding[];
}

@Component({
  selector: 'bd-mitigation-dialog',
  standalone: true,
  imports: [
    FormsModule, MatDialogModule, MatButtonModule,
    MatFormFieldModule, MatInputModule, MatSelectModule, MatSnackBarModule,
  ],
  template: `
    @if (data.findings.length === 1) {
      <h2 mat-dialog-title>Mitigate {{ data.findings[0].cve_id }}</h2>
    } @else {
      <h2 mat-dialog-title>Mitigate {{ data.findings.length }} findings</h2>
    }

    <mat-dialog-content>
      @if (data.findings.length === 1) {
        <p class="context">
          <strong>{{ data.findings[0].package_name }}</strong>
          @if (data.findings[0].package_version) { {{'@'}}{{ data.findings[0].package_version }} }
          — {{ data.findings[0].severity }} ({{ data.findings[0].cvss_score }})
        </p>
      } @else {
        <p class="context">
          Applying mitigation to <strong>{{ data.findings.length }}</strong> selected findings.
        </p>
      }

      <mat-form-field appearance="outline" class="full-width">
        <mat-label>Mitigation Type</mat-label>
        <mat-select [(ngModel)]="type">
          <mat-option value="NOT_APPLICABLE">Not Applicable</mat-option>
          <mat-option value="FALSE_FINDING">False Finding</mat-option>
          <mat-option value="MITIGATED_BY_PROJECT">Mitigated by Project</mat-option>
        </mat-select>
      </mat-form-field>

      <mat-form-field appearance="outline" class="full-width">
        <mat-label>Description / Justification</mat-label>
        <textarea matInput [(ngModel)]="description" rows="4"
                  placeholder="Explain why this finding is mitigated..."></textarea>
      </mat-form-field>
    </mat-dialog-content>

    <mat-dialog-actions align="end">
      <button mat-button mat-dialog-close>Cancel</button>
      <button mat-raised-button color="primary"
              [disabled]="!type || !description.trim() || saving"
              (click)="save()">
        {{ saving ? 'Saving...' : 'Save Mitigation' }}
      </button>
    </mat-dialog-actions>
  `,
  styles: [`
    .full-width { width: 100%; }
    .context { color: #666; margin: 0 0 16px; }
  `]
})
export class MitigationDialogComponent {
  private svc   = inject(ProjectService);
  private ref   = inject(MatDialogRef<MitigationDialogComponent>);
  private snack = inject(MatSnackBar);
  data          = inject<MitigationDialogData>(MAT_DIALOG_DATA);

  type: MitigationType | '' = '';
  description = '';
  saving = false;

  save() {
    if (!this.type) return;
    this.saving = true;
    const payload = {
      type: this.type as MitigationType,
      description: this.description,
    };

    const ids = this.data.findings.map(f => f.id);
    const request$ = ids.length === 1
      ? this.svc.createMitigation(ids[0], payload)
      : this.svc.createBulkMitigation(ids, payload);

    request$.subscribe({
      next: m => this.ref.close(m),
      error: () => {
        this.snack.open('Failed to save mitigation', 'OK', { duration: 3000 });
        this.saving = false;
      },
    });
  }
}
