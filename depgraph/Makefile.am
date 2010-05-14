# Files 
DEPWORDS = master.depword noun.depword v.depword \
 av.depword a.depword\
 ajv.depword noun-variant.depword \
 fix.depword conjugate.depword \
 conjugate.table
INCLUDES = -I$(top_srcdir)/ -DSRCDIR=\"$(srcdir)\"
CLEANFILES = anthy.dep
EXTRA_DIST = indepword.txt $(DEPWORDS)

# Generate the dictionary
noinst_PROGRAMS = mkdepgraph
mkdepgraph_SOURCES = mkdepgraph.c
mkdepgraph_LDADD =  ../src-main/libanthy.la ../src-worddic/libanthydic.la

anthy.dep : mkdepgraph
	./mkdepgraph

noinst_DATA = anthy.dep
