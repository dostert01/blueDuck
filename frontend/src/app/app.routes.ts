import { Routes } from '@angular/router';

export const routes: Routes = [
  {
    path: '',
    redirectTo: 'projects',
    pathMatch: 'full'
  },
  {
    path: 'projects',
    loadComponent: () =>
      import('./features/projects/project-list/project-list.component')
        .then(m => m.ProjectListComponent)
  },
  {
    path: 'projects/:projectId',
    loadComponent: () =>
      import('./features/projects/project-detail/project-detail.component')
        .then(m => m.ProjectDetailComponent)
  },
  {
    path: 'projects/:projectId/versions/:versionId/reports/:reportId',
    loadComponent: () =>
      import('./features/reports/report-detail/report-detail.component')
        .then(m => m.ReportDetailComponent)
  },
  {
    path: 'cpe-mappings',
    loadComponent: () =>
      import('./features/cpe-mappings/cpe-mapping-list.component')
        .then(m => m.CpeMappingListComponent)
  },
  {
    path: '**',
    redirectTo: 'projects'
  }
];
