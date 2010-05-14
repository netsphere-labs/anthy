EXTRA_DIST = dep.sorted

noinst_PROGRAMS = mkmatrix
INCLUDES = -I$(top_srcdir)/include
mkmatrix_LDADD = ../src-worddic/libanthydic.la -lm
mkmatrix_SOURCES = mkmatrix.c

noinst_DATA = matrix
CLEANFILES = matrix

matrix	: mkmatrix $(top_srcdir)/mkmatrix/dep.sorted
	./mkmatrix $(top_srcdir)/mkmatrix/dep.sorted -o matrix
