import { Component, inject, signal, OnInit } from '@angular/core';
import { Router } from '@angular/router';
import { MatTableModule } from '@angular/material/table';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatDialogModule, MatDialog } from '@angular/material/dialog';
import { MatSnackBar, MatSnackBarModule } from '@angular/material/snack-bar';
import { MatTooltipModule } from '@angular/material/tooltip';
import { DatePipe } from '@angular/common';
import { ProjectService } from '../../../core/services/project.service';
import { Project } from '../../../core/models/project.model';
import { NewProjectDialogComponent } from '../new-project-dialog/new-project-dialog.component';

@Component({
  selector: 'bd-project-list',
  standalone: true,
  imports: [
    MatTableModule, MatButtonModule, MatIconModule,
    MatDialogModule, MatSnackBarModule, MatTooltipModule, DatePipe,
  ],
  template: `
    <div class="page-header">
      <h1>Projects</h1>
      <button mat-raised-button color="primary" (click)="openNewProject()">
        <mat-icon>add</mat-icon> New Project
      </button>
    </div>

    <table mat-table [dataSource]="projects()" class="mat-elevation-z2 full-width">
      <ng-container matColumnDef="name">
        <th mat-header-cell *matHeaderCellDef>Name</th>
        <td mat-cell *matCellDef="let p">
          <a (click)="navigate(p)" class="project-link">{{ p.name }}</a>
        </td>
      </ng-container>

      <ng-container matColumnDef="git_url">
        <th mat-header-cell *matHeaderCellDef>Git URL</th>
        <td mat-cell *matCellDef="let p">{{ p.git_url }}</td>
      </ng-container>

      <ng-container matColumnDef="last_synced_at">
        <th mat-header-cell *matHeaderCellDef>Last Synced</th>
        <td mat-cell *matCellDef="let p">
          {{ p.last_synced_at ? (p.last_synced_at | date:'short') : '—' }}
        </td>
      </ng-container>

      <ng-container matColumnDef="actions">
        <th mat-header-cell *matHeaderCellDef></th>
        <td mat-cell *matCellDef="let p">
          <button mat-icon-button matTooltip="Sync" (click)="sync(p); $event.stopPropagation()">
            <mat-icon>sync</mat-icon>
          </button>
          <button mat-icon-button matTooltip="Delete" color="warn"
                  (click)="delete(p); $event.stopPropagation()">
            <mat-icon>delete</mat-icon>
          </button>
        </td>
      </ng-container>

      <tr mat-header-row *matHeaderRowDef="columns"></tr>
      <tr mat-row *matRowDef="let row; columns: columns;" (click)="navigate(row)"
          class="clickable-row"></tr>
    </table>
  `,
  styles: [`
    .page-header { display: flex; align-items: center; gap: 16px; margin-bottom: 16px; }
    h1 { margin: 0; }
    .full-width { width: 100%; }
    .project-link { cursor: pointer; color: inherit; text-decoration: underline; }
    .clickable-row { cursor: pointer; }
    .clickable-row:hover { background: rgba(0,0,0,.04); }
  `]
})
export class ProjectListComponent implements OnInit {
  private svc    = inject(ProjectService);
  private router = inject(Router);
  private dialog = inject(MatDialog);
  private snack  = inject(MatSnackBar);

  projects = signal<Project[]>([]);
  columns  = ['name', 'git_url', 'last_synced_at', 'actions'];

  ngOnInit() { this.load(); }

  load() {
    this.svc.getProjects().subscribe(ps => this.projects.set(ps));
  }

  navigate(p: Project) {
    this.router.navigate(['/projects', p.id]);
  }

  openNewProject() {
    this.dialog.open(NewProjectDialogComponent, { width: '480px' })
      .afterClosed().subscribe(result => {
        if (result) this.load();
      });
  }

  sync(p: Project) {
    this.svc.syncProject(p.id).subscribe({
      next: () => this.snack.open(`Sync started for "${p.name}"`, 'OK', { duration: 3000 }),
      error: () => this.snack.open('Sync failed', 'OK', { duration: 3000 }),
    });
  }

  delete(p: Project) {
    if (!confirm(`Delete project "${p.name}"?`)) return;
    this.svc.deleteProject(p.id).subscribe({
      next: () => { this.snack.open('Deleted', 'OK', { duration: 2000 }); this.load(); },
      error: () => this.snack.open('Delete failed', 'OK', { duration: 3000 }),
    });
  }
}
