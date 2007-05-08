SUBDIRS = glade icons

applicationsdir = $(datadir)/application-registry
applications_DATA = goobox.applications

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

bonobodir       = $(libdir)/bonobo
serverdir       = $(libdir)/bonobo/servers
server_in_files = GNOME_Goobox.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files).in Makefile
	sed -e "s|\@BONOBODIR\@|$(bonobodir)|" $< > $@

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = 			\
	goobox.schemas		\
	goobox.schemas.in	\
	$(applications_DATA)	\
	$(desktop_in_in_files)	\
	$(desktop_in_files)	\
	$(desktop_DATA)		\
	$(icon_DATA)		\
	$(server_in_files)


DISTCLEANFILES =                \
	$(schema_DATA)		\
	$(desktop_in_files)     \
	$(desktop_DATA)		\
	$(server_DATA)		\
	$(server_DATA).in
