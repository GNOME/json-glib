# SPDX-FileCopyrightText: 2016 Thibault Saunier
# SPDX-FileCopyrightText: 2017 Emmanuele Bassi
# SPDX-FileCopyrightText: 2018 Collabora Ltd
# SPDX-FileCopyrightText: 2020 Collabora Ltd
#
# SPDX-License-Identifier: LGPL-2.1-or-later

install_header_subdir = json_api_name / 'json-glib'
install_header_dir = json_includedir / install_header_subdir

configure_file(output: 'config.h', configuration: cdata)

source_h = [
  'json-builder.h',
  'json-generator.h',
  'json-gobject.h',
  'json-gvariant.h',
  'json-parser.h',
  'json-path.h',
  'json-reader.h',
  'json-types.h',
  'json-utils.h',
  'json-version-macros.h'
]

json_glib_enums = gnome.mkenums_simple(
  'json-enum-types',
  sources: source_h,
  header_prefix: '#include <json-glib/json-version-macros.h>',
  body_prefix: '#include "config.h"',
  decorator: 'JSON_AVAILABLE_IN_1_0',
  install_header: true,
  install_dir: install_header_dir,
)

# Keep a reference to the generated header, for internal dependencies
json_enum_types_h = json_glib_enums.get(1)

source_c = [
  'json-array.c',
  'json-builder.c',
  'json-debug.c',
  'json-gboxed.c',
  'json-generator.c',
  'json-gobject.c',
  'json-gvariant.c',
  'json-node.c',
  'json-object.c',
  'json-parser.c',
  'json-path.c',
  'json-reader.c',
  'json-scanner.c',
  'json-serializable.c',
  'json-utils.c',
  'json-value.c',
]

version_data = configuration_data()
version_data.set('JSON_MAJOR_VERSION', json_version_major)
version_data.set('JSON_MINOR_VERSION', json_version_minor)
version_data.set('JSON_MICRO_VERSION', json_version_micro)
version_data.set('JSON_VERSION', meson.project_version())

json_version_h = configure_file(
  input: 'json-version.h.in',
  output: 'json-version.h',
  configuration: version_data,
  install_dir: install_header_dir,
)

install_headers(source_h + [ 'json-glib.h', ], subdir: install_header_subdir)

json_c_args = [
  '-DG_LOG_DOMAIN="Json"',
  '-DG_LOG_USE_STRUCTURED=1',
  '-DJSON_COMPILATION',
  '-DJSON_LOCALEDIR="@0@"'.format(json_localedir)
]

json_lib = library(
  json_api_name,
  source_c + json_glib_enums,
  version: libversion,
  soversion: soversion,
  include_directories: root_dir,
  dependencies: [gio_dep],
  c_args: json_c_args + common_cflags,
  link_args: common_ldflags,
  gnu_symbol_visibility: 'hidden',
  darwin_versions: darwin_versions,
  install: true,
)

pkgg = import('pkgconfig')
pkgg.generate(
  libraries: json_lib,
  subdirs: json_api_name,
  version: json_version,
  name: 'JSON-GLib',
  filebase: json_api_name,
  description: 'JSON Parser for GLib.',
  requires: 'gio-2.0',
)

if gir.found()
  gir_args = [
    '--quiet',
    '--warn-all',
    '-DJSON_COMPILATION',
  ]

  json_glib_gir = gnome.generate_gir(
    json_lib,
    sources: source_c + source_h + json_glib_enums + [ json_version_h ],
    namespace: 'Json',
    nsversion: json_api_version,
    identifier_prefix: 'Json',
    symbol_prefix: 'json',
    export_packages: json_api_name,
    includes: [ 'GObject-2.0', 'Gio-2.0', ],
    header: 'json-glib/json-glib.h',
    install: true,
    extra_args: gir_args,
    fatal_warnings: get_option('werror'),
  )
else
  json_glib_gir = []
endif

json_glib_dep = declare_dependency(
  link_with: json_lib,
  include_directories: root_dir,
  dependencies: [gio_dep],
  sources: [ json_enum_types_h, json_glib_gir ],
)

tools = [
  [ 'json-glib-validate', [ 'json-glib-validate.c', ] ],
  [ 'json-glib-format', [ 'json-glib-format.c', ] ],
]

foreach t: tools
  bin_name = t[0]
  bin_sources = t[1]

  executable(bin_name, bin_sources,
             c_args: json_c_args,
             dependencies: json_glib_dep,
             install: true)
endforeach

if get_option('tests')
  subdir('tests')
endif
