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

BUILT_SOURCES =				\
	goo-marshal.c			\
	goo-marshal.h

GTH_MARSHALLERS =			\
	gthumb-marshal.c		\
	gthumb-marshal.h

GTH_ENUMS =				\
	gthumb-enum-types.h		\
	gthumb-enum-types.c

GTH_HEADER_FILES =			\
	gth-image-list.h		

GTH_SOURCES = 				\
	$(GTH_HEADER_FILES)		\
	$(GTH_MARSHALLERS)		\
	$(GTH_ENUMS)			\
	gth-image-list.c		\
	gthumb-slide.c			\
	gthumb-slide.h

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
	$(GTH_SOURCES)				\
	$(gnome_media_cddbslave2_idl_sources)	\
	$(gnome_goobox_idl_sources)		\
	actions.c				\
	actions.h				\
	cddb-slave-client.c			\
	cddb-slave-client.h			\
	dlg-cover-chooser.c			\
	dlg-cover-chooser.h			\
	dlg-encoder-options.c			\
	dlg-encoder-options.h			\
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
	goo-cdrom-bsd.c				\
	goo-cdrom-bsd.h				\
	goo-cdrom-linux.c			\
	goo-cdrom-linux.h			\
	goo-cdrom-solaris.c			\
	goo-cdrom-solaris.h			\
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
	goo-volume-tool-button.c		\
	goo-volume-tool-button.h		\
	goo-window.c				\
	goo-window.h				\
	goo-stock.c				\
	goo-stock.h				\
	gtk-file-chooser-preview.c		\
	gtk-file-chooser-preview.h		\
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

gthumb-marshal.h: gthumb-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) gthumb-marshal.list --header --prefix=gthumb_marshal > $@

gthumb-marshal.c: gthumb-marshal.h gthumb-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"gthumb-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) gthumb-marshal.list --body --prefix=gthumb_marshal >> $@

gthumb-enum-types.h: $(GTH_HEADER_FILES) $(GLIB_MKENUMS)
	$(GLIB_MKENUMS) \
		--fhead "#ifndef GTHUMB_ENUM__TYPES_H\n#define GTHUMB_ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GTH_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* GTHUMB_ENUM_TYPES_H */" \
		$(GTH_HEADER_FILES) > xgen-geth \
	&& (cmp -s xgen-geth $(srcdir)/gthumb-enum-types.h || cp xgen-geth $(srcdir)/gthumb-enum-types.h ) \
	&& rm -f xgen-geth

gthumb-enum-types.c: $(GTH_HEADER_FILES) $(GLIB_MKENUMS) $(srcdir)/gthumb-enum-types.h
	$(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(GTH_HEADER_FILES) > xgen-getc \
	&& cp xgen-getc $(srcdir)/gthumb-enum-types.c  \
	&& rm -f xgen-getc

goobox_LDADD = 					\
	$(GOO_LIBS) 				\
	$(SYSTEM_LIBS)

EXTRA_DIST = 					\
	goo-marshal.list 			\
	gthumb-marshal.list 			\
	GNOME_Media_CDDBSlave2.idl		\
	GNOME_Goobox.idl

CLEANFILES = 					\
	$(BUILT_SOURCES) 			\
	$(GTH_MARSHALLERS) 			\
	$(gnome_media_cddbslave2_idl_sources) 	\
	$(gnome_goobox_idl_sources) 		\
	gnome_media_cddbslave2_idl_stamp	\
	gnome_goobox_idl_stamp
