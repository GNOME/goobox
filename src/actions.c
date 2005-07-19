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
activate_action_play_selected (GtkAction *action, 
			       gpointer   data)
{
	GooWindow *window = data;
	goo_window_play_selected (window);
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



static void
show_help (GooWindow  *window,
	   const char *section)
{
	GError *err = NULL;  

        gnome_help_display ("goobox", section, &err);
        
        if (err != NULL) {
                GtkWidget *dialog;
                
                dialog = _gtk_message_dialog_new (GTK_WINDOW (window),
						  GTK_DIALOG_DESTROY_WITH_PARENT, 
						  GTK_STOCK_DIALOG_ERROR,
						  _("Could not display help"),
						  err->message,
						  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
						  NULL);
                gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
                g_signal_connect (G_OBJECT (dialog), "response",
                                  G_CALLBACK (gtk_widget_destroy),
                                  NULL);
                
                gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
                
                gtk_widget_show (dialog);
                
                g_error_free (err);
        }
}


void
activate_action_manual (GtkAction *action, 
			gpointer   data)
{
	GooWindow *window = data;
	show_help (window, NULL);
}


void
activate_action_shortcuts (GtkAction *action, 
			   gpointer   data)
{
	GooWindow *window = data;
	show_help (window, "goobox-shortcuts");
}


void
activate_action_about (GtkAction *action, 
		       gpointer   data)
{
	GooWindow        *window = data;
	static GtkWidget *about = NULL;
#ifndef HAVE_GTK_2_5
	GdkPixbuf        *logo;
#endif
	const char       *authors[] = {
		"Paolo Bacchilega <paolo.bacchilega@libero.it>", 
		NULL
	};
	/*
	  const char       *documenters [] = {
	  NULL
	  };*/
	const char       *translator_credits = _("translator_credits");

	if (about != NULL) {
		gtk_window_present (GTK_WINDOW (about));
		return;
	}

#ifdef HAVE_GTK_2_5

	about = gtk_about_dialog_new ();
	g_object_set (about,
		      "name",  _("Goobox"),
		      "version", VERSION,
		      "copyright", "Copyright \xc2\xa9 2004-2005 Free Software Foundation, Inc.",
		      "comments", _("CD player and ripper"),
		      "authors", authors,
		      /*"documenters", documenters,*/
		      "translator_credits", strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
		      "website", NULL,
		      "website_label", NULL,
		      "license", NULL,
		      NULL);

#else

	logo = gdk_pixbuf_new_from_file (PIXMAPSDIR "/goobox.png", NULL);
	about = gnome_about_new (_("Goobox"), 
				 VERSION,
				 "Copyright \xc2\xa9 2004-2005 Free Software Foundation, Inc.",
				 _("CD player and ripper"),
				 authors,
				 NULL /*documenters*/,
				 strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
				 logo);
	if (logo != NULL)
                g_object_unref (logo);

#endif

	gtk_window_set_destroy_with_parent (GTK_WINDOW (about), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (about), 
				      GTK_WINDOW (window));

	g_signal_connect (G_OBJECT (about), 
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed), 
			  &about);

	gtk_widget_show (about);
}


void
activate_action_quit (GtkAction *action, 
		      gpointer   data)
{
	goo_window_close (GOO_WINDOW (data));
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


static void
sj_watch_func (GPid     pid,
	       gint     status,
	       gpointer data)
{
	GooWindow *window = data;
	g_spawn_close_pid (pid);
	goo_window_set_hibernate (window, FALSE);
}


static gboolean
exec_sj (const char  *command_line,
	 GError     **error,
	 gpointer     data)
{
	gboolean   retval;
	gchar    **argv = NULL;
	GPid       child_pid;

	g_return_val_if_fail (command_line != NULL, FALSE);
	
	if (!g_shell_parse_argv (command_line,
				 NULL, &argv,
				 error))
		return FALSE;

	retval = g_spawn_async (NULL,
				argv,
				NULL,
				G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
				NULL,
				NULL,
				&child_pid,
				error);
	g_strfreev (argv);

	g_child_watch_add (child_pid,
			   sj_watch_func,
			   data);

	return retval;
}


void
activate_action_extract (GtkAction *action, 
			 gpointer   data)
{
	GooWindow *window = data;

	if (preferences_get_use_sound_juicer ()) {
		GError *error = NULL;

		goo_window_set_hibernate (window, TRUE);		

		if (! exec_sj ("sound-juicer", &error, data))
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window), 
							   _("Could not execute command"), 
							   &error);

	} else
		dlg_extract (window);
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


void
activate_action_pick_cover_from_disk (GtkAction *action, 
				      gpointer   data)
{
	goo_window_pick_cover_from_disk (GOO_WINDOW (data));
}


void
activate_action_remove_cover (GtkAction *action, 
			      gpointer   data)
{
	goo_window_remove_cover (GOO_WINDOW (data));
}


void
activate_action_search_cover_on_internet (GtkAction *action, 
					  gpointer   data)
{
	goo_window_search_cover_on_internet (GOO_WINDOW (data));
}


void
activate_action_toggle_visibility (GtkAction *action, 
				   gpointer   data)
{
	goo_window_toggle_visibility (GOO_WINDOW (data));
}
