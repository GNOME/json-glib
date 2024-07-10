# SPDX-FileCopyrightText: 2016 Thibault Saunier
# SPDX-FileCopyrightText: 2017 Emmanuele Bassi
# SPDX-FileCopyrightText: 2020 Endless
# SPDX-FileCopyrightText: 2020 Jan-Michael Brummer
#
# SPDX-License-Identifier: LGPL-2.1-or-later

tests = [
  'array',
  'boxed',
  'builder',
  'generator',
  'gvariant',
  'invalid',
  'node',
  'object',
  'parser',
  'path',
  'reader',
  'serialize-simple',
  'serialize-complex',
  'serialize-full',
]

test_data = [
  'invalid.json',
  'skip-bom.json',
  'stream-load.json',
]

installed_test_bindir = json_libexecdir / 'installed-tests' / json_api_name
installed_test_datadir = json_datadir / 'installed-tests' / json_api_name

if get_option('installed_tests')
  install_data(test_data, install_dir: installed_test_bindir)
endif

foreach t: tests
  installed_test = '@0@.test'.format(t)
  data = custom_target(
    installed_test,
    output: installed_test,
    command: [
      python3,
      gen_installed_test,
      '--testdir=@0@'.format(installed_test_bindir),
      '--testname=@0@'.format(t),
      '--outdir=@OUTDIR@',
      '--outfile=@0@'.format(installed_test),
    ],
    install: get_option('installed_tests'),
    install_dir: installed_test_datadir,
  )

  exe = executable(
    t, '@0@.c'.format(t),
    c_args: json_c_args,
    install: get_option('installed_tests'),
    install_dir: installed_test_bindir,
    dependencies: [ json_glib_dep, ],
  )

  test(
    t, exe,
    args: [ '--tap', '-k' ],
    env: [
      'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
      'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
    ],
  )
endforeach

if get_option('conformance')
  subdir('conformance')
endif
