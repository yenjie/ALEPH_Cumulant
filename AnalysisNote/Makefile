TEX = main

.PHONY: all clean

all: $(TEX).pdf

$(TEX).pdf: $(TEX).tex references.bib figures/v2_multiplicity_beam.pdf figures/v2_multiplicity_thrust.pdf figures/v2_eta_gap_data_mc_compare_beam.pdf figures/v2_eta_gap_data_mc_compare_thrust.pdf figures/v2_eta_gap_1p6_data_mc_compare_beam.pdf figures/v2_eta_gap_1p6_data_mc_compare_thrust.pdf
	pdflatex -interaction=nonstopmode $(TEX).tex
	bibtex $(TEX)
	pdflatex -interaction=nonstopmode $(TEX).tex
	pdflatex -interaction=nonstopmode $(TEX).tex

clean:
	rm -f $(TEX).aux $(TEX).bbl $(TEX).blg $(TEX).log $(TEX).out $(TEX).toc $(TEX).pdf
