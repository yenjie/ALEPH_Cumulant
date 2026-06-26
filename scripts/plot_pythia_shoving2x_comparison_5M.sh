#!/usr/bin/env bash
set -euo pipefail

NO_BASE=${NO_BASE:-output/pythia_noshoving_zpole_5M_pt04_summary.root}
SH_BASE=${SH_BASE:-output/pythia_shoving_zpole_5M_pt04_summary.root}
SH2_BASE=${SH2_BASE:-output/pythia_shoving2x_zpole_5M_pt04_summary.root}
NO_PREFIX=${NO_PREFIX:-output/pythia_noshoving_zpole_5M_nonflow}
SH_PREFIX=${SH_PREFIX:-output/pythia_shoving_zpole_5M_nonflow}
SH2_PREFIX=${SH2_PREFIX:-output/pythia_shoving2x_zpole_5M_nonflow}
OUT_PREFIX=${OUT_PREFIX:-output/pythia_zpole_5M_nonflow}
BASE_OUT=${BASE_OUT:-output/pythia_zpole_5M_noshoving_vs_shoving2x_pt04_compare}

# Refresh the nominal no-shoving vs shoving pair CSVs/PDFs used as one input to
# the three-sample overlay.  This is fast and keeps the pairwise files in sync
# with the current plotting executable.
scripts/plot_pythia_nonflow_suppression_5M.sh

compare_v2_2x() {
  local case_name=$1
  local no_summary="${NO_PREFIX}_${case_name}_summary.root"
  local sh2_summary="${SH2_PREFIX}_${case_name}_summary.root"
  local out="${OUT_PREFIX}_${case_name}_noshoving_vs_shoving2x"
  echo "=== compare v2 ${case_name}, shoving2x ==="
  bin/compare_v2_multiplicity \
    --DataSummary "$no_summary" \
    --MCSummary "$sh2_summary" \
    --OutputPrefix "$out" \
    --DataLabel 'Pythia no shoving' \
    --MCLabel 'Pythia shoving x2' \
    --RatioMCOverData 1 \
    --RatioLabel 'shoving x2 / no shoving'
}

compare_eta_gap_2x() {
  local case_name=$1
  local gap_label=$2
  local no_summary=$3
  local sh2_summary=$4
  local out="${OUT_PREFIX}_${case_name}_noshoving_vs_shoving2x"
  echo "=== compare eta gap ${case_name}, shoving2x ==="
  bin/compare_v22_eta_gap \
    --DataSummary "$no_summary" \
    --MCSummary "$sh2_summary" \
    --OutputPrefix "$out" \
    --DataLabel 'Pythia no shoving' \
    --MCLabel 'Pythia shoving x2' \
    --RatioMCOverData 1 \
    --RatioLabel 'shoving x2 / no shoving' \
    --GapLabel "$gap_label" \
    --TrackLabel 'generator charged particles, p_{T}>0.4 GeV'
}

compare_two_sub_2x() {
  local case_name=$1
  local no_summary=$2
  local sh2_summary=$3
  local out="${OUT_PREFIX}_${case_name}_noshoving_vs_shoving2x"
  echo "=== compare two-subevent ${case_name}, shoving2x ==="
  bin/compare_two_subevent_v2_multiplicity \
    --DataSummary "$no_summary" \
    --MCSummary "$sh2_summary" \
    --OutputPrefix "$out" \
    --DataLabel 'Pythia no shoving' \
    --MCLabel 'Pythia shoving x2' \
    --RatioMCOverData 1 \
    --RatioLabel 'shoving x2 / no shoving'
}

bin/compare_v2_multiplicity \
  --DataSummary "$NO_BASE" \
  --MCSummary "$SH2_BASE" \
  --OutputPrefix "$BASE_OUT" \
  --DataLabel 'Pythia no shoving' \
  --MCLabel 'Pythia shoving x2' \
  --RatioMCOverData 1 \
  --RatioLabel 'shoving x2 / no shoving'

# Eta-gap v2{2} comparisons.
compare_eta_gap_2x etagap1p0 '|#Delta#eta|>1.0' "${NO_PREFIX}_etagap1p0_twosub1p0_summary.root" "${SH2_PREFIX}_etagap1p0_twosub1p0_summary.root"
compare_eta_gap_2x etagap1p6 '|#Delta#eta|>1.6' "${NO_PREFIX}_etagap1p6_twosub1p6_summary.root" "${SH2_PREFIX}_etagap1p6_twosub1p6_summary.root"
compare_eta_gap_2x etagap2p0 '|#Delta#eta|>2.0' "${NO_PREFIX}_etagap2p0_twosub2p0_summary.root" "${SH2_PREFIX}_etagap2p0_twosub2p0_summary.root"
compare_eta_gap_2x etagap2p5 '|#Delta#eta|>2.5' "${NO_PREFIX}_etagap2p5_summary.root" "${SH2_PREFIX}_etagap2p5_summary.root"
compare_eta_gap_2x etagap3p0 '|#Delta#eta|>3.0' "${NO_PREFIX}_etagap3p0_summary.root" "${SH2_PREFIX}_etagap3p0_summary.root"

# Two-subevent v2{2k} comparisons.
compare_two_sub_2x twosub0p0 "$NO_BASE" "$SH2_BASE"
compare_two_sub_2x twosub1p0 "${NO_PREFIX}_etagap1p0_twosub1p0_summary.root" "${SH2_PREFIX}_etagap1p0_twosub1p0_summary.root"
compare_two_sub_2x twosub1p6 "${NO_PREFIX}_etagap1p6_twosub1p6_summary.root" "${SH2_PREFIX}_etagap1p6_twosub1p6_summary.root"
compare_two_sub_2x twosub2p0 "${NO_PREFIX}_etagap2p0_twosub2p0_summary.root" "${SH2_PREFIX}_etagap2p0_twosub2p0_summary.root"

# Full-cumulant comparisons under track and event selections.
for case_name in pt04to1 pt04to2 pt1to3 positive negative thrustlt0p90 thrustlt0p85 sphgt0p12 sphgt0p22; do
  compare_v2_2x "$case_name"
done

scripts/plot_pythia_shoving2x_overlays_5M.py
