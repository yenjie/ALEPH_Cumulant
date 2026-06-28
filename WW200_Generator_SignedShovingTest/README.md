# WW200 Generator Signed Shoving Test

This directory contains a generator-level signed-response study for the already generated hadronic
`e+e- -> W+W- -> q qbar q qbar` Pythia samples at `sqrt(s)=200 GeV`.

No new events were generated for this task.

## Inputs

The three input trees are:

- `output/pythia_noshoving_ww200_5M.root`: Pythia WW, no shoving.
- `output/pythia_shoving_ww200_5M.root`: Pythia WW, nominal Gleipnir shoving.
- `output/pythia_shoving2x_ww200_5M.root`: Pythia WW, enhanced shoving.

Each tree is named `t` and contains 5,000,000 entries.  The branch inventory is saved in
`ww200_generator_sample_inventory.txt`.

The trees contain final visible particles (`px`, `py`, `pz`, `pid`, `pwflag`, `charge`) and final-particle
production vertices (`vx`, `vy`, `vz`).  They do not contain a full Pythia event record, W-daughter links,
mother/daughter indices, parton records, or string-piece records.  Because the available vertex branches
cannot be separated reliably into prompt hadronization vertices versus displaced decays, the direct
geometry-plane sign test is not claimed here.  The signed pair observables are therefore sign-preserving
two-particle response tests, not a direct spatial geometry-plane test.

## Baseline Reproduction

The existing generator-level WW baseline workflow was checked from the already available summary ROOT
files:

- inclusive charged particles with `pT>0.4 GeV`;
- multiplicity bins `[0,10), [10,15), [15,20), [20,25), [25,30), [30,35), [35,40), [40,inf)`;
- beam-axis and thrust-axis `v2{2,4,6,8}`;
- rapidity-gap `v2{2}` with `|Delta eta|>1.6`;
- two-subevent `v2{2}` with total gap 1.6;
- `1<pT<3 GeV` cross-check.

Representative reproduced plots are copied to `plots/baseline_reproduction/`.

## Signed c2 Outputs

`ww200_signed_c2_summary.csv` exports the signed underlying two-particle correlator:

`c2{2} = <cos 2(phi_i - phi_j)>`.

It covers:

- inclusive all-particle charged pairs;
- rapidity-gap pairs with `|Delta eta|>1.0, 1.6, 2.0, 2.5, 3.0`;
- two-subevent pairs with `eta<-b` and `eta>b`, for `b=0, 0.5, 0.8, 1.0`;
- beam-axis and thrust-axis coordinates;
- no shoving, nominal shoving, and enhanced shoving.

The uncertainties are the existing 16-block jackknife uncertainties from the cumulant summary files.
For sample differences, plots use `sqrt(sigma_sample^2 + sigma_no_shoving^2)`.

Key signed `c2` observations:

- Inclusive `c2` increases with shoving in both axes.  In the thrust axis the response is strong; for
  `[30,35)`, `Delta c2 = +0.00430` for nominal shoving and `+0.00862` for enhanced shoving.
- With `|Delta eta|>1.6`, the beam-axis response changes sign in several mid/high multiplicity bins.
  For `[30,35)`, enhanced shoving gives `Delta c2 = -9.18e-4`, about `-2.34 sigma`.
- With `|Delta eta|>1.6`, the thrust-axis response stays positive in the same bins.  For `[30,35)`,
  enhanced shoving gives `Delta c2 = +2.22e-3`, about `5.59 sigma`.
- Two-subevent results show axis and gap dependence.  With `b=0.8`, the thrust-axis response is positive
  in `[25,30)` and above, while the beam-axis enhanced response is negative in several bins.

The main signed-response plots are under `plots/signed_c2_*.pdf`.

## Signed VnDelta Outputs

`ww200_signed_VnDelta_summary.csv` exports long-range thrust-axis Fourier coefficients from direct
pair averages in `1.6<|Delta eta|<3.2`:

`VnDelta = <cos n(phi_i - phi_j)>`, for `n=1,2,3`.

The signed square-root proxy is

`v_n^sgn = sign(VnDelta) sqrt(|VnDelta|)`.

The VnDelta pass was run as 16 entry-range jobs per sample.  Raw block sums are kept in
`vndelta_raw/`, logs in `vndelta_logs/`, and the final uncertainties are delete-one-block jackknife
uncertainties computed from those raw block sums.

For the standard multiplicity bins, the thrust-axis `V2Delta` response is mostly positive:

| Bin | no shoving | nominal | enhanced | Delta nominal | Delta enhanced |
|---|---:|---:|---:|---:|---:|
| `[20,25)` | -0.002296 | -0.000903 | 0.002588 | +0.001393 | +0.004885 |
| `[25,30)` | 0.002295 | 0.004579 | 0.006091 | +0.002284 | +0.003796 |
| `[30,35)` | 0.009531 | 0.010777 | 0.012437 | +0.001246 | +0.002906 |
| `[35,40)` | 0.015545 | 0.016770 | 0.018789 | +0.001225 | +0.003244 |
| `[40,inf)` | 0.018769 | 0.019262 | 0.021885 | +0.000494 | +0.003116 |

The high-multiplicity split shows one non-monotonic nominal bin:

- `[40,45)`: nominal `Delta V2Delta = -3.13e-4`, enhanced `Delta V2Delta = +3.04e-3`.
- `[50,inf)`: both nominal and enhanced differences are negative but statistically weak in this sample.

The main VnDelta plots are:

- `plots/signed_V2Delta_thrust_standard.pdf`
- `plots/signed_V2Delta_thrust_high_multiplicity.pdf`

## Commands

Signed c2 and plot regeneration:

```bash
python3 WW200_Generator_SignedShovingTest/make_signed_outputs.py
```

The VnDelta analyzer was compiled directly because it is a task-local executable:

```bash
g++ -O3 -std=c++17 -Wall -Wextra -Iinclude $(root-config --cflags) \
  src/WW200SignedVnDelta.cpp -o bin/ww200_signed_vndelta $(root-config --glibs)
```

Each sample was processed in 16 entry ranges with `--RawOutput 1`; the raw block CSVs are the inputs
to `make_signed_outputs.py`.

## Interpretation

The existing `v2{2}` magnitude plots are not sign tests because `v2{2}=sqrt(c2{2})` is positive whenever
`c2{2}>0`.  The signed tests here show the direction of the two-particle harmonic response.

The already-generated WW samples show a statistically significant signed shoving response in several
`Delta c2` and `Delta V2Delta` bins.  The sign depends on the axis and nonflow handle.  In particular,
the thrust-axis long-range `V2Delta` response is mostly positive and enhanced shoving usually produces
a larger response than nominal shoving.  Beam-axis gap and two-subevent `c2` can be negative in some
mid/high multiplicity bins.

This is generator-level only.  It should not be interpreted as a statement about WW data.
