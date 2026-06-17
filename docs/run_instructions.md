# Run Instructions

This page is the end-to-end recipe for reproducing the charged-particle cumulant outputs in this repository. The commands assume an analysis machine that can read the ALEPH input ROOT files under `/mnt/data3/data3/yjlee`, `/data/yjlee`, or `/raid5/data/yjlee`. If you run elsewhere, replace the input paths with local copies of the same ROOT files.

## 1. Clone and Build

```bash
git clone git@github.com:yenjie/ALEPH_Cumulant.git
cd ALEPH_Cumulant
```

If SSH access is not configured, use HTTPS instead:

```bash
git clone https://github.com/yenjie/ALEPH_Cumulant.git
cd ALEPH_Cumulant
```

Load a ROOT environment with `root-config` in `PATH`, then build:

```bash
root-config --version
make
make check
```

`make check` runs the internal correlator self-test. A successful check prints:

```text
Correlator self-test passed
```

## 2. Input Samples

The current note figures use the merged LEP1 1992-1995 reconstructed data sample and the available 1994 reconstructed-MC baseline:

```bash
DATA=/mnt/data3/data3/yjlee/StudyMultSamples/ALEPH/LEP1/LEP1Merged_20200611.root
MC=/mnt/data3/data3/yjlee/StudyMultSamples/LEP1MCMerged.root
BINS=0,10,15,20,25,30,35,40,999
DATA_ARGS="--InputFormat array --Tree t --LabPtMin 0.4 --ThrustPtMin 0.4 --MultiplicityBins ${BINS}"
MC_ARGS="--InputFormat array --Tree t --LabPtMin 0.4 --ThrustPtMin 0.4 --MultiplicityBins ${BINS}"
```

Checked entry counts:

- Data: 3,050,610 entries, with year counts 1992: 551,474; 1993: 538,601; 1994: 1,365,440; 1995: 595,095.
- MC: 771,597 reconstructed entries, all from 1994.

A converted 1991 ROOT sample was not found in the available merged samples. The 1991 ASCII event-record lists exist under `/data/yjlee/StudyMult/DataProcessing/paths`, but they require StudyMult `scan.cc` preprocessing before they can be used by this analyzer.

Charged particles are selected with StudyMult `pwflag` values `0`, `1`, or `2`. Both beam-axis and thrust-axis cumulants are binned versus laboratory selected charged-particle multiplicity. The thrust-selected multiplicity is written only as a diagnostic histogram in the current production.

## 3. Quick Smoke Test

Run over a small entry range before launching full production:

```bash
mkdir -p output
bin/aleph_charged_cumulants \
  --Input "$DATA" \
  --Output output/smoke_data_1000.root \
  --StartEntry 0 \
  --EndEntry 1000 \
  $DATA_ARGS

bin/aleph_cumulant_summary \
  --Input output/smoke_data_1000.root \
  --Output output/smoke_data_1000_summary.root
```

This single-file summary does not have jackknife statistical errors. Use the chunked production below for quoted uncertainties.

## 4. Inclusive Full-Event Production

Run data and MC with 16 chunks. `ALEPH_MAX_PARALLEL` controls how many chunk jobs run at the same time.

```bash
ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  "$DATA" \
  output/lep1_1992_1995_data_charged_pt04 \
  16 \
  $DATA_ARGS

ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  "$MC" \
  output/lep1_1994_mc_merged_charged_pt04 \
  16 \
  $MC_ARGS
```

Each command writes `*_chunks/chunk_*.root`, `*_merged.root`, `*_summary.root`, `*_logs/chunk_*.log`, `*_merge.log`, and `*_summary.log`.

## 5. Eta-Gap v2{2} Production

The eta-gap comparison uses the same event and track selection, plus an explicit pair cut in the selected axis frame. The note includes both `|Delta eta| > 2.0` and `|Delta eta| > 1.6`.

```bash
ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  "$DATA" \
  output/lep1_1992_1995_data_charged_pt04_etagap \
  16 \
  $DATA_ARGS \
  --EtaGapMin 2.0

ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  "$MC" \
  output/lep1_1994_mc_merged_charged_pt04_etagap \
  16 \
  $MC_ARGS \
  --EtaGapMin 2.0

ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  "$DATA" \
  output/lep1_1992_1995_data_charged_pt04_etagap1p6 \
  16 \
  $DATA_ARGS \
  --EtaGapMin 1.6

ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  "$MC" \
  output/lep1_1994_mc_merged_charged_pt04_etagap1p6 \
  16 \
  $MC_ARGS \
  --EtaGapMin 1.6
```

The summary writes `hV2_2EtaGapOverInclusive_{beam,thrust}`. This ratio is formed in each leave-one-chunk sample and then jackknifed, so the gap/inclusive uncertainty preserves the same-sample covariance.

## 6. Two-Subevent Production

The two-subevent cumulant samples `k` particles from subevent A and `k` particles from subevent B for `v2{2k}`. With `--TwoSubeventEtaBoundary 0.0`, the split is `eta < 0` and `eta > 0`, i.e. a two-hemisphere split, not a finite eta-gap suppression.

```bash
ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  "$DATA" \
  output/lep1_1992_1995_data_charged_pt04_twosub \
  16 \
  $DATA_ARGS \
  --TwoSubeventEtaBoundary 0.0

ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  "$MC" \
  output/lep1_1994_mc_merged_charged_pt04_twosub \
  16 \
  $MC_ARGS \
  --TwoSubeventEtaBoundary 0.0
```

## 7. Make Plots and CSV Tables

Standalone data and MC plots:

```bash
bin/plot_v2_multiplicity \
  --Input output/lep1_1992_1995_data_charged_pt04_summary.root \
  --OutputPrefix output/lep1_1992_1995_data_charged_pt04_v2

bin/plot_v2_multiplicity \
  --Input output/lep1_1994_mc_merged_charged_pt04_summary.root \
  --OutputPrefix output/lep1_1994_mc_merged_charged_pt04_v2
```

Data/MC comparison:

```bash
bin/compare_v2_multiplicity \
  --DataSummary output/lep1_1992_1995_data_charged_pt04_summary.root \
  --MCSummary output/lep1_1994_mc_merged_charged_pt04_summary.root \
  --OutputPrefix output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_compare \
  --DataLabel 'ALEPH 1992-1995 data' \
  --MCLabel 'ALEPH 1994 MC'
```

Eta-gap comparisons:

```bash
bin/compare_v22_eta_gap \
  --DataSummary output/lep1_1992_1995_data_charged_pt04_etagap_summary.root \
  --MCSummary output/lep1_1994_mc_merged_charged_pt04_etagap_summary.root \
  --OutputPrefix output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap_compare \
  --DataLabel 'ALEPH 1992-1995 data' \
  --MCLabel 'ALEPH 1994 MC' \
  --GapLabel '|#Delta#eta|>2.0'

bin/compare_v22_eta_gap \
  --DataSummary output/lep1_1992_1995_data_charged_pt04_etagap1p6_summary.root \
  --MCSummary output/lep1_1994_mc_merged_charged_pt04_etagap1p6_summary.root \
  --OutputPrefix output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap1p6_compare \
  --DataLabel 'ALEPH 1992-1995 data' \
  --MCLabel 'ALEPH 1994 MC' \
  --GapLabel '|#Delta#eta|>1.6'
```

Two-subevent comparison:

```bash
bin/compare_two_subevent_v2_multiplicity \
  --DataSummary output/lep1_1992_1995_data_charged_pt04_twosub_summary.root \
  --MCSummary output/lep1_1994_mc_merged_charged_pt04_twosub_summary.root \
  --OutputPrefix output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_twosub_compare \
  --DataLabel 'ALEPH 1992-1995 data' \
  --MCLabel 'ALEPH 1994 MC'
```

The plotting programs write both `.pdf` and `.csv` files under `output/`.

## 8. Update and Build the Analysis Note

The note uses stable copies of the plot PDFs and CSVs under `AnalysisNote/figures/`. After regenerating plots, copy the outputs used by the note:

```bash
cp output/lep1_1992_1995_data_charged_pt04_v2_beam.csv AnalysisNote/figures/v2_multiplicity_beam.csv
cp output/lep1_1992_1995_data_charged_pt04_v2_beam.pdf AnalysisNote/figures/v2_multiplicity_beam.pdf
cp output/lep1_1992_1995_data_charged_pt04_v2_thrust.csv AnalysisNote/figures/v2_multiplicity_thrust.csv
cp output/lep1_1992_1995_data_charged_pt04_v2_thrust.pdf AnalysisNote/figures/v2_multiplicity_thrust.pdf
cp output/lep1_1994_mc_merged_charged_pt04_v2_beam.csv AnalysisNote/figures/v2_multiplicity_mc_beam.csv
cp output/lep1_1994_mc_merged_charged_pt04_v2_beam.pdf AnalysisNote/figures/v2_multiplicity_mc_beam.pdf
cp output/lep1_1994_mc_merged_charged_pt04_v2_thrust.csv AnalysisNote/figures/v2_multiplicity_mc_thrust.csv
cp output/lep1_1994_mc_merged_charged_pt04_v2_thrust.pdf AnalysisNote/figures/v2_multiplicity_mc_thrust.pdf
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_compare_beam.csv AnalysisNote/figures/v2_data_mc_compare_beam.csv
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_compare_beam.pdf AnalysisNote/figures/v2_data_mc_compare_beam.pdf
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_compare_thrust.csv AnalysisNote/figures/v2_data_mc_compare_thrust.csv
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_compare_thrust.pdf AnalysisNote/figures/v2_data_mc_compare_thrust.pdf
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap_compare_beam.csv AnalysisNote/figures/v2_eta_gap_data_mc_compare_beam.csv
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap_compare_beam.pdf AnalysisNote/figures/v2_eta_gap_data_mc_compare_beam.pdf
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap_compare_thrust.csv AnalysisNote/figures/v2_eta_gap_data_mc_compare_thrust.csv
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap_compare_thrust.pdf AnalysisNote/figures/v2_eta_gap_data_mc_compare_thrust.pdf
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap1p6_compare_beam.csv AnalysisNote/figures/v2_eta_gap_1p6_data_mc_compare_beam.csv
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap1p6_compare_beam.pdf AnalysisNote/figures/v2_eta_gap_1p6_data_mc_compare_beam.pdf
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap1p6_compare_thrust.csv AnalysisNote/figures/v2_eta_gap_1p6_data_mc_compare_thrust.csv
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_etagap1p6_compare_thrust.pdf AnalysisNote/figures/v2_eta_gap_1p6_data_mc_compare_thrust.pdf
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_twosub_compare_beam.csv AnalysisNote/figures/v2_two_sub_data_mc_compare_beam.csv
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_twosub_compare_beam.pdf AnalysisNote/figures/v2_two_sub_data_mc_compare_beam.pdf
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_twosub_compare_thrust.csv AnalysisNote/figures/v2_two_sub_data_mc_compare_thrust.csv
cp output/lep1_1992_1995_data_vs_1994_mc_merged_charged_pt04_twosub_compare_thrust.pdf AnalysisNote/figures/v2_two_sub_data_mc_compare_thrust.pdf
```

Then rebuild the PDF:

```bash
make note
```

The built note is `AnalysisNote/main.pdf`.

## 9. Important Analysis Conventions

- Statistical errors on summary histograms are delete-one-chunk jackknife errors when the summary is built from chunk files.
- For real-valued `v2{m}` histograms, if any leave-one chunk has the wrong cumulant sign, the plotted point is suppressed rather than assigned an error from only the sign-valid samples.
- Invalid or missing plotted points are represented in CSVs as zero content with zero error. Use the ROOT histograms and note text when interpreting sign changes.
- The eta-gap observable uses pseudorapidity relative to the selected axis. Beam-axis and thrust-axis eta gaps are separate observables.
- The current results are detector-level charged-particle angular-correlation cumulants. Physics interpretation as collective flow requires additional correction, closure, and systematic studies.

## 10. Clean Rebuild

To remove compiled binaries and rebuild:

```bash
make clean
make
make check
```

Generated ROOT files, logs, chunk directories, and binaries are ignored by git. The tracked reproducibility artifacts are the source code, scripts, README, detailed instructions, analysis note source, and selected note figures/CSVs.
