/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GOO
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include "goo-stock.h"
#include "icons/pixbufs.h"


static struct {
	const char    *stock_id;
	gconstpointer  default_pixbuf;
	gconstpointer  menu_pixbuf;
} items[] = {
	{ GOO_STOCK_EJECT,        eject_24_rgba,        NULL },
	{ GOO_STOCK_EXTRACT,      extract_24_rgba,      extract_16_rgba },
	{ GOO_STOCK_NEXT,         next_24_rgba,         NULL },
	{ GOO_STOCK_PAUSE,        pause_24_rgba,        NULL },
	{ GOO_STOCK_PLAY,         play_24_rgba,         NULL },
	{ GOO_STOCK_PREV,         prev_24_rgba,         NULL },
	{ GOO_STOCK_STOP,         stop_24_rgba,         NULL },
	{ GOO_STOCK_NO_COVER,     no_cover_48_rgba,     NULL },
	{ GOO_STOCK_VOLUME_MAX,   volume_max_24_rgba,   volume_16_rgba },
	{ GOO_STOCK_VOLUME_MED,   volume_med_24_rgba,   volume_16_rgba },
	{ GOO_STOCK_VOLUME_MIN,   volume_min_24_rgba,   volume_16_rgba },
	{ GOO_STOCK_VOLUME_ZERO,  volume_zero_24_rgba,  volume_16_rgba },
	{ GOO_STOCK_WEB,          web_24_rgba,          web_16_rgba },
};


static const GtkStockItem stock_items [] = {
	{ GOO_STOCK_EXTRACT, N_("_Extract"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_VOLUME_MIN, N_("V_olume"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_VOLUME_MED, N_("V_olume"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_VOLUME_MAX, N_("V_olume"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_VOLUME_ZERO, N_("V_olume"), 0, 0, GETTEXT_PACKAGE }
};


static gboolean stock_initialized = FALSE;


void
goo_stock_init (void)
{
	GtkIconFactory *factory;
	int             i;

	if (stock_initialized)
		return;
	stock_initialized = TRUE;

	gtk_stock_add_static (stock_items, G_N_ELEMENTS (stock_items));

	factory = gtk_icon_factory_new ();
	for (i = 0; i < G_N_ELEMENTS (items); i++) {
		GtkIconSet    *set;
		GtkIconSource *source;
		GdkPixbuf     *pixbuf;

		set = gtk_icon_set_new ();

		source = gtk_icon_source_new ();

		if (items[i].menu_pixbuf != NULL) {
			pixbuf = gdk_pixbuf_new_from_inline (-1, 
							     items[i].menu_pixbuf, 
							     FALSE, 
							     NULL);
			gtk_icon_source_set_pixbuf (source, pixbuf);

			gtk_icon_source_set_size_wildcarded (source, FALSE);
			gtk_icon_source_set_size (source, GTK_ICON_SIZE_MENU);
			gtk_icon_set_add_source (set, source);

			g_object_unref (pixbuf);
		}

		pixbuf = gdk_pixbuf_new_from_inline (-1, 
						     items[i].default_pixbuf, 
						     FALSE, 
						     NULL);
		
		gtk_icon_source_set_pixbuf (source, pixbuf);

		gtk_icon_source_set_size_wildcarded (source, FALSE);
		gtk_icon_source_set_state_wildcarded (source, TRUE);
		gtk_icon_source_set_direction_wildcarded (source, TRUE);
		gtk_icon_source_set_size (source, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_icon_set_add_source (set, source);

		gtk_icon_source_set_size_wildcarded (source, TRUE);
		gtk_icon_source_set_state_wildcarded (source, TRUE);
		gtk_icon_source_set_direction_wildcarded (source, TRUE);
		gtk_icon_set_add_source (set, source);

		gtk_icon_factory_add (factory, items[i].stock_id, set);

		gtk_icon_set_unref (set);
		gtk_icon_source_free (source);
		g_object_unref (pixbuf);
	}

	gtk_icon_factory_add_default (factory);

	g_object_unref (factory);
}
