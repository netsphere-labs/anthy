# Files 
DEPWORDS = master.depword noun.depword v.depword \
 av.depword a.depword\
 ajv.depword noun-variant.depword \
 fix.depword conjugate.depword \
 conjugate.table
INCLUDES = -I$(top_srcdir)/include -DSRCDIR=\"$(srcdir)\"
CLEANFILES = anthy.dep
EXTRA_DIST = indepword.txt $(DEPWORDS)

# Generate the dictionary
noinst_PROGRAMS = mkdepgraph
mkdepgraph_SOURCES = mkdepgraph.c
mkdepgraph_LDADD = ../src-diclib/libanthydic.la ../src-main/libanthy.la

anthy.dep : mkdepgraph
	./mkdepgraph

# To install
pkgdata_DATA = anthy.dep
