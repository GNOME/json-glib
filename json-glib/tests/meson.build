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
  'stream-load.json',
]

installed_test_bindir = join_paths(json_libexecdir, 'installed-tests', json_api_name)
installed_test_datadir = join_paths(json_datadir, 'installed-tests', json_api_name)

install_data(test_data, install_dir: installed_test_bindir)

foreach t: tests
  installed_test = '@0@.test'.format(t)
  data = custom_target(installed_test,
                       output: installed_test,
                       command: [
                         python3,
                         gen_installed_test,
                         '--testdir=@0@'.format(installed_test_bindir),
                         '--testname=@0@'.format(t),
                         '--outdir=@OUTDIR@',
                         '--outfile=@0@'.format(installed_test),
                       ],
                       install: true,
                       install_dir: installed_test_datadir)

  exe = executable(t, '@0@.c'.format(t),
                   c_args: json_c_args,
                   install: true,
                   install_dir: installed_test_bindir,
                   dependencies: [ json_glib_dep, ])

  test(t, exe,
       args: [ '--tap', '-k' ],
       env: [
         'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
         'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
       ])
endforeach
