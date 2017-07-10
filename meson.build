project('json-glib', 'c', version: '1.3.1',
        license: 'LGPLv2.1+',
        default_options: [
          'warning_level=1',
          'buildtype=debugoptimized',
          'c_std=c99',
        ],
        meson_version: '>= 0.40.1')

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
libversion = '@0@.@1@.@2@'.format(soversion, json_binary_age - json_interface_age, json_interface_age)

# Paths
json_includedir = join_paths(get_option('prefix'), get_option('includedir'))
json_datadir = join_paths(get_option('prefix'), get_option('datadir'))
json_mandir = join_paths(get_option('prefix'), get_option('mandir'))
json_localedir = join_paths(get_option('prefix'), get_option('localedir'))
json_libexecdir = join_paths(get_option('prefix'), get_option('libexecdir'))

# Dependencies
glib_req_version = '>= 2.44.0'
gobject_dep = dependency('gobject-2.0', version: glib_req_version)
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
    '-we4819', # the file contains a character that cannot be represented in the current code page
  ]
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_cflags = [
    '-Wpointer-arith',
    '-Wmissing-declarations',
    '-Wformat=2',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wold-style-definition',
    '-Wdeclaration-after-statement',
    '-Wunused',
    '-Wno-uninitialized',
    '-Wshadow',
    '-Wcast-align',
    '-Wmissing-noreturn',
    '-Wmissing-format-attribute',
    '-Wlogical-op',
    '-Wno-discarded-qualifiers',
    '-Werror=implicit',
    '-Werror=nonnull',
    '-Werror=init-self',
    '-Werror=main',
    '-Werror=missing-braces',
    '-Werror=sequence-point',
    '-Werror=return-type',
    '-Werror=trigraphs',
    '-Werror=array-bounds',
    '-Werror=write-strings',
    '-Werror=address',
    '-Werror=int-to-pointer-cast',
    '-Werror=pointer-to-int-cast',
    '-Werror=empty-body',
    '-fno-strict-aliasing',
    '-Wno-int-conversion',
  ]
else
  test_cflags = []
endif

if get_option('default_library') != 'static'
  if host_system == 'windows'
    cdata.set('DLL_EXPORT', true)
    cdata.set('_JSON_EXTERN', '__declspec(dllexport) extern')
    if cc.get_id() != 'msvc'
      test_cflags += '-fvisibility=hidden'
    endif
  else
    cdata.set('_JSON_EXTERN', '__attribute__((visibility("default"))) extern')
    test_cflags += '-fvisibility=hidden'
  endif
endif

common_cflags = []
foreach cflag: test_cflags
  if cc.has_argument(cflag)
    common_cflags += cflag
  endif
endforeach

common_ldflags = []

if host_system == 'linux'
  foreach ldflag: [ '-Wl,-Bsymbolic', '-Wl,-z,relro', '-Wl,-z,now', ]
    if cc.has_argument(ldflag)
      common_ldflags += ldflag
    endif
  endforeach
endif

# Maintain compatibility with autotools
if host_system == 'darwin'
  common_ldflags += [ '-compatibility_version 1', '-current_version 1.0', ]
endif

root_dir = include_directories('.')

gnome = import('gnome')
gir = find_program('g-ir-scanner', required: false)
build_gir = gir.found() and not meson.is_cross_build() and not get_option('disable_introspection')

python3 = import('python3').find_python()
gen_installed_test = files('build-aux/gen-installed-test.py')

subdir('json-glib')

subdir('po')

if get_option('enable-gtk-doc')
  subdir('doc')
endif
