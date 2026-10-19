#!/usr/bin/env python3
"""Validate complete paired data, reproduce summaries and export result plots."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.ticker import NullLocator

ROOT = Path(__file__).resolve().parents[1]
KEY = ['family', 'size', 'param', 'seed']
CELL = KEY[:3]
ALGORITHMS = ['ek', 'dinic']
METRICS = ['scans', 'bfs', 'augmentations']
NAMES = {'ek': 'Edmonds–Karp', 'dinic': 'Dinic'}
COLORS = {'ek': '#bf641b', 'dinic': '#176c9c'}


def validate(data, plan, cpu=False):
    expected = {tuple(c[k] for k in KEY) + (a, r)
                for c in plan['cases'] for a in ALGORITHMS
                for r in range(plan['repetitions'])}
    actual = set(map(tuple, data[KEY + ['algo', 'repeat']].to_numpy()))
    if actual != expected or len(data) != len(expected):
        raise ValueError('Data must contain every planned algorithm and repeat exactly once')
    for column in ['time_us'] + (['cpu_time_us'] if cpu else []):
        if not (np.isfinite(data[column]) & (data[column] > 0)).all():
            raise ValueError('Timings must be finite and positive')
    batches = {tuple(c[k] for k in KEY): c.get('batch', plan['batch']) for c in plan['cases']}
    if not all(row.batch == batches[tuple(getattr(row, k) for k in KEY)]
               for row in data.itertuples()):
        raise ValueError('Batch sizes differ from the plan')
    for column in ['flow', 'edges', 'graph_hash']:
        if not (data.groupby(KEY)[column].nunique() == 1).all():
            raise ValueError(f'Input or result disagreement: {column}')
    if not (data.groupby(KEY + ['algo'])[METRICS].nunique() == 1).all().all():
        raise ValueError('Operation counts change between repeats')


def paired_ratios(groups, metric):
    wide = groups.pivot(index=KEY, columns='algo', values=metric).reset_index()
    rng = np.random.default_rng(20260930)
    rows = []
    for cell, block in wide.groupby(CELL):
        logs = np.log(block.ek.to_numpy() / block.dinic.to_numpy())
        samples = np.exp(rng.choice(logs, size=(10000, len(logs)), replace=True).mean(axis=1))
        rows.append(dict(zip(CELL, cell)) | {
            'metric': metric, 'numerator': 'ek', 'denominator': 'dinic',
            'ratio': float(np.exp(logs.mean())), 'ci_low': float(np.quantile(samples, .025)),
            'ci_high': float(np.quantile(samples, .975)), 'seeds': len(logs)})
    return pd.DataFrame(rows)


def spread(series):
    return {'median': float(series.median()), 'p95': float(series.quantile(.95)),
            'max': float(series.max())}


def plot_family(summary, family, parameter, figures):
    fig, ax = plt.subplots(figsize=(7.5, 4.2))
    for algorithm in ALGORITHMS:
        block = summary[(summary.family == family) & (summary.param == parameter)
                        & (summary.algo == algorithm)].sort_values('size')
        ax.plot(block['size'], block.median_us, 'o-', label=NAMES[algorithm], color=COLORS[algorithm])
        ax.fill_between(block['size'], block.seed_min_us, block.seed_max_us,
                        color=COLORS[algorithm], alpha=.13)
    ax.set(xscale='log', yscale='log', xlabel='Vertices', ylabel='Solve time (µs)',
           title=f'{family.capitalize()} inputs   parameter {parameter}')
    sizes = sorted(summary.loc[summary.family == family, 'size'].unique())
    ax.set_xticks(sizes, labels=[str(size) for size in sizes])
    ax.xaxis.set_minor_locator(NullLocator())
    ax.grid(alpha=.2); ax.legend(); fig.tight_layout()
    fig.savefig(figures / f'{family}-scaling.png'); plt.close(fig)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--data-dir', type=Path, default=ROOT / 'results')
    parser.add_argument('--figures-dir', type=Path)
    args = parser.parse_args()
    results = args.data_dir.resolve()
    figures = args.figures_dir or (ROOT / 'figures' if results == ROOT / 'results' else results / 'figures')
    figures.mkdir(parents=True, exist_ok=True)
    plan = json.loads((results / 'experiment-plan.json').read_text())
    data = pd.read_csv(results / 'full-raw.csv')
    validate(data, plan)
    groups = data.groupby(KEY + ['algo'], as_index=False).agg(
        time_us=('time_us', 'median'), min_us=('time_us', 'min'), max_us=('time_us', 'max'),
        edges=('edges', 'first'), flow=('flow', 'first'),
        **{m: (m, 'first') for m in METRICS})
    groups.to_csv(results / 'instance-summary.csv', index=False)
    ratios = paired_ratios(groups, 'time_us')
    ratios.to_csv(results / 'paired-ratios.csv', index=False)
    summary = groups.groupby(CELL + ['algo'], as_index=False).agg(
        median_us=('time_us', 'median'), seed_min_us=('time_us', 'min'), seed_max_us=('time_us', 'max'),
        median_scans=('scans', 'median'), median_bfs=('bfs', 'median'), median_flow=('flow', 'median'),
        edges=('edges', 'median'))
    summary.to_csv(results / 'cell-summary.csv', index=False)
    facts = {'instances': len(plan['cases']), 'cells': len(ratios), 'raw_rows': len(data),
             'timed_solves': int(data.batch.sum()), 'repeat_max_min': spread(groups.max_us / groups.min_us),
             'zero_scan_groups': int((groups.scans == 0).sum()),
             'raw_sha256': hashlib.sha256((results / 'full-raw.csv').read_bytes()).hexdigest(),
             'winning_instances': groups.loc[groups.groupby(KEY).time_us.idxmin()].algo.value_counts().to_dict(),
             'interval_method': '10000 paired seed bootstraps of geometric mean ratios, seed 20260930; exploratory 95% percentile intervals'}
    plt.rcParams.update({'font.size': 12, 'axes.spines.top': False, 'axes.spines.right': False, 'figure.dpi': 180})
    for family, parameter in {'sparse': .5, 'dense': .3, 'layered': .125, 'bipartite': .6}.items():
        plot_family(summary, family, parameter, figures)
    fig, ax = plt.subplots(figsize=(7.5, 4.2))
    for algorithm in ALGORITHMS:
        block = groups[(groups.algo == algorithm) & (groups.scans > 0)]
        ax.scatter(block.scans, block.time_us, s=14, alpha=.55, label=NAMES[algorithm], color=COLORS[algorithm])
    ax.set(xscale='log', yscale='log', xlabel='Residual arc inspections', ylabel='Median solve time (µs)',
           title='Residual exploration and elapsed time')
    ax.legend(); ax.grid(alpha=.2); fig.tight_layout(); fig.savefig(figures / 'scans-time.png'); plt.close(fig)
    (results / 'analysis-summary.json').write_text(json.dumps(facts, indent=2) + '\n')

    cpu_path = results / 'cpu-raw.csv'
    if cpu_path.exists():
        cpu = pd.read_csv(cpu_path); validate(cpu, plan, cpu=True)
        merged = cpu.merge(data, on=KEY + ['algo', 'repeat'], suffixes=('_cpu', '_main'), validate='one_to_one')
        for column in ['graph_hash', 'flow', 'edges', 'batch'] + METRICS:
            if not (merged[column + '_cpu'] == merged[column + '_main']).all():
                raise ValueError(f'CPU follow-up differs in {column}')
        cg = cpu.groupby(KEY + ['algo'], as_index=False).agg(
            cpu_us=('cpu_time_us', 'median'), wall_us=('time_us', 'median'),
            cpu_min=('cpu_time_us', 'min'), cpu_max=('cpu_time_us', 'max'),
            wall_min=('time_us', 'min'), wall_max=('time_us', 'max'), batch=('batch', 'first'))
        cg.to_csv(results / 'cpu-instance-summary.csv', index=False)
        pd.concat([paired_ratios(cg, m) for m in ('cpu_us', 'wall_us')]).to_csv(results / 'cpu-paired-ratios.csv', index=False)
        calibration = json.loads((results / 'cpu-clock-calibration.json').read_text())
        facts['cpu'] = {'rows': len(cpu), 'cpu_repeat_max_min': spread(cg.cpu_max / cg.cpu_min),
                        'wall_repeat_max_min': spread(cg.wall_max / cg.wall_min),
                        'groups_below_10x_resolution': int((cg.cpu_us * cg.batch < calibration['cpu_clock_resolution_ns'] / 100).sum())}
        (results / 'analysis-summary.json').write_text(json.dumps(facts, indent=2) + '\n')
    print(json.dumps(facts, indent=2))


if __name__ == '__main__':
    main()
