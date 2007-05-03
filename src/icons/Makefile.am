IMAGES=					\
	reset-16.png			\
	reset-24.png

noinst_DATA = pixbufs.h
CLEANFILES = $(noinst_DATA)

pixbufs.h: $(IMAGES)
	gdk-pixbuf-csource --raw --build-list reset_16_rgba $(srcdir)/reset-16.png > $(srcdir)/pixbufs.h
	gdk-pixbuf-csource --raw --build-list reset_24_rgba $(srcdir)/reset-24.png >> $(srcdir)/pixbufs.h

EXTRA_DIST = $(IMAGES) pixbufs.h
