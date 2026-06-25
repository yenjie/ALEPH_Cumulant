#!/usr/bin/env bash
set -euo pipefail

CHUNKS=${CHUNKS:-16}
export ALEPH_MAX_PARALLEL=${ALEPH_MAX_PARALLEL:-16}
BINS=${BINS:-0,10,15,20,25,30,35,40,999}
NO_SHOVING=${NO_SHOVING:-output/pythia_noshoving_zpole_5M.root}
SHOVING=${SHOVING:-output/pythia_shoving_zpole_5M.root}
RUN_NONFLOW=${RUN_NONFLOW:-1}

scripts/build_pythia_zpole_5M_samples.sh

BASE_ARGS=(--Tree t --InputFormat vector --LabPtMin 0.4 --ThrustPtMin 0.4 --MultiplicityBins "$BINS")

scripts/run_chunks.sh "$NO_SHOVING" output/pythia_noshoving_zpole_5M_pt04 "$CHUNKS" "${BASE_ARGS[@]}"
scripts/run_chunks.sh "$SHOVING" output/pythia_shoving_zpole_5M_pt04 "$CHUNKS" "${BASE_ARGS[@]}"

if [[ "$RUN_NONFLOW" != "0" ]]; then
  scripts/run_pythia_nonflow_suppression_5M.sh
fi
