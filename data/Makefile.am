SUBDIRS = ui icons

desktop_in_in_files = goobox.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in) 
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

@INTLTOOL_DESKTOP_RULE@

schemadir = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = goobox.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$(schema_DATA)
endif

EXTRA_DIST = 			\
	goobox.schemas		\
	goobox.schemas.in	\
	$(desktop_in_in_files)	\
	$(desktop_in_files)	\
	$(desktop_DATA)		\
	$(icon_DATA)


DISTCLEANFILES =                \
	$(schema_DATA)		\
	$(desktop_in_files)     \
	$(desktop_DATA)

-include $(top_srcdir)/git.mk
