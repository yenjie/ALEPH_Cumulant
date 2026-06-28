#!/usr/bin/env bash
set -euo pipefail

PYTHIA_ROOT=${PYTHIA_ROOT:-/mnt/data2/data2/yjlee/pythia/pythia8/develop/pythia-shoving-branch}
PYTHIA8_CONFIG=${PYTHIA8_CONFIG:-${PYTHIA_ROOT}/bin/pythia8-config}
PYTHIA8DATA=${PYTHIA8DATA:-${PYTHIA_ROOT}/share/Pythia8/xmldoc}
ROOT_BINDIR=${ROOT_BINDIR:-$(root-config --bindir)}
HADD=${HADD:-${ROOT_BINDIR}/hadd}
ECM=${ECM:-200.0}
BASE_EVENTS=${BASE_EVENTS:-3000000}
EXTRA_EVENTS=${EXTRA_EVENTS:-2000000}
BASE_SEED=${BASE_SEED:-12345}
EXTRA_SEED=${EXTRA_SEED:-67890}
NOMINAL_SHOVING_REPULSION_FACTOR=${NOMINAL_SHOVING_REPULSION_FACTOR:-0.25}
ENHANCED_SHOVING_REPULSION_FACTOR=${ENHANCED_SHOVING_REPULSION_FACTOR:-0.50}
GENERATION_MAX_PARALLEL=${GENERATION_MAX_PARALLEL:-3}
TOTAL_EVENTS=$((BASE_EVENTS + EXTRA_EVENTS))

NO_BASE=${NO_BASE:-output/pythia_noshoving_qq200_3M.root}
NO_EXTRA=${NO_EXTRA:-output/pythia_noshoving_qq200_extra2M_seed67890.root}
NO_OUT=${NO_OUT:-output/pythia_noshoving_qq200_5M.root}
NO_BASE_LOG=${NO_BASE_LOG:-output/pythia_noshoving_qq200_3M.log}
NO_EXTRA_LOG=${NO_EXTRA_LOG:-output/pythia_noshoving_qq200_extra2M_seed67890.log}
NO_HADD_LOG=${NO_HADD_LOG:-output/pythia_noshoving_qq200_5M_hadd.log}

SH_BASE=${SH_BASE:-output/pythia_shoving_qq200_3M.root}
SH_EXTRA=${SH_EXTRA:-output/pythia_shoving_qq200_extra2M_seed67890.root}
SH_OUT=${SH_OUT:-output/pythia_shoving_qq200_5M.root}
SH_BASE_LOG=${SH_BASE_LOG:-output/pythia_shoving_qq200_3M.log}
SH_EXTRA_LOG=${SH_EXTRA_LOG:-output/pythia_shoving_qq200_extra2M_seed67890.log}
SH_HADD_LOG=${SH_HADD_LOG:-output/pythia_shoving_qq200_5M_hadd.log}

SH2_BASE=${SH2_BASE:-output/pythia_shoving2x_qq200_3M.root}
SH2_EXTRA=${SH2_EXTRA:-output/pythia_shoving2x_qq200_extra2M_seed67890.root}
SH2_OUT=${SH2_OUT:-output/pythia_shoving2x_qq200_5M.root}
SH2_BASE_LOG=${SH2_BASE_LOG:-output/pythia_shoving2x_qq200_3M.log}
SH2_EXTRA_LOG=${SH2_EXTRA_LOG:-output/pythia_shoving2x_qq200_extra2M_seed67890.log}
SH2_HADD_LOG=${SH2_HADD_LOG:-output/pythia_shoving2x_qq200_5M_hadd.log}

mkdir -p output
make pythia PYTHIA8_CONFIG="${PYTHIA8_CONFIG}"

entry_count() {
  local file=$1
  bin/aleph_charged_cumulants --Input "${file}" --Tree t --InputFormat vector --PrintEntries 1 2>/dev/null | tail -n 1
}

sample_ready() {
  local output=$1
  local expected=$2
  if [[ ! -f "${output}" ]]; then
    return 1
  fi
  local entries
  entries=$(entry_count "${output}" || true)
  if [[ "${entries}" == "${expected}" ]]; then
    echo "Using existing complete ${output} (${entries} entries)"
    return 0
  fi
  echo "Removing incomplete ${output}: found '${entries}', expected ${expected}" >&2
  rm -f "${output}"
  return 1
}

run_sample() {
  local output=$1
  local log=$2
  local events=$3
  local seed=$4
  local enable_shoving=$5
  local repulsion=$6
  if sample_ready "${output}" "${events}"; then
    return
  fi
  EVENTS="${events}" \
  SEED="${seed}" \
  ECM="${ECM}" \
  OUTPUT="${output}" \
  LOG="${log}" \
  ENABLE_SHOVING="${enable_shoving}" \
  SHOVING_REPULSION_FACTOR="${repulsion}" \
  PYTHIA_ROOT="${PYTHIA_ROOT}" \
  PYTHIA8_CONFIG="${PYTHIA8_CONFIG}" \
  PYTHIA8DATA="${PYTHIA8DATA}" \
  RUN_MAKE=0 \
    scripts/run_pythia_qq200_sample.sh
  sample_ready "${output}" "${events}"
}

running_jobs=0
launch() {
  local name=$1
  shift
  echo "Launching ${name}"
  (run_sample "$@") &
  running_jobs=$((running_jobs + 1))
  if [[ "${running_jobs}" -ge "${GENERATION_MAX_PARALLEL}" ]]; then
    wait -n
    running_jobs=$((running_jobs - 1))
  fi
}

launch no_base "${NO_BASE}" "${NO_BASE_LOG}" "${BASE_EVENTS}" "${BASE_SEED}" 0 "${NOMINAL_SHOVING_REPULSION_FACTOR}"
launch no_extra "${NO_EXTRA}" "${NO_EXTRA_LOG}" "${EXTRA_EVENTS}" "${EXTRA_SEED}" 0 "${NOMINAL_SHOVING_REPULSION_FACTOR}"
launch sh_base "${SH_BASE}" "${SH_BASE_LOG}" "${BASE_EVENTS}" "${BASE_SEED}" 1 "${NOMINAL_SHOVING_REPULSION_FACTOR}"
launch sh_extra "${SH_EXTRA}" "${SH_EXTRA_LOG}" "${EXTRA_EVENTS}" "${EXTRA_SEED}" 1 "${NOMINAL_SHOVING_REPULSION_FACTOR}"
launch sh2_base "${SH2_BASE}" "${SH2_BASE_LOG}" "${BASE_EVENTS}" "${BASE_SEED}" 1 "${ENHANCED_SHOVING_REPULSION_FACTOR}"
launch sh2_extra "${SH2_EXTRA}" "${SH2_EXTRA_LOG}" "${EXTRA_EVENTS}" "${EXTRA_SEED}" 1 "${ENHANCED_SHOVING_REPULSION_FACTOR}"

while [[ "${running_jobs}" -gt 0 ]]; do
  wait -n
  running_jobs=$((running_jobs - 1))
done

"${HADD}" -f "${NO_OUT}" "${NO_BASE}" "${NO_EXTRA}" >"${NO_HADD_LOG}" 2>&1
"${HADD}" -f "${SH_OUT}" "${SH_BASE}" "${SH_EXTRA}" >"${SH_HADD_LOG}" 2>&1
"${HADD}" -f "${SH2_OUT}" "${SH2_BASE}" "${SH2_EXTRA}" >"${SH2_HADD_LOG}" 2>&1

for sample in "${NO_OUT}" "${SH_OUT}" "${SH2_OUT}"; do
  entries=$(entry_count "${sample}")
  echo "${sample}: ${entries} entries"
  if [[ "${entries}" != "${TOTAL_EVENTS}" ]]; then
    echo "Expected ${TOTAL_EVENTS} entries in ${sample}, got ${entries}" >&2
    exit 1
  fi
done

sha256sum "${NO_BASE}" "${NO_EXTRA}" "${NO_OUT}" \
          "${SH_BASE}" "${SH_EXTRA}" "${SH_OUT}" \
          "${SH2_BASE}" "${SH2_EXTRA}" "${SH2_OUT}"
