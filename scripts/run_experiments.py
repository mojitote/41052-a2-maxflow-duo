#!/usr/bin/env python3
"""Execute the fixed matrix and preserve each new collection in its own directory."""
import argparse
import datetime
import hashlib
import json
import platform
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--algorithms', choices=['ek', 'pair'], default='ek')
    parser.add_argument('--clock', choices=['wall', 'cpu'], default='wall')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--smoke', action='store_true')
    parser.add_argument('--compiler', default='clang++', help='Compiler used for the selected binary')
    args = parser.parse_args()
    output = args.output.resolve()
    executable = ROOT / 'build' / ('benchmark_cpu' if args.clock == 'cpu' else 'benchmark')
    if args.algorithms == 'pair':
        executable = executable.with_name(executable.name + '_pair')
    if not executable.exists():
        raise RuntimeError(f'Build the selected benchmark first: {executable}')
    plan = json.loads((ROOT / 'results/experiment-plan.json').read_text())
    output.mkdir(parents=True, exist_ok=True)
    target = output / ('cpu-raw.csv' if args.clock == 'cpu' else 'full-raw.csv')
    if target.exists():
        raise RuntimeError(f'Choose a fresh output directory or clock: {target} already exists')
    cases = plan['cases']
    if args.smoke:
        cases = [next(c for c in cases if c['family'] == f) for f in ['sparse', 'dense', 'layered', 'bipartite']]
        plan = dict(plan, cases=cases, repetitions=2)
    destination_plan = output / 'experiment-plan.json'
    if destination_plan.exists() and json.loads(destination_plan.read_text()) != plan:
        raise RuntimeError('Output directory has a different experiment plan')
    destination_plan.write_text(json.dumps(plan, indent=2) + '\n')
    if args.clock == 'cpu':
        calibration = subprocess.check_output([str(executable), '--calibrate'], text=True)
        json.loads(calibration)
        (output / 'cpu-clock-calibration.json').write_text(calibration)
    metadata = {'started_at_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'platform': platform.platform(), 'machine': platform.machine(),
                'compiler': subprocess.check_output([args.compiler, '--version'], text=True),
                'binary_sha256': digest(executable), 'algorithms': ['ek', 'dinic'] if args.algorithms == 'pair' else ['ek'],
                'plan_sha256': digest(destination_plan), 'repetitions': plan['repetitions'],
                'cases_requested': len(cases), 'cases_completed': 0,
                'source_sha256': {str(p.relative_to(ROOT)): digest(p) for folder in ['src', 'include']
                                  for p in sorted((ROOT / folder).glob('*')) if p.is_file()}}
    if platform.system() == 'Darwin':
        metadata['hardware'] = subprocess.check_output(['sysctl', '-n', 'machdep.cpu.brand_string', 'hw.memsize', 'hw.ncpu'], text=True).splitlines()
    environment_path = output / (args.clock + '-environment.json')
    environment_path.write_text(json.dumps(metadata, indent=2) + '\n')
    header_written = False
    with target.open('w') as stream:
        for index, case in enumerate(cases):
            command = [str(executable), case['family'], str(case['size']), str(case['param']), str(case['seed']),
                       str(plan['repetitions']), str(case.get('batch', plan['batch']))]
            run = subprocess.run(command, text=True, capture_output=True, timeout=120, check=True)
            lines = run.stdout.splitlines()
            stream.write('\n'.join(lines[1:] if header_written else lines) + '\n')
            stream.flush(); header_written = True
            metadata['cases_completed'] = index + 1
            if (index + 1) % 20 == 0 or index + 1 == len(cases):
                environment_path.write_text(json.dumps(metadata, indent=2) + '\n')
                print(f'{args.clock}: {index + 1}/{len(cases)} inputs', flush=True)
    metadata.update(completed_at_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    cases_completed=len(cases), raw_sha256=digest(target))
    environment_path.write_text(json.dumps(metadata, indent=2) + '\n')


if __name__ == '__main__':
    main()
