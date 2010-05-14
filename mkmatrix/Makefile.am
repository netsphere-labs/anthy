EXTRA_DIST = input.txt parsed.txt

noinst_PROGRAMS = mkmatrix
INCLUDES = -I$(top_srcdir)/include
mkmatrix_LDADD = ../src-worddic/libanthydic.la
mkmatrix_SOURCES = mkmatrix.c

pkgdata_DATA = matrix
CLEANFILES = matrix

matrix	: mkmatrix $(top_srcdir)/mkmatrix/parsed.txt
	./mkmatrix $(top_srcdir)/mkmatrix/parsed.txt -o matrix

$(top_srcdir)/mkmatrix/parsed.txt	: $(top_srcdir)/mkmatrix/input.txt
	../src-util/anthy-morphological-analyzer < $(top_srcdir)/mkmatrix/input.txt | tee $(top_srcdir)/mkmatrix/parsed.txt
