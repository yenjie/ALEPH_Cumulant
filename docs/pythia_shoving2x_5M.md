# Pythia Shoving Factor-Two Study, 5M Events

This document records the three-sample generator-level comparison of Pythia no-shoving, nominal Gleipnir shoving, and a factor-two shoving configuration.

## Samples

- Inputs: `output/pythia_noshoving_zpole_5M.root`, `output/pythia_shoving_zpole_5M.root`, and `output/pythia_shoving2x_zpole_5M.root`.
- The nominal shoving sample uses `Gleipnir:repulsionFactor = 0.25`; the factor-two sample uses `Gleipnir:repulsionFactor = 0.50` with the other Gleipnir settings unchanged.
- The factor-two sample is the merge of `output/pythia_shoving2x_zpole_3M.root` and `output/pythia_shoving2x_zpole_extra2M_seed67890.root`, and the merged tree was required to contain exactly 5,000,000 entries.
- Seed-12345 factor-two chunk: 3000000 events, 61053110 StudyMult-selected charged particles.
- Seed-67890 factor-two extension: 2000000 events, 40695712 StudyMult-selected charged particles.
- All three configurations use the same hard process, visible-particle convention, multiplicity bins `0,10,15,20,25,30,35,40,999`, and 16-block jackknife summaries.

## Commands

```bash
scripts/build_pythia_shoving2x_zpole_5M_sample.sh
ALEPH_MAX_PARALLEL=16 CHUNKS=16 RUN_NONFLOW=1 scripts/run_pythia_shoving2x_5M_analysis.sh
scripts/plot_pythia_shoving2x_comparison_5M.sh
scripts/summarize_pythia_shoving2x_5M.py
```

## Result Summary

The table gives the most significant factor-two shoving-minus-no-shoving bin for each technique, axis, and observable. Significances use `sqrt(err_no^2+err_sample^2)` and do not include covariance between generator configurations. The scale column is `(shoving2x-no shoving)/(nominal shoving-no shoving)` in the selected bin.

| Technique | Case | Axis | Observable | Bin | Nominal/no | 2x/no | 2x significance | 2x/nominal effect |
|---|---|---|---|---:|---:|---:|---:|---:|
| baseline | Nominal charged pT>0.4 GeV | beam | v2{2} | 20--25 | 1.0047 | 1.0101 | 26.45 | 2.133 |
| baseline | Nominal charged pT>0.4 GeV | beam | v2{4} | 15--20 | 1.0039 | 1.0076 | 18.47 | 1.936 |
| baseline | Nominal charged pT>0.4 GeV | beam | v2{6} | 15--20 | 1.004 | 1.0077 | 18.18 | 1.928 |
| baseline | Nominal charged pT>0.4 GeV | beam | v2{8} | 15--20 | 1.004 | 1.0077 | 18.14 | 1.927 |
| baseline | Nominal charged pT>0.4 GeV | thrust | v2{2} | 15--20 | 1.0161 | 1.0336 | 44.62 | 2.081 |
| baseline | Nominal charged pT>0.4 GeV | thrust | v2{4} | 25--30 | 1.0315 | 1.0503 | 8.452 | 1.596 |
| baseline | Nominal charged pT>0.4 GeV | thrust | v2{6} | 25--30 | 1.0525 | 1.0784 | 6.609 | 1.494 |
| baseline | Nominal charged pT>0.4 GeV | thrust | v2{8} | 25--30 | 1.0852 | 1.1221 | 5.209 | 1.432 |
| eta_gap | \|delta eta\|>1.0 | beam | gap v2{2} | 15--20 | 1.0039 | 1.0079 | 14.7 | 2.041 |
| eta_gap | \|delta eta\|>1.0 | thrust | gap v2{2} | 15--20 | 1.0195 | 1.0406 | 28.23 | 2.081 |
| eta_gap | \|delta eta\|>1.6 | beam | gap v2{2} | 15--20 | 1.0032 | 1.0077 | 11.93 | 2.433 |
| eta_gap | \|delta eta\|>1.6 | thrust | gap v2{2} | 15--20 | 1.0186 | 1.0424 | 16.5 | 2.285 |
| eta_gap | \|delta eta\|>2.0 | beam | gap v2{2} | 15--20 | 1.003 | 1.0088 | 12.81 | 2.888 |
| eta_gap | \|delta eta\|>2.0 | thrust | gap v2{2} | 15--20 | 1.0167 | 1.0424 | 12.78 | 2.542 |
| eta_gap | \|delta eta\|>2.5 | beam | gap v2{2} | 15--20 | 1.0034 | 1.0097 | 9.235 | 2.866 |
| eta_gap | \|delta eta\|>2.5 | thrust | gap v2{2} | 15--20 | 1.0128 | 1.041 | 8.293 | 3.203 |
| eta_gap | \|delta eta\|>3.0 | beam | gap v2{2} | 15--20 | 1.0046 | 1.0101 | 6.322 | 2.182 |
| eta_gap | \|delta eta\|>3.0 | thrust | gap v2{2} | 15--20 | 1.0133 | 1.0399 | 5.447 | 2.987 |
| event_shape | Sphericity>0.12 | beam | v2{2} | 20--25 | 1.0049 | 1.0105 | 9.762 | 2.124 |
| event_shape | Sphericity>0.12 | beam | v2{4} | 20--25 | 1.0056 | 1.0093 | 4.861 | 1.676 |
| event_shape | Sphericity>0.12 | beam | v2{6} | 25--30 | 1.005 | 1.0133 | 4.446 | 2.675 |
| event_shape | Sphericity>0.12 | beam | v2{8} | 25--30 | 1.0051 | 1.0136 | 4.348 | 2.666 |
| event_shape | Sphericity>0.12 | thrust | v2{2} | 15--20 | 1.014 | 1.0296 | 25.61 | 2.112 |
| event_shape | Sphericity>0.12 | thrust | v2{4} | 20--25 | 1.0203 | 1.0352 | 17.12 | 1.73 |
| event_shape | Sphericity>0.12 | thrust | v2{6} | 20--25 | 1.0207 | 1.0366 | 17.09 | 1.765 |
| event_shape | Sphericity>0.12 | thrust | v2{8} | 15--20 | 1.0157 | 1.0281 | 17.55 | 1.79 |
| event_shape | Sphericity>0.22 | beam | v2{2} | 20--25 | 1.0087 | 1.0162 | 10.8 | 1.868 |
| event_shape | Sphericity>0.22 | beam | v2{4} | 25--30 | 1.0012 | 1.0179 | 3.759 | 14.78 |
| event_shape | Sphericity>0.22 | beam | v2{6} | 25--30 | 1.0009 | 1.0201 | 3.038 | 21.47 |
| event_shape | Sphericity>0.22 | beam | v2{8} | 25--30 | 1.0009 | 1.0204 | 2.757 | 23.39 |
| event_shape | Sphericity>0.22 | thrust | v2{2} | 20--25 | 1.0189 | 1.0333 | 21.54 | 1.763 |
| event_shape | Sphericity>0.22 | thrust | v2{4} | 20--25 | 1.0162 | 1.0303 | 14.84 | 1.873 |
| event_shape | Sphericity>0.22 | thrust | v2{6} | 20--25 | 1.0155 | 1.0307 | 14.24 | 1.975 |
| event_shape | Sphericity>0.22 | thrust | v2{8} | 20--25 | 1.0155 | 1.0309 | 14.29 | 1.994 |
| event_shape | Thrust<0.85 | beam | v2{2} | 20--25 | 1.0071 | 1.0152 | 9.826 | 2.123 |
| event_shape | Thrust<0.85 | beam | v2{4} | 25--30 | 1.0005 | 1.0111 | 2.83 | 22.83 |
| event_shape | Thrust<0.85 | beam | v2{6} | 25--30 | 1.001 | 1.012 | 2.633 | 12.35 |
| event_shape | Thrust<0.85 | beam | v2{8} | 25--30 | 1.0011 | 1.012 | 2.503 | 10.57 |
| event_shape | Thrust<0.85 | thrust | v2{2} | 20--25 | 1.0191 | 1.0338 | 24.57 | 1.766 |
| event_shape | Thrust<0.85 | thrust | v2{4} | 20--25 | 1.0178 | 1.0315 | 15.43 | 1.772 |
| event_shape | Thrust<0.85 | thrust | v2{6} | 20--25 | 1.018 | 1.0323 | 15.73 | 1.792 |
| event_shape | Thrust<0.85 | thrust | v2{8} | 20--25 | 1.0181 | 1.0326 | 15.86 | 1.798 |
| event_shape | Thrust<0.90 | beam | v2{2} | 20--25 | 1.0044 | 1.0098 | 10.1 | 2.226 |
| event_shape | Thrust<0.90 | beam | v2{4} | 20--25 | 1.0033 | 1.0064 | 4.204 | 1.969 |
| event_shape | Thrust<0.90 | beam | v2{6} | 20--25 | 1.0031 | 1.0067 | 4.031 | 2.14 |
| event_shape | Thrust<0.90 | beam | v2{8} | 20--25 | 1.0031 | 1.0068 | 3.95 | 2.195 |
| event_shape | Thrust<0.90 | thrust | v2{2} | 15--20 | 1.0145 | 1.0313 | 29.04 | 2.154 |
| event_shape | Thrust<0.90 | thrust | v2{4} | 15--20 | 1.0152 | 1.0304 | 20.96 | 1.996 |
| event_shape | Thrust<0.90 | thrust | v2{6} | 15--20 | 1.0168 | 1.0321 | 19.64 | 1.905 |
| event_shape | Thrust<0.90 | thrust | v2{8} | 15--20 | 1.0169 | 1.0318 | 21.15 | 1.884 |
| track_selection | negative charged particles | beam | v2{2} | 0--10 | 1.0031 | 1.0058 | 23.69 | 1.892 |
| track_selection | negative charged particles | beam | v2{4} | 0--10 | 1.003 | 1.0058 | 22.22 | 1.921 |
| track_selection | negative charged particles | beam | v2{6} | 0--10 | 1.003 | 1.0058 | 22.64 | 1.917 |
| track_selection | negative charged particles | beam | v2{8} | 0--10 | 1.003 | 1.0058 | 22.63 | 1.917 |
| track_selection | negative charged particles | thrust | v2{2} | 10--15 | 1.0196 | 1.0367 | 31.76 | 1.871 |
| track_selection | negative charged particles | thrust | v2{4} | 10--15 | 1.0231 | 1.0361 | 6.593 | 1.561 |
| track_selection | negative charged particles | thrust | v2{6} | 10--15 | 1.0621 | 1.0898 | 4.329 | 1.447 |
| track_selection | negative charged particles | thrust | v2{8} | 15--20 | 1.0094 | 1.0401 | 1.839 | 4.244 |
| track_selection | positive charged particles | beam | v2{2} | 10--15 | 1.0052 | 1.0108 | 23.01 | 2.075 |
| track_selection | positive charged particles | beam | v2{4} | 10--15 | 1.0056 | 1.0114 | 18.6 | 2.053 |
| track_selection | positive charged particles | beam | v2{6} | 10--15 | 1.0057 | 1.0117 | 18.15 | 2.043 |
| track_selection | positive charged particles | beam | v2{8} | 10--15 | 1.0058 | 1.0118 | 17.97 | 2.038 |
| track_selection | positive charged particles | thrust | v2{2} | 10--15 | 1.0188 | 1.0351 | 28.61 | 1.864 |
| track_selection | positive charged particles | thrust | v2{4} | 10--15 | 1.0182 | 1.0384 | 8.64 | 2.105 |
| track_selection | positive charged particles | thrust | v2{6} | 10--15 | 1.0179 | 1.0553 | 3.463 | 3.086 |
| track_selection | positive charged particles | thrust | v2{8} | 15--20 | 0.98867 | 1.0464 | 1.955 | -4.096 |
| track_selection | 0.4<pT<1 GeV | beam | v2{2} | 0--10 | 1.009 | 1.0183 | 33.21 | 2.03 |
| track_selection | 0.4<pT<1 GeV | beam | v2{4} | 0--10 | 1.008 | 1.0151 | 11.66 | 1.886 |
| track_selection | 0.4<pT<1 GeV | beam | v2{6} | 0--10 | 1.0084 | 1.0163 | 12.52 | 1.931 |
| track_selection | 0.4<pT<1 GeV | beam | v2{8} | 0--10 | 1.0086 | 1.0165 | 12.46 | 1.925 |
| track_selection | 0.4<pT<1 GeV | thrust | v2{2} | 0--10 | 1.0319 | 1.0643 | 44.58 | 2.015 |
| track_selection | 0.4<pT<1 GeV | thrust | v2{4} | 10--15 | 1.0583 | 1.0691 | 3.124 | 1.186 |
| track_selection | 0.4<pT<1 GeV | thrust | v2{6} | 15--20 | 0.84071 | 1.1237 | 1.27 | -0.7765 |
| track_selection | 0.4<pT<1 GeV | thrust | v2{8} | 15--20 | 1.0465 | 1.1299 | 1.011 | 2.793 |
| track_selection | 0.4<pT<2 GeV | beam | v2{2} | 10--15 | 1.0068 | 1.0131 | 24.3 | 1.917 |
| track_selection | 0.4<pT<2 GeV | beam | v2{4} | 10--15 | 1.007 | 1.0135 | 21.13 | 1.924 |
| track_selection | 0.4<pT<2 GeV | beam | v2{6} | 10--15 | 1.0072 | 1.0138 | 21.13 | 1.909 |
| track_selection | 0.4<pT<2 GeV | beam | v2{8} | 10--15 | 1.0073 | 1.0138 | 20.95 | 1.906 |
| track_selection | 0.4<pT<2 GeV | thrust | v2{2} | 10--15 | 1.0219 | 1.0435 | 50.76 | 1.983 |
| track_selection | 0.4<pT<2 GeV | thrust | v2{4} | 15--20 | 1.0394 | 1.0602 | 7.965 | 1.528 |
| track_selection | 0.4<pT<2 GeV | thrust | v2{6} | 15--20 | 1.0566 | 1.094 | 6.128 | 1.66 |
| track_selection | 0.4<pT<2 GeV | thrust | v2{8} | 20--25 | 1.0802 | 1.1182 | 4.942 | 1.475 |
| track_selection | 1<pT<3 GeV | beam | v2{2} | 0--10 | 1.0028 | 1.0056 | 20.75 | 1.969 |
| track_selection | 1<pT<3 GeV | beam | v2{4} | 0--10 | 1.0031 | 1.0057 | 20.67 | 1.798 |
| track_selection | 1<pT<3 GeV | beam | v2{6} | 0--10 | 1.0031 | 1.0056 | 20.45 | 1.792 |
| track_selection | 1<pT<3 GeV | beam | v2{8} | 0--10 | 1.0031 | 1.0056 | 20.5 | 1.789 |
| track_selection | 1<pT<3 GeV | thrust | v2{2} | 0--10 | 1.008 | 1.0153 | 23.66 | 1.906 |
| track_selection | 1<pT<3 GeV | thrust | v2{4} | 10--15 | 1.0069 | 1.0211 | 9.835 | 3.06 |
| track_selection | 1<pT<3 GeV | thrust | v2{6} | 10--15 | 1.0071 | 1.0224 | 10.37 | 3.156 |
| track_selection | 1<pT<3 GeV | thrust | v2{8} | 10--15 | 1.0073 | 1.0229 | 10.46 | 3.142 |
| two_subevent | two subevent, eta<0 vs eta>0 | beam | two-sub v2{2} | 20--25 | 1.0047 | 1.01 | 22.08 | 2.128 |
| two_subevent | two subevent, eta<0 vs eta>0 | beam | two-sub v2{4} | 15--20 | 1.0044 | 1.0084 | 15.36 | 1.901 |
| two_subevent | two subevent, eta<0 vs eta>0 | beam | two-sub v2{6} | 15--20 | 1.0047 | 1.0087 | 14.97 | 1.859 |
| two_subevent | two subevent, eta<0 vs eta>0 | beam | two-sub v2{8} | 15--20 | 1.0047 | 1.0087 | 14.84 | 1.845 |
| two_subevent | two subevent, eta<0 vs eta>0 | thrust | two-sub v2{2} | 15--20 | 1.0307 | 1.0652 | 17.04 | 2.123 |
| two_subevent | two subevent, eta<0 vs eta>0 | thrust | two-sub v2{8} | 20--25 | 1.0434 | 1.1121 | 0.449 | 2.58 |
| two_subevent | two subevent, gap 1.0 | beam | two-sub v2{2} | 15--20 | 1.0031 | 1.0068 | 14.26 | 2.209 |
| two_subevent | two subevent, gap 1.0 | beam | two-sub v2{4} | 15--20 | 1.0043 | 1.0097 | 14.64 | 2.28 |
| two_subevent | two subevent, gap 1.0 | beam | two-sub v2{6} | 15--20 | 1.0046 | 1.0107 | 14.33 | 2.309 |
| two_subevent | two subevent, gap 1.0 | beam | two-sub v2{8} | 15--20 | 1.0048 | 1.0111 | 14.04 | 2.304 |
| two_subevent | two subevent, gap 1.0 | thrust | two-sub v2{2} | 15--20 | 1.0346 | 1.0778 | 8.509 | 2.245 |
| two_subevent | two subevent, gap 1.6 | beam | two-sub v2{2} | 15--20 | 1.0028 | 1.007 | 12.93 | 2.526 |
| two_subevent | two subevent, gap 1.6 | beam | two-sub v2{4} | 15--20 | 1.0035 | 1.0095 | 10.5 | 2.744 |
| two_subevent | two subevent, gap 1.6 | beam | two-sub v2{6} | 15--20 | 1.0038 | 1.0106 | 9.148 | 2.804 |
| two_subevent | two subevent, gap 1.6 | beam | two-sub v2{8} | 15--20 | 1.0041 | 1.0112 | 8.272 | 2.69 |
| two_subevent | two subevent, gap 1.6 | thrust | two-sub v2{2} | 15--20 | 1.0243 | 1.059 | 4.79 | 2.425 |
| two_subevent | two subevent, gap 2.0 | beam | two-sub v2{2} | 15--20 | 1.0029 | 1.0077 | 12.18 | 2.654 |
| two_subevent | two subevent, gap 2.0 | beam | two-sub v2{4} | 15--20 | 1.0046 | 1.0112 | 7.607 | 2.419 |
| two_subevent | two subevent, gap 2.0 | beam | two-sub v2{6} | 15--20 | 1.0052 | 1.013 | 5.954 | 2.488 |
| two_subevent | two subevent, gap 2.0 | beam | two-sub v2{8} | 10--15 | 1.006 | 1.0055 | 5.38 | 0.9145 |
| two_subevent | two subevent, gap 2.0 | thrust | two-sub v2{2} | 10--15 | 1.0368 | 1.079 | 3.304 | 2.147 |

## Output Figures

The three-sample overlays are written as `output/pythia_zpole_5M_*_noshoving_vs_shoving_vs_shoving2x*.pdf` and copied to `docs/figures`, `AnalysisNote/figures`, and `overleaf_note/figures`. The ratio panels show nominal shoving/no-shoving and factor-two shoving/no-shoving together.

## Interpretation Notes

- This comparison tests response to increasing the Gleipnir repulsion factor, not a calibrated uncertainty on the shoving model.
- The configurations are statistically matched by setup and seed blocks, but they are not event-by-event paired because shoving changes random-number consumption during hadronization.
- High-order cumulants are included only where the signed cumulant gives a real, positive plotted value in the jackknife summary.

