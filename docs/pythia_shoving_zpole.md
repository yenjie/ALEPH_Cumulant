# Pythia Shoving Z-Pole Sample

This note records the local 5M-event generator-level Pythia8/Gleipnir shoving and no-shoving Z-pole samples used for the cumulant analysis.

## Source Branch

Located tarballs:

- `/mnt/data3/data3/janicechen/pythia-shoving-branch.tgz`
- `/mnt/data2/data2/yjlee/pythia/pythia8/develop/pythia-shoving-branch.tgz.1`

The production used the already-built extracted branch:

```text
/mnt/data2/data2/yjlee/pythia/pythia8/develop/pythia-shoving-branch
```

The branch reports Pythia version 8.306 and contains `lib/libpythia8.so`, `bin/pythia8-config`, and `share/Pythia8/xmldoc`.

## Generator

Build the generator from this repository with:

```bash
make pythia PYTHIA8_CONFIG=/mnt/data2/data2/yjlee/pythia/pythia8/develop/pythia-shoving-branch/bin/pythia8-config
```

The target compiles `src/GeneratePythiaZPoleRoot.cpp` into `bin/generate_pythia_zpole_root`. The old Pythia branch was built with the old GCC string ABI, so this target explicitly uses `-D_GLIBCXX_USE_CXX11_ABI=0` and avoids ROOT `std::string` branches.

The current 5M samples are produced and validated with:

```bash
scripts/build_pythia_zpole_5M_samples.sh
```

This script reuses the existing seed-12345 3M samples when present, generates independent seed-67890 2M extensions when needed, merges each pair with `hadd`, and requires exactly 5,000,000 entries in each merged ROOT tree.

For a direct standalone production rather than the merged 3M+2M extension workflow, the wrappers are:

```bash
scripts/run_pythia_noshoving_zpole_5M.sh
scripts/run_pythia_shoving_zpole_5M.sh
```

Useful override example:

```bash
EVENTS=10000 SEED=12345 OUTPUT=output/test.root LOG=output/test.log scripts/run_pythia_shoving_zpole_5M.sh
```

## Physics Settings

The default wrapper produces visible final particles for:

```text
e+ e- at sqrt(s) = 91.1876 GeV
WeakSingleBoson:ffbar2gmZ = on
23:onMode = off
23:onIfAny = 1 2 3 4 5
PDF:lepton = off
SpaceShower:QEDshowerByL = off
```

Gleipnir shoving is enabled with the settings adapted from the local `withrope.cc` example in the Pythia branch:

```text
StringInteractions:model = 3
Gleipnir:shoving = on
Gleipnir:StringR = 1.0
Gleipnir:tauString = 1.0
Gleipnir:tauHad = 2.0
Gleipnir:nPushMaxCalc = 0
Gleipnir:pushPT = 0.02
Gleipnir:extendGluonRegions = on
Gleipnir:repulsionFactor = 0.25
Fragmentation:setVertices = on
PartonVertex:setVertex = on
PartonVertex:modeRadiation = 2
```

The output tree is named `t` and uses vector branches compatible with `bin/aleph_charged_cumulants`: `px`, `py`, `pz`, `pwflag`, and optional `charge`. Neutrinos are dropped by default. The `pwflag` convention follows the local StudyMult-style shoving examples: charged hadrons `0`, electrons `1`, muons `2`, photons `4`, and other neutral visible particles `5`.

## Produced Samples

The current 5M samples were built on 2026-06-25 from:

```text
output/pythia_noshoving_zpole_3M.root
output/pythia_noshoving_zpole_extra2M_seed67890.root
output/pythia_shoving_zpole_3M.root
output/pythia_shoving_zpole_extra2M_seed67890.root
```

Merged outputs:

```text
output/pythia_noshoving_zpole_5M.root
output/pythia_shoving_zpole_5M.root
```

Final merged readback validation with `bin/aleph_charged_cumulants --PrintEntries 1` reports exactly 5,000,000 entries for both ROOT trees.

No-shoving composition:

```text
Seed 12345 base: 3,000,000 events, 128,547,297 visible particles, 61,044,510 StudyMult-selected charged particles
Seed 67890 extension: 2,000,000 events, 85,702,655 visible particles, 40,695,572 StudyMult-selected charged particles
Merged total: 5,000,000 events, 214,249,952 visible particles, 101,740,082 StudyMult-selected charged particles
```

Shoving composition:

```text
Seed 12345 base: 3,000,000 events, 128,542,960 visible particles, 61,053,512 StudyMult-selected charged particles
Seed 67890 extension: 2,000,000 events, 85,709,957 visible particles, 40,699,476 StudyMult-selected charged particles
Merged total: 5,000,000 events, 214,252,917 visible particles, 101,752,988 StudyMult-selected charged particles
```

The seed-67890 shoving extension log reports:

```text
Gleipnir:nEvents: 2,000,000
Gleipnir:overlaps: 2.604
Gleipnir:pushAccept: 6.083
Gleipnir:pushSuccRate: 6.258e-01
```

Checksums from the current files:

```text
28db7bbd1bbc26a84cc51cc4bfc75503e00b37e4082c68ac40dc20170527c353  output/pythia_noshoving_zpole_5M.root
d09b7a9318faf1b709f5369833e93e828326beb93ffbeb6014e26d1ff6cd2a18  output/pythia_shoving_zpole_5M.root
ca59cc6ac6c279953e39d7cb7dadfebe7eeedbabb7156cd46c980b2a19c09145  output/pythia_noshoving_zpole_extra2M_seed67890.root
0d208246b97edee4d80ab8047a41b05d485043fe3c59eef79ba0f57c077e61c3  output/pythia_shoving_zpole_extra2M_seed67890.root
```

## Nominal Cumulant Processing

The shoving and no-shoving samples were processed with the same 16-chunk cumulant workflow:

```bash
ALEPH_MAX_PARALLEL=16 CHUNKS=16 RUN_NONFLOW=0 scripts/run_pythia_shoving_5M_analysis.sh
```

Equivalent explicit commands:

```bash
ALEPH_MAX_PARALLEL=16 scripts/run_chunks.sh \
  output/pythia_noshoving_zpole_5M.root \
  output/pythia_noshoving_zpole_5M_pt04 \
  16 \
  --Tree t --InputFormat vector \
  --LabPtMin 0.4 --ThrustPtMin 0.4 \
  --MultiplicityBins 0,10,15,20,25,30,35,40,999

ALEPH_MAX_PARALLEL=16 scripts/run_chunks.sh \
  output/pythia_shoving_zpole_5M.root \
  output/pythia_shoving_zpole_5M_pt04 \
  16 \
  --Tree t --InputFormat vector \
  --LabPtMin 0.4 --ThrustPtMin 0.4 \
  --MultiplicityBins 0,10,15,20,25,30,35,40,999
```

Both summary logs report `Set jackknife errors from 16 input blocks`.

The nominal comparison was produced with:

```bash
bin/compare_v2_multiplicity \
  --DataSummary output/pythia_noshoving_zpole_5M_pt04_summary.root \
  --MCSummary output/pythia_shoving_zpole_5M_pt04_summary.root \
  --OutputPrefix output/pythia_zpole_5M_noshoving_vs_shoving_pt04_compare \
  --DataLabel 'Pythia no shoving' \
  --MCLabel 'Pythia shoving' \
  --RatioMCOverData 1 \
  --RatioLabel 'shoving / no shoving'
```

Tracked nominal comparison outputs. The PDF panels show the two samples in the upper part of each observable pad and the `shoving/no-shoving` ratio in the lower part; the CSV files include both `data_over_mc` and `mc_over_data` columns, where `mc_over_data` is the shoving/no-shoving ratio for this production.

```text
docs/figures/pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_beam.pdf
docs/figures/pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_beam.csv
docs/figures/pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_thrust.pdf
docs/figures/pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_thrust.csv
```

Largest observed nominal shoving-minus-no-shoving differences in the current 5M comparison:

```text
Beam axis:
  v2{2}: max |delta/sigma| = 11.55 in Ntrk 20-25, shoving/no-shoving = 1.0047
  v2{4}: max |delta/sigma| = 8.99 in Ntrk 10-15, shoving/no-shoving = 1.0030
  v2{6}: max |delta/sigma| = 8.96 in Ntrk 10-15, shoving/no-shoving = 1.0031
  v2{8}: max |delta/sigma| = 8.96 in Ntrk 10-15, shoving/no-shoving = 1.0031

Thrust axis:
  v2{2}: max |delta/sigma| = 21.40 in Ntrk 15-20, shoving/no-shoving = 1.0161
  v2{4}: max |delta/sigma| = 5.04 in Ntrk 25-30, shoving/no-shoving = 1.0315
  v2{6}: max |delta/sigma| = 4.24 in Ntrk 25-30, shoving/no-shoving = 1.0525
  v2{8}: max |delta/sigma| = 3.58 in Ntrk 25-30, shoving/no-shoving = 1.0852
```

## Nonflow-Suppression Matrix

The full 5M nonflow-suppression matrix is run with:

```bash
ALEPH_MAX_PARALLEL=16 CHUNKS=16 RUN_NONFLOW=1 scripts/run_pythia_shoving_5M_analysis.sh
scripts/plot_pythia_nonflow_suppression_5M.sh
scripts/summarize_pythia_shoving_nonflow.py
```

The matrix contains 14 cases per generator configuration:

- two-particle rapidity gaps: `|delta eta| > 1.0, 1.6, 2.0, 2.5, 3.0`;
- two-subevent cumulants: `eta<0` vs `eta>0`, plus excluded middle-region gaps of 1.0, 1.6, and 2.0;
- track selections: `0.4<pT<1`, `0.4<pT<2`, `1<pT<3`, positive charge only, and negative charge only;
- event-shape selections: `Thrust<0.90`, `Thrust<0.85`, `Sphericity>0.12`, and `Sphericity>0.22`.

For generated vector trees, `bin/aleph_charged_cumulants` computes thrust and sphericity from visible final-particle momenta if the stored event-shape branches are absent.
The plotting script passes `--RatioMCOverData 1 --RatioLabel 'shoving / no shoving'` to all three comparison plotters. For eta-gap cases, the comparison PDF contains the main inclusive/gap overlay, the existing gap/inclusive panel, and an additional shoving/no-shoving ratio panel.

Detailed results and the full metrics table are documented in:

```text
docs/pythia_shoving_nonflow_5M.md
docs/figures/pythia_shoving_nonflow_5M_metrics.csv
```

The same seed is used within each seed block for the two productions, but the samples should be treated as statistically matched generator configurations rather than event-by-event paired samples because enabling shoving changes the random-number consumption during hadronization. Ratio and significance values use the exported jackknife uncertainties and do not include covariance between the two generator configurations.
