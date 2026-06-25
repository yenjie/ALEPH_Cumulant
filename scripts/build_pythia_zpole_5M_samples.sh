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

NO_BASE=${NO_BASE:-output/pythia_noshoving_zpole_3M.root}
SH_BASE=${SH_BASE:-output/pythia_shoving_zpole_3M.root}
NO_BASE_LOG=${NO_BASE_LOG:-output/pythia_noshoving_zpole_3M.log}
SH_BASE_LOG=${SH_BASE_LOG:-output/pythia_shoving_zpole_3M.log}
NO_EXTRA=${NO_EXTRA:-output/pythia_noshoving_zpole_extra2M_seed67890.root}
SH_EXTRA=${SH_EXTRA:-output/pythia_shoving_zpole_extra2M_seed67890.root}
NO_EXTRA_LOG=${NO_EXTRA_LOG:-output/pythia_noshoving_zpole_extra2M_seed67890.log}
SH_EXTRA_LOG=${SH_EXTRA_LOG:-output/pythia_shoving_zpole_extra2M_seed67890.log}
NO_OUT=${NO_OUT:-output/pythia_noshoving_zpole_5M.root}
SH_OUT=${SH_OUT:-output/pythia_shoving_zpole_5M.root}
NO_HADD_LOG=${NO_HADD_LOG:-output/pythia_noshoving_zpole_5M_hadd.log}
SH_HADD_LOG=${SH_HADD_LOG:-output/pythia_shoving_zpole_5M_hadd.log}

mkdir -p output
make pythia PYTHIA8_CONFIG="${PYTHIA8_CONFIG}"

if [[ ! -f "${NO_BASE}" ]]; then
  EVENTS="${BASE_EVENTS}" SEED="${BASE_SEED}" OUTPUT="${NO_BASE}" LOG="${NO_BASE_LOG}"     PYTHIA_ROOT="${PYTHIA_ROOT}" PYTHIA8_CONFIG="${PYTHIA8_CONFIG}" PYTHIA8DATA="${PYTHIA8DATA}"     scripts/run_pythia_noshoving_zpole_3M.sh
fi

if [[ ! -f "${SH_BASE}" ]]; then
  EVENTS="${BASE_EVENTS}" SEED="${BASE_SEED}" OUTPUT="${SH_BASE}" LOG="${SH_BASE_LOG}"     PYTHIA_ROOT="${PYTHIA_ROOT}" PYTHIA8_CONFIG="${PYTHIA8_CONFIG}" PYTHIA8DATA="${PYTHIA8DATA}"     scripts/run_pythia_shoving_zpole_3M.sh
fi

if [[ ! -f "${NO_EXTRA}" ]]; then
  EVENTS="${EXTRA_EVENTS}" SEED="${EXTRA_SEED}" OUTPUT="${NO_EXTRA}" LOG="${NO_EXTRA_LOG}"     PYTHIA_ROOT="${PYTHIA_ROOT}" PYTHIA8_CONFIG="${PYTHIA8_CONFIG}" PYTHIA8DATA="${PYTHIA8DATA}"     scripts/run_pythia_noshoving_zpole_3M.sh
fi

if [[ ! -f "${SH_EXTRA}" ]]; then
  EVENTS="${EXTRA_EVENTS}" SEED="${EXTRA_SEED}" OUTPUT="${SH_EXTRA}" LOG="${SH_EXTRA_LOG}"     PYTHIA_ROOT="${PYTHIA_ROOT}" PYTHIA8_CONFIG="${PYTHIA8_CONFIG}" PYTHIA8DATA="${PYTHIA8DATA}"     scripts/run_pythia_shoving_zpole_3M.sh
fi

"${HADD}" -f "${NO_OUT}" "${NO_BASE}" "${NO_EXTRA}" >"${NO_HADD_LOG}" 2>&1
"${HADD}" -f "${SH_OUT}" "${SH_BASE}" "${SH_EXTRA}" >"${SH_HADD_LOG}" 2>&1

for sample in "${NO_OUT}" "${SH_OUT}"; do
  entries=$(bin/aleph_charged_cumulants --Input "${sample}" --Tree t --InputFormat vector --PrintEntries 1 | tail -n 1)
  echo "${sample}: ${entries} entries"
  if [[ "${entries}" != "5000000" ]]; then
    echo "Expected 5000000 entries in ${sample}, got ${entries}" >&2
    exit 1
  fi
done
