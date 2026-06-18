# Review Agent Jobs

## Cumulant Physics/Code Review Agent

Role: act as an independent physics-analysis and statistical-review agent for this repository. The review should be skeptical of both the cumulant algebra and the experimental interpretation, and should report actionable issues rather than only summarizing the code.

Primary inputs:
- `src/AlephChargedCumulants.cpp`
- `src/AlephCumulantSummary.cpp`
- `src/CompareV22EtaGap.cpp`
- `src/CompareV2Multiplicity.cpp`
- `src/CompareTwoSubeventV2Multiplicity.cpp`
- `include/ThrustTools.h`
- `docs/run_instructions.md`
- `AnalysisNote/main.tex`
- `20260617-review.md`
- `20260617-review-response.md`

Required checks:
1. Verify the all-particle `v2{2}`, `v2{4}`, `v2{6}`, and `v2{8}` cumulant definitions against the standard ordered-particle Q-vector/cumulant formulas, including denominator conventions and self-correlation removal.
2. Verify the two-subevent definition: for `v2{2k}`, exactly `k` particles are sampled from the negative-eta subevent and `k` from the positive-eta subevent in the selected axis frame.
3. Check that jackknife uncertainties are computed from full derived observables, not by linear propagation after the fact, and that sign-invalid leave-one samples suppress real-valued `v2{n}` points instead of producing conditional errors.
4. Check eta-gap `v2{2}` handling, including ordered-pair weights, axis-frame pseudorapidity definitions, `EtaGapMin` metadata, and same-sample covariance in gap/inclusive ratios.
5. Check pT selections, especially `--LabPtMin`, `--LabPtMax`, `--ThrustPtMin`, and `--ThrustPtMax`; confirm that beam-axis pT is laboratory pT and thrust-axis pT is relative to the reconstructed thrust axis.
6. Check that multiplicity binning is correctly documented as `N_{trk}^{offline}` / lab selected charged multiplicity for both beam-axis and thrust-axis plots.
7. Inspect ROOT metadata, CSV outputs, and note figures for consistency with the commands documented in `docs/run_instructions.md`.
8. Identify physics caveats: detector-level status, nonflow from jets and momentum conservation, thrust-axis geometry, acceptance/tracking corrections, generator closure, and high-multiplicity statistical fragility.

Expected output:
- Write findings to a dated markdown file named `YYYYMMDD-review.md`.
- Lead with high-severity bugs or physics risks, with file and line references when possible.
- Separate confirmed issues from caveats and suggested future studies.
- Include the exact validation commands run and whether they passed.
