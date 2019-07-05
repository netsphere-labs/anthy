# Files
noinst_SCRIPTS =
AM_CPPFLAGS = -I$(top_srcdir)/ -DSRCDIR=\"$(srcdir)\"
EXTRA_DIST = corpus_info weak_words

noinst_PROGRAMS = mkfiledic
mkfiledic_SOURCES = mkfiledic.c
mkfiledic_LDADD = ../src-diclib/libdiclib.la

if MAINTAINER_MODE
anthy.dic: anthy.dic3
	cp -p anthy.dic3 anthy.dic

anthy.dic0: mkfiledic ../mkworddic/anthy.wdic ../depgraph/anthy.dep
	./mkfiledic -i -o anthy.dic0

parsed_data0: $(top_srcdir)/corpus/corpus.?.txt ../calctrans/proccorpus anthy.dic0
	../calctrans/proccorpus -d anthy.dic0 \
		$(top_srcdir)/corpus/corpus.?.txt > parsed_data0

parsed_data1: $(top_srcdir)/corpus/corpus.?.txt ../calctrans/proccorpus anthy.dic1
	../calctrans/proccorpus -d anthy.dic1 \
		$(top_srcdir)/corpus/corpus.?.txt > parsed_data1

parsed_data2: $(top_srcdir)/corpus/corpus.?.txt ../calctrans/proccorpus anthy.dic2
	../calctrans/proccorpus -d anthy.dic2 \
		$(top_srcdir)/corpus/corpus.?.txt > parsed_data2

parsed_data3: $(top_srcdir)/corpus/corpus.?.txt ../calctrans/proccorpus anthy.dic3
	../calctrans/proccorpus -d anthy.dic3 \
		$(top_srcdir)/corpus/corpus.?.txt > parsed_data3

parsed_data4: $(top_srcdir)/corpus/corpus.?.txt ../calctrans/proccorpus anthy.dic4
	../calctrans/proccorpus -d anthy.dic4 \
		$(top_srcdir)/corpus/corpus.?.txt > parsed_data4

anthy.dic1: mkfiledic ../mkworddic/anthy.wdic ../depgraph/anthy.dep parsed_data0
	../calctrans/calctrans parsed_data0 -o corpus_info
	../calctrans/calctrans parsed_data0 -e -o weak_words
	../calctrans/calctrans -c corpus_info weak_words
	./mkfiledic -o anthy.dic1

anthy.dic2: mkfiledic ../mkworddic/anthy.wdic ../depgraph/anthy.dep \
	    parsed_data0 parsed_data1
	../calctrans/calctrans parsed_data0 parsed_data1 -o corpus_info
	../calctrans/calctrans parsed_data0 parsed_data1 -e -o weak_words
	../calctrans/calctrans -c corpus_info weak_words
	./mkfiledic -o anthy.dic2

anthy.dic3: mkfiledic ../mkworddic/anthy.wdic ../depgraph/anthy.dep \
	    parsed_data0 parsed_data1 parsed_data2
	../calctrans/calctrans parsed_data0 parsed_data1 parsed_data2 -o corpus_info
	../calctrans/calctrans parsed_data0 parsed_data1 parsed_data2 -e -o weak_words
	../calctrans/calctrans -c corpus_info weak_words
	./mkfiledic -o anthy.dic3

anthy.dic4: mkfiledic ../mkworddic/anthy.wdic ../depgraph/anthy.dep \
	    parsed_data0 parsed_data1 parsed_data2 parsed_data3
	../calctrans/calctrans parsed_data0 parsed_data1 parsed_data2 parsed_data3 -o corpus_info
	../calctrans/calctrans parsed_data0 parsed_data1 parsed_data2 parsed_data3 -e -o weak_words
	../calctrans/calctrans -c corpus_info weak_words
	./mkfiledic -o anthy.dic4

anthy.dic5: mkfiledic ../mkworddic/anthy.wdic ../depgraph/anthy.dep \
	    parsed_data0 parsed_data1 parsed_data2 parsed_data3 parsed_data4
	../calctrans/calctrans parsed_data0 parsed_data1 parsed_data2 parsed_data3 parsed_data4 -o corpus_info
	../calctrans/calctrans parsed_data0 parsed_data1 parsed_data2 parsed_data3 parsed_data4 -e -o weak_words
	../calctrans/calctrans -c corpus_info weak_words
	./mkfiledic -o anthy.dic5
else
anthy.dic: mkfiledic ../mkworddic/anthy.wdic ../depgraph/anthy.dep \
	   corpus_info weak_words
	../calctrans/calctrans -c $(srcdir)/corpus_info $(srcdir)/weak_words
	./mkfiledic -o anthy.dic
endif

# To install 
pkgdata_DATA = anthy.dic

CLEANFILES = anthy.cand_info anthy.trans_info anthy.corpus_array \
	anthy.corpus_bucket anthy.weak_words anthy.feature_info \
	parsed_data* anthy.dic*

MAINTAINERCLEANFILES= corpus_info weak_words
