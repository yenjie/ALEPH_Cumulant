#!/usr/bin/env python3
import csv
import math
import re
from pathlib import Path

OUTPUT_DIR = Path('output')
DOCS_DIR = Path('docs')
FIG_DIR = DOCS_DIR / 'figures'
RESULT_CSV = FIG_DIR / 'pythia_shoving_nonflow_5M_metrics.csv'
RESULT_MD = DOCS_DIR / 'pythia_shoving_nonflow_5M.md'

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


def add_metric(rows, technique, case, label, axis, observable, mult_bin, no_value, no_err, sh_value, sh_err):
    if no_value is None or sh_value is None or no_err is None or sh_err is None:
        return
    # The plotting helpers export zero for bins where the signed cumulant does
    # not give a real, positive v2 value.  Do not let those placeholder bins
    # drive the shoving/non-shoving significance table.
    if no_value <= 0.0 or sh_value <= 0.0:
        return
    sigma = math.hypot(no_err, sh_err)
    if sigma <= 0:
        return
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
        'shoving_over_no_shoving': sh_value / no_value,
        'delta_over_sigma': (sh_value - no_value) / sigma,
    })


def read_v2_csv(path, technique, case, label, rows):
    if not path.exists():
        return
    with path.open(newline='') as handle:
        for row in csv.DictReader(handle):
            add_metric(rows, technique, case, label, row['axis'], row['quantity'], row['multiplicity_bin'],
                       safe_float(row.get('data')), safe_float(row.get('data_err')),
                       safe_float(row.get('mc')), safe_float(row.get('mc_err')))


def read_eta_csv(path, technique, case, label, rows):
    if not path.exists():
        return
    with path.open(newline='') as handle:
        for row in csv.DictReader(handle):
            add_metric(rows, technique, case, label, row['axis'], 'v2_gap', row['multiplicity_bin'],
                       safe_float(row.get('data_v22_gap')), safe_float(row.get('data_v22_gap_err')),
                       safe_float(row.get('mc_v22_gap')), safe_float(row.get('mc_v22_gap_err')))


def csv_path(case, kind, axis):
    if kind == 'baseline':
        return OUTPUT_DIR / f'pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_{axis}.csv'
    return OUTPUT_DIR / f'pythia_zpole_5M_nonflow_{case}_noshoving_vs_shoving_{axis}.csv'


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
        if key not in best or abs(row['delta_over_sigma']) > abs(best[key]['delta_over_sigma']):
            best[key] = row
    return list(best.values())


def parse_log(path):
    text = path.read_text(errors='replace') if path.exists() else ''
    info = {}
    m = re.search(r'Wrote\s+(\d+)\s+events.*with\s+(\d+)\s+visible particles and\s+(\d+)\s+StudyMult-selected charged particles', text)
    if m:
        info['events'], info['visible'], info['charged'] = m.groups()
    return info


def fmt(value, digits=4):
    return f'{value:.{digits}g}'


def main():
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    rows = collect_rows()
    if not rows:
        raise SystemExit('No 5M Pythia shoving/nonflow CSV rows found. Run scripts/plot_pythia_nonflow_suppression_5M.sh first.')
    fieldnames = ['technique', 'case', 'label', 'axis', 'observable', 'multiplicity_bin',
                  'no_shoving', 'no_shoving_err', 'shoving', 'shoving_err',
                  'shoving_over_no_shoving', 'delta_over_sigma']
    with RESULT_CSV.open('w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

    best = sorted(best_rows(rows), key=lambda r: (r['technique'], r['case'], r['axis'], r['observable']))
    no_info = parse_log(OUTPUT_DIR / 'pythia_noshoving_zpole_5M.log')
    sh_info = parse_log(OUTPUT_DIR / 'pythia_shoving_zpole_5M.log')
    no_ext_info = parse_log(OUTPUT_DIR / 'pythia_noshoving_zpole_extra2M_seed67890.log')
    sh_ext_info = parse_log(OUTPUT_DIR / 'pythia_shoving_zpole_extra2M_seed67890.log')

    lines = []
    lines.append('# Pythia Shoving Nonflow-Suppression Study, 5M Events')
    lines.append('')
    lines.append('This document records the 5M-event generator-level comparison of Pythia no-shoving and Gleipnir-shoving Z-pole samples under the nonflow-suppression handles used in the analysis note.')
    lines.append('')
    lines.append('## Samples')
    lines.append('')
    lines.append('- Final 5M inputs: `output/pythia_noshoving_zpole_5M.root` and `output/pythia_shoving_zpole_5M.root`.')
    lines.append('- Composition used for this production: existing seed-12345 3M samples plus independent seed-67890 2M extensions merged with `hadd`.')
    if no_ext_info and sh_ext_info:
        lines.append(f"- Seed-67890 no-shoving extension: {no_ext_info.get('events', 'unknown')} events, {no_ext_info.get('charged', 'unknown')} StudyMult-selected charged particles.")
        lines.append(f"- Seed-67890 shoving extension: {sh_ext_info.get('events', 'unknown')} events, {sh_ext_info.get('charged', 'unknown')} StudyMult-selected charged particles.")
    lines.append('- Final merged entry count was checked with `bin/aleph_charged_cumulants --PrintEntries 1` and required to be exactly 5,000,000 entries for each sample.')
    lines.append('- Both configurations use the same hard process, visible-particle convention, multiplicity bins `0,10,15,20,25,30,35,40,999`, and 16-block jackknife summaries.')
    lines.append('')
    lines.append('## Commands')
    lines.append('')
    lines.append('```bash')
    lines.append('scripts/build_pythia_zpole_5M_samples.sh')
    lines.append('ALEPH_MAX_PARALLEL=16 CHUNKS=16 RUN_NONFLOW=1 scripts/run_pythia_shoving_5M_analysis.sh')
    lines.append('scripts/plot_pythia_nonflow_suppression_5M.sh')
    lines.append('scripts/summarize_pythia_shoving_nonflow.py')
    lines.append('```')
    lines.append('')
    lines.append('## Nonflow-Suppression Techniques')
    lines.append('')
    lines.append('- Rapidity gaps for the two-particle observable: `|delta eta| > 1.0, 1.6, 2.0, 2.5, 3.0`.')
    lines.append('- Two-subevent cumulants: `eta<0` versus `eta>0`, and excluded middle-region gaps of 1.0, 1.6, and 2.0 from `TwoSubeventEtaBoundary = 0.5, 0.8, 1.0`.')
    lines.append('- Track selections: `0.4<pT<1`, `0.4<pT<2`, `1<pT<3`, positive charges only, and negative charges only.')
    lines.append('- Event-shape selections: `Thrust<0.90`, `Thrust<0.85`, `Sphericity>0.12`, and `Sphericity>0.22`. For generated vector trees the analyzer computes thrust and sphericity from visible final-particle momenta because those branches are not stored in the generated tree.')
    lines.append('')
    lines.append('## Result Summary')
    lines.append('')
    lines.append('The table gives the most significant shoving-minus-no-shoving bin for each technique, axis, and observable. The significance is `(shoving-no-shoving)/sqrt(err_no^2+err_sh^2)`. The ratio is `shoving/no-shoving`.')
    lines.append('')
    lines.append('| Technique | Case | Axis | Observable | Bin | Ratio | Significance |')
    lines.append('|---|---|---|---|---:|---:|---:|')
    for row in best:
        label = row['label'].replace('|', r'\|')
        observable = row['observable'].replace('|', r'\|')
        lines.append(f"| {row['technique']} | {label} | {row['axis']} | {observable} | {row['multiplicity_bin'].replace('_', '--')} | {fmt(row['shoving_over_no_shoving'], 5)} | {fmt(row['delta_over_sigma'], 4)} |")
    lines.append('')
    lines.append('## Output Figures')
    lines.append('')
    lines.append('The per-technique overlays are written as `output/pythia_zpole_5M_nonflow_<case>_noshoving_vs_shoving_<axis>.pdf`; the full 5M PDF/CSV figure set is copied to `docs/figures`, `AnalysisNote/figures`, and `overleaf_note/figures`. The nominal comparison is `pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_<axis>.pdf`. Each comparison PDF now includes a `shoving/no-shoving` ratio panel using the same jackknife errors propagated in quadrature; eta-gap comparison PDFs also retain the gap/inclusive diagnostic panel.')
    lines.append('')
    lines.append('## Interpretation Notes')
    lines.append('')
    lines.append('- The comparison tests whether the shoving/no-shoving difference remains after common nonflow-suppression handles, not whether the absolute Pythia cumulant is collective flow.')
    lines.append('- The two samples are matched by generator setup and seed choices, but they are not event-by-event paired after shoving is enabled because the shoving model changes random-number consumption during hadronization.')
    lines.append('- High-order cumulants are shown only where the signed cumulant gives a real-valued `v2{2k}` in the jackknife summary.')
    lines.append('- Ratio and significance values in this note use the exported jackknife uncertainties and do not include covariance between the two generator configurations.')
    lines.append('')
    RESULT_MD.write_text('\n'.join(lines) + '\n')
    print(f'Wrote {RESULT_CSV}')
    print(f'Wrote {RESULT_MD}')

if __name__ == '__main__':
    main()
