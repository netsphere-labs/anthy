EXTRA_DIST = test.txt balance.txt relation.txt regression.txt $(SWIG_FILES)
CLEANFILES = anthy_swig_wrap.c anthy_swig_wrap.o anthy.so

SWIG_FILES = extconf.rb test.rb anthy_swig.i

INCLUDES = -I$(top_srcdir)/include -DSRCDIR=\"$(srcdir)\" \
	  -DTEST_HOME=\""`pwd`"\"

noinst_PROGRAMS = anthy checklib
anthy_SOURCES = main.c
checklib_SOURCES = check.c

anthy_LDADD = ../src-main/libanthy.la ../src-diclib/libanthydic.la
checklib_LDADD = ../src-main/libanthy.la ../src-diclib/libanthydic.la

mostlyclean-local:
	-rm -rf .anthy*
