po_dir = join_paths(meson.source_root(), 'po')

gnome.compile_schemas()
install_data('org.gnome.Goobox.gschema.xml',
  install_dir : join_paths(datadir, 'glib-2.0', 'schemas')
)

desktop_data = configuration_data()
desktop_data.set('VERSION', meson.project_version())            
desktop_in_file = configure_file(
  input : 'org.gnome.Goobox.desktop.in.in',
  output : 'org.gnome.Goobox.desktop.in',
  configuration : desktop_data
)

i18n.merge_file(
  input : desktop_in_file,
  output : 'org.gnome.Goobox.desktop',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'applications')
)

subdir('appdata')
subdir('icons')
