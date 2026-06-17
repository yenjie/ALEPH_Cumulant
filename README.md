# ALEPH Charged-Particle Cumulants

Standalone ROOT/C++ analysis repository for charged-particle multi-particle cumulants in ALEPH LEP1 e+e- data. The implementation is based on the local StudyMult branch conventions and the nearby ALEPH cumulant prototypes, with the physics target aligned with the CMS multi-particle-correlation strategy in arXiv:1606.06198.

## What It Does

- Selects charged particles with StudyMult `pwflag` values `0`, `1`, or `2`.
- Supports both common ALEPH ROOT layouts:
  - vector branches: `px`, `py`, `pz`, `pwflag`, optional `passEventSelection`
  - StudyMult fixed arrays: `nParticle`, `px[nParticle]`, `py[nParticle]`, `pz[nParticle]`, `pwflag[nParticle]`
    - accepts both newer `Short_t` and older `Float_t` `pwflag` leaves
- Computes integrated charged-particle Q-cumulant sums for harmonic `n=2` by default:
  - `<2>`, `<4>`, `<6>`, `<8>`
  - derived `c2{2}`, `c2{4}`, `c2{6}`, `c2{8}`
  - derived `v2{2}`, `v2{4}`, `v2{6}`, `v2{8}`
- Computes full-event and three-subevent `v224 = <exp(i(2 phi1 + 2 phi2 - 4 phi3))>`.
- Computes eta-gap two-particle observables, including `v2{2, |Delta eta| > 2}` and `v2{2, |Delta eta| > 1.6}`, for comparison with inclusive `v2{2}`.
- Runs both beam-axis azimuth and thrust-axis azimuth in one pass.
- Writes mergeable numerator and denominator histograms for chunked processing.
- Writes jackknife statistical uncertainties on summary histograms when run from chunk files.

## Detailed Run Instructions

For a complete GitHub-checkout workflow, including clone/build, smoke test, full data/MC production, eta-gap and two-subevent jobs, plot generation, and analysis-note rebuild, see [`docs/run_instructions.md`](docs/run_instructions.md).

GitHub and Overleaf project links are recorded in [`docs/github_overleaf_info.md`](docs/github_overleaf_info.md).

Sample provenance and checked event counts are recorded in [`docs/sample_audit_20260617.md`](docs/sample_audit_20260617.md). The current figures use the merged LEP1 1992--1995 data sample `LEP1Merged_20200611.root` with 3,050,610 entries, compared to the merged 1994 MC baseline.

## Build

```bash
make
```

Requires ROOT with `root-config` in `PATH`.

Run the internal formula check:

```bash
make check
```

This compares the production ordered-correlator routine against brute-force distinct-particle sums for `<2>`, `<4>`, `<6>`, `<8>`, and the full-event `v224` numerator.

## Quick LEP1 1994 Run

```bash
scripts/run_lep1_1994_example.sh
```

This uses:

```text
/raid5/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp/LEP1Data1994_recons_aftercut-MERGED_thrust_pt04_t.root
```

Outputs:

- `output/lep1_1994_charged_pt04_merged.root`
- `output/lep1_1994_charged_pt04_summary.root`

The summary output contains central values and, for chunked runs, delete-one-chunk jackknife bin errors.
For real-valued `v2{m}` outputs, bins with sign-invalid leave-one jackknife samples are suppressed instead of assigning conditional errors from only the valid samples.

## Manual Run

Single process:

```bash
bin/aleph_charged_cumulants \
  --Input /path/to/input.root \
  --Output output/charged_cumulants.root \
  --Tree t \
  --LabPtMin 0.4 \
  --ThrustPtMin 0.4 \
  --MultiplicityBins 0,10,15,20,25,30,35,40,999

bin/aleph_cumulant_summary \
  --Input output/charged_cumulants.root \
  --Output output/charged_cumulants_summary.root
```

For statistical errors, provide independent chunk files instead of a single merged file:

```bash
bin/aleph_cumulant_summary \
  --Inputs output/charged_pt04_chunks/chunk_000.root,output/charged_pt04_chunks/chunk_001.root \
  --Output output/charged_cumulants_summary.root
```

Chunked:

```bash
scripts/run_chunks.sh /path/to/input.root output/charged_pt04 16 --Tree t --InputFormat auto
```

## Important Options

- `--InputFormat auto|vector|array`: auto-detects by default.
- `--UsePassEventSelection 1`: use `passEventSelection` when the branch exists.
- `--RequireHighPurity 1`: require `highPurity` when available; fails if requested and missing.
- `--ThrustChargedOnly 1`: compute the thrust axis from charged particles only. Default uses all particles for the thrust axis and charged particles for cumulants, matching the local prototypes.
- `--Harmonic 2`: harmonic for `<2k>` cumulant sums.
- `--SubeventEtaBoundary 0.5`: three-subevent split in the selected coordinate system.
- `--EtaGapMin 2.0`: minimum `|Delta eta|` for the eta-gap two-particle comparison. The default corresponds to `|Delta eta| > 2`.
- Multiplicity binning is by lab-frame selected charged multiplicity for both beam-axis and thrust-axis cumulants; the thrust selected multiplicity is also stored as a diagnostic histogram.

## Analysis Note

The repository includes an Electron-Positron Alliance-style analysis note in `AnalysisNote/`. Rebuild it with:

```bash
make note
```

The compiled note is `AnalysisNote/main.pdf`; the included v2-vs-multiplicity figures and CSV tables are copied into `AnalysisNote/figures/` for stable note provenance.

The current note figures were produced from the merged LEP1 data and merged 1994 MC samples:

```text
Data: /mnt/data3/data3/yjlee/StudyMultSamples/ALEPH/LEP1/LEP1Merged_20200611.root
MC:   /mnt/data3/data3/yjlee/StudyMultSamples/LEP1MCMerged.root
```

Both samples are read as StudyMult array trees with `--InputFormat array --Tree t`. The data sample contains 3,050,610 entries from 1992--1995; no converted 1991 ROOT file was found in the checked StudyMult sample directories. The MC sample contains 771,597 entries and is used as the 1994 baseline comparison.

The full reproducible workflow is in [`docs/run_instructions.md`](docs/run_instructions.md). It covers the inclusive cumulants, MC-only plots, data/MC comparisons, `|Delta eta| > 2.0`, the added `|Delta eta| > 1.6` rapidity-gap comparison, and the two-subevent production.

## Local Provenance

See `docs/local_sources.md` for the StudyMult and prototype files used to build this repository.

