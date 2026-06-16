#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <input.root> <output-prefix> [chunk-count] [extra analyzer args ...]"
  exit 1
fi

INPUT_FILE=$1
OUTPUT_PREFIX=$2
if [[ $# -ge 3 ]]; then
  CHUNK_COUNT=$3
  shift 3
else
  CHUNK_COUNT=16
  shift 2
fi
EXTRA_ARGS=("$@")

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_DIR=$(cd -- "${SCRIPT_DIR}/.." && pwd)
MAX_PARALLEL=${ALEPH_MAX_PARALLEL:-16}

if [[ ! -x "${REPO_DIR}/bin/aleph_charged_cumulants" ]]; then
  echo "Missing analyzer executable. Run: make"
  exit 1
fi

if [[ ! -f "$INPUT_FILE" ]]; then
  echo "Input file not found: $INPUT_FILE"
  exit 1
fi

mkdir -p "$(dirname "$OUTPUT_PREFIX")"
CHUNK_DIR="${OUTPUT_PREFIX}_chunks"
LOG_DIR="${OUTPUT_PREFIX}_logs"
rm -rf "$CHUNK_DIR" "$LOG_DIR"
mkdir -p "$CHUNK_DIR" "$LOG_DIR"

ENTRIES=$("${REPO_DIR}/bin/aleph_charged_cumulants" --Input "$INPUT_FILE" --PrintEntries 1 "${EXTRA_ARGS[@]}" | tail -n 1)
if [[ -z "$ENTRIES" || "$ENTRIES" -le 0 ]]; then
  echo "Failed to determine number of entries for $INPUT_FILE"
  exit 1
fi

CHUNK_SIZE=$(( (ENTRIES + CHUNK_COUNT - 1) / CHUNK_COUNT ))

for (( i=0; i<CHUNK_COUNT; ++i )); do
  START=$(( i * CHUNK_SIZE ))
  END=$(( START + CHUNK_SIZE ))
  if [[ "$START" -ge "$ENTRIES" ]]; then
    break
  fi
  if [[ "$END" -gt "$ENTRIES" ]]; then
    END=$ENTRIES
  fi

  OUT_FILE="${CHUNK_DIR}/chunk_$(printf '%03d' "$i").root"
  LOG_FILE="${LOG_DIR}/chunk_$(printf '%03d' "$i").log"

  echo "Launching chunk $i: [$START, $END)"
  "${REPO_DIR}/bin/aleph_charged_cumulants" \
    --Input "$INPUT_FILE" \
    --Output "$OUT_FILE" \
    --StartEntry "$START" \
    --EndEntry "$END" \
    "${EXTRA_ARGS[@]}" \
    >"$LOG_FILE" 2>&1 &

  while [[ $(jobs -pr | wc -l) -ge $MAX_PARALLEL ]]; do
    wait -n
  done
done

wait

MERGED_FILE="${OUTPUT_PREFIX}_merged.root"
SUMMARY_FILE="${OUTPUT_PREFIX}_summary.root"
rm -f "$MERGED_FILE" "$SUMMARY_FILE"
"${REPO_DIR}/bin/merge_correlation_chunks" "$MERGED_FILE" "${CHUNK_DIR}"/chunk_*.root >"${OUTPUT_PREFIX}_merge.log" 2>&1
"${REPO_DIR}/bin/aleph_cumulant_summary" --Input "$MERGED_FILE" --Output "$SUMMARY_FILE" >"${OUTPUT_PREFIX}_summary.log" 2>&1

echo "Merged output written to $MERGED_FILE"
echo "Summary output written to $SUMMARY_FILE"

