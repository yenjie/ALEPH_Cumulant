#!/usr/bin/env python3
import csv
import math
import re
from pathlib import Path

OUTPUT_DIR = Path('output')
DOCS_DIR = Path('docs')
FIG_DIR = DOCS_DIR / 'figures'
RESULT_CSV = FIG_DIR / 'pythia_shoving2x_5M_metrics.csv'
RESULT_MD = DOCS_DIR / 'pythia_shoving2x_5M.md'

CASES = [
    ('baseline', 'nominal_pt04', 'Nominal charged pT>0.4 GeV', 'v2'),
    ('eta_gap', 'etagap1p0', '|delta eta|>1.0', 'eta'),
    ('eta_gap', 'etagap1p6', '|delta eta|>1.6', 'eta'),
    ('eta_gap', 'etagap2p0', '|delta eta|>2.0', 'eta'),
    ('eta_gap', 'etagap2p5', '|delta eta|>2.5', 'eta'),
    ('eta_gap', 'etagap3p0', '|delta eta|>3.0', 'eta'),
    ('two_subevent', 'twosub0p0', 'two subevent, eta<0 vs eta>0', 'twosub'),
    ('two_subevent', 'twosub1p0', 'two subevent, gap 1.0', 'twosub'),
    ('two_subevent', 'twosub1p6', 'two subevent, gap 1.6', 'twosub'),
    ('two_subevent', 'twosub2p0', 'two subevent, gap 2.0', 'twosub'),
    ('track_selection', 'pt04to1', '0.4<pT<1 GeV', 'v2'),
    ('track_selection', 'pt04to2', '0.4<pT<2 GeV', 'v2'),
    ('track_selection', 'pt1to3', '1<pT<3 GeV', 'v2'),
    ('track_selection', 'positive', 'positive charged particles', 'v2'),
    ('track_selection', 'negative', 'negative charged particles', 'v2'),
    ('event_shape', 'thrustlt0p90', 'Thrust<0.90', 'v2'),
    ('event_shape', 'thrustlt0p85', 'Thrust<0.85', 'v2'),
    ('event_shape', 'sphgt0p12', 'Sphericity>0.12', 'v2'),
    ('event_shape', 'sphgt0p22', 'Sphericity>0.22', 'v2'),
]

OBS_LABELS = {
    'hV2_2': 'v2{2}',
    'hV2_4': 'v2{4}',
    'hV2_6': 'v2{6}',
    'hV2_8': 'v2{8}',
    'hV2TwoSub_2': 'two-sub v2{2}',
    'hV2TwoSub_4': 'two-sub v2{4}',
    'hV2TwoSub_6': 'two-sub v2{6}',
    'hV2TwoSub_8': 'two-sub v2{8}',
    'v2_gap': 'gap v2{2}',
}


def safe_float(value):
    try:
        out = float(value)
    except (TypeError, ValueError):
        return None
    if not math.isfinite(out):
        return None
    return out


def fmt(value, digits=4):
    if value is None or not math.isfinite(value):
        return 'n/a'
    return f'{value:.{digits}g}'


def add_metric(rows, technique, case, label, axis, observable, mult_bin,
               no_value, no_err, sh_value, sh_err, sh2_value, sh2_err):
    values = (no_value, no_err, sh_value, sh_err, sh2_value, sh2_err)
    if any(value is None for value in values):
        return
    if no_value <= 0.0 or sh_value <= 0.0 or sh2_value <= 0.0:
        return
    sigma_sh = math.hypot(no_err, sh_err)
    sigma_sh2 = math.hypot(no_err, sh2_err)
    if sigma_sh <= 0.0 or sigma_sh2 <= 0.0:
        return
    effect = sh_value - no_value
    effect2 = sh2_value - no_value
    scale = effect2 / effect if effect != 0.0 else None
    rows.append({
        'technique': technique,
        'case': case,
        'label': label,
        'axis': axis,
        'observable': OBS_LABELS.get(observable, observable),
        'multiplicity_bin': mult_bin,
        'no_shoving': no_value,
        'no_shoving_err': no_err,
        'shoving': sh_value,
        'shoving_err': sh_err,
        'shoving2x': sh2_value,
        'shoving2x_err': sh2_err,
        'shoving_over_no_shoving': sh_value / no_value,
        'shoving2x_over_no_shoving': sh2_value / no_value,
        'delta_shoving_over_sigma': effect / sigma_sh,
        'delta_shoving2x_over_sigma': effect2 / sigma_sh2,
        'effect_scale_2x_over_nominal': scale,
    })


def read_v2_csv(path, technique, case, label, rows):
    if not path.exists():
        return
    with path.open(newline='') as handle:
        for row in csv.DictReader(handle):
            add_metric(rows, technique, case, label, row['axis'], row['quantity'], row['multiplicity_bin'],
                       safe_float(row.get('noshoving')), safe_float(row.get('noshoving_err')),
                       safe_float(row.get('shoving')), safe_float(row.get('shoving_err')),
                       safe_float(row.get('shoving2x')), safe_float(row.get('shoving2x_err')))


def read_eta_csv(path, technique, case, label, rows):
    if not path.exists():
        return
    with path.open(newline='') as handle:
        for row in csv.DictReader(handle):
            add_metric(rows, technique, case, label, row['axis'], 'v2_gap', row['multiplicity_bin'],
                       safe_float(row.get('noshoving_v22_gap')), safe_float(row.get('noshoving_v22_gap_err')),
                       safe_float(row.get('shoving_v22_gap')), safe_float(row.get('shoving_v22_gap_err')),
                       safe_float(row.get('shoving2x_v22_gap')), safe_float(row.get('shoving2x_v22_gap_err')))


def csv_path(case, kind, axis):
    if kind == 'baseline':
        return OUTPUT_DIR / f'pythia_zpole_5M_noshoving_vs_shoving_vs_shoving2x_pt04_compare_{axis}.csv'
    return OUTPUT_DIR / f'pythia_zpole_5M_nonflow_{case}_noshoving_vs_shoving_vs_shoving2x_{axis}.csv'


def collect_rows():
    rows = []
    for technique, case, label, kind in CASES:
        for axis in ('beam', 'thrust'):
            if kind == 'eta':
                read_eta_csv(csv_path(case, 'nonflow', axis), technique, case, label, rows)
            else:
                read_v2_csv(csv_path(case, 'baseline' if case == 'nominal_pt04' else 'nonflow', axis),
                            technique, case, label, rows)
    return rows


def best_rows(rows):
    best = {}
    for row in rows:
        key = (row['technique'], row['case'], row['axis'], row['observable'])
        if key not in best or abs(row['delta_shoving2x_over_sigma']) > abs(best[key]['delta_shoving2x_over_sigma']):
            best[key] = row
    return list(best.values())


def parse_log(path):
    text = path.read_text(errors='replace') if path.exists() else ''
    info = {}
    m = re.search(r'Wrote\s+(\d+)\s+events.*with\s+(\d+)\s+visible particles and\s+(\d+)\s+StudyMult-selected charged particles', text)
    if m:
        info['events'], info['visible'], info['charged'] = m.groups()
    return info


def main():
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    rows = collect_rows()
    if not rows:
        raise SystemExit('No three-sample shoving2x CSV rows found. Run scripts/plot_pythia_shoving2x_comparison_5M.sh first.')

    fieldnames = [
        'technique', 'case', 'label', 'axis', 'observable', 'multiplicity_bin',
        'no_shoving', 'no_shoving_err', 'shoving', 'shoving_err',
        'shoving2x', 'shoving2x_err', 'shoving_over_no_shoving',
        'shoving2x_over_no_shoving', 'delta_shoving_over_sigma',
        'delta_shoving2x_over_sigma', 'effect_scale_2x_over_nominal',
    ]
    with RESULT_CSV.open('w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

    best = sorted(best_rows(rows), key=lambda r: (r['technique'], r['case'], r['axis'], r['observable']))
    sh2_3m_info = parse_log(OUTPUT_DIR / 'pythia_shoving2x_zpole_3M.log')
    sh2_ext_info = parse_log(OUTPUT_DIR / 'pythia_shoving2x_zpole_extra2M_seed67890.log')

    lines = []
    lines.append('# Pythia Shoving Factor-Two Study, 5M Events')
    lines.append('')
    lines.append('This document records the three-sample generator-level comparison of Pythia no-shoving, nominal Gleipnir shoving, and a factor-two shoving configuration.')
    lines.append('')
    lines.append('## Samples')
    lines.append('')
    lines.append('- Inputs: `output/pythia_noshoving_zpole_5M.root`, `output/pythia_shoving_zpole_5M.root`, and `output/pythia_shoving2x_zpole_5M.root`.')
    lines.append('- The nominal shoving sample uses `Gleipnir:repulsionFactor = 0.25`; the factor-two sample uses `Gleipnir:repulsionFactor = 0.50` with the other Gleipnir settings unchanged.')
    lines.append('- The factor-two sample is the merge of `output/pythia_shoving2x_zpole_3M.root` and `output/pythia_shoving2x_zpole_extra2M_seed67890.root`, and the merged tree was required to contain exactly 5,000,000 entries.')
    if sh2_3m_info:
        lines.append(f"- Seed-12345 factor-two chunk: {sh2_3m_info.get('events', 'unknown')} events, {sh2_3m_info.get('charged', 'unknown')} StudyMult-selected charged particles.")
    if sh2_ext_info:
        lines.append(f"- Seed-67890 factor-two extension: {sh2_ext_info.get('events', 'unknown')} events, {sh2_ext_info.get('charged', 'unknown')} StudyMult-selected charged particles.")
    lines.append('- All three configurations use the same hard process, visible-particle convention, multiplicity bins `0,10,15,20,25,30,35,40,999`, and 16-block jackknife summaries.')
    lines.append('')
    lines.append('## Commands')
    lines.append('')
    lines.append('```bash')
    lines.append('scripts/build_pythia_shoving2x_zpole_5M_sample.sh')
    lines.append('ALEPH_MAX_PARALLEL=16 CHUNKS=16 RUN_NONFLOW=1 scripts/run_pythia_shoving2x_5M_analysis.sh')
    lines.append('scripts/plot_pythia_shoving2x_comparison_5M.sh')
    lines.append('scripts/summarize_pythia_shoving2x_5M.py')
    lines.append('```')
    lines.append('')
    lines.append('## Result Summary')
    lines.append('')
    lines.append('The table gives the most significant factor-two shoving-minus-no-shoving bin for each technique, axis, and observable. Significances use `sqrt(err_no^2+err_sample^2)` and do not include covariance between generator configurations. The scale column is `(shoving2x-no shoving)/(nominal shoving-no shoving)` in the selected bin.')
    lines.append('')
    lines.append('| Technique | Case | Axis | Observable | Bin | Nominal/no | 2x/no | 2x significance | 2x/nominal effect |')
    lines.append('|---|---|---|---|---:|---:|---:|---:|---:|')
    for row in best:
        label = row['label'].replace('|', r'\|')
        observable = row['observable'].replace('|', r'\|')
        lines.append(
            f"| {row['technique']} | {label} | {row['axis']} | {observable} | "
            f"{row['multiplicity_bin'].replace('_', '--')} | "
            f"{fmt(row['shoving_over_no_shoving'], 5)} | "
            f"{fmt(row['shoving2x_over_no_shoving'], 5)} | "
            f"{fmt(row['delta_shoving2x_over_sigma'], 4)} | "
            f"{fmt(row['effect_scale_2x_over_nominal'], 4)} |"
        )
    lines.append('')
    lines.append('## Output Figures')
    lines.append('')
    lines.append('The three-sample overlays are written as `output/pythia_zpole_5M_*_noshoving_vs_shoving_vs_shoving2x*.pdf` and copied to `docs/figures`, `AnalysisNote/figures`, and `overleaf_note/figures`. The ratio panels show nominal shoving/no-shoving and factor-two shoving/no-shoving together.')
    lines.append('')
    lines.append('## Interpretation Notes')
    lines.append('')
    lines.append('- This comparison tests response to increasing the Gleipnir repulsion factor, not a calibrated uncertainty on the shoving model.')
    lines.append('- The configurations are statistically matched by setup and seed blocks, but they are not event-by-event paired because shoving changes random-number consumption during hadronization.')
    lines.append('- High-order cumulants are included only where the signed cumulant gives a real, positive plotted value in the jackknife summary.')
    lines.append('')
    RESULT_MD.write_text('\n'.join(lines) + '\n')
    print(f'Wrote {RESULT_CSV}')
    print(f'Wrote {RESULT_MD}')


if __name__ == '__main__':
    main()
