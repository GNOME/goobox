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
	{ GOO_STOCK_RESET, reset_24_rgba, reset_16_rgba }
};


static const GtkStockItem stock_items [] = {
	{ GOO_STOCK_EXTRACT, N_("_Extract"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_RESET, N_("_Reset"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_VOLUME_MIN, N_("V_olume"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_VOLUME_MED, N_("V_olume"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_VOLUME_MAX, N_("V_olume"), 0, 0, GETTEXT_PACKAGE },
	{ GOO_STOCK_VOLUME_ZERO, N_("V_olume"), 0, 0, GETTEXT_PACKAGE }
};


static struct {
	const char  *id;
	GtkIconSize  size;
} stock_items_from_theme [] = { 
	{ GOO_STOCK_VOLUME_MAX, GTK_ICON_SIZE_LARGE_TOOLBAR }, 
	{ GOO_STOCK_VOLUME_MED, GTK_ICON_SIZE_LARGE_TOOLBAR },
	{ GOO_STOCK_VOLUME_MIN, GTK_ICON_SIZE_LARGE_TOOLBAR },
	{ GOO_STOCK_VOLUME_ZERO, GTK_ICON_SIZE_LARGE_TOOLBAR },
	{ GOO_STOCK_NO_DISC, GTK_ICON_SIZE_DIALOG },
	{ GOO_STOCK_AUDIO_CD, GTK_ICON_SIZE_DIALOG }, 
	{ GOO_STOCK_DATA_DISC, GTK_ICON_SIZE_DIALOG }
};


static gboolean stock_initialized = FALSE;


static void
init_stock_icons_again (GtkIconTheme *icon_theme,
                        gpointer      user_data)
{
	stock_initialized = FALSE;
	goo_stock_init ();
}


void
goo_stock_init (void)
{
	GtkIconFactory *factory;
	GtkIconTheme   *theme;
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

	theme = gtk_icon_theme_get_default ();
	g_signal_connect (theme, 
			  "changed",
			  G_CALLBACK (init_stock_icons_again),
			  NULL);

	for (i = 0; i < G_N_ELEMENTS (stock_items_from_theme); i++) {
		GdkPixbuf     *pixbuf;
		GtkIconSet    *set;
		GtkIconSource *source;
		GError        *error = NULL;
		int            w, h;
		
		gtk_icon_size_lookup (stock_items_from_theme[i].size, &w, &h);	
		pixbuf = gtk_icon_theme_load_icon (theme, stock_items_from_theme[i].id, w, 0, &error);
		if (pixbuf == NULL) {
			g_print ("Goo-WARNING **: %s\n", error->message);
			g_clear_error (&error);
			continue;
		}
		
		set = gtk_icon_set_new ();
		source = gtk_icon_source_new ();
		
		gtk_icon_source_set_pixbuf (source, pixbuf);

		gtk_icon_source_set_size_wildcarded (source, FALSE);
		gtk_icon_source_set_state_wildcarded (source, TRUE);
		gtk_icon_source_set_direction_wildcarded (source, TRUE);
		gtk_icon_source_set_size (source, stock_items_from_theme[i].size);
		gtk_icon_set_add_source (set, source);

		gtk_icon_source_set_size_wildcarded (source, TRUE);
		gtk_icon_source_set_state_wildcarded (source, TRUE);
		gtk_icon_source_set_direction_wildcarded (source, TRUE);
		gtk_icon_set_add_source (set, source);

		gtk_icon_factory_add (factory, stock_items_from_theme[i].id, set);
		
		gtk_icon_set_unref (set);
		gtk_icon_source_free (source);
		g_object_unref (pixbuf);
	}

	gtk_icon_factory_add_default (factory);
	g_object_unref (factory);
}
