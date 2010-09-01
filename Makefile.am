AUTOMAKE_OPTIONS =
EXTRA_DIST = README.en DIARY anthy-conf.in anthy-test-conf.in anthy.spec.in anthy.pc.in Doxyfile

SRCDIR = src-diclib src-worddic src-splitter src-ordering src-main src-util

SUBDIRS = $(SRCDIR) anthy depgraph mkworddic calctrans mkanthydic doc test alt-cannadic

sysconf_DATA = anthy-conf

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = anthy.pc

MOSTLYCLEANFILES = core

update_params:
	make -C calctrans update_params
	make
update_params0:
	make -C calctrans update_params0
	make
update_params2:
	make -C calctrans update_params2
	make
