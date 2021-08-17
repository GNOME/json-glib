if get_option('gtk_doc').enabled() and get_option('introspection').enabled()
  dependency('gi-docgen', version: '>= 2021.6',
    fallback: ['gi-docgen', 'dummy_dep'],
  )

  gidocgen = find_program('gi-docgen', required: get_option('gtk_doc'))

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
      '--quiet',
      '--fatal-warnings',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../json-glib'),
      '--config', json_toml,
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT@',
    ],
    depend_files: [ json_toml, json_content_files ],
    build_by_default: true,
    install: true,
    install_dir: json_docdir,
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

endif

xsltproc = find_program('xsltproc', required: false)
if get_option('man') and xsltproc.found()
  xlstproc_flags = [
    '--nonet',
    '--stringparam', 'man.output.quietly', '1',
    '--stringparam', 'funcsynopsis.style', 'ansi',
    '--stringparam', 'man.th.extra1.suppress', '1',
    '--stringparam', 'man.authors.section.enabled', '1',
    '--stringparam', 'man.copyright.section.enabled', '1',
  ]

  man_files = [
    'json-glib-format',
    'json-glib-validate',
  ]

  foreach m: man_files
    custom_target(
      m + ' man page',
      input: '@0@.xml'.format(m),
      output: '@0@.1'.format(m),
      command: [
        xsltproc,
        xlstproc_flags,
        '-o', '@OUTPUT@',
        'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl',
        '@INPUT@',
      ],
      install: true,
      install_dir: join_paths(json_mandir, 'man1'),
    )
  endforeach
endif
