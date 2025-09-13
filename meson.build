# SPDX-FileCopyrightText: 2016 Samsung
# SPDX-FileCopyrightText: 2017 Emmanuele Bassi
# SPDX-FileCopyrightText: 2018 Collabora Ltd
# SPDX-FileCopyrightText: 2020 Collabora Ltd
#
# SPDX-License-Identifier: LGPL-2.1-or-later

project('json-glib', 'c',
  version: '1.10.9',
  license: 'LGPL-2.1-or-later',
  default_options: [
    'warning_level=2',
    'buildtype=debugoptimized',
    'c_std=gnu99',
  ],
  meson_version: '>= 1.2.0',
)

# Versioning
json_version = meson.project_version()
version_arr = json_version.split('.')
json_version_major = version_arr[0].to_int()
json_version_minor = version_arr[1].to_int()
json_version_micro = version_arr[2].to_int()

json_interface_age = json_version_minor.is_even() ? json_version_micro : 0
json_binary_age = 100 * json_version_minor + json_version_micro
json_api_version = '1.0'

json_api_name = '@0@-@1@'.format(meson.project_name(), json_api_version)
json_gettext_domain = json_api_name

# Maintain compatibility with the old soname versioning
soversion = 0
current = json_binary_age - json_interface_age
revision = json_interface_age
libversion = '@0@.@1@.@2@'.format(soversion, json_binary_age - json_interface_age, json_interface_age)
darwin_versions = [ current + 1, '@0@.@1@'.format(current + 1, revision) ]

# Paths
json_includedir = get_option('prefix') / get_option('includedir')
json_datadir = get_option('prefix') / get_option('datadir')
json_mandir = get_option('prefix') / get_option('mandir')
json_localedir = get_option('prefix') / get_option('localedir')
json_libexecdir = get_option('prefix') / get_option('libexecdir')

# Dependencies
glib_req_version = '>= 2.72.0'
gio_dep = dependency('gio-2.0', version: glib_req_version)

# Configurations
cc = meson.get_compiler('c')
host_system = host_machine.system()

cdata = configuration_data()
check_headers = [
  ['unistd.h', 'HAVE_UNISTD_H'],
]

foreach h: check_headers
  if cc.has_header(h.get(0))
    cdata.set(h.get(1), 1)
  endif
endforeach

cdata.set_quoted('GETTEXT_PACKAGE', json_gettext_domain)

if get_option('default_library') == 'static'
  cdata.set('JSON_STATIC_BUILD', true)
endif

if get_option('debug')
  cdata.set('JSON_ENABLE_DEBUG', true)
endif

if cc.get_id() == 'msvc'
  # Compiler options taken from msvc_recommended_pragmas.h
  # in GLib, based on _Win32_Programming_ by Rector and Newcomer
  test_cflags = [
    '-we4002', # too many actual parameters for macro
    '-we4003', # not enough actual parameters for macro
    '-w14010', # single-line comment contains line-continuation character
    '-we4013', # 'function' undefined; assuming extern returning int
    '-w14016', # no function return type; using int as default
    '-we4020', # too many actual parameters
    '-we4021', # too few actual parameters
    '-we4027', # function declared without formal parameter list
    '-we4029', # declared formal parameter list different from definition
    '-we4033', # 'function' must return a value
    '-we4035', # 'function' : no return value
    '-we4045', # array bounds overflow
    '-we4047', # different levels of indirection
    '-we4049', # terminating line number emission
    '-we4053', # an expression of type void was used as an operand
    '-we4071', # no function prototype given
    '/utf-8',  # Set the input and exec encoding to utf-8
  ]
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_cflags = [
    '-Wcast-align',
    '-Wlogical-op',
    '-Wmissing-declarations',
    '-Wmissing-format-attribute',
    '-Wmissing-prototypes',
    '-Wmissing-noreturn',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wunused',
    '-Wno-discarded-qualifiers',
    '-Wno-int-conversion',
    '-fno-strict-aliasing',
    '-Wno-uninitialized',
    '-Werror=address',
    '-Werror=array-bounds',
    '-Werror=empty-body',
    '-Werror=format=2',
    '-Werror=implicit',
    '-Werror=init-self',
    '-Werror=int-to-pointer-cast',
    '-Werror=main',
    '-Werror=missing-braces',
    '-Werror=nested-externs',
    '-Werror=nonnull',
    '-Werror=pointer-to-int-cast',
    '-Werror=return-type',
    '-Werror=sequence-point',
    '-Werror=trigraphs',
    '-Werror=undef',
    '-Werror=write-strings',
  ]
else
  test_cflags = []
endif

common_cflags = cc.get_supported_arguments(test_cflags)

common_ldflags = cc.get_supported_link_arguments([
  '-Wl,-Bsymbolic',
  '-Wl,-z,relro',
  '-Wl,-z,now',
  '-Wl,--default-symver',
])

root_dir = include_directories('.')

gnome = import('gnome')

gir = find_program('g-ir-scanner', required: get_option('introspection'))
build_gir = gir.found() and get_option('introspection').allowed() and meson.can_run_host_binaries()
build_doc = build_gir and get_option('documentation').allowed()

if get_option('introspection').disabled() and get_option('documentation').enabled()
  error('Cannot build documentation without introspection')
endif

python3 = import('python').find_installation()
gen_installed_test = files('build-aux/gen-installed-test.py')

subdir('json-glib')

# xgettext is optional (on Windows for instance)
if find_program('xgettext', required: get_option('nls')).found()
  subdir('po')
endif

subdir('doc')

if not meson.is_subproject()
  meson.add_dist_script('build-aux/dist-docs.py')
endif

summary({
    'prefix': get_option('prefix'),
    'includedir': json_includedir,
    'libdir': get_option('prefix') / get_option('libdir'),
    'datadir': json_datadir,
  },
  section: 'Directories',
)

summary({
    'Localization': get_option('nls'),
    'Introspection': build_gir,
    'Documentation': build_doc,
    'Manual pages': get_option('man'),
    'Tests': get_option('tests'),
    'Installed tests': get_option('installed_tests'),
    'Conformance tests': get_option('conformance'),
  },
  section: 'Build',
  bool_yn: true,
)
