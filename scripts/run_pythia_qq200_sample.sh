#!/usr/bin/env bash
set -euo pipefail

PYTHIA_ROOT=${PYTHIA_ROOT:-/mnt/data2/data2/yjlee/pythia/pythia8/develop/pythia-shoving-branch}
PYTHIA8_CONFIG=${PYTHIA8_CONFIG:-${PYTHIA_ROOT}/bin/pythia8-config}
PYTHIA8DATA=${PYTHIA8DATA:-${PYTHIA_ROOT}/share/Pythia8/xmldoc}
EVENTS=${EVENTS:-1000000}
SEED=${SEED:-12345}
ECM=${ECM:-200.0}
OUTPUT=${OUTPUT:-output/pythia_qq200.root}
LOG=${LOG:-output/pythia_qq200.log}
ENABLE_SHOVING=${ENABLE_SHOVING:-0}
SHOVING_REPULSION_FACTOR=${SHOVING_REPULSION_FACTOR:-0.25}
REPORT_EVERY=${REPORT_EVERY:-50000}

mkdir -p "$(dirname "${OUTPUT}")" "$(dirname "${LOG}")"

if [[ "${RUN_MAKE:-1}" == "1" ]]; then
  make pythia PYTHIA8_CONFIG="${PYTHIA8_CONFIG}"
fi

bin/generate_pythia_zpole_root \
  --Output "${OUTPUT}" \
  --Events "${EVENTS}" \
  --Seed "${SEED}" \
  --ECM "${ECM}" \
  --Process zpole \
  --PythiaData "${PYTHIA8DATA}" \
  --EnableShoving "${ENABLE_SHOVING}" \
  --ShovingRepulsionFactor "${SHOVING_REPULSION_FACTOR}" \
  --ApplyAlephLikeAcceptance 0 \
  --ReportEvery "${REPORT_EVERY}" \
  2>&1 | tee "${LOG}"
