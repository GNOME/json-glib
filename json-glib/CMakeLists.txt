set(JSON_GLIB_HEADERS
    json-builder.h
    json-generator.h
    json-gobject.h
    json-gvariant.h
    json-parser.h
    json-path.h
    json-reader.h
    json-types.h
    json-utils.h
    json-version-macros.h
)

set(JSON_GLIB_PUBLIC_HEADERS
    ${JSON_GLIB_HEADERS}
    json-enum-types.h
    json-glib.h
    json-version.h
)

set(JSON_GLIB_SRCS
    json-array.c
    json-builder.c
    json-debug.c
    json-gboxed.c
    json-generator.c
    json-gobject.c
    json-gvariant.c
    json-node.c
    json-object.c
    json-parser.c
    json-path.c
    json-reader.c
    json-scanner.c
    json-serializable.c
    json-utils.c
    json-value.c
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.h
  COMMAND ${GLIB2_MKENUMS}
  ARGS
    --template ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.h.in
    ${JSON_GLIB_HEADERS} > ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${JSON_GLIB_HEADERS}
          ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.h.in
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.c
  COMMAND ${GLIB2_MKENUMS}
  ARGS
    --template ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.c.in
    ${JSON_GLIB_HEADERS} > ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${JSON_GLIB_SRCS}
          ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.h
          ${CMAKE_CURRENT_SOURCE_DIR}/json-enum-types.c.in
)

add_definitions(-DG_LOG_DOMAIN="Json")
add_definitions(-DG_LOG_USE_STRUCTURED=1)
add_definitions(-DJSON_COMPILATION)
add_definitions(-DJSON_LOCALEDIR=${CMAKE_INSTALL_LOCALEDIR})

configure_file(json-version.h.in json-version.h @ONLY)
configure_file(config.h.in config.h)

set(JSON_GLIB_PC "${CMAKE_CURRENT_BINARY_DIR}/${JSON_API_NAME}.pc")
configure_file(json-glib.pc.in ${JSON_GLIB_PC} @ONLY)

add_library(json_glib SHARED ${JSON_GLIB_SRCS} json-enum-types.c)
set_target_properties(json_glib PROPERTIES
    OUTPUT_NAME ${JSON_API_NAME}
    VERSION ${JSON_VERSION}
    SOVERSION ${JSON_SOVERSION}
    FRAMEWORK TRUE
    PUBLIC_HEADER "${JSON_GLIB_PUBLIC_HEADERS}"
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)
target_link_libraries(json_glib ${GLIB2_LIBRARIES})
target_include_directories(json_glib PUBLIC include)

install(TARGETS json_glib
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${JSON_API_NAME}
)

install(FILES
    ${JSON_GLIB_PC}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
