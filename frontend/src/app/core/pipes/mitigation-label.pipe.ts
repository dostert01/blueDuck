import { Pipe, PipeTransform } from '@angular/core';

const LABELS: Record<string, string> = {
  NOT_APPLICABLE: 'Not Applicable',
  FALSE_FINDING: 'False Finding',
  MITIGATED_BY_PROJECT: 'Mitigated',
};

@Pipe({ name: 'mitigationLabel', standalone: true })
export class MitigationLabelPipe implements PipeTransform {
  transform(value: string): string {
    return LABELS[value] || value;
  }
}
