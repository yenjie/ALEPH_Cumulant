#!/usr/bin/env bash

set -euo pipefail

MC=${MC:-/mnt/data3/data3/yjlee/StudyMultSamples/LEP1MCMerged.root}
CHUNKS=${CHUNKS:-16}
export ALEPH_MAX_PARALLEL=${ALEPH_MAX_PARALLEL:-8}

BINS="0,10,15,20,25,30,35,40,999"
BASE_ARGS=(--InputFormat array --Tree t --LabPtMin 0.4 --ThrustPtMin 0.4 --MultiplicityBins "$BINS")

run_case() {
  local label=$1
  local prefix=$2
  shift 2
  echo "=== ${label}: ${prefix} ==="
  scripts/run_chunks.sh "$MC" "$prefix" "$CHUNKS" "${BASE_ARGS[@]}" "$@"
}

# Eta-gap two-particle scans. The nominal 1.6 and 2.0 outputs already exist in
# the main production, but keeping the commands here documents the complete scan.
run_case "eta gap 1.0" output/nonflow_mc_pt04_etagap1p0 --EtaGapMin 1.0
run_case "eta gap 2.5" output/nonflow_mc_pt04_etagap2p5 --EtaGapMin 2.5
run_case "eta gap 3.0" output/nonflow_mc_pt04_etagap3p0 --EtaGapMin 3.0

# Two-subevent scans. Boundary b means A: eta < -b and B: eta > b, so the
# excluded middle region corresponds to a 2b gap between subevents.
run_case "two-subevent gap 1.0" output/nonflow_mc_pt04_twosub_gap1p0 --TwoSubeventEtaBoundary 0.5
run_case "two-subevent gap 1.6" output/nonflow_mc_pt04_twosub_gap1p6 --TwoSubeventEtaBoundary 0.8
run_case "two-subevent gap 2.0" output/nonflow_mc_pt04_twosub_gap2p0 --TwoSubeventEtaBoundary 1.0

# Soft-track selections to suppress hard jet fragments.
run_case "soft pT 0.4-1" output/nonflow_mc_pt04to1 --LabPtMax 1.0 --ThrustPtMax 1.0
run_case "soft pT 0.4-2" output/nonflow_mc_pt04to2 --LabPtMax 2.0 --ThrustPtMax 2.0

# Charge-sign selections. These probe unlike-sign resonance/charge-balance
# contamination by comparing positive-only and negative-only charged tracks.
run_case "positive tracks" output/nonflow_mc_positive_pt04 --ChargeSelection positive
run_case "negative tracks" output/nonflow_mc_negative_pt04 --ChargeSelection negative

# Anti-jetty event-shape selections. These use event-level StudyMult branches.
run_case "thrust < 0.90" output/nonflow_mc_thrustlt0p90_pt04 --ThrustMax 0.90
run_case "thrust < 0.85" output/nonflow_mc_thrustlt0p85_pt04 --ThrustMax 0.85
run_case "sphericity > 0.12" output/nonflow_mc_sphericitygt0p12_pt04 --SphericityMin 0.12
run_case "sphericity > 0.22" output/nonflow_mc_sphericitygt0p22_pt04 --SphericityMin 0.22
