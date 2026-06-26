#!/usr/bin/env python3
import csv
import math
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec, GridSpecFromSubplotSpec
import numpy as np

OUTPUT_DIR = Path('output')

V2_QUANTITIES = [
    ('hV2_2', 'v2{2}'),
    ('hV2_4', 'v2{4}'),
    ('hV2_6', 'v2{6}'),
    ('hV2_8', 'v2{8}'),
]
TWO_SUB_QUANTITIES = [
    ('hV2TwoSub_2', 'two-sub v2{2}'),
    ('hV2TwoSub_4', 'two-sub v2{4}'),
    ('hV2TwoSub_6', 'two-sub v2{6}'),
    ('hV2TwoSub_8', 'two-sub v2{8}'),
]
ETA_CASES = [
    ('etagap1p0', '|Delta eta|>1.0'),
    ('etagap1p6', '|Delta eta|>1.6'),
    ('etagap2p0', '|Delta eta|>2.0'),
    ('etagap2p5', '|Delta eta|>2.5'),
    ('etagap3p0', '|Delta eta|>3.0'),
]
TWO_SUB_CASES = [
    ('twosub0p0', 'two subevent eta<0 vs eta>0'),
    ('twosub1p0', 'two subevent gap 1.0'),
    ('twosub1p6', 'two subevent gap 1.6'),
    ('twosub2p0', 'two subevent gap 2.0'),
]
V2_CASES = [
    ('pt04to1', '0.4 < pT < 1 GeV'),
    ('pt04to2', '0.4 < pT < 2 GeV'),
    ('pt1to3', '1 < pT < 3 GeV'),
    ('positive', 'positive charged particles'),
    ('negative', 'negative charged particles'),
    ('thrustlt0p90', 'Thrust < 0.90'),
    ('thrustlt0p85', 'Thrust < 0.85'),
    ('sphgt0p12', 'Sphericity > 0.12'),
    ('sphgt0p22', 'Sphericity > 0.22'),
]

SAMPLE_STYLES = {
    'no': {'label': 'Pythia no shoving', 'color': 'black', 'marker': 'o'},
    'sh': {'label': 'Pythia shoving', 'color': '#d62728', 'marker': 's'},
    'sh2': {'label': 'Pythia shoving x2', 'color': '#2ca02c', 'marker': '^'},
}


def read_rows(path):
    path = Path(path)
    if not path.exists():
        raise FileNotFoundError(path)
    with path.open(newline='') as handle:
        return list(csv.DictReader(handle))


def as_float(row, key):
    try:
        value = float(row.get(key, 'nan'))
    except ValueError:
        return math.nan
    return value if math.isfinite(value) else math.nan


def valid(value, err):
    return math.isfinite(value) and math.isfinite(err) and not (value == 0.0 and err == 0.0) and value > 0.0


def bin_label(label):
    return label.replace('_', '-')


def arrays_for(rows, value_key, err_key, bins):
    by_bin = {row['multiplicity_bin']: row for row in rows}
    values = []
    errors = []
    for b in bins:
        row = by_bin[b]
        value = as_float(row, value_key)
        err = as_float(row, err_key)
        if not valid(value, err):
            value = math.nan
            err = math.nan
        values.append(value)
        errors.append(err)
    return np.asarray(values, dtype=float), np.asarray(errors, dtype=float)


def errorbar(ax, x, y, yerr, *, label, color, marker, offset=0.0, ms=4.5, lw=1.2):
    mask = np.isfinite(y) & np.isfinite(yerr)
    if not np.any(mask):
        return
    ax.errorbar(x[mask] + offset, y[mask], yerr=yerr[mask], fmt=marker, color=color,
                markersize=ms, elinewidth=lw, capsize=2.4, linewidth=0.0, label=label)


def ratio_limits(*series):
    lows = []
    highs = []
    for y, err in series:
        mask = np.isfinite(y) & np.isfinite(err)
        if np.any(mask):
            lows.extend((y[mask] - err[mask]).tolist())
            highs.extend((y[mask] + err[mask]).tolist())
    if not lows:
        return 0.8, 1.2
    lo = min(min(lows), 1.0)
    hi = max(max(highs), 1.0)
    span = hi - lo
    if span < 0.08:
        center = 0.5 * (hi + lo)
        lo = center - 0.04
        hi = center + 0.04
        span = hi - lo
    return max(0.0, lo - 0.18 * span), hi + 0.18 * span


def write_v2_overlay_csv(path, rows_sh, rows_sh2, quantities):
    path = Path(path)
    fieldnames = [
        'axis', 'multiplicity_bin', 'quantity',
        'noshoving', 'noshoving_err', 'shoving', 'shoving_err', 'shoving2x', 'shoving2x_err',
        'shoving_over_noshoving', 'shoving_over_noshoving_err',
        'shoving2x_over_noshoving', 'shoving2x_over_noshoving_err',
    ]
    by_key_2x = {(row['quantity'], row['multiplicity_bin']): row for row in rows_sh2}
    with path.open('w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows_sh:
            key = (row['quantity'], row['multiplicity_bin'])
            if key not in by_key_2x:
                continue
            row2 = by_key_2x[key]
            writer.writerow({
                'axis': row['axis'],
                'multiplicity_bin': row['multiplicity_bin'],
                'quantity': row['quantity'],
                'noshoving': row['data'],
                'noshoving_err': row['data_err'],
                'shoving': row['mc'],
                'shoving_err': row['mc_err'],
                'shoving2x': row2['mc'],
                'shoving2x_err': row2['mc_err'],
                'shoving_over_noshoving': row['mc_over_data'],
                'shoving_over_noshoving_err': row['mc_over_data_err'],
                'shoving2x_over_noshoving': row2['mc_over_data'],
                'shoving2x_over_noshoving_err': row2['mc_over_data_err'],
            })


def plot_v2_overlay(pair_sh_csv, pair_sh2_csv, output_prefix, axis, title, quantities):
    rows_sh_all = read_rows(pair_sh_csv)
    rows_sh2_all = read_rows(pair_sh2_csv)
    bins = []
    for row in rows_sh_all:
        if row['quantity'] == quantities[0][0] and row['multiplicity_bin'] not in bins:
            bins.append(row['multiplicity_bin'])
    x = np.arange(len(bins), dtype=float)
    labels = [bin_label(b) for b in bins]

    fig = plt.figure(figsize=(13.0, 9.2))
    outer = GridSpec(2, 2, figure=fig, hspace=0.34, wspace=0.24)
    for iq, (quantity, qlabel) in enumerate(quantities):
        sub = GridSpecFromSubplotSpec(2, 1, subplot_spec=outer[iq], height_ratios=[3.2, 1.25], hspace=0.06)
        ax = fig.add_subplot(sub[0])
        rax = fig.add_subplot(sub[1], sharex=ax)
        rows_sh = [row for row in rows_sh_all if row['quantity'] == quantity]
        rows_sh2 = [row for row in rows_sh2_all if row['quantity'] == quantity]

        no_y, no_e = arrays_for(rows_sh, 'data', 'data_err', bins)
        sh_y, sh_e = arrays_for(rows_sh, 'mc', 'mc_err', bins)
        sh2_y, sh2_e = arrays_for(rows_sh2, 'mc', 'mc_err', bins)
        sh_r, sh_re = arrays_for(rows_sh, 'mc_over_data', 'mc_over_data_err', bins)
        sh2_r, sh2_re = arrays_for(rows_sh2, 'mc_over_data', 'mc_over_data_err', bins)

        errorbar(ax, x, no_y, no_e, label=SAMPLE_STYLES['no']['label'], color=SAMPLE_STYLES['no']['color'], marker=SAMPLE_STYLES['no']['marker'], offset=-0.16)
        errorbar(ax, x, sh_y, sh_e, label=SAMPLE_STYLES['sh']['label'], color=SAMPLE_STYLES['sh']['color'], marker=SAMPLE_STYLES['sh']['marker'], offset=0.0)
        errorbar(ax, x, sh2_y, sh2_e, label=SAMPLE_STYLES['sh2']['label'], color=SAMPLE_STYLES['sh2']['color'], marker=SAMPLE_STYLES['sh2']['marker'], offset=0.16)
        ax.set_ylabel(qlabel, fontsize=12)
        ax.grid(axis='y', alpha=0.25)
        ax.tick_params(axis='x', labelbottom=False)
        ax.text(0.02, 0.90, f'{axis} axis, {qlabel}', transform=ax.transAxes, fontsize=11)
        finite_max = []
        for series in (no_y + no_e, sh_y + sh_e, sh2_y + sh2_e):
            finite = series[np.isfinite(series)]
            if finite.size:
                finite_max.append(float(np.max(finite)))
        ymax = max(finite_max) if finite_max else math.nan
        if math.isfinite(ymax) and ymax > 0:
            ax.set_ylim(0, ymax * 1.25)
        if iq == 0:
            ax.legend(frameon=False, fontsize=10, loc='upper right')

        errorbar(rax, x, sh_r, sh_re, label='shoving/no', color=SAMPLE_STYLES['sh']['color'], marker='s', offset=-0.06, ms=4.0)
        errorbar(rax, x, sh2_r, sh2_re, label='shoving x2/no', color=SAMPLE_STYLES['sh2']['color'], marker='^', offset=0.06, ms=4.0)
        rax.axhline(1.0, color='0.35', linestyle='--', linewidth=1.0)
        rax.set_ylabel('ratio/no', fontsize=10)
        rax.set_ylim(*ratio_limits((sh_r, sh_re), (sh2_r, sh2_re)))
        rax.grid(axis='y', alpha=0.25)
        rax.set_xticks(x)
        rax.set_xticklabels(labels, rotation=55, ha='right', fontsize=8)
        if iq >= 2:
            rax.set_xlabel('Ntrk offline', fontsize=11)
        if iq == 0:
            rax.legend(frameon=False, fontsize=8, loc='upper right')

    fig.suptitle(title, fontsize=15)
    fig.tight_layout(rect=[0, 0, 1, 0.96])
    fig.savefig(str(output_prefix) + f'_{axis}.pdf')
    fig.savefig(str(output_prefix) + f'_{axis}.png', dpi=160)
    plt.close(fig)
    write_v2_overlay_csv(str(output_prefix) + f'_{axis}.csv', rows_sh_all, rows_sh2_all, quantities)


def write_eta_overlay_csv(path, rows_sh, rows_sh2):
    path = Path(path)
    by_bin_2x = {row['multiplicity_bin']: row for row in rows_sh2}
    fieldnames = [
        'axis', 'multiplicity_bin',
        'noshoving_v22', 'noshoving_v22_err', 'noshoving_v22_gap', 'noshoving_v22_gap_err',
        'noshoving_gap_over_inclusive', 'noshoving_gap_over_inclusive_err',
        'shoving_v22', 'shoving_v22_err', 'shoving_v22_gap', 'shoving_v22_gap_err',
        'shoving_gap_over_inclusive', 'shoving_gap_over_inclusive_err',
        'shoving2x_v22', 'shoving2x_v22_err', 'shoving2x_v22_gap', 'shoving2x_v22_gap_err',
        'shoving2x_gap_over_inclusive', 'shoving2x_gap_over_inclusive_err',
        'shoving_over_noshoving_v22', 'shoving_over_noshoving_v22_err',
        'shoving_over_noshoving_v22_gap', 'shoving_over_noshoving_v22_gap_err',
        'shoving2x_over_noshoving_v22', 'shoving2x_over_noshoving_v22_err',
        'shoving2x_over_noshoving_v22_gap', 'shoving2x_over_noshoving_v22_gap_err',
    ]
    with path.open('w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows_sh:
            row2 = by_bin_2x.get(row['multiplicity_bin'])
            if row2 is None:
                continue
            writer.writerow({
                'axis': row['axis'],
                'multiplicity_bin': row['multiplicity_bin'],
                'noshoving_v22': row['data_v22'],
                'noshoving_v22_err': row['data_v22_err'],
                'noshoving_v22_gap': row['data_v22_gap'],
                'noshoving_v22_gap_err': row['data_v22_gap_err'],
                'noshoving_gap_over_inclusive': row['data_gap_over_inclusive'],
                'noshoving_gap_over_inclusive_err': row['data_gap_over_inclusive_err'],
                'shoving_v22': row['mc_v22'],
                'shoving_v22_err': row['mc_v22_err'],
                'shoving_v22_gap': row['mc_v22_gap'],
                'shoving_v22_gap_err': row['mc_v22_gap_err'],
                'shoving_gap_over_inclusive': row['mc_gap_over_inclusive'],
                'shoving_gap_over_inclusive_err': row['mc_gap_over_inclusive_err'],
                'shoving2x_v22': row2['mc_v22'],
                'shoving2x_v22_err': row2['mc_v22_err'],
                'shoving2x_v22_gap': row2['mc_v22_gap'],
                'shoving2x_v22_gap_err': row2['mc_v22_gap_err'],
                'shoving2x_gap_over_inclusive': row2['mc_gap_over_inclusive'],
                'shoving2x_gap_over_inclusive_err': row2['mc_gap_over_inclusive_err'],
                'shoving_over_noshoving_v22': row['mc_over_data_v22'],
                'shoving_over_noshoving_v22_err': row['mc_over_data_v22_err'],
                'shoving_over_noshoving_v22_gap': row['mc_over_data_v22_gap'],
                'shoving_over_noshoving_v22_gap_err': row['mc_over_data_v22_gap_err'],
                'shoving2x_over_noshoving_v22': row2['mc_over_data_v22'],
                'shoving2x_over_noshoving_v22_err': row2['mc_over_data_v22_err'],
                'shoving2x_over_noshoving_v22_gap': row2['mc_over_data_v22_gap'],
                'shoving2x_over_noshoving_v22_gap_err': row2['mc_over_data_v22_gap_err'],
            })


def eta_arrays(rows, key, err_key, bins):
    return arrays_for(rows, key, err_key, bins)


def plot_eta_overlay(pair_sh_csv, pair_sh2_csv, output_prefix, axis, title, gap_label):
    rows_sh = read_rows(pair_sh_csv)
    rows_sh2 = read_rows(pair_sh2_csv)
    bins = [row['multiplicity_bin'] for row in rows_sh]
    x = np.arange(len(bins), dtype=float)
    labels = [bin_label(b) for b in bins]

    no_i, no_ie = eta_arrays(rows_sh, 'data_v22', 'data_v22_err', bins)
    no_g, no_ge = eta_arrays(rows_sh, 'data_v22_gap', 'data_v22_gap_err', bins)
    sh_i, sh_ie = eta_arrays(rows_sh, 'mc_v22', 'mc_v22_err', bins)
    sh_g, sh_ge = eta_arrays(rows_sh, 'mc_v22_gap', 'mc_v22_gap_err', bins)
    sh2_i, sh2_ie = eta_arrays(rows_sh2, 'mc_v22', 'mc_v22_err', bins)
    sh2_g, sh2_ge = eta_arrays(rows_sh2, 'mc_v22_gap', 'mc_v22_gap_err', bins)

    no_gr, no_gre = eta_arrays(rows_sh, 'data_gap_over_inclusive', 'data_gap_over_inclusive_err', bins)
    sh_gr, sh_gre = eta_arrays(rows_sh, 'mc_gap_over_inclusive', 'mc_gap_over_inclusive_err', bins)
    sh2_gr, sh2_gre = eta_arrays(rows_sh2, 'mc_gap_over_inclusive', 'mc_gap_over_inclusive_err', bins)

    sh_ri, sh_rie = eta_arrays(rows_sh, 'mc_over_data_v22', 'mc_over_data_v22_err', bins)
    sh_rg, sh_rge = eta_arrays(rows_sh, 'mc_over_data_v22_gap', 'mc_over_data_v22_gap_err', bins)
    sh2_ri, sh2_rie = eta_arrays(rows_sh2, 'mc_over_data_v22', 'mc_over_data_v22_err', bins)
    sh2_rg, sh2_rge = eta_arrays(rows_sh2, 'mc_over_data_v22_gap', 'mc_over_data_v22_gap_err', bins)

    fig = plt.figure(figsize=(11.5, 10.5))
    gs = GridSpec(3, 1, figure=fig, height_ratios=[3.0, 1.4, 1.5], hspace=0.10)
    ax = fig.add_subplot(gs[0])
    mid = fig.add_subplot(gs[1], sharex=ax)
    low = fig.add_subplot(gs[2], sharex=ax)

    samples = [
        ('no', no_i, no_ie, no_g, no_ge),
        ('sh', sh_i, sh_ie, sh_g, sh_ge),
        ('sh2', sh2_i, sh2_ie, sh2_g, sh2_ge),
    ]
    for idx, (sample, inc, inc_e, gap, gap_e) in enumerate(samples):
        style = SAMPLE_STYLES[sample]
        offset = (-0.18, 0.0, 0.18)[idx]
        errorbar(ax, x, inc, inc_e, label=style['label'] + ' inclusive', color=style['color'], marker='o', offset=offset)
        errorbar(ax, x, gap, gap_e, label=style['label'] + ' ' + gap_label, color=style['color'], marker='s', offset=offset + 0.04)
    ax.set_ylabel('v2{2}', fontsize=12)
    ax.grid(axis='y', alpha=0.25)
    ax.tick_params(axis='x', labelbottom=False)
    ax.text(0.02, 0.90, f'{axis} axis', transform=ax.transAxes, fontsize=11)
    ymax = np.nanmax([np.nanmax(no_i + no_ie), np.nanmax(no_g + no_ge), np.nanmax(sh_i + sh_ie), np.nanmax(sh_g + sh_ge), np.nanmax(sh2_i + sh2_ie), np.nanmax(sh2_g + sh2_ge)])
    if math.isfinite(ymax) and ymax > 0:
        ax.set_ylim(0, ymax * 1.22)
    ax.legend(frameon=False, fontsize=8, ncol=2, loc='upper right')

    errorbar(mid, x, no_gr, no_gre, label='no shoving', color=SAMPLE_STYLES['no']['color'], marker='o', offset=-0.12)
    errorbar(mid, x, sh_gr, sh_gre, label='shoving', color=SAMPLE_STYLES['sh']['color'], marker='s', offset=0.0)
    errorbar(mid, x, sh2_gr, sh2_gre, label='shoving x2', color=SAMPLE_STYLES['sh2']['color'], marker='^', offset=0.12)
    mid.set_ylabel('gap/incl.', fontsize=11)
    mid.grid(axis='y', alpha=0.25)
    mid.tick_params(axis='x', labelbottom=False)
    mid.legend(frameon=False, fontsize=8, loc='upper right', ncol=3)
    ymax_mid = np.nanmax([np.nanmax(no_gr + no_gre), np.nanmax(sh_gr + sh_gre), np.nanmax(sh2_gr + sh2_gre)])
    if math.isfinite(ymax_mid) and ymax_mid > 0:
        mid.set_ylim(0, ymax_mid * 1.22)

    errorbar(low, x, sh_ri, sh_rie, label='shoving/no inclusive', color=SAMPLE_STYLES['sh']['color'], marker='o', offset=-0.14, ms=4.0)
    errorbar(low, x, sh_rg, sh_rge, label='shoving/no gap', color=SAMPLE_STYLES['sh']['color'], marker='s', offset=-0.05, ms=4.0)
    errorbar(low, x, sh2_ri, sh2_rie, label='shoving x2/no inclusive', color=SAMPLE_STYLES['sh2']['color'], marker='o', offset=0.05, ms=4.0)
    errorbar(low, x, sh2_rg, sh2_rge, label='shoving x2/no gap', color=SAMPLE_STYLES['sh2']['color'], marker='s', offset=0.14, ms=4.0)
    low.axhline(1.0, color='0.35', linestyle='--', linewidth=1.0)
    low.set_ylabel('ratio/no', fontsize=11)
    low.set_ylim(*ratio_limits((sh_ri, sh_rie), (sh_rg, sh_rge), (sh2_ri, sh2_rie), (sh2_rg, sh2_rge)))
    low.grid(axis='y', alpha=0.25)
    low.set_xticks(x)
    low.set_xticklabels(labels, rotation=55, ha='right', fontsize=9)
    low.set_xlabel('Ntrk offline', fontsize=11)
    low.legend(frameon=False, fontsize=8, loc='upper right', ncol=2)

    fig.suptitle(title, fontsize=15)
    fig.tight_layout(rect=[0, 0, 1, 0.96])
    fig.savefig(str(output_prefix) + f'_{axis}.pdf')
    fig.savefig(str(output_prefix) + f'_{axis}.png', dpi=160)
    plt.close(fig)

    # Compact sample-ratio figure, analogous to the previous _datamc output.
    fig, axr = plt.subplots(figsize=(11.5, 5.2))
    errorbar(axr, x, sh_ri, sh_rie, label='shoving/no inclusive', color=SAMPLE_STYLES['sh']['color'], marker='o', offset=-0.14, ms=5.0)
    errorbar(axr, x, sh_rg, sh_rge, label='shoving/no ' + gap_label, color=SAMPLE_STYLES['sh']['color'], marker='s', offset=-0.05, ms=5.0)
    errorbar(axr, x, sh2_ri, sh2_rie, label='shoving x2/no inclusive', color=SAMPLE_STYLES['sh2']['color'], marker='o', offset=0.05, ms=5.0)
    errorbar(axr, x, sh2_rg, sh2_rge, label='shoving x2/no ' + gap_label, color=SAMPLE_STYLES['sh2']['color'], marker='s', offset=0.14, ms=5.0)
    axr.axhline(1.0, color='0.35', linestyle='--', linewidth=1.0)
    axr.set_ylabel('ratio/no', fontsize=12)
    axr.set_xlabel('Ntrk offline', fontsize=12)
    axr.set_xticks(x)
    axr.set_xticklabels(labels, rotation=55, ha='right', fontsize=9)
    axr.set_ylim(*ratio_limits((sh_ri, sh_rie), (sh_rg, sh_rge), (sh2_ri, sh2_rie), (sh2_rg, sh2_rge)))
    axr.grid(axis='y', alpha=0.25)
    axr.legend(frameon=False, fontsize=9, ncol=2, loc='upper right')
    axr.set_title(f'{title}, {axis} axis sample ratios', fontsize=14)
    fig.tight_layout()
    fig.savefig(str(output_prefix) + f'_datamc_{axis}.pdf')
    fig.savefig(str(output_prefix) + f'_datamc_{axis}.png', dpi=160)
    plt.close(fig)

    write_eta_overlay_csv(str(output_prefix) + f'_{axis}.csv', rows_sh, rows_sh2)


def pair_prefix(kind, case, sh2=False):
    suffix = 'noshoving_vs_shoving2x' if sh2 else 'noshoving_vs_shoving'
    if kind == 'baseline':
        return OUTPUT_DIR / f'pythia_zpole_5M_{suffix}_pt04_compare'
    return OUTPUT_DIR / f'pythia_zpole_5M_nonflow_{case}_{suffix}'


def out_prefix(kind, case):
    suffix = 'noshoving_vs_shoving_vs_shoving2x'
    if kind == 'baseline':
        return OUTPUT_DIR / f'pythia_zpole_5M_{suffix}_pt04_compare'
    return OUTPUT_DIR / f'pythia_zpole_5M_nonflow_{case}_{suffix}'


def main():
    for axis in ('beam', 'thrust'):
        plot_v2_overlay(
            str(pair_prefix('baseline', '', False)) + f'_{axis}.csv',
            str(pair_prefix('baseline', '', True)) + f'_{axis}.csv',
            out_prefix('baseline', ''),
            axis,
            'Pythia Z-pole 5M nominal charged pT>0.4 GeV',
            V2_QUANTITIES,
        )

    for case, label in ETA_CASES:
        for axis in ('beam', 'thrust'):
            plot_eta_overlay(
                str(pair_prefix('nonflow', case, False)) + f'_{axis}.csv',
                str(pair_prefix('nonflow', case, True)) + f'_{axis}.csv',
                out_prefix('nonflow', case),
                axis,
                f'Pythia Z-pole 5M eta-gap comparison, {label}',
                label,
            )

    for case, label in TWO_SUB_CASES:
        for axis in ('beam', 'thrust'):
            plot_v2_overlay(
                str(pair_prefix('nonflow', case, False)) + f'_{axis}.csv',
                str(pair_prefix('nonflow', case, True)) + f'_{axis}.csv',
                out_prefix('nonflow', case),
                axis,
                f'Pythia Z-pole 5M {label}',
                TWO_SUB_QUANTITIES,
            )

    for case, label in V2_CASES:
        for axis in ('beam', 'thrust'):
            plot_v2_overlay(
                str(pair_prefix('nonflow', case, False)) + f'_{axis}.csv',
                str(pair_prefix('nonflow', case, True)) + f'_{axis}.csv',
                out_prefix('nonflow', case),
                axis,
                f'Pythia Z-pole 5M {label}',
                V2_QUANTITIES,
            )

    print('Wrote three-sample Pythia shoving2x overlay figures under output/')


if __name__ == '__main__':
    main()
