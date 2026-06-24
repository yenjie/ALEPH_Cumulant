# Pythia Shoving Z-Pole Sample

This note records the local production of a 1M event generator-level Pythia8/Gleipnir shoving sample for e+e- hadronic Z-pole studies.

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
make pythia   PYTHIA8_CONFIG=/mnt/data2/data2/yjlee/pythia/pythia8/develop/pythia-shoving-branch/bin/pythia8-config
```

The target compiles `src/GeneratePythiaZPoleRoot.cpp` into `bin/generate_pythia_zpole_root`. The old Pythia branch was built with the old GCC string ABI, so this target explicitly uses `-D_GLIBCXX_USE_CXX11_ABI=0` and avoids ROOT `std::string` branches.

Run the default 1M production with:

```bash
scripts/run_pythia_shoving_zpole_1M.sh
```

Useful overrides:

```bash
EVENTS=10000 SEED=12345 OUTPUT=output/test.root LOG=output/test.log   scripts/run_pythia_shoving_zpole_1M.sh
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

## Produced Sample

The 2026-06-24 production wrote:

```text
output/pythia_shoving_zpole_1M.root
output/pythia_shoving_zpole_1M.log
```

Summary from the log:

```text
Events written: 1,000,000
Visible final particles: 42,855,080
StudyMult-selected charged particles: 20,355,514
Pythia selected/accepted events: 1,000,000 / 1,000,000
Estimated cross section: 4.146e-05 mb
Gleipnir:nEvents: 1,000,000
Gleipnir:overlaps: 2.609
Gleipnir:pushAccept: 6.090
Gleipnir:pushSuccRate: 6.261e-01
```

Validation readback:

```bash
bin/aleph_charged_cumulants   --Input output/pythia_shoving_zpole_1M.root   --Output output/pythia_shoving_zpole_1M_readcheck_1000.root   --Tree t   --InputFormat vector   --StartEntry 0   --EndEntry 1000   --LabPtMin 0.4   --ThrustPtMin 0.4   --MultiplicityBins 0,10,15,20,25,30,35,40,999
```

The readback completed successfully.

## No-Shoving Control and Shoving Comparison

A matched no-shoving control sample was produced with the same Z-pole beam/process settings, same seed, and `--EnableShoving 0`:

```bash
scripts/run_pythia_noshoving_zpole_1M.sh
```

The 2026-06-24 no-shoving production wrote:

```text
output/pythia_noshoving_zpole_1M.root
output/pythia_noshoving_zpole_1M.log
```

No-shoving summary from the log:

```text
Events written: 1,000,000
Visible final particles: 42,851,393
StudyMult-selected charged particles: 20,346,862
Pythia selected/accepted events: 1,000,000 / 1,000,000
Estimated cross section: 4.146e-05 mb
```

The shoving and no-shoving samples were processed with the same 16-chunk cumulant workflow:

```bash
ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  output/pythia_noshoving_zpole_1M.root \
  output/pythia_noshoving_zpole_1M_pt04 \
  16 \
  --Tree t --InputFormat vector \
  --LabPtMin 0.4 --ThrustPtMin 0.4 \
  --MultiplicityBins 0,10,15,20,25,30,35,40,999

ALEPH_MAX_PARALLEL=8 scripts/run_chunks.sh \
  output/pythia_shoving_zpole_1M.root \
  output/pythia_shoving_zpole_1M_pt04 \
  16 \
  --Tree t --InputFormat vector \
  --LabPtMin 0.4 --ThrustPtMin 0.4 \
  --MultiplicityBins 0,10,15,20,25,30,35,40,999
```

The direct comparison was produced with:

```bash
bin/compare_v2_multiplicity \
  --DataSummary output/pythia_noshoving_zpole_1M_pt04_summary.root \
  --MCSummary output/pythia_shoving_zpole_1M_pt04_summary.root \
  --OutputPrefix output/pythia_zpole_1M_noshoving_vs_shoving_pt04_compare \
  --DataLabel 'Pythia no shoving' \
  --MCLabel 'Pythia shoving'
```

Tracked comparison outputs:

```text
docs/figures/pythia_zpole_1M_noshoving_vs_shoving_pt04_compare_beam.pdf
docs/figures/pythia_zpole_1M_noshoving_vs_shoving_pt04_compare_beam.csv
docs/figures/pythia_zpole_1M_noshoving_vs_shoving_pt04_compare_thrust.pdf
docs/figures/pythia_zpole_1M_noshoving_vs_shoving_pt04_compare_thrust.csv
```

Largest observed shoving-minus-no-shoving differences in the current comparison:

```text
Beam axis:
  v2{2}: max |delta/sigma| = 4.85 in Ntrk 10-15, shoving/no-shoving = 1.003
  v2{4}: max |delta/sigma| = 4.29 in Ntrk 10-15, shoving/no-shoving = 1.003
  v2{6}: max |delta/sigma| = 4.34 in Ntrk 10-15, shoving/no-shoving = 1.004
  v2{8}: max |delta/sigma| = 4.35 in Ntrk 10-15, shoving/no-shoving = 1.004

Thrust axis:
  v2{2}: max |delta/sigma| = 9.71 in Ntrk 20-25, shoving/no-shoving = 1.020
  v2{4}: max |delta/sigma| = 2.75 in Ntrk 25-30, shoving/no-shoving = 1.028
  v2{6}: max |delta/sigma| = 2.81 in Ntrk 25-30, shoving/no-shoving = 1.053
  v2{8}: max |delta/sigma| = 2.50 in Ntrk 25-30, shoving/no-shoving = 1.109
```

The same seed is used for the two productions, but the samples should be treated as statistically matched generator configurations rather than event-by-event paired samples because enabling shoving changes the random-number consumption during hadronization.

