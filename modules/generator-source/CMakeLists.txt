set(generator_source_HEADERS
    "generator-source-parser.h"
    "${CMAKE_CURRENT_BINARY_DIR}/generator-source-grammar.h"
)

set(generator_source_SOURCES
    "generator-source-plugin.c"
    "generator-source-parser.c"
    "${CMAKE_CURRENT_BINARY_DIR}/generator-source-grammar.c"
)

generate_y_from_ym(modules/generator-source/generator-source-grammar)

bison_target(generator-sourceGrammar
    ${CMAKE_CURRENT_BINARY_DIR}/generator-source-grammar.y
    ${CMAKE_CURRENT_BINARY_DIR}/generator-source-grammar.c
COMPILE_FLAGS ${BISON_FLAGS})

option(ENABLE_generator_source "Enable generator-source ON")

if (ENABLE_generator_source)
  include_directories (${CMAKE_CURRENT_BINARY_DIR})
  include_directories (${CMAKE_CURRENT_SOURCE_DIR})
  add_library(generator_source MODULE ${generator-source_SOURCES})
  target_link_libraries(generator_source PRIVATE syslog-ng)

  install(TARGETS generator_source
  LIBRARY DESTINATION lib/syslog-ng/
  COMPONENT generator_source)
endif()
