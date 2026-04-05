import { Component, inject } from '@angular/core';
import { FormBuilder, ReactiveFormsModule, Validators } from '@angular/forms';
import { MatDialogRef, MatDialogModule } from '@angular/material/dialog';
import { MatButtonModule } from '@angular/material/button';
import { MatInputModule } from '@angular/material/input';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatSnackBar, MatSnackBarModule } from '@angular/material/snack-bar';
import { ProjectService } from '../../../core/services/project.service';

@Component({
  selector: 'bd-new-project-dialog',
  standalone: true,
  imports: [
    ReactiveFormsModule, MatDialogModule, MatButtonModule,
    MatInputModule, MatFormFieldModule, MatSnackBarModule,
  ],
  template: `
    <h2 mat-dialog-title>New Project</h2>
    <mat-dialog-content>
      <form [formGroup]="form" class="dialog-form">
        <mat-form-field appearance="outline" class="full-width">
          <mat-label>Name</mat-label>
          <input matInput formControlName="name" placeholder="my-service" />
        </mat-form-field>

        <mat-form-field appearance="outline" class="full-width">
          <mat-label>Git URL</mat-label>
          <input matInput formControlName="git_url"
                 placeholder="https://github.com/org/repo.git" />
        </mat-form-field>

        <mat-form-field appearance="outline" class="full-width">
          <mat-label>Description</mat-label>
          <textarea matInput formControlName="description" rows="3"></textarea>
        </mat-form-field>
      </form>
    </mat-dialog-content>
    <mat-dialog-actions align="end">
      <button mat-button mat-dialog-close>Cancel</button>
      <button mat-raised-button color="primary"
              [disabled]="form.invalid || saving"
              (click)="save()">Create</button>
    </mat-dialog-actions>
  `,
  styles: [`.dialog-form { display: flex; flex-direction: column; }
            .full-width { width: 100%; }`]
})
export class NewProjectDialogComponent {
  private fb    = inject(FormBuilder);
  private svc   = inject(ProjectService);
  private ref   = inject(MatDialogRef<NewProjectDialogComponent>);
  private snack = inject(MatSnackBar);

  saving = false;

  form = this.fb.group({
    name:        ['', Validators.required],
    git_url:     ['', Validators.required],
    description: [''],
  });

  save() {
    if (this.form.invalid) return;
    this.saving = true;
    this.svc.createProject(this.form.value as any).subscribe({
      next: p => { this.ref.close(p); },
      error: () => {
        this.snack.open('Failed to create project', 'OK', { duration: 3000 });
        this.saving = false;
      }
    });
  }
}
