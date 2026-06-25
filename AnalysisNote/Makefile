TEX = main

.PHONY: all clean

all: $(TEX).pdf

FIGURES = \
	figures/v2_multiplicity_beam.pdf \
	figures/v2_multiplicity_thrust.pdf \
	figures/v2_lep2_data_multiplicity_beam.pdf \
	figures/v2_lep2_data_multiplicity_thrust.pdf \
	figures/v2_lep1_lep2_data_compare_beam.pdf \
	figures/v2_lep1_lep2_data_compare_thrust.pdf \
	figures/v2_multiplicity_mc_beam.pdf \
	figures/v2_multiplicity_mc_thrust.pdf \
	figures/v2_data_mc_compare_beam.pdf \
	figures/v2_data_mc_compare_thrust.pdf \
	figures/nonflow_mc_closure_gap_c22_beam.pdf \
	figures/nonflow_mc_closure_gap_c22_thrust.pdf \
	figures/nonflow_mc_closure_selection_c22_beam.pdf \
	figures/nonflow_mc_closure_selection_c22_thrust.pdf \
	figures/pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_beam.pdf \
	figures/pythia_zpole_5M_noshoving_vs_shoving_pt04_compare_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_etagap1p0_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_etagap1p0_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_etagap1p6_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_etagap1p6_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_etagap2p0_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_etagap2p0_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_etagap2p5_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_etagap2p5_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_etagap3p0_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_etagap3p0_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_twosub0p0_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_twosub0p0_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_twosub1p0_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_twosub1p0_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_twosub1p6_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_twosub1p6_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_twosub2p0_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_twosub2p0_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_pt04to1_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_pt04to1_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_pt04to2_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_pt04to2_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_pt1to3_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_pt1to3_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_positive_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_positive_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_negative_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_negative_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_thrustlt0p90_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_thrustlt0p90_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_thrustlt0p85_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_thrustlt0p85_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_sphgt0p12_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_sphgt0p12_noshoving_vs_shoving_thrust.pdf \
	figures/pythia_zpole_5M_nonflow_sphgt0p22_noshoving_vs_shoving_beam.pdf \
	figures/pythia_zpole_5M_nonflow_sphgt0p22_noshoving_vs_shoving_thrust.pdf \
	figures/v2_eta_gap_data_mc_compare_beam.pdf \
	figures/v2_eta_gap_data_mc_compare_thrust.pdf \
	figures/v2_eta_gap_1p6_data_mc_compare_beam.pdf \
	figures/v2_eta_gap_1p6_data_mc_ratio_beam.pdf \
	figures/v2_eta_gap_1p6_data_mc_compare_thrust.pdf \
	figures/v2_eta_gap_1p6_data_mc_ratio_thrust.pdf \
	figures/v2_pt1to3_data_mc_compare_beam.pdf \
	figures/v2_pt1to3_data_mc_compare_thrust.pdf \
	figures/v2_pt1to3_eta_gap_data_mc_compare_beam.pdf \
	figures/v2_pt1to3_eta_gap_data_mc_ratio_beam.pdf \
	figures/v2_pt1to3_eta_gap_data_mc_compare_thrust.pdf \
	figures/v2_pt1to3_eta_gap_data_mc_ratio_thrust.pdf \
	figures/v2_pt1to3_eta_gap_1p6_data_mc_compare_beam.pdf \
	figures/v2_pt1to3_eta_gap_1p6_data_mc_ratio_beam.pdf \
	figures/v2_pt1to3_eta_gap_1p6_data_mc_compare_thrust.pdf \
	figures/v2_pt1to3_eta_gap_1p6_data_mc_ratio_thrust.pdf \
	figures/v2_two_sub_data_mc_compare_beam.pdf \
	figures/v2_two_sub_data_mc_compare_thrust.pdf

$(TEX).pdf: $(TEX).tex references.bib $(FIGURES)
	pdflatex -interaction=nonstopmode $(TEX).tex
	bibtex $(TEX)
	pdflatex -interaction=nonstopmode $(TEX).tex
	pdflatex -interaction=nonstopmode $(TEX).tex

clean:
	rm -f $(TEX).aux $(TEX).bbl $(TEX).blg $(TEX).log $(TEX).out $(TEX).toc $(TEX).pdf
