project('goobox', 'c', 
  license : 'GPL2+',
  version : '3.4.2',
  meson_version : '>=0.43'
)

glib_version = '>=2.36'
gtk_version = '>=3.10.0'
gstreamer_version = '>=1.0.0'
musicbrainz_version = '>=5.0.0'
libcoverart_version = '>=1.0.0'
libnotify_version = '>=0.4.3'

gnome = import('gnome')
i18n = import('i18n')
gettext_package = meson.project_name()
prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

# Optional dependencies

if get_option('disable-libcoverart')
  use_libcoverart = false
else
  libcoverart_dep = dependency('libcoverart', version : libcoverart_version, required : false)
  use_libcoverart = libcoverart_dep.found()
endif

if get_option('disable-libnotify')
  use_libnotify = false  
else
  libnotify_dep = dependency('libnotify', version : libnotify_version, required : false)
  use_libnotify = libnotify_dep.found()
endif

# config.h

config_data = configuration_data()
config_data.set_quoted('GETTEXT_PACKAGE', gettext_package)
config_data.set_quoted('GOO_LOCALEDIR', join_paths(prefix, get_option('localedir')))
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
if use_libnotify
  config_data.set('ENABLE_NOTIFICATION', 1)
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
  '         libnotify: @0@'.format(use_libnotify),  
  ''
]
message('\n'.join(summary))