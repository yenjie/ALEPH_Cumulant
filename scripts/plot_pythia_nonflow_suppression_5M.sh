#!/usr/bin/env bash

set -euo pipefail

NO_BASE=${NO_BASE:-output/pythia_noshoving_zpole_5M_pt04_summary.root}
SH_BASE=${SH_BASE:-output/pythia_shoving_zpole_5M_pt04_summary.root}
NO_PREFIX=${NO_PREFIX:-output/pythia_noshoving_zpole_5M_nonflow}
SH_PREFIX=${SH_PREFIX:-output/pythia_shoving_zpole_5M_nonflow}
OUT_PREFIX=${OUT_PREFIX:-output/pythia_zpole_5M_nonflow}
BASE_OUT=${BASE_OUT:-output/pythia_zpole_5M_noshoving_vs_shoving_pt04_compare}

compare_v2() {
  local case_name=$1
  local no_summary="${NO_PREFIX}_${case_name}_summary.root"
  local sh_summary="${SH_PREFIX}_${case_name}_summary.root"
  local out="${OUT_PREFIX}_${case_name}_noshoving_vs_shoving"
  echo "=== compare v2 ${case_name} ==="
  bin/compare_v2_multiplicity     --DataSummary "$no_summary"     --MCSummary "$sh_summary"     --OutputPrefix "$out"     --DataLabel 'Pythia no shoving'     --MCLabel 'Pythia shoving'
}

compare_eta_gap() {
  local case_name=$1
  local gap_label=$2
  local no_summary=$3
  local sh_summary=$4
  local out="${OUT_PREFIX}_${case_name}_noshoving_vs_shoving"
  echo "=== compare eta gap ${case_name} ==="
  bin/compare_v22_eta_gap     --DataSummary "$no_summary"     --MCSummary "$sh_summary"     --OutputPrefix "$out"     --DataLabel 'Pythia no shoving'     --MCLabel 'Pythia shoving'     --GapLabel "$gap_label"     --TrackLabel 'generator charged particles, p_{T}>0.4 GeV'
}

compare_two_sub() {
  local case_name=$1
  local no_summary=$2
  local sh_summary=$3
  local out="${OUT_PREFIX}_${case_name}_noshoving_vs_shoving"
  echo "=== compare two-subevent ${case_name} ==="
  bin/compare_two_subevent_v2_multiplicity     --DataSummary "$no_summary"     --MCSummary "$sh_summary"     --OutputPrefix "$out"     --DataLabel 'Pythia no shoving'     --MCLabel 'Pythia shoving'
}

bin/compare_v2_multiplicity   --DataSummary "$NO_BASE"   --MCSummary "$SH_BASE"   --OutputPrefix "$BASE_OUT"   --DataLabel 'Pythia no shoving'   --MCLabel 'Pythia shoving'

# Eta-gap v2{2} comparisons.
compare_eta_gap etagap1p0 '|#Delta#eta|>1.0' "${NO_PREFIX}_etagap1p0_twosub1p0_summary.root" "${SH_PREFIX}_etagap1p0_twosub1p0_summary.root"
compare_eta_gap etagap1p6 '|#Delta#eta|>1.6' "${NO_PREFIX}_etagap1p6_twosub1p6_summary.root" "${SH_PREFIX}_etagap1p6_twosub1p6_summary.root"
compare_eta_gap etagap2p0 '|#Delta#eta|>2.0' "${NO_PREFIX}_etagap2p0_twosub2p0_summary.root" "${SH_PREFIX}_etagap2p0_twosub2p0_summary.root"
compare_eta_gap etagap2p5 '|#Delta#eta|>2.5' "${NO_PREFIX}_etagap2p5_summary.root" "${SH_PREFIX}_etagap2p5_summary.root"
compare_eta_gap etagap3p0 '|#Delta#eta|>3.0' "${NO_PREFIX}_etagap3p0_summary.root" "${SH_PREFIX}_etagap3p0_summary.root"

# Two-subevent v2{2k} comparisons.
compare_two_sub twosub0p0 "$NO_BASE" "$SH_BASE"
compare_two_sub twosub1p0 "${NO_PREFIX}_etagap1p0_twosub1p0_summary.root" "${SH_PREFIX}_etagap1p0_twosub1p0_summary.root"
compare_two_sub twosub1p6 "${NO_PREFIX}_etagap1p6_twosub1p6_summary.root" "${SH_PREFIX}_etagap1p6_twosub1p6_summary.root"
compare_two_sub twosub2p0 "${NO_PREFIX}_etagap2p0_twosub2p0_summary.root" "${SH_PREFIX}_etagap2p0_twosub2p0_summary.root"

# Full-cumulant comparisons under track and event selections.
for case_name in pt04to1 pt04to2 pt1to3 positive negative thrustlt0p90 thrustlt0p85 sphgt0p12 sphgt0p22; do
  compare_v2 "$case_name"
done
