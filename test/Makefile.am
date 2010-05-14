SUBDIRS =

EXTRA_DIST = test.txt test.exp

INCLUDES = -I$(top_srcdir)/include -DSRCDIR=\"$(srcdir)\" \
	  -DTEST_HOME=\""`pwd`"\"

noinst_PROGRAMS = anthy checklib prediction test-matrix test-tt
anthy_SOURCES = main.c
checklib_SOURCES = check.c
prediction_SOURCES = prediction.c
test_matrix_SOURCES= test-matrix.c
test_tt_SOURCES= test-tt.c

anthy_LDADD = ../src-util/libconvdb.la ../src-main/libanthy.la ../src-worddic/libanthydic.la
checklib_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la
prediction_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la
test_matrix_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la
test_tt_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la

mostlyclean-local:
	-rm -rf .anthy*
