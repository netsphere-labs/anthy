# EXTRA_DIST = \
#  corpus.0.txt corpus.1.txt corpus.2.txt \
#  corpus.3.txt corpus.4.txt corpus.5.txt \
#  corpus_info weak_words

noinst_PROGRAMS = calctrans proccorpus
AM_CPPFLAGS = -I$(top_srcdir)/

calctrans_SOURCES = calctrans.c input_set.c input_set.h corpus.c
calctrans_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la -lm

proccorpus_SOURCES = proccorpus.c
proccorpus_LDADD = ../src-util/libconvdb.la ../src-main/libanthy.la ../src-worddic/libanthydic.la
