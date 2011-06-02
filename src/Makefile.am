SUBDIRS=icons

bin_PROGRAMS = goobox

if RUN_IN_PLACE
uidir = $(abs_top_srcdir)/data/ui
else
uidir = $(datadir)/goobox/ui
endif

INCLUDES =						\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-I$(top_srcdir)/copy-n-paste/ 			\
	-DGOO_PREFIX=\"$(prefix)\"           		\
        -DGOO_SYSCONFDIR=\"$(sysconfdir)\"   		\
        -DGOO_DATADIR=\"$(datadir)\"     	    	\
        -DGOO_LIBDIR=\"$(libdir)\"  	         	\
	-DGOO_UIDIR=\""$(uidir)"\"     			\
	$(DISABLE_DEPRECATED)				\
	$(DBUS_CFLAGS)					\
	$(GOO_CFLAGS)					\
	$(LIBNOTIFY_CFLAGS)

BUILT_SOURCES =				\
	goo-marshal.c			\
	goo-marshal.h

goobox_SOURCES = 				\
	$(BUILT_SOURCES)			\
	actions.c				\
	actions.h				\
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
	goo-error.c				\
	goo-error.h				\
	goo-player.c				\
	goo-player.h				\
	goo-player-info.c			\
	goo-player-info.h			\
	goo-stock.c				\
	goo-stock.h				\
	goo-volume-tool-button.c		\
	goo-volume-tool-button.h		\
	goo-window.c				\
	goo-window.h				\
	gth-user-dir.c				\
	gth-user-dir.h				\
	gth-window.c				\
	gth-window.h				\
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
	typedefs.h				\
	ui.h					

goo-marshal.h: goo-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=goo_marshal > $@

goo-marshal.c: goo-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"goo-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=goo_marshal >> $@

goobox_LDADD = 				\
	$(top_builddir)/copy-n-paste/libeggsmclient.la  \
	$(DBUS_LIBS)			\
	$(GOO_LIBS) 			\
	$(LIBNOTIFY_LIBS)		\
	$(SYSTEM_LIBS)

EXTRA_DIST = 				\
	goo-marshal.list 

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
