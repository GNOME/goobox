SUBDIRS = icons ui

bin_PROGRAMS = goobox

if RUN_IN_PLACE
uidir = $(abs_top_srcdir)/data/ui
else
uidir = $(datadir)/goobox/ui
endif

AM_CPPFLAGS =						\
	-DGOO_PREFIX=\"$(prefix)\"           		\
        -DGOO_SYSCONFDIR=\"$(sysconfdir)\"   		\
        -DGOO_DATADIR=\"$(datadir)\"     	    	\
        -DGOO_LIBDIR=\"$(libdir)\"  	         	\
	-DGOO_UIDIR=\""$(uidir)"\"     			\
	$(GOO_CFLAGS)					\
	$(SMCLIENT_CFLAGS)				\
	$(LIBCOVERART_CFLAGS)				\
	$(LIBNOTIFY_CFLAGS)

BUILT_SOURCES =			\
	goo-marshal.c		\
	goo-marshal.h		\
	goo-resources.c		\
	goo-resources.h		\
	$(NULL)
	
goobox_SOURCES = 				\
	$(BUILT_SOURCES)			\
	album-info.c				\
	album-info.h				\
	dlg-cover-chooser.c			\
	dlg-cover-chooser.h			\
	dlg-extract.c				\
	dlg-extract.h				\
	dlg-preferences.c			\
	dlg-preferences.h			\
	dlg-properties.c			\
	dlg-properties.h			\
	dlg-ripper.c				\
	dlg-ripper.h				\
	gio-utils.c				\
	gio-utils.h				\
	glib-utils.c				\
	glib-utils.h				\
	gnome-desktop-thumbnail.c		\
	gnome-desktop-thumbnail.h		\
	gnome-thumbnail-pixbuf-utils.c		\
	goo-application.c			\
	goo-application.h			\
	goo-application-actions-callbacks.c	\
	goo-application-actions-callbacks.h	\
	goo-application-actions-entries.h	\
	goo-error.c				\
	goo-error.h				\
	goo-player.c				\
	goo-player.h				\
	goo-player-bar.c			\
	goo-player-bar.h			\
	goo-player-info.c			\
	goo-player-info.h			\
	goo-window.c				\
	goo-window.h				\
	goo-window-actions-callbacks.c		\
	goo-window-actions-callbacks.h		\
	goo-window-actions-entries.h		\
	gth-user-dir.c				\
	gth-user-dir.h				\
	gtk-file-chooser-preview.c		\
	gtk-file-chooser-preview.h		\
	gtk-utils.c				\
	gtk-utils.h				\
	main.c					\
	main.h					\
	metadata.c				\
	metadata.h				\
	preferences.h				\
	track-info.c				\
	track-info.h				\
	typedefs.h					

goo-marshal.h: goo-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=goo_marshal > $@

goo-marshal.c: goo-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"goo-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=goo_marshal >> $@

RESOURCES_DEP = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir=$(srcdir) $(srcdir)/goobox.gresource.xml)

goo-resources.c: goobox.gresource.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name goo $(srcdir)/goobox.gresource.xml

goo-resources.h: goobox.gresource.xml $(RESOURCES_DEP)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name goo $(srcdir)/goobox.gresource.xml

goobox_LDADD = 						\
	$(GOO_LIBS) 					\
	$(M_LIBS)					\
	$(SMCLIENT_LIBS)				\
	$(LIBCOVERART_LIBS)				\
	$(LIBNOTIFY_LIBS)				\
	$(SYSTEM_LIBS)

EXTRA_DIST = 				\
	goobox.gresource.xml		\
	goo-marshal.list 

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
