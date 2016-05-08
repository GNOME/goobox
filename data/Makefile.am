SUBDIRS = appdata icons

desktop_in_in_files = goobox.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in) 
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

@INTLTOOL_DESKTOP_RULE@

gsettings_SCHEMAS = org.gnome.Goobox.gschema.xml

@GSETTINGS_RULES@

convertdir = $(datadir)/GConf/gsettings
convert_DATA = goobox.convert

EXTRA_DIST = 				\
	$(convert_DATA)			\
	$(gsettings_SCHEMAS)		\
	$(desktop_in_in_files)		\
	$(desktop_in_files)		\
	$(desktop_DATA)			\
	$(icon_DATA)


DISTCLEANFILES =			\
	$(desktop_in_files)     	\
	$(desktop_DATA)

-include $(top_srcdir)/git.mk
