import { Component, inject, signal, OnInit } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MatTableModule } from '@angular/material/table';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatInputModule } from '@angular/material/input';
import { MatSnackBar, MatSnackBarModule } from '@angular/material/snack-bar';
import { ProjectService } from '../../core/services/project.service';
import { CpeMapping } from '../../core/models/project.model';

@Component({
  selector: 'bd-cpe-mapping-list',
  standalone: true,
  imports: [
    FormsModule, MatTableModule, MatButtonModule, MatIconModule,
    MatFormFieldModule, MatInputModule, MatSnackBarModule,
  ],
  template: `
    <div class="page-header">
      <h1>CPE Mappings</h1>
      <button mat-raised-button color="primary" (click)="startAdd()">
        <mat-icon>add</mat-icon> Add Mapping
      </button>
    </div>

    <table mat-table [dataSource]="mappings()" class="mat-elevation-z2 full-width">

      <ng-container matColumnDef="ecosystem">
        <th mat-header-cell *matHeaderCellDef>Ecosystem</th>
        <td mat-cell *matCellDef="let m">
          @if (editingId() === m.id) {
            <mat-form-field class="cell-field">
              <input matInput [(ngModel)]="editRow.ecosystem">
            </mat-form-field>
          } @else {
            {{ m.ecosystem }}
          }
        </td>
      </ng-container>

      <ng-container matColumnDef="package_name">
        <th mat-header-cell *matHeaderCellDef>Package Name</th>
        <td mat-cell *matCellDef="let m">
          @if (editingId() === m.id) {
            <mat-form-field class="cell-field">
              <input matInput [(ngModel)]="editRow.package_name">
            </mat-form-field>
          } @else {
            {{ m.package_name }}
          }
        </td>
      </ng-container>

      <ng-container matColumnDef="cpe_vendor">
        <th mat-header-cell *matHeaderCellDef>CPE Vendor</th>
        <td mat-cell *matCellDef="let m">
          @if (editingId() === m.id) {
            <mat-form-field class="cell-field">
              <input matInput [(ngModel)]="editRow.cpe_vendor">
            </mat-form-field>
          } @else {
            {{ m.cpe_vendor }}
          }
        </td>
      </ng-container>

      <ng-container matColumnDef="cpe_product">
        <th mat-header-cell *matHeaderCellDef>CPE Product</th>
        <td mat-cell *matCellDef="let m">
          @if (editingId() === m.id) {
            <mat-form-field class="cell-field">
              <input matInput [(ngModel)]="editRow.cpe_product">
            </mat-form-field>
          } @else {
            {{ m.cpe_product }}
          }
        </td>
      </ng-container>

      <ng-container matColumnDef="git_url_pattern">
        <th mat-header-cell *matHeaderCellDef>Git URL Pattern</th>
        <td mat-cell *matCellDef="let m">
          @if (editingId() === m.id) {
            <mat-form-field class="cell-field">
              <input matInput [(ngModel)]="editRow.git_url_pattern">
            </mat-form-field>
          } @else {
            {{ m.git_url_pattern }}
          }
        </td>
      </ng-container>

      <ng-container matColumnDef="actions">
        <th mat-header-cell *matHeaderCellDef>Actions</th>
        <td mat-cell *matCellDef="let m">
          @if (editingId() === m.id) {
            <button mat-icon-button color="primary" (click)="save(m.id)"
                    title="Save">
              <mat-icon>check</mat-icon>
            </button>
            <button mat-icon-button (click)="cancelEdit()" title="Cancel">
              <mat-icon>close</mat-icon>
            </button>
          } @else {
            <button mat-icon-button (click)="startEdit(m)" title="Edit">
              <mat-icon>edit</mat-icon>
            </button>
            <button mat-icon-button color="warn" (click)="remove(m.id)"
                    title="Delete">
              <mat-icon>delete</mat-icon>
            </button>
          }
        </td>
      </ng-container>

      <tr mat-header-row *matHeaderRowDef="columns"></tr>
      <tr mat-row *matRowDef="let row; columns: columns;"
          [class.editing]="editingId() === row.id"></tr>
    </table>

    @if (adding()) {
      <div class="add-form mat-elevation-z2">
        <h3>New Mapping</h3>
        <div class="form-row">
          <mat-form-field>
            <mat-label>Ecosystem</mat-label>
            <input matInput [(ngModel)]="newRow.ecosystem">
          </mat-form-field>
          <mat-form-field>
            <mat-label>Package Name</mat-label>
            <input matInput [(ngModel)]="newRow.package_name">
          </mat-form-field>
          <mat-form-field>
            <mat-label>CPE Vendor</mat-label>
            <input matInput [(ngModel)]="newRow.cpe_vendor">
          </mat-form-field>
          <mat-form-field>
            <mat-label>CPE Product</mat-label>
            <input matInput [(ngModel)]="newRow.cpe_product">
          </mat-form-field>
          <mat-form-field>
            <mat-label>Git URL Pattern</mat-label>
            <input matInput [(ngModel)]="newRow.git_url_pattern">
          </mat-form-field>
        </div>
        <div class="form-actions">
          <button mat-raised-button color="primary" (click)="saveNew()">Save</button>
          <button mat-button (click)="adding.set(false)">Cancel</button>
        </div>
      </div>
    }

    @if (mappings().length === 0) {
      <p class="empty-state">No CPE mappings configured yet.</p>
    }
  `,
  styles: [`
    .page-header { display: flex; align-items: center; gap: 16px; margin-bottom: 16px; }
    h1 { margin: 0; }
    .full-width { width: 100%; }
    .cell-field { width: 100%; }
    .editing { background: #e3f2fd; }
    .add-form { margin-top: 24px; padding: 16px; border-radius: 4px; }
    .add-form h3 { margin-top: 0; }
    .form-row { display: flex; gap: 12px; flex-wrap: wrap; }
    .form-row mat-form-field { flex: 1; min-width: 160px; }
    .form-actions { display: flex; gap: 8px; margin-top: 8px; }
    .empty-state { color: #666; font-style: italic; margin-top: 16px; }
  `]
})
export class CpeMappingListComponent implements OnInit {
  private svc   = inject(ProjectService);
  private snack = inject(MatSnackBar);

  mappings   = signal<CpeMapping[]>([]);
  editingId  = signal<number | null>(null);
  adding     = signal(false);

  columns = ['ecosystem', 'package_name', 'cpe_vendor', 'cpe_product', 'git_url_pattern', 'actions'];

  editRow = { ecosystem: '', package_name: '', cpe_vendor: '', cpe_product: '', git_url_pattern: '' };
  newRow  = { ecosystem: '', package_name: '', cpe_vendor: '', cpe_product: '', git_url_pattern: '' };

  ngOnInit() {
    this.load();
  }

  load() {
    this.svc.getCpeMappings().subscribe(m => this.mappings.set(m));
  }

  startAdd() {
    this.cancelEdit();
    this.newRow = { ecosystem: '', package_name: '', cpe_vendor: '', cpe_product: '', git_url_pattern: '' };
    this.adding.set(true);
  }

  saveNew() {
    if (!this.newRow.ecosystem || !this.newRow.package_name ||
        !this.newRow.cpe_vendor || !this.newRow.cpe_product) {
      this.snack.open('Ecosystem, package name, CPE vendor, and CPE product are required', 'OK', { duration: 3000 });
      return;
    }
    this.svc.createCpeMapping(this.newRow).subscribe({
      next: () => { this.adding.set(false); this.load(); },
      error: () => this.snack.open('Failed to create mapping', 'OK', { duration: 3000 }),
    });
  }

  startEdit(m: CpeMapping) {
    this.adding.set(false);
    this.editingId.set(m.id);
    this.editRow = {
      ecosystem: m.ecosystem,
      package_name: m.package_name,
      cpe_vendor: m.cpe_vendor,
      cpe_product: m.cpe_product,
      git_url_pattern: m.git_url_pattern,
    };
  }

  cancelEdit() {
    this.editingId.set(null);
  }

  save(id: number) {
    if (!this.editRow.ecosystem || !this.editRow.package_name ||
        !this.editRow.cpe_vendor || !this.editRow.cpe_product) {
      this.snack.open('Ecosystem, package name, CPE vendor, and CPE product are required', 'OK', { duration: 3000 });
      return;
    }
    this.svc.updateCpeMapping(id, this.editRow).subscribe({
      next: () => { this.editingId.set(null); this.load(); },
      error: () => this.snack.open('Failed to update mapping', 'OK', { duration: 3000 }),
    });
  }

  remove(id: number) {
    this.svc.deleteCpeMapping(id).subscribe({
      next: () => this.load(),
      error: () => this.snack.open('Failed to delete mapping', 'OK', { duration: 3000 }),
    });
  }
}
