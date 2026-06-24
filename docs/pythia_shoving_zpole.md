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
