# Sample Audit, 2026-06-17

This note records the sample provenance checked for the ALEPH charged-particle cumulant production. It has been updated after switching the current figures from the 1994-only vector-format sample to the available merged LEP1 1992-1995 StudyMult ROOT sample.

## Current Samples Used by the Figures

| Role | Input | Format | Input entries | Selected events in multiplicity bins | Main output |
|---|---|---:|---:|---:|---|
| Data | `/mnt/data3/data3/yjlee/StudyMultSamples/ALEPH/LEP1/LEP1Merged_20200611.root` | StudyMult array | 3,050,610 | 3,050,610 | `output/lep1_1992_1995_data_charged_pt04_merged.root` |
| Reconstructed MC | `/mnt/data3/data3/yjlee/StudyMultSamples/LEP1MCMerged.root` | StudyMult array | 771,597 | 749,082 | `output/lep1_1994_mc_merged_charged_pt04_merged.root` |

The data tree has the following year composition:

| Year | Entries |
|---|---:|
| 1992 | 551,474 |
| 1993 | 538,601 |
| 1994 | 1,365,440 |
| 1995 | 595,095 |
| Total | 3,050,610 |

The merged MC tree contains 771,597 reconstructed entries, all from 1994. It is used as a detector-level MC baseline rather than a strict year-matched comparison to the 1992-1995 data sample.

The selected data and MC event counts by laboratory charged-particle multiplicity bin are:

| Multiplicity bin | Data events | MC events |
|---|---:|---:|
| `[0,10)` | 241,919 | 51,269 |
| `[10,15)` | 1,046,419 | 255,772 |
| `[15,20)` | 1,132,225 | 282,116 |
| `[20,25)` | 500,481 | 127,126 |
| `[25,30)` | 113,682 | 28,994 |
| `[30,35)` | 14,738 | 3,530 |
| `[35,40)` | 1,095 | 269 |
| `[40,999)` | 51 | 6 |
| Total | 3,050,610 | 749,082 |

## Eta-Gap and Two-Subevent Effective Counts

For `|Delta eta| > 2.0`, data events with at least one accepted gap pair:

| Multiplicity bin | Beam axis | Thrust axis |
|---|---:|---:|
| `[0,10)` | 67,401 | 206,648 |
| `[10,15)` | 354,856 | 1,003,731 |
| `[15,20)` | 439,597 | 1,113,837 |
| `[20,25)` | 218,599 | 496,709 |
| `[25,30)` | 54,949 | 113,296 |
| `[30,35)` | 7,655 | 14,708 |
| `[35,40)` | 605 | 1,094 |
| `[40,999)` | 25 | 51 |
| Total | 1,143,687 | 2,950,074 |

For `|Delta eta| > 1.6`, data events with at least one accepted gap pair:

| Multiplicity bin | Beam axis | Thrust axis |
|---|---:|---:|
| `[0,10)` | 111,958 | 211,692 |
| `[10,15)` | 563,146 | 1,015,568 |
| `[15,20)` | 682,139 | 1,120,538 |
| `[20,25)` | 335,086 | 498,386 |
| `[25,30)` | 83,084 | 113,486 |
| `[30,35)` | 11,560 | 14,722 |
| `[35,40)` | 900 | 1,095 |
| `[40,999)` | 40 | 51 |
| Total | 1,787,913 | 2,975,538 |

For the two-subevent result, data events with at least one two-subevent pair:

| Multiplicity bin | Beam axis | Thrust axis |
|---|---:|---:|
| `[0,10)` | 237,521 | 192,833 |
| `[10,15)` | 1,044,328 | 975,846 |
| `[15,20)` | 1,131,787 | 1,102,461 |
| `[20,25)` | 500,452 | 495,266 |
| `[25,30)` | 113,680 | 113,230 |
| `[30,35)` | 14,738 | 14,709 |
| `[35,40)` | 1,095 | 1,095 |
| `[40,999)` | 51 | 51 |
| Total | 3,043,652 | 2,895,491 |

## Older Samples Checked

The previous 1994-only vector-format files are still available and were used for earlier note versions:

- Data: `/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp/LEP1Data1994_recons_aftercut-MERGED_thrust_pt04_t.root`, 1,365,440 entries, 1,321,026 selected in the multiplicity bins.
- MC: `/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp/alephMCRecoAfterCutPaths_1994_thrust_pt04_t.root`, 771,597 entries, 749,082 selected in the multiplicity bins.

The older local StudyMult fixed-array prototype files are:

- `/data/yjlee/StudyMult/DataFiles/ROOTfiles/cleaned_ALEPH_Data-all.aleph.root`
- `/data/yjlee/StudyMult/DataFiles/ROOTfiles/cleaned_ALEPH_Data2-all.aleph.root`

Both contain 59,874 entries in tree `t`. The old output `output/studymult_charged_pt04_merged.root` has the same 59,874 selected events, with per-bin counts 7,483, 17,665, 16,856, 9,452, 4,753, 2,287, 955, and 423. These are prototype counts and should not be quoted for the current figures.

## 1991 Status

The available local LEP1 data path lists include a separate 1991 list:

| Year | Path list | Listed files |
|---|---|---:|
| 1991 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1991.txt` | 15 |
| 1992 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1992.txt` | 31 |
| 1993 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1993.txt` | 27 |
| 1994 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1994.txt` | 171 |
| 1995 | `/data/yjlee/StudyMult/DataProcessing/paths/alephDataPaths_LEP1_1995.txt` | 31 |
| Total |  | 275 |

Representative entries from these path lists, for example `/data/flowex/Datasamples/LEP1_MAIN/LEP1/1991/LEP1Data1991_recons_aftercut-001.aleph`, are ASCII ALEPH event records beginning with `ALEPH_DATA RUN = ...`, not ROOT TTrees. A direct probe with `bin/aleph_charged_cumulants --PrintEntries` fails with `not a ROOT file`, and `rootls` does not treat them as ROOT inputs. The existing StudyMult conversion path is `DataProcessing/src/scan.cc`, driven historically by `DataProcessing/bash/processLEP1.sh`; that script covers 1992, 1993, 1994, and 1995, while a complete 1991-1995 rerun should explicitly add and convert the 1991 path list.
