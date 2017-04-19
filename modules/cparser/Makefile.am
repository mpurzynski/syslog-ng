module_LTLIBRARIES      += modules/cparser/libcparser.la
modules_cparser_libcparser_la_SOURCES = \
  modules/cparser/cparser-grammar.y       \
  modules/cparser/cparser-parser.c        \
  modules/cparser/cparser-parser.h        \
  modules/cparser/cparser-plugin.c

BUILT_SOURCES       +=      \
  modules/cparser/cparser-grammar.y       \
  modules/cparser/cparser-grammar.c       \
  modules/cparser/cparser-grammar.h

EXTRA_DIST        +=      \
  modules/cparser/cparser-grammar.ym

modules_cparser_libcparser_la_CPPFLAGS  =     \
  $(AM_CPPFLAGS)            \
  -I$(top_srcdir)/modules/cparser        \
  -I$(top_builddir)/modules/cparser
modules_cparser_libcparser_la_LIBADD  = $(MODULE_DEPS_LIBS)
modules_cparser_libcparser_la_LDFLAGS = $(MODULE_LDFLAGS)
modules_cparser_libcparser_la_DEPENDENCIES= $(MODULE_DEPS_LIBS)

modules/cparser modules/cparser/ mod-cparser mod-file: modules/cparser/libcparser.la
.PHONY: modules/cparser/ mod-cparser mod-file

