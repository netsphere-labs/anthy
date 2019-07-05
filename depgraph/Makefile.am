# Files 
DEPWORDS = conjugate.table conjugate.depword fix.depword noun.depword \
	   noun-variant.depword av.depword v.depword \
	   a.depword  ajv.depword master.depword
AM_CPPFLAGS = -I$(top_srcdir)/ -DINDEPWORD_INPUT_FILENAME=\"$(srcdir)/indepword-wt.txt\"
CLEANFILES = anthy.dep all.depword
EXTRA_DIST = indepword-wt.txt $(DEPWORDS) indepword.txt

# Generate the dictionary
noinst_PROGRAMS = mkdepgraph
mkdepgraph_SOURCES = mkdepgraph.c
mkdepgraph_LDADD =  ../src-main/libanthy.la ../src-worddic/libanthydic.la

anthy.dep : mkdepgraph all.depword indepword-wt.txt
	./mkdepgraph

all.depword: $(DEPWORDS)
	(cd $(srcdir)&& cat $(DEPWORDS)) | sed -n /^[^#]/p > $@

noinst_DATA = anthy.dep all.depword
