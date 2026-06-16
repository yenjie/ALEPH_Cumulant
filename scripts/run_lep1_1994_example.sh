#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_DIR=$(cd -- "${SCRIPT_DIR}/.." && pwd)

INPUT=${1:-/raid5/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp/LEP1Data1994_recons_aftercut-MERGED_thrust_pt04_t.root}
CHUNKS=${2:-16}
OUTPUT_PREFIX=${3:-"${REPO_DIR}/output/lep1_1994_charged_pt04"}

"${REPO_DIR}/scripts/run_chunks.sh" "$INPUT" "$OUTPUT_PREFIX" "$CHUNKS" \
  --Tree t \
  --InputFormat auto \
  --UsePassEventSelection 1 \
  --LabPtMin 0.4 \
  --ThrustPtMin 0.4 \
  --MultiplicityBins 0,10,15,20,25,30,35,40,999

