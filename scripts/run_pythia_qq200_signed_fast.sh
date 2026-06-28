#!/usr/bin/env bash
set -euo pipefail

CHUNKS=${CHUNKS:-16}
ALEPH_MAX_PARALLEL=${ALEPH_MAX_PARALLEL:-16}
OUT_DIR=${OUT_DIR:-LEP2QQ200_Generator_SignedShovingTest}
C2_RAW_DIR="${OUT_DIR}/signed_c2_raw"
VN_RAW_DIR="${OUT_DIR}/vndelta_raw"
LOG_DIR="${OUT_DIR}/signed_fast_logs"
NO_SHOVING=${NO_SHOVING:-output/pythia_noshoving_qq200_5M.root}
SHOVING=${SHOVING:-output/pythia_shoving_qq200_5M.root}
SHOVING2X=${SHOVING2X:-output/pythia_shoving2x_qq200_5M.root}

mkdir -p bin "${C2_RAW_DIR}" "${VN_RAW_DIR}" "${LOG_DIR}"

if [[ ! -x bin/qq200_signed_study || src/QQ200SignedStudy.cpp -nt bin/qq200_signed_study ]]; then
  g++ -O3 -std=c++17 -Wall -Wextra -Iinclude $(root-config --cflags) \
    src/QQ200SignedStudy.cpp -o bin/qq200_signed_study $(root-config --glibs)
fi

run_sample() {
  local input=$1
  local sample=$2
  local setting=$3
  local entries
  entries=$(bin/aleph_charged_cumulants --Input "${input}" --Tree t --InputFormat vector --PrintEntries 1 | tail -n 1)
  local chunk_size=$(( (entries + CHUNKS - 1) / CHUNKS ))
  local running_jobs=0

  for (( i=0; i<CHUNKS; ++i )); do
    local start=$(( i * chunk_size ))
    local end=$(( start + chunk_size ))
    if [[ "${start}" -ge "${entries}" ]]; then
      break
    fi
    if [[ "${end}" -gt "${entries}" ]]; then
      end=${entries}
    fi

    local tag
    tag=$(printf "%02d" "${i}")
    local c2_out="${C2_RAW_DIR}/${setting}_block_${tag}.csv"
    local vn_out="${VN_RAW_DIR}/${setting}_block_${tag}.csv"
    local log="${LOG_DIR}/${setting}_block_${tag}.log"
    if [[ -f "${c2_out}" && -f "${vn_out}" ]]; then
      echo "Using existing ${c2_out} and ${vn_out}"
      continue
    fi

    echo "Launching ${setting} block ${tag}: [${start}, ${end})"
    bin/qq200_signed_study \
      --Input "${input}" \
      --OutputC2 "${c2_out}" \
      --OutputVn "${vn_out}" \
      --Tree t \
      --Sample "${sample}" \
      --Setting "${setting}" \
      --StartEntry "${start}" \
      --EndEntry "${end}" \
      --BlockLabel "${i}" \
      >"${log}" 2>&1 &

    running_jobs=$((running_jobs + 1))
    if [[ "${running_jobs}" -ge "${ALEPH_MAX_PARALLEL}" ]]; then
      wait -n
      running_jobs=$((running_jobs - 1))
    fi
  done

  while [[ "${running_jobs}" -gt 0 ]]; do
    wait -n
    running_jobs=$((running_jobs - 1))
  done
}

run_sample "${NO_SHOVING}" "Pythia qqbar 200 GeV no shoving" "no_shoving"
run_sample "${SHOVING}" "Pythia qqbar 200 GeV nominal shoving" "nominal_shoving"
run_sample "${SHOVING2X}" "Pythia qqbar 200 GeV enhanced shoving" "enhanced_shoving"
