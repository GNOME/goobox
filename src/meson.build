# Dependencies

c_comp = meson.get_compiler('c')
libm_dep = c_comp.find_library('m')
thread_dep = dependency('threads')
glib_dep = dependency('glib-2.0', version : glib_version)
gthread_dep = dependency('gthread-2.0')
gtk_dep = dependency('gtk+-3.0', version : gtk_version)
gstreamer_dep = dependency('gstreamer-1.0', version : gstreamer_version)
brasero_dep = dependency('libbrasero-media3')
musicbrainz_dep = dependency('libmusicbrainz5', version : musicbrainz_version)
libdiscid_dep = dependency('libdiscid')

# C args

c_args = [ '-fPIC', '-DPIC', '-Wl,--as-needed' ]
if get_option('buildtype').contains('debug')
  c_args += [ '-DDEBUG' ]
  test_args = [
    '-Wall',
    '-Wcast-align',
    '-Wtype-limits',
    '-Wclobbered',
    '-Wempty-body',
    '-Wignored-qualifiers',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wpointer-arith',
    '-Wno-sign-compare',
    '-Wformat-security'
  ]
else
  c_args += [ '-Wno-deprecated-declarations' ]
  test_args = [ '-Wall' ]
endif
c_args += c_comp.get_supported_arguments(test_args)

# Sources

source_files = files(
  'album-info.c',
  'dlg-cover-chooser.c',
  'dlg-extract.c',
  'dlg-preferences.c',
  'dlg-properties.c',
  'dlg-ripper.c',
  'gio-utils.c',
  'glib-utils.c',
  'gnome-desktop-thumbnail.c',
  'gnome-thumbnail-pixbuf-utils.c',
  'goo-application.c',
  'goo-application-actions-callbacks.c',
  'goo-error.c',
  'goo-player.c',
  'goo-player-info.c',
  'goo-player-progress.c',
  'goo-window.c',
  'goo-window-actions-callbacks.c',
  'gth-user-dir.c',
  'gtk-file-chooser-preview.c',
  'gtk-utils.c',
  'main.c',
  'metadata.c',
  'track-info.c'
)
gresource_files = gnome.compile_resources('goo-resources', 'goobox.gresource.xml', c_name : 'goo')
marshal_files = gnome.genmarshal('goo-marshal', prefix : 'goo_marshal', sources : 'goo-marshal.list' )

# Build targets

executable('goobox',
  sources : [
    config_file,
    source_files,
    gresource_files,
    marshal_files
  ],
  dependencies : [
    libm_dep,
    thread_dep,
    glib_dep,
    gthread_dep,
    gtk_dep,
    gstreamer_dep,
    brasero_dep,
    musicbrainz_dep,
    libdiscid_dep,
    use_libcoverart ? libcoverart_dep : []
  ],
  include_directories : config_inc,
  c_args : c_args,
  install : true
)
