# Sample Audit, 2026-06-17

This note records the sample provenance checked for the ALEPH charged-particle cumulant production. It was written because the analysis note still contained an old 59,874-event StudyMult count while the current figures use the matched LEP1 1994 vector-format production.

## Current Samples Used by the Figures

| Role | Input | Format | Input entries | Selected events in multiplicity bins | Main output |
|---|---|---:|---:|---:|---|
| Data | `/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp/LEP1Data1994_recons_aftercut-MERGED_thrust_pt04_t.root` | vector | 1,365,440 | 1,321,026 | `output/lep1_1994_data_charged_pt04_merged.root` |
| Reconstructed MC | `/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp/alephMCRecoAfterCutPaths_1994_thrust_pt04_t.root` | vector | 771,597 | 749,082 | `output/lep1_1994_mc_charged_pt04_merged.root` |

These are the samples used for the current standalone data plots, standalone MC plots, data/MC comparison plots, eta-gap plots, and two-subevent plots in `AnalysisNote/figures/`.

The selected data event counts by laboratory charged-particle multiplicity bin are:

| Multiplicity bin | Data events | MC events |
|---|---:|---:|
| `[0,10)` | 91,054 | 51,269 |
| `[10,15)` | 454,384 | 255,772 |
| `[15,20)` | 497,271 | 282,116 |
| `[20,25)` | 220,815 | 127,126 |
| `[25,30)` | 50,506 | 28,994 |
| `[30,35)` | 6,527 | 3,530 |
| `[35,40)` | 449 | 269 |
| `[40,999)` | 20 | 6 |
| Total | 1,321,026 | 749,082 |

## Old StudyMult Prototype Sample

The older local StudyMult fixed-array files are:

- `/data/yjlee/StudyMult/DataFiles/ROOTfiles/cleaned_ALEPH_Data-all.aleph.root`
- `/data/yjlee/StudyMult/DataFiles/ROOTfiles/cleaned_ALEPH_Data2-all.aleph.root`

Both contain 59,874 entries in tree `t`. The old output `output/studymult_charged_pt04_merged.root` has the same 59,874 selected events, with per-bin counts 7,483, 17,665, 16,856, 9,452, 4,753, 2,287, 955, and 423. These are prototype counts and should not be quoted for the current figures.

## All-LEP1 Data Availability

The current production is 1994-only, not all LEP1. The available local LEP1 data path lists are:

| Year | Path list | Listed files |
|---|---|---:|
| 1991 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1991.txt` | 15 |
| 1992 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1992.txt` | 31 |
| 1993 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1993.txt` | 27 |
| 1994 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1994.txt` | 171 |
| 1995 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1995.txt` | 31 |
| Total |  | 275 |

A targeted check of `/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp` found ready vector-format LEP1 data ROOT files for 1994 only. Before quoting an all-LEP1 result, the 1991, 1992, 1993, 1994, and 1995 inputs need to be converted or merged into a compatible ROOT input, or processed year-by-year into chunk ROOT files and then summarized together.

Representative entries from these path lists, for example `/data/flowex/Datasamples/LEP1_MAIN/LEP1/1991/LEP1Data1991_recons_aftercut-001.aleph` and `/data/flowex/Datasamples/LEP1_MAIN/LEP1/1994/LEP1Data1994_recons_aftercut-001.aleph`, are ASCII ALEPH event records beginning with `ALEPH_DATA RUN = ...`, not ROOT TTrees. A direct probe with `bin/aleph_charged_cumulants --PrintEntries` fails with `not a ROOT file`, and `rootls` does not treat them as ROOT inputs. The existing StudyMult conversion path is `DataProcessing/src/scan.cc`, driven historically by `DataProcessing/bash/processLEP1.sh`; that script covers 1992, 1993, 1994, and 1995, while an all-LEP1 rerun should explicitly add the separate 1991 path list.

## Required Follow-Up for All LEP1 Statistics

1. Produce or identify compatible vector-format ROOT inputs for 1991-1995 data using the five LEP1 path lists above.
2. Run the cumulant analyzer on all years with the same charged-particle selection, `pT > 0.4` GeV, multiplicity bins, axis definitions, eta-gap settings, and two-subevent settings.
3. Combine all chunk ROOT files at the histogram-summary level so the central cumulants and delete-one-chunk jackknife uncertainties are recomputed from the full LEP1 sample.
4. Regenerate the note figures and update the sample-size table from the combined output histograms.
