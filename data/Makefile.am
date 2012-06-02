SUBDIRS = icons

desktop_in_in_files = goobox.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in) 
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

@INTLTOOL_DESKTOP_RULE@

gsettingsschema_in_files = org.gnome.Goobox.gschema.xml.in
gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
.PRECIOUS: $(gsettings_SCHEMAS)

@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

convertdir = $(datadir)/GConf/gsettings
convert_DATA = goobox.convert

EXTRA_DIST = 				\
	$(convert_DATA)			\
	$(gsettingsschema_in_files)	\
	$(desktop_in_in_files)		\
	$(desktop_in_files)		\
	$(desktop_DATA)			\
	$(icon_DATA)


DISTCLEANFILES =			\
	$(desktop_in_files)     	\
	$(desktop_DATA)

CLEANFILES = 				\
	$(gsettings_SCHEMAS)		\
	$(NULL)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
