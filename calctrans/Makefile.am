EXTRA_DIST = \
 corpus.0.txt corpus.1.txt corpus.2.txt \
 corpus.3.txt corpus.4.txt corpus.5.txt \
 corpus_info weak_words

noinst_PROGRAMS = calctrans proccorpus
INCLUDES = -I$(top_srcdir)/

calctrans_SOURCES = calctrans.c input_set.c input_set.h corpus.c
calctrans_LDADD = ../src-main/libanthy.la ../src-worddic/libanthydic.la -lm

proccorpus_SOURCES = proccorpus.c
proccorpus_LDADD = ../src-util/libconvdb.la ../src-main/libanthy.la ../src-worddic/libanthydic.la

calctrans_outputs = corpus_info weak_words
dict_source_files = anthy.cand_info anthy.trans_info anthy.corpus_array anthy.corpus_bucket anthy.weak_words

parsed_data: $(srcdir)/corpus.?.txt proccorpus ../initial-dic/initial-anthy.dic
	./proccorpus $(srcdir)/corpus.?.txt > parsed_data

corpus_info: parsed_data calctrans
	./calctrans parsed_data -o corpus_info

weak_words: parsed_data calctrans
	./calctrans parsed_data -e -o weak_words

$(dict_source_files): $(calctrans_outputs) calctrans
	./calctrans -c $(calctrans_outputs)

noinst_DATA = $(dict_source_files)
CLEANFILES = $(dict_source_files) parsed_data anthy.feature_info $(calctrans_outputs)
