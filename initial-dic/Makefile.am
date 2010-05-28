CLEANFILES = initial-anthy.dic
INCLUDES = -I$(top_srcdir)/ -DSRCDIR=\"$(srcdir)\"

noinst_PROGRAMS = mkfiledic0
mkfiledic0_SOURCES = mkfiledic0.c
mkfiledic0_LDADD = ../src-diclib/libdiclib.la

initial-anthy.dic : mkfiledic0 ../mkworddic/anthy.wdic ../depgraph/anthy.dep
	./mkfiledic0

noinst_DATA = initial-anthy.dic
