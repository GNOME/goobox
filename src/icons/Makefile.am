IMAGES=					\
	eject-24.png			\
	extract-16.png			\
	extract-24.png			\
	next-24.png			\
	no-cover-48.png			\
	pause-24.png			\
	play-24.png			\
	prev-24.png			\
	stop-24.png			\
	volume-16.png			\
	volume-24.png			\
	web-16.png			\
	web-24.png

noinst_DATA = pixbufs.h
CLEANFILES = $(noinst_DATA)

pixbufs.h: $(IMAGES)
	gdk-pixbuf-csource --raw --build-list eject_24_rgba $(srcdir)/eject-24.png > $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list extract_16_rgba $(srcdir)/extract-16.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list extract_24_rgba $(srcdir)/extract-24.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list next_24_rgba $(srcdir)/next-24.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list no_cover_48_rgba $(srcdir)/no-cover-48.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list pause_24_rgba $(srcdir)/pause-24.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list play_24_rgba $(srcdir)/play-24.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list prev_24_rgba $(srcdir)/prev-24.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list stop_24_rgba $(srcdir)/stop-24.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list volume_16_rgba $(srcdir)/volume-16.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list volume_24_rgba $(srcdir)/volume-24.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list web_16_rgba $(srcdir)/web-16.png >> $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list web_24_rgba $(srcdir)/web-24.png >> $(srcdir)/pixbufs.h

EXTRA_DIST = $(IMAGES) pixbufs.h
