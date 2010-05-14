EXTRA_DIST = test.txt balance.txt relation.txt regression.txt $(SWIG_FILES)
CLEANFILES = anthy_swig_wrap.c anthy_swig_wrap.o anthy.so

SWIG_FILES = extconf.rb test.rb anthy_swig.i

INCLUDES = -I$(top_srcdir)/include -DSRCDIR=\"$(srcdir)\" \
	  -DTEST_HOME=\""`pwd`"\"

noinst_PROGRAMS = anthy checklib prediction test-matrix test-tt
anthy_SOURCES = main.c
checklib_SOURCES = check.c
prediction_SOURCES = prediction.c
test_matrix_SOURCES= test-matrix.c
test_tt_SOURCES= test-tt.c

anthy_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la
checklib_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la
prediction_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la
test_matrix_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la
test_tt_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la

mostlyclean-local:
	-rm -rf .anthy*
