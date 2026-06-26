#!/usr/bin/env bash
set -euo pipefail

PYTHIA_ROOT=${PYTHIA_ROOT:-/mnt/data2/data2/yjlee/pythia/pythia8/develop/pythia-shoving-branch}
PYTHIA8_CONFIG=${PYTHIA8_CONFIG:-${PYTHIA_ROOT}/bin/pythia8-config}
PYTHIA8DATA=${PYTHIA8DATA:-${PYTHIA_ROOT}/share/Pythia8/xmldoc}
ROOT_BINDIR=${ROOT_BINDIR:-$(root-config --bindir)}
HADD=${HADD:-${ROOT_BINDIR}/hadd}
BASE_EVENTS=${BASE_EVENTS:-3000000}
EXTRA_EVENTS=${EXTRA_EVENTS:-2000000}
BASE_SEED=${BASE_SEED:-12345}
EXTRA_SEED=${EXTRA_SEED:-67890}
SHOVING_REPULSION_FACTOR=${SHOVING_REPULSION_FACTOR:-0.50}

SH2_BASE=${SH2_BASE:-output/pythia_shoving2x_zpole_3M.root}
SH2_EXTRA=${SH2_EXTRA:-output/pythia_shoving2x_zpole_extra2M_seed67890.root}
SH2_OUT=${SH2_OUT:-output/pythia_shoving2x_zpole_5M.root}
SH2_BASE_LOG=${SH2_BASE_LOG:-output/pythia_shoving2x_zpole_3M.log}
SH2_EXTRA_LOG=${SH2_EXTRA_LOG:-output/pythia_shoving2x_zpole_extra2M_seed67890.log}
SH2_HADD_LOG=${SH2_HADD_LOG:-output/pythia_shoving2x_zpole_5M_hadd.log}

mkdir -p output
make pythia PYTHIA8_CONFIG="${PYTHIA8_CONFIG}"

if [[ ! -f "${SH2_BASE}" ]]; then
  EVENTS="${BASE_EVENTS}" \
  SEED="${BASE_SEED}" \
  OUTPUT="${SH2_BASE}" \
  LOG="${SH2_BASE_LOG}" \
  SHOVING_REPULSION_FACTOR="${SHOVING_REPULSION_FACTOR}" \
  PYTHIA_ROOT="${PYTHIA_ROOT}" \
  PYTHIA8_CONFIG="${PYTHIA8_CONFIG}" \
  PYTHIA8DATA="${PYTHIA8DATA}" \
    scripts/run_pythia_shoving_zpole_3M.sh
fi

if [[ ! -f "${SH2_EXTRA}" ]]; then
  EVENTS="${EXTRA_EVENTS}" \
  SEED="${EXTRA_SEED}" \
  OUTPUT="${SH2_EXTRA}" \
  LOG="${SH2_EXTRA_LOG}" \
  SHOVING_REPULSION_FACTOR="${SHOVING_REPULSION_FACTOR}" \
  PYTHIA_ROOT="${PYTHIA_ROOT}" \
  PYTHIA8_CONFIG="${PYTHIA8_CONFIG}" \
  PYTHIA8DATA="${PYTHIA8DATA}" \
    scripts/run_pythia_shoving_zpole_3M.sh
fi

"${HADD}" -f "${SH2_OUT}" "${SH2_BASE}" "${SH2_EXTRA}" >"${SH2_HADD_LOG}" 2>&1
entries=$(bin/aleph_charged_cumulants --Input "${SH2_OUT}" --Tree t --InputFormat vector --PrintEntries 1 | tail -n 1)
echo "${SH2_OUT}: ${entries} entries"
if [[ "${entries}" != "5000000" ]]; then
  echo "Expected 5000000 entries in ${SH2_OUT}, got ${entries}" >&2
  exit 1
fi

sha256sum "${SH2_BASE}" "${SH2_EXTRA}" "${SH2_OUT}"
