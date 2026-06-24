# LEP1 MC Nonflow-Suppression Study Plan

Date: 2026-06-24

Goal: identify which literature-motivated nonflow-suppression handles drive the
LEP1 reconstructed MC cumulant results toward zero. Since `v2{2}=sqrt(c2{2})`
is positive by construction when `c2{2}>0`, closure should be judged primarily
with the signed cumulants/correlators and their jackknife uncertainties. The v2
plots remain the presentation observable.

## Literature handles

| Family | Literature motivation | ALEPH implementation status | Production in this study |
|---|---|---|---|
| Higher-order cumulants, `v2{4}`, `v2{6}`, `v2{8}` | Multi-particle cumulants suppress few-particle nonflow relative to `v2{2}`, but small systems can still be dominated by nonflow. See Bilandzic et al. exact cumulant framework and Jia et al. small-system caveat. | Already implemented for inclusive and two-subevent cumulants. | Use existing LEP1 MC nominal and two-subevent summaries. |
| Two-particle pseudorapidity gaps | Long-range two-particle analyses use large `|Delta eta|` gaps to reduce short-range jet, resonance, HBT/Coulomb correlations. | Already implemented as `hV2_2EtaGap`; gap value controlled by `--EtaGapMin`. | Existing 1.6 and 2.0; new scan at 1.0, 2.5, 3.0. |
| Two-subevent cumulants with an eta gap | Jia et al. and ATLAS show subevent cumulants suppress dijet nonflow; ATLAS finds the three-subevent four-particle method least sensitive to short-range jet correlations. | Two-subevent `k` vs `k` cumulants are implemented; positive `--TwoSubeventEtaBoundary=b` excludes `|eta|<b`, producing a `2b` gap. | Existing boundary 0.0; new boundaries 0.5, 0.8, 1.0. |
| Three-subevent cumulants | Three-subevent `c_n{4}` is the strongest published cumulant nonflow suppression for small systems. | Only the mixed-harmonic `v224` three-subevent observable is currently implemented, not `c2{4}`. | Marked for follow-up implementation if two-subevent closure fails; not included in the first production matrix. |
| Soft-track / high-pT suppression | Jet fragments dominate nonflow; restricting to softer particles is a common cross-check, while high pT usually enhances nonflow. | Existing pT min/max switches for beam and thrust axes. | Existing `1<pT<3`; new `0.4<pT<1` and `0.4<pT<2`. |
| Charge-sign selections | Unlike-sign resonance decays and local charge conservation can be isolated by comparing all-charge and same-sign/charge-selected correlations. | Added `--ChargeSelection all|positive|negative|nonzero`. This is a charge-selected cumulant sample, not a pair-level same-sign-only correlator. | New positive-only and negative-only MC runs. |
| Anti-jetty event-shape selections | In e+e-, high thrust tags two-jet topology. Cutting to lower thrust or higher sphericity tests whether the apparent v2 is jet-shape nonflow. | Added `--ThrustMin/Max` and `--SphericityMin/Max` using StudyMult event branches. | New `Thrust<0.90`, `Thrust<0.85`, `Sphericity>0.12`, `Sphericity>0.22`. |
| Low-multiplicity/template or peripheral subtraction | CMS/ATLAS small-system two-particle analyses subtract a low-activity reference or template to remove dijets; Lim et al. emphasize generator closure tests. | Current code stores cumulant moments, not full `Delta phi` templates. A harmonic-level diagnostic subtraction can be built, but it is not equivalent to the published template fit. | To be evaluated from summary histograms as a diagnostic, not a nominal correction. |
| `1/N` nonflow scaling subtraction | Recent work formulates residual nonflow subtraction for multi-particle observables using approximate `1/N^(m-1)` scaling, with caveats and multiplicity reweighting. | Can be tested as a post-processing diagnostic on signed cumulants; not a replacement for a full template analysis. | To be evaluated after the production matrix. |
| Delta-eta dependent/independent decomposition | STAR/Xu-Yi-Wang style methods decompose `Delta eta` dependent nonflow from `Delta eta` independent flow using eta-bin symmetry. | Current output does not store eta-bin correlation matrices, only integrated gap moments. | Not carried out in this pass; requires new eta-binned correlator histograms. |
| Event-plane/scalar-product/Lee-Yang-zero methods | Lee-Yang zeroes and event-plane variants suppress nonflow using global flow-vector information and/or separated event planes. | Not natural for the current e+e cumulant analysis and would require a separate event-plane/LYZ estimator. | Not carried out in this pass. |
| PID/resonance vetoes | Explicit resonance/HBT/Coulomb studies need PID and pair invariant-mass selections. | MC has `pid`, but the current charged-particle analysis and data interpretation are not PID-calibrated. | Not carried out as a nominal study. |

## Production matrix

The reproducible production script is:

```bash
scripts/run_nonflow_suppression_mc.sh
```

It runs the following new LEP1 MC output prefixes:

- `output/nonflow_mc_pt04_etagap1p0`
- `output/nonflow_mc_pt04_etagap2p5`
- `output/nonflow_mc_pt04_etagap3p0`
- `output/nonflow_mc_pt04_twosub_gap1p0`
- `output/nonflow_mc_pt04_twosub_gap1p6`
- `output/nonflow_mc_pt04_twosub_gap2p0`
- `output/nonflow_mc_pt04to1`
- `output/nonflow_mc_pt04to2`
- `output/nonflow_mc_positive_pt04`
- `output/nonflow_mc_negative_pt04`
- `output/nonflow_mc_thrustlt0p90_pt04`
- `output/nonflow_mc_thrustlt0p85_pt04`
- `output/nonflow_mc_sphericitygt0p12_pt04`
- `output/nonflow_mc_sphericitygt0p22_pt04`

Existing LEP1 MC outputs included in the comparison:

- `output/lep1_1994_mc_merged_charged_pt04_summary.root`
- `output/lep1_1994_mc_merged_charged_pt04_etagap1p6_summary.root`
- `output/lep1_1994_mc_merged_charged_pt04_etagap_summary.root`
- `output/lep1_1994_mc_merged_charged_pt04_twosub_summary.root`
- `output/lep1_1994_mc_merged_charged_pt1to3_summary.root`

## References

- CMS Collaboration, Evidence for collectivity in pp collisions at the LHC, https://arxiv.org/abs/1606.06198
- J. Jia, M. Zhou, A. Trzupek, Revealing long-range multi-particle collectivity in small collision systems via subevent cumulants, https://arxiv.org/abs/1701.03830
- ATLAS Collaboration, Measurement of multi-particle azimuthal correlations with the subevent cumulant method, https://arxiv.org/abs/1708.03559
- S. H. Lim et al., Examination of Flow and Non-Flow Factorization Methods in Small Collision Systems, https://arxiv.org/abs/1902.11290
- N. M. Abdelwahab et al. (STAR), Isolation of Flow and Nonflow Correlations by Two- and Four-Particle Cumulant Measurements, https://arxiv.org/abs/1409.2043
- Y. Feng and F. Wang, Review of nonflow estimation methods and uncertainties in relativistic heavy-ion collisions, https://arxiv.org/abs/2407.12731
- Z. Wang et al., Nonflow Subtraction Beyond Two-Particle Correlations, https://arxiv.org/abs/2606.10258
- A. Bilandzic, R. Snellings, S. Voloshin, Flow analysis with cumulants: direct calculations, https://arxiv.org/abs/1010.0233
- A. Bilandzic, N. van der Kolk, J.-Y. Ollitrault, R. Snellings, Event-plane flow analysis without non-flow effects, https://arxiv.org/abs/0801.3915


## First-pass LEP1 MC closure result

The generated outputs are under `output/`; tracked snapshots are under
`docs/figures/`:

- `docs/figures/nonflow_mc_closure.csv`
- `docs/figures/nonflow_mc_closure_gap_c22_beam.pdf`
- `docs/figures/nonflow_mc_closure_gap_c22_thrust.pdf`
- `docs/figures/nonflow_mc_closure_gap_v22_beam.pdf`
- `docs/figures/nonflow_mc_closure_gap_v22_thrust.pdf`
- `docs/figures/nonflow_mc_closure_selection_c22_beam.pdf`
- `docs/figures/nonflow_mc_closure_selection_c22_thrust.pdf`
- `docs/figures/nonflow_mc_closure_selection_v22_beam.pdf`
- `docs/figures/nonflow_mc_closure_selection_v22_thrust.pdf`

Closure criterion: because `v2{2}` is positive definite when `c2{2}>0`, the
MC-zero test is based on the signed `c2{2}` or the signed eta-gap two-particle
correlator and its jackknife uncertainty. A successful nonflow suppression would
make the signed observable compatible with zero across multiplicity bins.

Summary of the closest-to-zero methods ranked by RMS signed `c2{2}` or gap
correlator over valid multiplicity bins:

| Axis | Method family | Best method | RMS signed observable | Mean `v2{2}` | Zero-compatible bins |
|---|---|---:|---:|---:|---:|
| beam | gap/subevent | `|Delta eta|>3.0` | 0.128 | 0.383 | 2/7 bins within 2 sigma |
| beam | track/event selection | `0.4<pT<1` | 0.171 | 0.420 | 1/5 bins within 2 sigma |
| thrust | gap/subevent | `|Delta eta|>3.0` | 0.0359 | 0.180 | 1/8 bins within 2 sigma |
| thrust | track/event selection | `0.4<pT<2` | 0.137 | 0.368 | 0/7 bins within 2 sigma |

Conclusion from this first pass: none of the tested suppressions makes the LEP1
reconstructed MC cumulant result statistically consistent with zero in all
multiplicity bins. The most effective reductions are large eta gaps and the
two-subevent gap scans on the thrust axis, followed by soft-track selections and
anti-jetty event-shape cuts. The beam-axis observable remains especially far
from zero even for very large eta gaps, which is consistent with strong residual
e+e- topology/nonflow rather than a collective MC signal.


### Harmonic-level subtraction diagnostics

The diagnostic subtraction output is generated at `output/nonflow_mc_subtraction_diagnostics.csv`;
the tracked snapshot is:

- `docs/figures/nonflow_mc_subtraction_diagnostics.csv`

These are not nominal corrections because the current analysis does not store
full `Delta phi` templates or eta-binned pair matrices. They are only closure
stress tests on the integrated signed `c2{2}` harmonic.

Results:

| Axis | Diagnostic | Result |
|---|---|---|
| beam | weighted `c2{2} = a + b/N` fit | `a = 0.4550 +/- 0.0006`, not compatible with zero |
| thrust | weighted `c2{2} = a + b/N` fit | `a = 0.1884 +/- 0.0006`, not compatible with zero |
| beam | low-multiplicity scaled subtraction | residual `c2{2}` remains about 0.15--0.35 after the first bin |
| thrust | low-multiplicity scaled subtraction | residual `c2{2}` remains about 0.08--0.15 after the first bin |

The simple harmonic-level subtractions therefore do not make LEP1 MC close to
zero either. A faithful template method would require storing the full
`Delta phi` correlation functions by multiplicity and axis.

Immediate next checks:

1. Implement and validate the genuine three-subevent `c2{4}` cumulant, because
   ATLAS reports this as least sensitive to short-range jet correlations.
2. Add eta-binned `Delta eta` correlation matrices so the STAR-style
   `Delta eta`-dependent/non-independent decomposition can be tested instead of
   only integrated gap scans.
3. Add full `Delta phi` template/peripheral-subtraction histograms by
   multiplicity and axis. The harmonic-level diagnostic above shows that a
   simple integrated subtraction is not enough.
