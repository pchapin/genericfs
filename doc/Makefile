#
# FILE   : Makefile
# SUBJECT: Makefile for the GenericFS documentation.
# AUTHOR : Peter C. Chapin
#

all:	GenericFS.pdf

# Compile the documentation.
GenericFS.pdf:	GenericFS.tex
	pdflatex GenericFS.tex
	pdflatex GenericFS.tex
	pdflatex GenericFS.tex

clean:
	rm -rf GenericFS.aux GenericFS.log \
	       GenericFS.out GenericFS.toc \
	       GenericFS.bbl GenericFS.blg
	rm -rf GenericFS.pdf
