SUBDIRS=icons
DIST_SUBDIRS=icons

gladedir = $(datadir)/goobox/glade

bin_PROGRAMS = goobox

if ENABLE_DEPRECATIONS
DISABLE_DEPRECATED = -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_VFS_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED -DBONOBO_DISABLE_DEPRECATED
else
DISABLE_DEPRECATED =
endif

INCLUDES =						\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-DGOO_PREFIX=\"$(prefix)\"           		\
        -DGOO_SYSCONFDIR=\"$(sysconfdir)\"   		\
        -DGOO_DATADIR=\"$(datadir)\"     	    	\
        -DGOO_LIBDIR=\"$(libdir)\"  	         	\
	-DPIXMAPSDIR=\""$(datadir)/pixmaps"\"		\
	-DGLADEDIR=\""$(gladedir)"\" 			\
	-DGOO_GLADEDIR=\""$(gladedir)"\"     		\
	$(DISABLE_DEPRECATED)				\
	$(GOO_CFLAGS)

gnome_media_cddbslave2_idl_sources =           	\
	GNOME_Media_CDDBSlave2-stubs.c         	\
	GNOME_Media_CDDBSlave2-skels.c         	\
	GNOME_Media_CDDBSlave2.h         	\
	GNOME_Media_CDDBSlave2-common.c

$(gnome_media_cddbslave2_idl_sources): gnome_media_cddbslave2_idl_stamp
gnome_media_cddbslave2_idl_stamp: GNOME_Media_CDDBSlave2.idl $(ORBIT_IDL)
	$(ORBIT_IDL) $(IDL_INCLUDES) $<
	touch $@

gnome_goobox_idl_sources =           	\
	GNOME_Goobox-stubs.c         	\
	GNOME_Goobox-skels.c         	\
	GNOME_Goobox.h         		\
	GNOME_Goobox-common.c

$(gnome_goobox_idl_sources): gnome_goobox_idl_stamp
gnome_goobox_idl_stamp: GNOME_Goobox.idl $(ORBIT_IDL)
	$(ORBIT_IDL) $(IDL_INCLUDES) $<
	touch $@

BUILT_SOURCES =			\
	goo-marshal.c		\
	goo-marshal.h

BACON_SOURCES = 			\
	cd-drive.c			\
	cd-drive.h			\
	bacon-cd-selection.c		\
	bacon-cd-selection.h		\
	dvd_plus_rw_utils.cpp		\
	transport.hxx

goobox_SOURCES = 				\
	$(BUILT_SOURCES)			\
	$(BACON_SOURCES)			\
	$(gnome_media_cddbslave2_idl_sources)	\
	$(gnome_goobox_idl_sources)		\
	actions.c				\
	actions.h				\
	cddb-slave-client.c			\
	cddb-slave-client.h			\
	dlg-extract.c				\
	dlg-extract.h				\
	dlg-preferences.c			\
	dlg-preferences.h			\
	dlg-ripper.c				\
	dlg-ripper.h				\
	eggtrayicon.c				\
	eggtrayicon.h				\
	file-list.c				\
	file-list.h				\
	file-utils.c				\
	file-utils.h				\
	gconf-utils.c				\
	gconf-utils.h				\
	glib-utils.c				\
	glib-utils.h				\
	gnome-vfs-helpers.c			\
	gnome-vfs-helpers.h			\
	goo-application.c			\
	goo-application.h			\
	goo-cdrom.c				\
	goo-cdrom.h				\
	goo-cdrom-linux.c			\
	goo-cdrom-linux.h			\
	goo-error.c				\
	goo-error.h				\
	goo-player.c				\
	goo-player.h				\
	goo-player-cd.c				\
	goo-player-cd.h				\
	goo-player-info.c			\
	goo-player-info.h			\
	goo-volume-button.c			\
	goo-volume-button.h			\
	goo-window.c				\
	goo-window.h				\
	goo-stock.c				\
	goo-stock.h				\
	gtk-utils.c				\
	gtk-utils.h				\
	main.c					\
	main.h					\
	preferences.c				\
	preferences.h				\
	song-info.c				\
	song-info.h				\
	track-info.c				\
	track-info.h				\
	typedefs.h				\
	ui.h					\
	utf8-fnmatch.c				\
	utf8-fnmatch.h

goo-marshal.h: goo-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=goo_marshal > $@

goo-marshal.c: goo-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"goo-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=goo_marshal >> $@

goobox_LDADD = $(GOO_LIBS)

EXTRA_DIST = 					\
	goo-marshal.list 			\
	GNOME_Media_CDDBSlave2.idl		\
	GNOME_Goobox.idl

CLEANFILES = 					\
	$(BUILT_SOURCES) 			\
	$(gnome_media_cddbslave2_idl_sources) 	\
	$(gnome_goobox_idl_sources) 		\
	gnome_media_cddbslave2_idl_stamp	\
	gnome_goobox_idl_stamp
