IMAGES=			\
	reset-16.png	\
	reset-24.png

BUILT_SOURCES = pixbufs.h
CLEANFILES = pixbufs.h

pixbufs.h: $(IMAGES)
	gdk-pixbuf-csource --raw --build-list reset_16_rgba $(srcdir)/reset-16.png > $@
	gdk-pixbuf-csource --raw --build-list reset_24_rgba $(srcdir)/reset-24.png >> $@

EXTRA_DIST = $(IMAGES) $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk
