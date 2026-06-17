TEX = main

.PHONY: all clean

all: $(TEX).pdf

$(TEX).pdf: $(TEX).tex references.bib figures/v2_multiplicity_beam.pdf figures/v2_multiplicity_thrust.pdf
	pdflatex -interaction=nonstopmode $(TEX).tex
	bibtex $(TEX)
	pdflatex -interaction=nonstopmode $(TEX).tex
	pdflatex -interaction=nonstopmode $(TEX).tex

clean:
	rm -f $(TEX).aux $(TEX).bbl $(TEX).blg $(TEX).log $(TEX).out $(TEX).toc $(TEX).pdf
