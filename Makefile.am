AUTOMAKE_OPTIONS = dejagnu
EXTRA_DIST = README.en DIARY anthy-conf.in anthy-test-conf.in anthy.spec.in anthy.pc.in Doxyfile

SRCDIR = src-diclib src-splitter src-ordering src-main src-util

SUBDIRS = $(SRCDIR) include depgraph mkanthydic doc test cannadic
sysconf_DATA = anthy-conf

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = anthy.pc

EXPECT = expect
RUNTEST = runtest
RUNTESTFLAGS=

check-DEJAGNU: site.exp
	if [ -d testsuite ]; then \
	  true; \
	else \
	  mkdir testsuite; \
	fi
	rm -f testsuite/site.exp
	cp site.exp testsuite/site.exp
	rootme=`pwd`; export rootme; \
	runtest=$(RUNTEST); \
	srcdir=`cd ${srcdir}; pwd` ; export srcdir ; \
	cd testsuite; \
	if $(SHELL) -c "$$runtest --version" > /dev/null 2>&1; then \
	  $$runtest --tool $(DEJATOOL) --srcdir $${srcdir}/testsuite \
		$(RUNTESTFLAGS); \
	else echo "WARNING: could not find \`runtest'" 1>&2; :;\
	fi

MOSTLYCLEANFILES = core \
	testsuite/*.o testsuite/*.out testsuite/anthy.log testsuite/anthy.sum
