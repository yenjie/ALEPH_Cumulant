TEX = main

.PHONY: all clean

all: $(TEX).pdf

FIGURES = \
	figures/v2_multiplicity_beam.pdf \
	figures/v2_multiplicity_thrust.pdf \
	figures/v2_multiplicity_mc_beam.pdf \
	figures/v2_multiplicity_mc_thrust.pdf \
	figures/v2_data_mc_compare_beam.pdf \
	figures/v2_data_mc_compare_thrust.pdf \
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
