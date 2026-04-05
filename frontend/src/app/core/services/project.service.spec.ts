import { TestBed } from '@angular/core/testing';
import { HttpTestingController, provideHttpClientTesting } from '@angular/common/http/testing';
import { provideHttpClient } from '@angular/common/http';
import { ProjectService } from './project.service';
import { Project } from '../models/project.model';

describe('ProjectService', () => {
  let service: ProjectService;
  let httpMock: HttpTestingController;

  beforeEach(() => {
    TestBed.configureTestingModule({
      providers: [provideHttpClient(), provideHttpClientTesting(), ProjectService],
    });
    service = TestBed.inject(ProjectService);
    httpMock = TestBed.inject(HttpTestingController);
  });

  afterEach(() => httpMock.verify());

  it('should be created', () => {
    expect(service).toBeTruthy();
  });

  it('getProjects() calls GET /api/projects', () => {
    const mockProjects: Project[] = [
      {
        id: 1, name: 'test', git_url: 'https://github.com/test/test',
        description: '', created_at: '2024-01-01T00:00:00Z', last_synced_at: null
      }
    ];

    service.getProjects().subscribe(projects => {
      expect(projects.length).toBe(1);
      expect(projects[0].name).toBe('test');
    });

    const req = httpMock.expectOne('/api/projects');
    expect(req.request.method).toBe('GET');
    req.flush(mockProjects);
  });

  it('deleteProject() calls DELETE /api/projects/:id', () => {
    service.deleteProject(42).subscribe();

    const req = httpMock.expectOne('/api/projects/42');
    expect(req.request.method).toBe('DELETE');
    req.flush(null, { status: 204, statusText: 'No Content' });
  });

  it('syncProject() calls POST /api/projects/:id/sync', () => {
    service.syncProject(1).subscribe(r => {
      expect(r.status).toBe('sync started');
    });

    const req = httpMock.expectOne('/api/projects/1/sync');
    expect(req.request.method).toBe('POST');
    req.flush({ status: 'sync started' });
  });
});
