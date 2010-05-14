AUTOMAKE_OPTIONS = dejagnu
EXTRA_DIST = README.en DIARY anthy-conf.in anthy-test-conf.in anthy.spec.in anthy.pc.in Doxyfile

SRCDIR = src-diclib src-worddic src-splitter src-ordering src-main src-util

SUBDIRS = $(SRCDIR) include depgraph mkworddic mkmatrix mkanthydic doc test cannadic calctrans
sysconf_DATA = anthy-conf

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = anthy.pc

MOSTLYCLEANFILES = core
