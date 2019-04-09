project('goobox', 'c',
  license : 'GPL2+',
  version : '3.6.0',
  meson_version : '>=0.43'
)

glib_version = '>=2.40'
gtk_version = '>=3.22.0'
gstreamer_version = '>=1.0.0'
musicbrainz_version = '>=5.0.0'
libcoverart_version = '>=1.0.0'

gnome = import('gnome')
i18n = import('i18n')
gettext_package = meson.project_name()
prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

meson.add_install_script('postinstall.py')

# Optional dependencies

if get_option('disable-libcoverart')
  use_libcoverart = false
else
  libcoverart_dep = dependency('libcoverart', version : libcoverart_version, required : false)
  use_libcoverart = libcoverart_dep.found()
endif

# config.h

config_data = configuration_data()
config_data.set_quoted('GETTEXT_PACKAGE', gettext_package)
config_data.set_quoted('GOO_LOCALEDIR', join_paths(prefix, get_option('localedir')))
if get_option('buildtype').contains('debug')
  config_data.set('DEBUG', 1)
endif
if get_option('run-in-place')
  config_data.set_quoted('GOO_UIDIR', join_paths(meson.source_root(), 'src', 'ui'))
else
  config_data.set_quoted('GOO_UIDIR', join_paths(datadir, meson.project_name(), 'ui'))
endif
config_data.set_quoted('PACKAGE_NAME', meson.project_name())
config_data.set_quoted('PACKAGE_VERSION', meson.project_version())
if use_libcoverart
  config_data.set('HAVE_LIBCOVERART', 1)
endif
config_file = configure_file(output : 'config.h', configuration : config_data)
config_inc = include_directories('.')

# Subdirectories

subdir('data')
subdir('help')
subdir('po')
subdir('src')

# Summary

summary = [
  'configuration summary:',
  '',
  '           project: @0@ @1@'.format(meson.project_name(), meson.project_version()),
  '            prefix: @0@'.format(prefix),
  '       libcoverart: @0@'.format(use_libcoverart),
  ''
]
message('\n'.join(summary))
