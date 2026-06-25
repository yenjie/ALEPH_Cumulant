#!/usr/bin/env bash

set -euo pipefail

CHUNKS=${CHUNKS:-16}
export ALEPH_MAX_PARALLEL=${ALEPH_MAX_PARALLEL:-16}
BINS=${BINS:-0,10,15,20,25,30,35,40,999}
NO_SHOVING=${NO_SHOVING:-output/pythia_noshoving_zpole_5M.root}
SHOVING=${SHOVING:-output/pythia_shoving_zpole_5M.root}

BASE_ARGS=(--Tree t --InputFormat vector --LabPtMin 0.4 --ThrustPtMin 0.4 --MultiplicityBins "$BINS")

run_case() {
  local sample=$1
  local input=$2
  local case_name=$3
  shift 3
  local prefix="output/pythia_${sample}_zpole_5M_nonflow_${case_name}"
  echo "=== ${sample} ${case_name}: ${prefix} ==="
  scripts/run_chunks.sh "$input" "$prefix" "$CHUNKS" "${BASE_ARGS[@]}" "$@"
}

run_sample() {
  local sample=$1
  local input=$2

  # Eta-gap and two-subevent scans.  A two-subevent boundary b means
  # A: eta < -b and B: eta > b, so the excluded middle region is 2b.
  run_case "$sample" "$input" etagap1p0_twosub1p0 --EtaGapMin 1.0 --TwoSubeventEtaBoundary 0.5
  run_case "$sample" "$input" etagap1p6_twosub1p6 --EtaGapMin 1.6 --TwoSubeventEtaBoundary 0.8
  run_case "$sample" "$input" etagap2p0_twosub2p0 --EtaGapMin 2.0 --TwoSubeventEtaBoundary 1.0
  run_case "$sample" "$input" etagap2p5 --EtaGapMin 2.5
  run_case "$sample" "$input" etagap3p0 --EtaGapMin 3.0

  # Track selections.
  run_case "$sample" "$input" pt04to1 --LabPtMax 1.0 --ThrustPtMax 1.0
  run_case "$sample" "$input" pt04to2 --LabPtMax 2.0 --ThrustPtMax 2.0
  run_case "$sample" "$input" pt1to3 --LabPtMin 1.0 --LabPtMax 3.0 --ThrustPtMin 1.0 --ThrustPtMax 3.0
  run_case "$sample" "$input" positive --ChargeSelection positive
  run_case "$sample" "$input" negative --ChargeSelection negative

  # Event-shape selections.  For generated vector trees, the analyzer computes
  # thrust and sphericity from the visible final-particle momenta when branches
  # are absent.
  run_case "$sample" "$input" thrustlt0p90 --ThrustMax 0.90
  run_case "$sample" "$input" thrustlt0p85 --ThrustMax 0.85
  run_case "$sample" "$input" sphgt0p12 --SphericityMin 0.12
  run_case "$sample" "$input" sphgt0p22 --SphericityMin 0.22
}

run_sample noshoving "$NO_SHOVING"
run_sample shoving "$SHOVING"
