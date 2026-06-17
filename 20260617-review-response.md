# 2026-06-17 Review Response

This response records the code and note changes made after `20260617-review.md`.

## Addressed in Code

1. Eta-gap/inclusive ratio uncertainties now preserve same-sample covariance. `src/AlephCumulantSummary.cpp` writes `hV2_2EtaGapOverInclusive_{beam,thrust}` by forming the ratio in the central sample and in every delete-one-chunk sample, then jackknifing that ratio directly. `src/CompareV22EtaGap.cpp` reads those histograms for the lower panel and CSV output instead of propagating independent errors.

2. Real-valued `v2{m}` jackknife errors no longer use only sign-valid leave-one samples. If any leave-one sample is sign-invalid for a plotted `hV2_*` or `hV2TwoSub_*` point, the point is suppressed with zero content/error. Raw cumulant histograms remain available for sign-change studies.

3. The small-multiplicity thrust-axis shortcut was removed. `include/ThrustTools.h` now uses the signed-sum candidate search for all multiplicities and falls back to the first nonzero momentum direction only if no candidate axis can be built.

4. Beam-axis and thrust-axis plots now label the horizontal axis as laboratory selected charged multiplicity. The README and analysis note state that thrust-axis cumulants are currently binned by lab selected charged multiplicity, while thrust-selected multiplicity is stored only as a diagnostic.

5. The two-subevent documentation now describes `TwoSubeventEtaBoundary = 0.0` as a two-hemisphere split, not as a finite eta-gap nonflow suppression.

## Remaining Analysis Caveats

The following review points are documented but not solved by this code-only update: detector-response corrections, acceptance/tracking systematics, generator-level closure, and broader nonflow interpretation in LEP1 e+e- events. These require additional production and systematic studies, not just a cumulant-summary code change.

## Validation

- `make check` passed, including the correlator self-test.
- Six summary ROOT files were regenerated from the existing 16 chunk files per sample, preserving delete-one-chunk jackknife errors.
- Standalone data/MC, data-vs-MC, eta-gap, and two-subevent PDFs/CSVs were regenerated.
- `make note` rebuilt `AnalysisNote/main.pdf` successfully as a 16-page PDF.
