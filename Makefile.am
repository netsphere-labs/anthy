AUTOMAKE_OPTIONS =
ACLOCAL_AMFLAGS=-I m4

EXTRA_DIST = COPYING.LIB2_1 \
	README.2007.ja THANKS.2007.ja NEWS.2007.ja INSTALL.2006.ja \
	ChangeLog.2001-2009 \
	anthy-conf.in anthy-test-conf.in anthy.spec.in anthy.pc.in Doxyfile \
	corpus/corpus.?.txt

SRCDIR = src-diclib src-worddic src-splitter src-ordering src-main src-util

SUBDIRS = $(SRCDIR) anthy depgraph mkworddic calctrans mkanthydic doc test alt-cannadic

sysconf_DATA = anthy-conf

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = anthy.pc

MOSTLYCLEANFILES = core
