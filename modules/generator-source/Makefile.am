module_LTLIBRARIES      += modules/generator-source/libgenerator-source.la
modules_generator_source_libgenerator_source_la_SOURCES = \
  modules/generator-source/generator-source-grammar.y       \
  modules/generator-source/generator-source-parser.c        \
  modules/generator-source/generator-source-parser.h        \
  modules/generator-source/generator-source-plugin.c

BUILT_SOURCES       +=      \
  modules/generator-source/generator-source-grammar.y       \
  modules/generator-source/generator-source-grammar.c       \
  modules/generator-source/generator-source-grammar.h

EXTRA_DIST        +=      \
  modules/generator-source/generator-source-grammar.ym

modules_generator_source_libgenerator_source_la_CPPFLAGS  =     \
  $(AM_CPPFLAGS)            \
  -I$(top_srcdir)/modules/generator-source        \
  -I$(top_builddir)/modules/generator-source
modules_generator_source_libgenerator_source_la_LIBADD  = $(MODULE_DEPS_LIBS)
modules_generator_source_libgenerator_source_la_LDFLAGS = $(MODULE_LDFLAGS)
modules_generator_source_libgenerator_source_la_DEPENDENCIES= $(MODULE_DEPS_LIBS)

modules/generator-source modules/generator-source/ mod-generator-source mod-file: modules/generator-source/libgenerator-source.la
.PHONY: modules/generator-source/ mod-generator-source mod-file

