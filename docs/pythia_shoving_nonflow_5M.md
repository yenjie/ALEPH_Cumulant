# Pythia Shoving Nonflow-Suppression Study, 5M Events

This document records the 5M-event generator-level comparison of Pythia no-shoving and Gleipnir-shoving Z-pole samples under the nonflow-suppression handles used in the analysis note.

## Samples

- Final 5M inputs: `output/pythia_noshoving_zpole_5M.root` and `output/pythia_shoving_zpole_5M.root`.
- Composition used for this production: existing seed-12345 3M samples plus independent seed-67890 2M extensions merged with `hadd`.
- Seed-67890 no-shoving extension: 2000000 events, 40695572 StudyMult-selected charged particles.
- Seed-67890 shoving extension: 2000000 events, 40699476 StudyMult-selected charged particles.
- Final merged entry count was checked with `bin/aleph_charged_cumulants --PrintEntries 1` and required to be exactly 5,000,000 entries for each sample.
- Both configurations use the same hard process, visible-particle convention, multiplicity bins `0,10,15,20,25,30,35,40,999`, and 16-block jackknife summaries.

## Commands

```bash
scripts/build_pythia_zpole_5M_samples.sh
ALEPH_MAX_PARALLEL=16 CHUNKS=16 RUN_NONFLOW=1 scripts/run_pythia_shoving_5M_analysis.sh
scripts/plot_pythia_nonflow_suppression_5M.sh
scripts/summarize_pythia_shoving_nonflow.py
```

## Nonflow-Suppression Techniques

- Rapidity gaps for the two-particle observable: `|delta eta| > 1.0, 1.6, 2.0, 2.5, 3.0`.
- Two-subevent cumulants: `eta<0` versus `eta>0`, and excluded middle-region gaps of 1.0, 1.6, and 2.0 from `TwoSubeventEtaBoundary = 0.5, 0.8, 1.0`.
- Track selections: `0.4<pT<1`, `0.4<pT<2`, `1<pT<3`, positive charges only, and negative charges only.
- Event-shape selections: `Thrust<0.90`, `Thrust<0.85`, `Sphericity>0.12`, and `Sphericity>0.22`. For generated vector trees the analyzer computes thrust and sphericity from visible final-particle momenta because those branches are not stored in the generated tree.

## Result Summary

The table gives the most significant shoving-minus-no-shoving bin for each technique, axis, and observable. The significance is `(shoving-no-shoving)/sqrt(err_no^2+err_sh^2)`. The ratio is `shoving/no-shoving`.

| Technique | Case | Axis | Observable | Bin | Ratio | Significance |
|---|---|---|---|---:|---:|---:|
| baseline | Nominal charged pT>0.4 GeV | beam | v2{2} | 20--25 | 1.0047 | 11.55 |
| baseline | Nominal charged pT>0.4 GeV | beam | v2{4} | 10--15 | 1.003 | 8.988 |
| baseline | Nominal charged pT>0.4 GeV | beam | v2{6} | 10--15 | 1.0031 | 8.964 |
| baseline | Nominal charged pT>0.4 GeV | beam | v2{8} | 10--15 | 1.0031 | 8.961 |
| baseline | Nominal charged pT>0.4 GeV | thrust | v2{2} | 15--20 | 1.0161 | 21.4 |
| baseline | Nominal charged pT>0.4 GeV | thrust | v2{4} | 25--30 | 1.0315 | 5.042 |
| baseline | Nominal charged pT>0.4 GeV | thrust | v2{6} | 25--30 | 1.0525 | 4.241 |
| baseline | Nominal charged pT>0.4 GeV | thrust | v2{8} | 25--30 | 1.0852 | 3.582 |
| eta_gap | \|delta eta\|>1.0 | beam | gap v2{2} | 15--20 | 1.0039 | 6.567 |
| eta_gap | \|delta eta\|>1.0 | thrust | gap v2{2} | 15--20 | 1.0195 | 11.6 |
| eta_gap | \|delta eta\|>1.6 | beam | gap v2{2} | 10--15 | 1.0032 | 5.962 |
| eta_gap | \|delta eta\|>1.6 | thrust | gap v2{2} | 10--15 | 1.0195 | 6.672 |
| eta_gap | \|delta eta\|>2.0 | beam | gap v2{2} | 10--15 | 1.0037 | 6.173 |
| eta_gap | \|delta eta\|>2.0 | thrust | gap v2{2} | 10--15 | 1.0219 | 5.518 |
| eta_gap | \|delta eta\|>2.5 | beam | gap v2{2} | 10--15 | 1.0037 | 4.313 |
| eta_gap | \|delta eta\|>2.5 | thrust | gap v2{2} | 10--15 | 1.0215 | 3.378 |
| eta_gap | \|delta eta\|>3.0 | beam | gap v2{2} | 10--15 | 1.0033 | 2.747 |
| eta_gap | \|delta eta\|>3.0 | thrust | gap v2{2} | 10--15 | 1.0272 | 2.728 |
| event_shape | Sphericity>0.12 | beam | v2{2} | 30--35 | 1.022 | 5.591 |
| event_shape | Sphericity>0.12 | beam | v2{4} | 30--35 | 1.0263 | 3.409 |
| event_shape | Sphericity>0.12 | beam | v2{6} | 30--35 | 1.0267 | 2.858 |
| event_shape | Sphericity>0.12 | beam | v2{8} | 30--35 | 1.0267 | 2.7 |
| event_shape | Sphericity>0.12 | thrust | v2{2} | 20--25 | 1.0204 | 13.51 |
| event_shape | Sphericity>0.12 | thrust | v2{4} | 20--25 | 1.0203 | 9.869 |
| event_shape | Sphericity>0.12 | thrust | v2{6} | 20--25 | 1.0207 | 9.764 |
| event_shape | Sphericity>0.12 | thrust | v2{8} | 20--25 | 1.0209 | 9.736 |
| event_shape | Sphericity>0.22 | beam | v2{2} | 20--25 | 1.0087 | 5.816 |
| event_shape | Sphericity>0.22 | beam | v2{4} | 20--25 | 1.0104 | 2.694 |
| event_shape | Sphericity>0.22 | beam | v2{6} | 20--25 | 1.0098 | 2.078 |
| event_shape | Sphericity>0.22 | beam | v2{8} | 20--25 | 1.0094 | 1.866 |
| event_shape | Sphericity>0.22 | thrust | v2{2} | 20--25 | 1.0189 | 10.97 |
| event_shape | Sphericity>0.22 | thrust | v2{4} | 20--25 | 1.0162 | 6.505 |
| event_shape | Sphericity>0.22 | thrust | v2{6} | 20--25 | 1.0155 | 6.183 |
| event_shape | Sphericity>0.22 | thrust | v2{8} | 20--25 | 1.0155 | 6.114 |
| event_shape | Thrust<0.85 | beam | v2{2} | 20--25 | 1.0071 | 4.741 |
| event_shape | Thrust<0.85 | beam | v2{4} | 20--25 | 1.0061 | 1.799 |
| event_shape | Thrust<0.85 | beam | v2{6} | 20--25 | 1.0044 | 1.214 |
| event_shape | Thrust<0.85 | beam | v2{8} | 10--15 | 0.98907 | -1.151 |
| event_shape | Thrust<0.85 | thrust | v2{2} | 20--25 | 1.0191 | 14.51 |
| event_shape | Thrust<0.85 | thrust | v2{4} | 20--25 | 1.0178 | 8.721 |
| event_shape | Thrust<0.85 | thrust | v2{6} | 20--25 | 1.018 | 8.615 |
| event_shape | Thrust<0.85 | thrust | v2{8} | 20--25 | 1.0181 | 8.467 |
| event_shape | Thrust<0.90 | beam | v2{2} | 20--25 | 1.0044 | 5.397 |
| event_shape | Thrust<0.90 | beam | v2{4} | 30--35 | 1.0233 | 3.183 |
| event_shape | Thrust<0.90 | beam | v2{6} | 30--35 | 1.0238 | 2.825 |
| event_shape | Thrust<0.90 | beam | v2{8} | 30--35 | 1.0238 | 2.712 |
| event_shape | Thrust<0.90 | thrust | v2{2} | 20--25 | 1.0194 | 14.59 |
| event_shape | Thrust<0.90 | thrust | v2{4} | 20--25 | 1.0184 | 9.366 |
| event_shape | Thrust<0.90 | thrust | v2{6} | 15--20 | 1.0168 | 9.616 |
| event_shape | Thrust<0.90 | thrust | v2{8} | 15--20 | 1.0169 | 10.33 |
| track_selection | negative charged particles | beam | v2{2} | 0--10 | 1.0031 | 11.38 |
| track_selection | negative charged particles | beam | v2{4} | 0--10 | 1.003 | 10.19 |
| track_selection | negative charged particles | beam | v2{6} | 0--10 | 1.003 | 10.13 |
| track_selection | negative charged particles | beam | v2{8} | 0--10 | 1.003 | 10.14 |
| track_selection | negative charged particles | thrust | v2{2} | 10--15 | 1.0196 | 17.57 |
| track_selection | negative charged particles | thrust | v2{4} | 10--15 | 1.0231 | 5.525 |
| track_selection | negative charged particles | thrust | v2{6} | 10--15 | 1.0621 | 3.193 |
| track_selection | negative charged particles | thrust | v2{8} | 15--20 | 1.0094 | 0.3246 |
| track_selection | positive charged particles | beam | v2{2} | 10--15 | 1.0052 | 11.15 |
| track_selection | positive charged particles | beam | v2{4} | 0--10 | 1.003 | 9.949 |
| track_selection | positive charged particles | beam | v2{6} | 0--10 | 1.003 | 9.997 |
| track_selection | positive charged particles | beam | v2{8} | 0--10 | 1.003 | 9.935 |
| track_selection | positive charged particles | thrust | v2{2} | 10--15 | 1.0188 | 16.35 |
| track_selection | positive charged particles | thrust | v2{4} | 10--15 | 1.0182 | 3.788 |
| track_selection | positive charged particles | thrust | v2{6} | 10--15 | 1.0179 | 1.165 |
| track_selection | positive charged particles | thrust | v2{8} | 15--20 | 0.98867 | -0.4705 |
| track_selection | 0.4<pT<1 GeV | beam | v2{2} | 0--10 | 1.009 | 16.13 |
| track_selection | 0.4<pT<1 GeV | beam | v2{4} | 0--10 | 1.008 | 6.17 |
| track_selection | 0.4<pT<1 GeV | beam | v2{6} | 0--10 | 1.0084 | 6.325 |
| track_selection | 0.4<pT<1 GeV | beam | v2{8} | 0--10 | 1.0086 | 6.102 |
| track_selection | 0.4<pT<1 GeV | thrust | v2{2} | 0--10 | 1.0319 | 30.28 |
| track_selection | 0.4<pT<1 GeV | thrust | v2{4} | 10--15 | 1.0583 | 3.339 |
| track_selection | 0.4<pT<1 GeV | thrust | v2{6} | 10--15 | 1.1869 | 1.187 |
| track_selection | 0.4<pT<1 GeV | thrust | v2{8} | 15--20 | 1.0465 | 0.3152 |
| track_selection | 0.4<pT<2 GeV | beam | v2{2} | 10--15 | 1.0068 | 12.27 |
| track_selection | 0.4<pT<2 GeV | beam | v2{4} | 10--15 | 1.007 | 10.17 |
| track_selection | 0.4<pT<2 GeV | beam | v2{6} | 0--10 | 1.0049 | 10.67 |
| track_selection | 0.4<pT<2 GeV | beam | v2{8} | 0--10 | 1.0049 | 10.82 |
| track_selection | 0.4<pT<2 GeV | thrust | v2{2} | 10--15 | 1.0219 | 22.3 |
| track_selection | 0.4<pT<2 GeV | thrust | v2{4} | 15--20 | 1.0394 | 5.02 |
| track_selection | 0.4<pT<2 GeV | thrust | v2{6} | 20--25 | 1.0571 | 3.519 |
| track_selection | 0.4<pT<2 GeV | thrust | v2{8} | 20--25 | 1.0802 | 3.448 |
| track_selection | 1<pT<3 GeV | beam | v2{2} | 0--10 | 1.0028 | 9.852 |
| track_selection | 1<pT<3 GeV | beam | v2{4} | 0--10 | 1.0031 | 10.06 |
| track_selection | 1<pT<3 GeV | beam | v2{6} | 0--10 | 1.0031 | 10.11 |
| track_selection | 1<pT<3 GeV | beam | v2{8} | 0--10 | 1.0031 | 10.15 |
| track_selection | 1<pT<3 GeV | thrust | v2{2} | 0--10 | 1.008 | 13.67 |
| track_selection | 1<pT<3 GeV | thrust | v2{4} | 0--10 | 1.01 | 5.366 |
| track_selection | 1<pT<3 GeV | thrust | v2{6} | 0--10 | 1.0132 | 6.128 |
| track_selection | 1<pT<3 GeV | thrust | v2{8} | 0--10 | 1.0152 | 7.617 |
| two_subevent | two subevent, eta<0 vs eta>0 | beam | two-sub v2{2} | 20--25 | 1.0047 | 9.251 |
| two_subevent | two subevent, eta<0 vs eta>0 | beam | two-sub v2{4} | 15--20 | 1.0044 | 7.583 |
| two_subevent | two subevent, eta<0 vs eta>0 | beam | two-sub v2{6} | 15--20 | 1.0047 | 7.205 |
| two_subevent | two subevent, eta<0 vs eta>0 | beam | two-sub v2{8} | 15--20 | 1.0047 | 7.153 |
| two_subevent | two subevent, eta<0 vs eta>0 | thrust | two-sub v2{2} | 15--20 | 1.0307 | 6.349 |
| two_subevent | two subevent, eta<0 vs eta>0 | thrust | two-sub v2{8} | 20--25 | 1.0434 | 0.1619 |
| two_subevent | two subevent, gap 1.0 | beam | two-sub v2{2} | 10--15 | 1.0025 | 6.389 |
| two_subevent | two subevent, gap 1.0 | beam | two-sub v2{4} | 10--15 | 1.0035 | 6.771 |
| two_subevent | two subevent, gap 1.0 | beam | two-sub v2{6} | 10--15 | 1.0038 | 6.799 |
| two_subevent | two subevent, gap 1.0 | beam | two-sub v2{8} | 10--15 | 1.0039 | 6.803 |
| two_subevent | two subevent, gap 1.0 | thrust | two-sub v2{2} | 20--25 | 1.0554 | 3.761 |
| two_subevent | two subevent, gap 1.0 | thrust | two-sub v2{8} | 35--40 | 0.99492 | -0.0165 |
| two_subevent | two subevent, gap 1.6 | beam | two-sub v2{2} | 10--15 | 1.0026 | 5.775 |
| two_subevent | two subevent, gap 1.6 | beam | two-sub v2{4} | 10--15 | 1.0038 | 5.33 |
| two_subevent | two subevent, gap 1.6 | beam | two-sub v2{6} | 10--15 | 1.0043 | 5.29 |
| two_subevent | two subevent, gap 1.6 | beam | two-sub v2{8} | 10--15 | 1.0043 | 5.184 |
| two_subevent | two subevent, gap 1.6 | thrust | two-sub v2{2} | 10--15 | 1.0563 | 2.601 |
| two_subevent | two subevent, gap 1.6 | thrust | two-sub v2{8} | 35--40 | 1.0529 | 0.1558 |
| two_subevent | two subevent, gap 2.0 | beam | two-sub v2{2} | 10--15 | 1.0026 | 4.607 |
| two_subevent | two subevent, gap 2.0 | beam | two-sub v2{4} | 10--15 | 1.0052 | 5.593 |
| two_subevent | two subevent, gap 2.0 | beam | two-sub v2{6} | 10--15 | 1.0059 | 5.6 |
| two_subevent | two subevent, gap 2.0 | beam | two-sub v2{8} | 10--15 | 1.006 | 5.533 |
| two_subevent | two subevent, gap 2.0 | thrust | two-sub v2{2} | 20--25 | 1.0364 | 1.734 |
| two_subevent | two subevent, gap 2.0 | thrust | two-sub v2{6} | 15--20 | 0.79973 | -0.5107 |

## Output Figures

The per-technique overlays are written as `output/pythia_zpole_5M_nonflow_<case>_noshoving_vs_shoving_<axis>.pdf`; the full 5M PDF/CSV figure set is copied to `docs/figures`, `AnalysisNote/figures`, and `overleaf_note/figures`. The nominal comparison is `pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_<axis>.pdf`.

## Interpretation Notes

- The comparison tests whether the shoving/no-shoving difference remains after common nonflow-suppression handles, not whether the absolute Pythia cumulant is collective flow.
- The two samples are matched by generator setup and seed choices, but they are not event-by-event paired after shoving is enabled because the shoving model changes random-number consumption during hadronization.
- High-order cumulants are shown only where the signed cumulant gives a real-valued `v2{2k}` in the jackknife summary.
- Ratio and significance values in this note use the exported jackknife uncertainties and do not include covariance between the two generator configurations.

