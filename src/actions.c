/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
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
#include <gnome.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gnome.h>
#include <libbonobo.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include "actions.h"
#include "main.h"
#include "gtk-utils.h"
#include "goo-window.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "preferences.h"
#include "dlg-extract.h"
#include "dlg-preferences.h"


void
activate_action_play (GtkAction *action, 
		      gpointer   data)
{
	GooWindow *window = data;
	goo_window_play (window);
}


void
activate_action_pause (GtkAction *action,
		       gpointer   data)
{
	GooWindow *window = data;
	goo_window_pause (window);
}


void
activate_action_toggle_play (GtkAction *action, 
			     gpointer   data)
{
	GooWindow *window = data;
	goo_window_toggle_play (window);
}


void
activate_action_stop (GtkAction *action, 
		      gpointer   data)
{
	GooWindow *window = data;
	goo_window_stop (window);
}


void
activate_action_next (GtkAction *action, 
		      gpointer   data)
{
	GooWindow *window = data;
	goo_window_next (window);
}


void
activate_action_prev (GtkAction *action, 
		      gpointer   data)
{
	GooWindow *window = data;
	goo_window_prev (window);
}


void 
activate_action_eject (GtkAction *action, 
		       gpointer   data)
{
	GooWindow *window = data;
	goo_window_eject (window);
}


void
activate_action_reload (GtkAction *action, 
			gpointer   data)
{
	GooWindow *window = data;
	goo_window_update (window);
}


void
activate_action_about (GtkAction *action, 
		       gpointer   data)
{
	GooWindow        *window = data;
	static GtkWidget *about = NULL;
	GdkPixbuf        *logo;
	const char       *authors[] = {
		"Paolo Bacchilega <paolo.bacchilega@libero.it>", 
		NULL
	};
	const char       *documenters [] = {
		NULL
	};
	const char       *translator_credits = _("translator_credits");

	if (about != NULL) {
		gtk_window_present (GTK_WINDOW (about));
		return;
	}

	logo = gdk_pixbuf_new_from_file (PIXMAPSDIR "/goobox.png", NULL);
	about = gnome_about_new (_("Goobox"), 
				 VERSION,
				 "Copyright \xc2\xa9 2004 Free Software Foundation, Inc.",
				 _("CD player and ripper"),
				 authors,
				 documenters,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 logo);
	if (logo != NULL)
                g_object_unref (logo);

	gtk_window_set_destroy_with_parent (GTK_WINDOW (about), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (about), 
				      GTK_WINDOW (window));

	g_signal_connect (G_OBJECT (about), 
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed), 
			  &about);

	gtk_widget_show_all (about);
}


void
activate_action_quit (GtkAction *action, 
		      gpointer   data)
{
	gtk_widget_destroy (GTK_WIDGET (main_window));
	bonobo_main_quit ();
}


void
activate_action_view_toolbar (GtkAction *action, 
			      gpointer   data)
{
	eel_gconf_set_boolean (PREF_UI_TOOLBAR, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
activate_action_view_statusbar (GtkAction *action, 
				gpointer   data)
{	
	eel_gconf_set_boolean (PREF_UI_STATUSBAR, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
activate_action_play_all (GtkAction *action, 
			  gpointer   data)
{	
	eel_gconf_set_boolean (PREF_PLAYLIST_PLAYALL, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
activate_action_repeat (GtkAction *action, 
			gpointer   data)
{	
	eel_gconf_set_boolean (PREF_PLAYLIST_REPEAT, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
activate_action_shuffle (GtkAction *action, 
			 gpointer   data)
{	
	eel_gconf_set_boolean (PREF_PLAYLIST_SHUFFLE, gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
activate_action_extract (GtkAction *action, 
			 gpointer   data)
{
	dlg_extract (GOO_WINDOW (data));
}


void
activate_action_preferences (GtkAction *action, 
			     gpointer   data)
{
	dlg_preferences (GOO_WINDOW (data));
}


void
activate_action_edit_cddata (GtkAction *action, 
			     gpointer   data)
{
	goo_window_edit_cddata (GOO_WINDOW (data));
}
