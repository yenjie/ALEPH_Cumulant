#!/usr/bin/env bash
set -euo pipefail

PYTHIA_ROOT=${PYTHIA_ROOT:-/mnt/data2/data2/yjlee/pythia/pythia8/develop/pythia-shoving-branch}
PYTHIA8_CONFIG=${PYTHIA8_CONFIG:-${PYTHIA_ROOT}/bin/pythia8-config}
PYTHIA8DATA=${PYTHIA8DATA:-${PYTHIA_ROOT}/share/Pythia8/xmldoc}
EVENTS=${EVENTS:-5000000}
SEED=${SEED:-12345}
OUTPUT=${OUTPUT:-output/pythia_shoving_zpole_5M.root}
SHOVING_REPULSION_FACTOR=${SHOVING_REPULSION_FACTOR:-0.25}
LOG=${LOG:-output/pythia_shoving_zpole_5M.log}

mkdir -p "$(dirname "${OUTPUT}")" "$(dirname "${LOG}")"

make pythia PYTHIA8_CONFIG="${PYTHIA8_CONFIG}"

bin/generate_pythia_zpole_root   --Output "${OUTPUT}"   --Events "${EVENTS}"   --Seed "${SEED}"   --PythiaData "${PYTHIA8DATA}"   --EnableShoving 1 \
  --ShovingRepulsionFactor "${SHOVING_REPULSION_FACTOR}"   --ApplyAlephLikeAcceptance 0   --ReportEvery 50000   2>&1 | tee "${LOG}"
