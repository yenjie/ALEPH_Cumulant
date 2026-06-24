#!/usr/bin/env python3
import csv
import math
from pathlib import Path

INPUT = Path('output/nonflow_mc_closure.csv')
OUTPUT = Path('output/nonflow_mc_subtraction_diagnostics.csv')

def center(label):
    lo, hi = label.split('_')
    lo = float(lo)
    hi = float(hi)
    if hi >= 999:
        return lo + 2.5
    return 0.5 * (lo + hi)

def weighted_fit_intercept_slope(points):
    # Fit y = a + b / N.
    s = sx = sy = sxx = sxy = 0.0
    for n, y, ey in points:
        if ey <= 0:
            continue
        x = 1.0 / n
        w = 1.0 / (ey * ey)
        s += w
        sx += w * x
        sy += w * y
        sxx += w * x * x
        sxy += w * x * y
    det = s * sxx - sx * sx
    if det <= 0:
        return 0.0, 0.0, 0.0
    a = (sxx * sy - sx * sxy) / det
    b = (s * sxy - sx * sy) / det
    ea = math.sqrt(sxx / det)
    return a, b, ea

rows = []
with INPUT.open() as f:
    for row in csv.DictReader(f):
        if row['group'] == 'gap' and row['method'] == 'inclusive':
            row['N'] = center(row['bin'])
            row['c2'] = float(row['c22_or_corr'])
            row['ec2'] = float(row['c22_or_corr_err'])
            rows.append(row)

with OUTPUT.open('w', newline='') as f:
    fieldnames = ['axis', 'bin', 'N_center', 'inclusive_c2', 'inclusive_c2_err',
                  'low_mult_scaled_c2', 'low_mult_scaled_c2_err', 'fit_1overN_intercept',
                  'fit_1overN_intercept_err', 'fit_1overN_slope', 'fit_1overN_residual_c2']
    writer = csv.DictWriter(f, fieldnames=fieldnames)
    writer.writeheader()
    for axis in ['beam', 'thrust']:
        axis_rows = [r for r in rows if r['axis'] == axis and r['ec2'] > 0]
        axis_rows.sort(key=lambda r: r['N'])
        fit_points = [(r['N'], r['c2'], r['ec2']) for r in axis_rows]
        a, b, ea = weighted_fit_intercept_slope(fit_points)
        ref = axis_rows[0]
        nref, cref, eref = ref['N'], ref['c2'], ref['ec2']
        for r in axis_rows:
            scaled = r['c2'] - cref * nref / r['N']
            escaled = math.sqrt(r['ec2']**2 + (nref / r['N'] * eref)**2)
            fit_residual = r['c2'] - b / r['N']
            writer.writerow({
                'axis': axis,
                'bin': r['bin'],
                'N_center': f'{r["N"]:.6g}',
                'inclusive_c2': f'{r["c2"]:.10g}',
                'inclusive_c2_err': f'{r["ec2"]:.10g}',
                'low_mult_scaled_c2': f'{scaled:.10g}',
                'low_mult_scaled_c2_err': f'{escaled:.10g}',
                'fit_1overN_intercept': f'{a:.10g}',
                'fit_1overN_intercept_err': f'{ea:.10g}',
                'fit_1overN_slope': f'{b:.10g}',
                'fit_1overN_residual_c2': f'{fit_residual:.10g}',
            })
        print(f'{axis}: 1/N fit intercept = {a:.6g} +/- {ea:.6g}, slope = {b:.6g}')
        print(f'{axis}: low-mult subtraction range = {min(float(row["low_mult_scaled_c2"]) for row in csv.DictReader(OUTPUT.open()) if row["axis"] == axis):.6g} ... reload after completion') if False else ''

print(f'Wrote {OUTPUT}')
