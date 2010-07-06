# Files 
DEPWORDS = conjugate.table conjugate.depword fix.depword noun.depword \
	   noun-variant.depword av.depword v.depword \
	   a.depword  ajv.depword master.depword
INCLUDES = -I$(top_srcdir)/ -DSRCDIR=\"$(srcdir)\"
CLEANFILES = anthy.dep all.depword
EXTRA_DIST = indepword-wt.txt $(DEPWORDS)

# Generate the dictionary
noinst_PROGRAMS = mkdepgraph
mkdepgraph_SOURCES = mkdepgraph.c
mkdepgraph_LDADD =  ../src-main/libanthy.la ../src-worddic/libanthydic.la

anthy.dep : mkdepgraph all.depword indepword-wt.txt
	./mkdepgraph

all.depword: $(DEPWORDS)
	cat $(DEPWORDS) | sed -n /^[^#]/p > $@

noinst_DATA = anthy.dep all.depword
