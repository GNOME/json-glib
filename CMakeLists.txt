project(json-glib C)
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(PkgConfig REQUIRED)
find_program(GLIB2_MKENUMS glib-mkenums)

include(GNUInstallDirs)
include(CheckIncludeFile)
include(FindGLib)

include_directories(
  SYSTEM
  ${GLIB2_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_definitions(${GLIB2_CFLAGS_OTHER})
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
if (HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H=1)
endif()

set(_JSON_EXTERN "__attribute__((visibility(\"default\"))) extern")

set(JSON_MAJOR_VERSION 1)
set(JSON_MINOR_VERSION 5)
set(JSON_MICRO_VERSION 1)
set(JSON_VERSION "${JSON_MAJOR_VERSION}.${JSON_MINOR_VERSION}.${JSON_MICRO_VERSION}")

set(JSON_API_VERSION "1.0")
set(JSON_API_NAME "${CMAKE_PROJECT_NAME}-${JSON_API_VERSION}")

math(EXPR MINOR_VERSION_IS_EVEN "${JSON_MINOR_VERSION} % 2")
if(MINOR_VERSION_IS_EVEN EQUAL 0)
    set(JSON_INTERFACE_AGE ${JSON_MICRO_VERSION})
else(MINOR_VERSION_IS_EVEN EQUAL 0)
    set(JSON_INTERFACE_AGE 0)
endif()
math(EXPR JSON_BINARY_AGE "(100 * ${JSON_MINOR_VERSION}) + ${JSON_MICRO_VERSION}")

set(JSON_GETTEXT_DOMAIN "${JSON_API_NAME}")

set(JSON_SOVERSION_MAJOR 0)
math(EXPR JSON_SOVERSION_MINOR "${JSON_BINARY_AGE} - ${JSON_INTERFACE_AGE}")
set(JSON_SOVERSION "${JSON_SOVERSION_MAJOR}.${JSON_SOVERSION_MINOR}.${JSON_INTERFACE_AGE}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align -Wlogical-op -Wmissing-declarations -Wmissing-format-attribute -Wmissing-prototypes -Wmissing-noreturn -Wold-style-definition -Wpointer-arith -Wshadow -Wstrict-prototypes -Wunused -Wno-discarded-qualifiers -Wno-int-conversion -fno-strict-aliasing -Wno-uninitialized -Werror=address -Werror=array-bounds -Werror=empty-body -Werror=format=2 -Werror=implicit -Werror=init-self -Werror=int-to-pointer-cast -Werror=main -Werror=missing-braces -Werror=nested-externs -Werror=nonnull -Werror=pointer-to-int-cast -Werror=return-type -Werror=sequence-point -Werror=trigraphs -Werror=undef -Werror=write-strings")
set(CMAKE_SHARED_LINKER_FLAGS "${LDFLAGS} -Bsymbolic -z relro -z now")

add_subdirectory(json-glib)
