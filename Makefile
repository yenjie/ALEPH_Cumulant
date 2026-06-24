ROOT_CONFIG ?= root-config
ROOTCFLAGS := $(shell $(ROOT_CONFIG) --cflags)
ROOTLIBS := $(shell $(ROOT_CONFIG) --glibs)
PYTHIA8_CONFIG ?= pythia8-config
PYTHIA8_CXXFLAGS := $(shell $(PYTHIA8_CONFIG) --cxxflags 2>/dev/null)
PYTHIA8_LDFLAGS := $(shell $(PYTHIA8_CONFIG) --ldflags --libs 2>/dev/null)

CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -Iinclude $(ROOTCFLAGS)
LDFLAGS ?= $(ROOTLIBS)

.PHONY: all check note pythia clean

all: bin/aleph_charged_cumulants bin/merge_correlation_chunks bin/aleph_cumulant_summary bin/plot_v2_multiplicity bin/compare_v2_multiplicity bin/compare_v2_data_samples bin/nonflow_closure_study bin/compare_two_subevent_v2_multiplicity bin/compare_v22_eta_gap

check: all
	bin/aleph_charged_cumulants --SelfTest 1

pythia: bin/generate_pythia_zpole_root

note:
	$(MAKE) -C AnalysisNote main.pdf

bin/aleph_charged_cumulants: src/AlephChargedCumulants.cpp include/CommandLine.h include/ThrustTools.h | bin
	$(CXX) $(CXXFLAGS) src/AlephChargedCumulants.cpp -o $@ $(LDFLAGS)

bin/merge_correlation_chunks: src/MergeCorrelationChunks.cpp | bin
	$(CXX) $(CXXFLAGS) src/MergeCorrelationChunks.cpp -o $@ $(LDFLAGS)

bin/aleph_cumulant_summary: src/AlephCumulantSummary.cpp include/CommandLine.h | bin
	$(CXX) $(CXXFLAGS) src/AlephCumulantSummary.cpp -o $@ $(LDFLAGS)

bin/plot_v2_multiplicity: src/PlotV2Multiplicity.cpp include/CommandLine.h | bin
	$(CXX) $(CXXFLAGS) src/PlotV2Multiplicity.cpp -o $@ $(LDFLAGS)

bin/compare_v2_multiplicity: src/CompareV2Multiplicity.cpp include/CommandLine.h | bin
	$(CXX) $(CXXFLAGS) src/CompareV2Multiplicity.cpp -o $@ $(LDFLAGS)

bin/compare_v2_data_samples: src/CompareV2DataSamples.cpp include/CommandLine.h | bin
	$(CXX) $(CXXFLAGS) src/CompareV2DataSamples.cpp -o $@ $(LDFLAGS)

bin/nonflow_closure_study: src/NonflowClosureStudy.cpp include/CommandLine.h | bin
	$(CXX) $(CXXFLAGS) src/NonflowClosureStudy.cpp -o $@ $(LDFLAGS)

bin/compare_two_subevent_v2_multiplicity: src/CompareTwoSubeventV2Multiplicity.cpp include/CommandLine.h | bin
	$(CXX) $(CXXFLAGS) src/CompareTwoSubeventV2Multiplicity.cpp -o $@ $(LDFLAGS)

bin/compare_v22_eta_gap: src/CompareV22EtaGap.cpp include/CommandLine.h | bin
	$(CXX) $(CXXFLAGS) src/CompareV22EtaGap.cpp -o $@ $(LDFLAGS)

bin/generate_pythia_zpole_root: src/GeneratePythiaZPoleRoot.cpp include/CommandLine.h | bin
	$(CXX) $(CXXFLAGS) $(PYTHIA8_CXXFLAGS) -std=c++17 -D_GLIBCXX_USE_CXX11_ABI=0 src/GeneratePythiaZPoleRoot.cpp -o $@ $(LDFLAGS) $(PYTHIA8_LDFLAGS)

bin:
	mkdir -p bin

clean:
	rm -f bin/aleph_charged_cumulants bin/merge_correlation_chunks bin/aleph_cumulant_summary bin/plot_v2_multiplicity bin/compare_v2_multiplicity bin/compare_v2_data_samples bin/nonflow_closure_study bin/compare_two_subevent_v2_multiplicity bin/compare_v22_eta_gap bin/generate_pythia_zpole_root

