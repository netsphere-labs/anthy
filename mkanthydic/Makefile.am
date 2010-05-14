# Files
noinst_SCRIPTS =
CLEANFILES = anthy.dic
INCLUDES = -I$(top_srcdir)/include -DSRCDIR=\"$(srcdir)\"

noinst_PROGRAMS = mkfiledic
mkfiledic_SOURCES = mkfiledic.c
mkfiledic_LDADD = ../src-diclib/libdiclib.la

anthy.dic : mkfiledic ../mkworddic/anthy.wdic ../depgraph/anthy.dep
	./mkfiledic


# To install 
pkgdata_DATA = anthy.dic
