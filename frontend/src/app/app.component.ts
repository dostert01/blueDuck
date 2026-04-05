import { Component, inject, signal } from '@angular/core';
import { RouterOutlet, RouterLink, RouterLinkActive } from '@angular/router';
import { MatToolbarModule } from '@angular/material/toolbar';
import { MatButtonModule } from '@angular/material/button';
import { MatIconModule } from '@angular/material/icon';
import { MatSidenavModule } from '@angular/material/sidenav';
import { MatListModule } from '@angular/material/list';
import { MatSnackBar, MatSnackBarModule } from '@angular/material/snack-bar';
import { ProjectService } from './core/services/project.service';
import { CveSource } from './core/models/project.model';

@Component({
  selector: 'bd-root',
  standalone: true,
  imports: [
    RouterOutlet, RouterLink, RouterLinkActive,
    MatToolbarModule, MatButtonModule, MatIconModule,
    MatSidenavModule, MatListModule, MatSnackBarModule,
  ],
  template: `
    <mat-sidenav-container class="sidenav-container">
      <mat-sidenav #sidenav mode="side" opened class="sidenav">
        <mat-nav-list>
          <a mat-list-item routerLink="/projects" routerLinkActive="active-link">
            <mat-icon matListItemIcon>folder</mat-icon>
            <span matListItemTitle>Projects</span>
          </a>
          <a mat-list-item routerLink="/cpe-mappings" routerLinkActive="active-link">
            <mat-icon matListItemIcon>swap_horiz</mat-icon>
            <span matListItemTitle>CPE Mappings</span>
          </a>
        </mat-nav-list>

        <div class="sidenav-footer">
          <mat-nav-list>
            @for (src of cveSources(); track src.name) {
              <a mat-list-item (click)="syncCveSource(src.name)">
                <mat-icon matListItemIcon>sync</mat-icon>
                <span matListItemTitle>Sync {{ src.name }}</span>
              </a>
            }
          </mat-nav-list>
        </div>
      </mat-sidenav>

      <mat-sidenav-content>
        <mat-toolbar color="primary">
          <span>blueDuck</span>
          <span class="bd-spacer"></span>
          <mat-icon>security</mat-icon>
        </mat-toolbar>
        <div class="content-area">
          <router-outlet></router-outlet>
        </div>
      </mat-sidenav-content>
    </mat-sidenav-container>
  `,
  styles: [`
    .sidenav-container { height: 100%; }
    .sidenav { width: 220px; padding-top: 8px; display: flex; flex-direction: column; }
    .sidenav-footer { margin-top: auto; border-top: 1px solid rgba(0,0,0,.12); }
    .content-area { padding: 24px; }
    .active-link { background: rgba(0,0,0,.08); }
  `]
})
export class AppComponent {
  private svc = inject(ProjectService);
  private snack = inject(MatSnackBar);

  cveSources = signal<CveSource[]>([]);

  constructor() {
    this.svc.getCveSources().subscribe(s => this.cveSources.set(s));
  }

  syncCveSource(name: string) {
    this.svc.syncCveSource(name).subscribe({
      next: () => this.snack.open(`${name} sync started`, 'OK', { duration: 3000 }),
      error: () => this.snack.open(`Failed to start ${name} sync`, 'OK', { duration: 3000 }),
    });
  }
}
