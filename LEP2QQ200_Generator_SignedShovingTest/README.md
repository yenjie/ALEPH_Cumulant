# Inclusive LEP2 200 GeV signed shoving study

This directory contains the generator-level signed two-particle study for inclusive hadronic
`e+e- -> gamma*/Z -> q qbar` events at `sqrt(s)=200 GeV`. It is intentionally separate
from the WW-specific study in `WW200_Generator_SignedShovingTest`.

The production uses charged final-state particles with `pT > 0.4 GeV` and the standard
offline multiplicity bins `[0,10), [10,15), [15,20), [20,25), [25,30), [30,35), [35,40),
[40,inf)`. The three generator settings are no shoving, nominal Gleipnir shoving with
`Gleipnir:repulsionFactor = 0.25`, and enhanced shoving with `Gleipnir:repulsionFactor = 0.50`.

## Run commands

```bash
chmod +x scripts/run_pythia_qq200_sample.sh \
  scripts/build_pythia_qq200_5M_samples.sh \
  scripts/run_pythia_qq200_signed_fast.sh

GENERATION_MAX_PARALLEL=3 scripts/build_pythia_qq200_5M_samples.sh
ALEPH_MAX_PARALLEL=16 CHUNKS=16 scripts/run_pythia_qq200_signed_fast.sh
LEP2QQ200_Generator_SignedShovingTest/make_signed_outputs.py
```

## Completed 5M result summary

The completed run used three merged 5M accepted-event samples:

- `output/pythia_noshoving_qq200_5M.root`
- `output/pythia_shoving_qq200_5M.root`
- `output/pythia_shoving2x_qq200_5M.root`

The signed thrust-axis response is positive in the representative inclusive, rapidity-gap,
two-subevent, and long-range `V_{2Delta}` observables. For example, in the
`N_{trk}^{offline}=[30,35)` bin:

- inclusive thrust `Delta c2`: nominal `+0.00701`, enhanced `+0.01290`;
- thrust `|Delta eta|>1.6` `Delta c2`: nominal `+0.00266`, enhanced `+0.00476`;
- thrust two-subevent `eta<-0.8` versus `eta>0.8` `Delta c2`: nominal `+0.00166`, enhanced `+0.00255`;
- long-range thrust `Delta V_{2Delta}`: nominal `+0.00382`, enhanced `+0.00698`.

The enhanced-shoving response is larger than the nominal response in these representative
thrust-axis bins. This is a signed two-particle response test; it does not by itself define
a spatial geometry-plane sign.

## Outputs

- `qq200_generator_sample_inventory.txt`: input ROOT file inventory and stored truth content.
- `qq200_signed_c2_summary.csv`: signed `c2{2}=<cos 2(phi_i-phi_j)>` for inclusive, gap,
  two-subevent, and `1 < pT < 3 GeV` selections.
- `qq200_signed_VnDelta_summary.csv`: long-range thrust-axis `V_{nDelta}` from
  `1.6 < |Delta eta| < 3.2`.
- `plots/`: signed `c2`, `Delta c2`, response-ratio, `V_{2Delta}`, and signed-proxy plots.

The generated ROOT trees save final-state particles and production vertices for the saved
particles, but not the full event record, parton dipoles, string pieces, or mother/daughter
indices. A controlled geometry-plane sign test is therefore not attempted for this sample.
