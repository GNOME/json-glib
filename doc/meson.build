# SPDX-FileCopyrightText: 2017 Emmanuele Bassi
#
# SPDX-License-Identifier: LGPL-2.1-or-later
rst_files = [
  {
    'name': 'json-glib-format',
    'section': '1',
  },
  {
    'name': 'json-glib-validate',
    'section': '1',
  },
]

rst2man_flags = [
  '--syntax-highlight=none',
]

rst2man = find_program('rst2man', 'rst2man.py', required: get_option('man'))
rst2html5 = find_program('rst2html5', 'rst2html5.py', required: get_option('documentation'))

gidocgen = find_program('gi-docgen', required: get_option('documentation'), native: true)
gidocgen_flags = [
  '--quiet',
  '--no-namespace-dir',
]

if get_option('werror')
  gidocgen_flags += ['--fatal-warnings']
endif

if get_option('man') and rst2man.found()
  foreach rst: rst_files
    name = rst.get('name')
    section = rst.get('section', '1')
    custom_target('man-@0@'.format(name),
      input: '@0@.rst'.format(name),
      output: '@0@.@1@'.format(name, section),
      command: [
        rst2man,
        rst2man_flags,
        '@INPUT@',
      ],
      capture: true,
      install: true,
      install_dir: get_option('mandir') / 'man@0@'.format(section),
    )
  endforeach
endif

if build_doc and gidocgen.found()
  json_docdir = json_datadir / 'doc'

  toml_conf = configuration_data()
  toml_conf.set('JSON_VERSION', meson.project_version())

  json_toml = configure_file(
    input: 'json-glib.toml.in',
    output: 'json-glib.toml',
    configuration: toml_conf,
  )

  json_content_files = [
    'json-gboxed.md',
    'json-gobject.md',
    'json-gvariant.md',
  ]

  custom_target('json-glib-doc',
    input: json_glib_gir[0],
    output: 'json-glib-1.0',
    command: [
      gidocgen,
      'generate',
      gidocgen_flags,
      '--config', json_toml,
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../json-glib'),
      '--output-dir=@OUTPUT@',
      '@INPUT@',
    ],
    depend_files: [ json_toml, json_content_files, 'urlmap.js' ],
    build_by_default: true,
    install: true,
    install_dir: json_docdir,
    install_tag: 'doc'
  )

  if get_option('tests')
    test('doc-check',
      gidocgen,
      args: [
        'check',
        '--config', json_toml,
        '--add-include-path=@0@'.format(meson.current_build_dir() / '../json-glib'),
        json_glib_gir[0],
      ],
      depends: json_glib_gir[0],
      suite: ['docs'],
    )
  endif

  foreach rst: rst_files
    name = rst.get('name')
    custom_target('html-@0@'.format(name),
      input: '@0@.rst'.format(name),
      output: '@0@.html'.format(name),
      command: [
        rst2html5,
        '@INPUT@',
      ],
      capture: true,
      install: true,
      install_dir: json_docdir / 'json-glib-1.0',
      install_tag: 'doc',
    )
  endforeach
endif
