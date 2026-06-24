ROOT_CONFIG ?= root-config
ROOTCFLAGS := $(shell $(ROOT_CONFIG) --cflags)
ROOTLIBS := $(shell $(ROOT_CONFIG) --glibs)

CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -Iinclude $(ROOTCFLAGS)
LDFLAGS ?= $(ROOTLIBS)

.PHONY: all check note clean

all: bin/aleph_charged_cumulants bin/merge_correlation_chunks bin/aleph_cumulant_summary bin/plot_v2_multiplicity bin/compare_v2_multiplicity bin/compare_v2_data_samples bin/nonflow_closure_study bin/compare_two_subevent_v2_multiplicity bin/compare_v22_eta_gap

check: all
	bin/aleph_charged_cumulants --SelfTest 1

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

bin:
	mkdir -p bin

clean:
	rm -f bin/aleph_charged_cumulants bin/merge_correlation_chunks bin/aleph_cumulant_summary bin/plot_v2_multiplicity bin/compare_v2_multiplicity bin/compare_v2_data_samples bin/nonflow_closure_study bin/compare_two_subevent_v2_multiplicity bin/compare_v22_eta_gap

