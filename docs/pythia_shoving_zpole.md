# Pythia Shoving Z-Pole Sample

This note records the local production of 3M-event generator-level Pythia8/Gleipnir shoving and no-shoving samples for e+e- hadronic Z-pole studies.

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

Run the current 3M productions with:

```bash
scripts/run_pythia_noshoving_zpole_3M.sh
scripts/run_pythia_shoving_zpole_3M.sh
```

Useful overrides:

```bash
EVENTS=10000 SEED=12345 OUTPUT=output/test.root LOG=output/test.log scripts/run_pythia_shoving_zpole_3M.sh
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

The output tree is named `t` and uses vector branches compatible with `bin/aleph_charged_cumulants`: `px`, `py`, `pz`, `pwflag`, and optional `charge`. Neutrinos are dropped by default. The `pwflag` convention follows the local StudyMult-style shoving examples: charged hadrons `0`, electrons `1`, muons `2`, photons `4`, other neutral visible particles `5`.

## Produced Samples

The 2026-06-24 3M production wrote:

```text
output/pythia_noshoving_zpole_3M.root
output/pythia_noshoving_zpole_3M.log
output/pythia_shoving_zpole_3M.root
output/pythia_shoving_zpole_3M.log
```

No-shoving summary from the log:

```text
Events written: 3,000,000
Visible final particles: 128,547,297
StudyMult-selected charged particles: 61,044,510
Pythia selected/accepted events: 3,000,000 / 3,000,000
Estimated cross section: 4.146e-05 mb
```

Shoving summary from the log:

```text
Events written: 3,000,000
Visible final particles: 128,542,960
StudyMult-selected charged particles: 61,053,512
Pythia selected/accepted events: 3,000,000 / 3,000,000
Estimated cross section: 4.146e-05 mb
Gleipnir:nEvents: 3,000,000
Gleipnir:overlaps: 2.605
Gleipnir:pushAccept: 6.088
Gleipnir:pushSuccRate: 6.259e-01
```

Readback validation with `bin/aleph_charged_cumulants --PrintEntries 1` reports 3,000,000 entries for both ROOT trees.

Checksums:

```text
97339286e28f6029f53a2c5b52604c879dc5e575499caa467bba32e245dff375  output/pythia_noshoving_zpole_3M.root
86b7c17b9f64966a72d5ce8fc9d8c018035c52603f96521b3b5a42a4faab311d  output/pythia_shoving_zpole_3M.root
548301e1bef4d2530f2c4eba6bd8f3bdea45e7425b16fff7f2aa8d04c4afbd6a  output/pythia_noshoving_zpole_3M_pt04_summary.root
67b0a94e2e2de5f2c87fc1a842df9cc9269011420ebbb00c0346e37f4083fb97  output/pythia_shoving_zpole_3M_pt04_summary.root
```

## No-Shoving Control and Shoving Comparison

The shoving and no-shoving samples were processed with the same 16-chunk cumulant workflow:

```bash
ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  output/pythia_noshoving_zpole_3M.root \
  output/pythia_noshoving_zpole_3M_pt04 \
  16 \
  --Tree t --InputFormat vector \
  --LabPtMin 0.4 --ThrustPtMin 0.4 \
  --MultiplicityBins 0,10,15,20,25,30,35,40,999

ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  output/pythia_shoving_zpole_3M.root \
  output/pythia_shoving_zpole_3M_pt04 \
  16 \
  --Tree t --InputFormat vector \
  --LabPtMin 0.4 --ThrustPtMin 0.4 \
  --MultiplicityBins 0,10,15,20,25,30,35,40,999
```

Both summary logs report `Set jackknife errors from 16 input blocks`.

The direct comparison was produced with:

```bash
bin/compare_v2_multiplicity \
  --DataSummary output/pythia_noshoving_zpole_3M_pt04_summary.root \
  --MCSummary output/pythia_shoving_zpole_3M_pt04_summary.root \
  --OutputPrefix output/pythia_zpole_3M_noshoving_vs_shoving_pt04_compare \
  --DataLabel 'Pythia no shoving' \
  --MCLabel 'Pythia shoving'
```

Tracked comparison outputs:

```text
docs/figures/pythia_zpole_3M_noshoving_vs_shoving_pt04_compare_beam.pdf
docs/figures/pythia_zpole_3M_noshoving_vs_shoving_pt04_compare_beam.csv
docs/figures/pythia_zpole_3M_noshoving_vs_shoving_pt04_compare_thrust.pdf
docs/figures/pythia_zpole_3M_noshoving_vs_shoving_pt04_compare_thrust.csv
```

Largest observed shoving-minus-no-shoving differences in the current 3M comparison:

```text
Beam axis:
  v2{2}: max |delta/sigma| = 8.88 in Ntrk 15-20, shoving/no-shoving = 1.0031
  v2{4}: max |delta/sigma| = 7.55 in Ntrk 15-20, shoving/no-shoving = 1.0034
  v2{6}: max |delta/sigma| = 7.38 in Ntrk 15-20, shoving/no-shoving = 1.0035
  v2{8}: max |delta/sigma| = 7.35 in Ntrk 15-20, shoving/no-shoving = 1.0035

Thrust axis:
  v2{2}: max |delta/sigma| = 15.61 in Ntrk 20-25, shoving/no-shoving = 1.020
  v2{4}: max |delta/sigma| = 3.63 in Ntrk 25-30, shoving/no-shoving = 1.029
  v2{6}: max |delta/sigma| = 3.69 in Ntrk 25-30, shoving/no-shoving = 1.053
  v2{8}: max |delta/sigma| = 3.16 in Ntrk 25-30, shoving/no-shoving = 1.086
```

The same seed is used for the two productions, but the samples should be treated as statistically matched generator configurations rather than event-by-event paired samples because enabling shoving changes the random-number consumption during hadronization.
