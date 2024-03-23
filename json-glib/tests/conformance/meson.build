# SPDX-FileCopyrightText: 2024 Emmanuele Bassi
#
# SPDX-License-Identifier: LGPL-2.1-or-later

implementation_tests = meson.current_source_dir() / 'impl.tests'
valid_tests = meson.current_source_dir() / 'valid.tests'
invalid_tests = meson.current_source_dir() / 'invalid.tests'

conformance = executable('conformance', 'conformance.c', dependencies: json_glib_dep)

test('conformance-implementation',
  conformance,
  args: [
    '--tap',
    '-k',
    '--prefix=i_',
    '--type=impl',
    '--input=@0@'.format(implementation_tests),
  ],
  protocol: 'tap',
  env: [
    'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
    'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  ],
)

test('conformance-valid',
  conformance,
  args: [
    '--tap',
    '-k',
    '--prefix=y_',
    '--type=valid',
    '--input=@0@'.format(valid_tests),
  ],
  protocol: 'tap',
  env: [
    'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
    'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  ],
)

test('conformance-invalid',
  conformance,
  args: [
    '--tap',
    '-k',
    '--prefix=n_',
    '--type=valid',
    '--input=@0@'.format(invalid_tests),
  ],
  protocol: 'tap',
  env: [
    'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
    'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  ],
)
