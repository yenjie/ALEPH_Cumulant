# Local Source Map

The repository was scaffolded from the local ALEPH/StudyMult material in `/data/yjlee` and nearby cumulant prototypes.

## Primary StudyMult Locations

- `/data/yjlee/StudyMult`
- `/data/yjlee/ALEPHSplittingFunction/StudyMult`
- `/data/yjlee/ALEPH_Develop/StudyMult`

The active StudyMult tree contains the ALEPH data-processing code, LEP1/LEP2 path lists, cleaned text inputs, and ROOT outputs.

## Relevant Files Read

- `/data/yjlee/StudyMult/DataProcessing/src/scan.cc`
  - Legacy ALEPH text-to-ROOT conversion logic.
  - Defines metadata extraction from paths and event end-of-event filling.
- `/data/yjlee/StudyMult/DataProcessing/include/particleDataLEP1.h`
  - Fixed-array tree branch layout: `nParticle`, `px`, `py`, `pz`, `pwflag`, `highPurity`, thrust-coordinate branches, and metadata.
  - `pwflag` convention documents charged particles as 0, 1, and 2.
- `/raid5/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/include/thrustTools.h`
  - Existing thrust-axis and thrust-coordinate helpers.
- `/data/yjlee/tutorial/ALEPHC24.cpp`
  - Prototype charged-particle `c2{2}`, `c2{4}`, `c2{6}`, and `c2{8}` numerator/denominator accumulation.
- `/data/yjlee/tutorial/ALEPHV224.cpp`
  - Prototype full-event and three-subevent `v224` accumulation.
- `/data/yjlee/tutorial/MergeCorrelationChunks.cpp`
  - Merge utility for chunked histogram outputs.
- `/data/yjlee/tutorial/run_aleph_c24_parallel.sh`
- `/data/yjlee/tutorial/run_aleph_v224_parallel.sh`
  - Chunking pattern reused by `scripts/run_chunks.sh`.

## Default Input Used By The Prototype

`/raid5/data/yjlee/ALEPH_Agentic_Event_Shape_Analysis/DataProcessing/temp/LEP1Data1994_recons_aftercut-MERGED_thrust_pt04_t.root`

The file exists locally and is a ROOT file. The analyzer defaults to this path for quick LEP1 1994 studies.

