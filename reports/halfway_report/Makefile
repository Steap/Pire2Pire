# Shittiest Makefile ever

all: clean report
	evince report.pdf

report:
	latex report
	dvipdf report.dvi

clean:
	rm -f *.aux *.log *.toc
