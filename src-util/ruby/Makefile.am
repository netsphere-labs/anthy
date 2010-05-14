MAINTAINERCLEANFILES	= Makefile.in

SWIG_RB_FEATURE = anthy
SWIG_RB_OPTIONS = $(SWIG_OPTIONS) -ruby -feature $(SWIG_RB_FEATURE)

CLEANFILES = anthy_swig_rb_wrap.* $(SWIG_FEATURE).$(RUBY_DLEXT)

anthy_swig_rb_wrap.c: $(SWIG_INTERFACE)
	$(SWIG) $(SWIG_RB_OPTIONS) -o $@ $(SWIG_INTERFACE)

rubylibdir = $(RUBY_SITE_LIB_DIR)
rubylib_DATA = anthy.rb

rubyextdir = $(RUBY_SITE_ARCH_DIR)
rubyext_LTLIBRARIES = anthy.la

anthy_la_SOURCES = anthy_swig_rb_wrap.c
anthy_la_CFLAGS = $(RUBY_CFLAGS)
anthy_la_LDFLAGS = -avoid-version \
		   -rpath $(rubyextdir) \
		   -module \
		   -shrext .$(RUBY_DLEXT) \
		   $(RUBY_LIBS)
anthy_la_LIBADD = $(top_builddir)/src-main/libanthy.la \
		  $(top_builddir)/src-worddic/libanthydic.la

EXTRA_DIST = $(rubylib_DATA)
