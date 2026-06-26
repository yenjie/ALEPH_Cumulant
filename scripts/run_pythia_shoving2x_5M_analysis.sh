#!/usr/bin/env bash
set -euo pipefail

CHUNKS=${CHUNKS:-16}
export ALEPH_MAX_PARALLEL=${ALEPH_MAX_PARALLEL:-16}
BINS=${BINS:-0,10,15,20,25,30,35,40,999}
SHOVING2X=${SHOVING2X:-output/pythia_shoving2x_zpole_5M.root}
RUN_NONFLOW=${RUN_NONFLOW:-1}

scripts/build_pythia_shoving2x_zpole_5M_sample.sh

BASE_ARGS=(--Tree t --InputFormat vector --LabPtMin 0.4 --ThrustPtMin 0.4 --MultiplicityBins "$BINS")

scripts/run_chunks.sh "$SHOVING2X" output/pythia_shoving2x_zpole_5M_pt04 "$CHUNKS" "${BASE_ARGS[@]}"

if [[ "$RUN_NONFLOW" == "0" ]]; then
  exit 0
fi

run_case() {
  local case_name=$1
  shift
  local prefix="output/pythia_shoving2x_zpole_5M_nonflow_${case_name}"
  echo "=== shoving2x ${case_name}: ${prefix} ==="
  scripts/run_chunks.sh "$SHOVING2X" "$prefix" "$CHUNKS" "${BASE_ARGS[@]}" "$@"
}

# Eta-gap and two-subevent scans.  A two-subevent boundary b means
# A: eta < -b and B: eta > b, so the excluded middle region is 2b.
run_case etagap1p0_twosub1p0 --EtaGapMin 1.0 --TwoSubeventEtaBoundary 0.5
run_case etagap1p6_twosub1p6 --EtaGapMin 1.6 --TwoSubeventEtaBoundary 0.8
run_case etagap2p0_twosub2p0 --EtaGapMin 2.0 --TwoSubeventEtaBoundary 1.0
run_case etagap2p5 --EtaGapMin 2.5
run_case etagap3p0 --EtaGapMin 3.0

# Track selections.
run_case pt04to1 --LabPtMax 1.0 --ThrustPtMax 1.0
run_case pt04to2 --LabPtMax 2.0 --ThrustPtMax 2.0
run_case pt1to3 --LabPtMin 1.0 --LabPtMax 3.0 --ThrustPtMin 1.0 --ThrustPtMax 3.0
run_case positive --ChargeSelection positive
run_case negative --ChargeSelection negative

# Event-shape selections.  For generated vector trees, the analyzer computes
# thrust and sphericity from the visible final-particle momenta when branches
# are absent.
run_case thrustlt0p90 --ThrustMax 0.90
run_case thrustlt0p85 --ThrustMax 0.85
run_case sphgt0p12 --SphericityMin 0.12
run_case sphgt0p22 --SphericityMin 0.22
