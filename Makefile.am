AUTOMAKE_OPTIONS =
EXTRA_DIST = \
    ChangeLog-old \
    DIARY \
    Doxyfile \
    README.en \
    anthy-unicode.conf.in anthy-unicode-test.conf.in \
    anthy-unicode.pc.in \
    anthy-unicode.spec.in \
    $(NULL)

SRCDIR = src-diclib src-worddic src-splitter src-ordering src-main src-util

SUBDIRS = $(SRCDIR) anthy depgraph mkworddic calctrans mkanthydic doc test alt-cannadic

sysconf_DATA = anthy-unicode.conf

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = anthy-unicode.pc

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

dist-hook:
	if test -d .git ; then \
	    git log --name-status --date=iso > $(distdir)/ChangeLog ; \
	fi
