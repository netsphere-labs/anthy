SUBDIRS =

EXTRA_DIST = test.txt test.exp prediction.c test-matrix.c

INCLUDES = -I$(top_srcdir)/ -DSRCDIR=\"$(srcdir)\" \
	  -DTEST_HOME=\""`pwd`"\"

noinst_PROGRAMS = anthy checklib
anthy_SOURCES = main.c
checklib_SOURCES = check.c

anthy_LDADD = ../src-util/libconvdb.la ../src-main/libanthy.la ../src-worddic/libanthydic.la
checklib_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la

mostlyclean-local:
	-rm -rf .anthy*
