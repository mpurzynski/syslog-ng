set(cparser_HEADERS
    "cparser-parser.h"
    "${CMAKE_CURRENT_BINARY_DIR}/cparser-grammar.h"
)

set(cparser_SOURCES
    "cparser-plugin.c"
    "cparser-parser.c"
    "${CMAKE_CURRENT_BINARY_DIR}/cparser-grammar.c"
)

generate_y_from_ym(modules/cparser/cparser-grammar)

bison_target(cparserGrammar
    ${CMAKE_CURRENT_BINARY_DIR}/cparser-grammar.y
    ${CMAKE_CURRENT_BINARY_DIR}/cparser-grammar.c
COMPILE_FLAGS ${BISON_FLAGS})

option(ENABLE_cparser "Enable cparser ON")

if (ENABLE_cparser)
  include_directories (${CMAKE_CURRENT_BINARY_DIR})
  include_directories (${CMAKE_CURRENT_SOURCE_DIR})
  add_library(cparser MODULE ${cparser_SOURCES})
  target_link_libraries(cparser PRIVATE syslog-ng)

  install(TARGETS cparser
  LIBRARY DESTINATION lib/syslog-ng/
  COMPONENT cparser)
endif()
