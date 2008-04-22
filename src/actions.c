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
#include "dlg-properties.h"


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
	GooWindow  *window = data;
	const char *authors[] = {
		"Paolo Bacchilega <paolo.bacchilega@libero.it>", 
		NULL
	};
	/*
	  const char     *documenters [] = {
	  NULL
	  };*/
	const char *translator_credits = _("translator_credits");
        char       *license_text;
        const char *license[] = {
                "CD Player is free software; you can redistribute it and/or modify"
                "it under the terms of the GNU General Public License as published by "
                "the Free Software Foundation; either version 2 of the License, or "
                "(at your option) any later version.",
                "CD Player is distributed in the hope that it will be useful, "
                "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
                "GNU General Public License for more details.",
                "You should have received a copy of the GNU General Public License "
                "along with CD Player; if not, write to the Free Software Foundation, Inc., "
                "51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA"
        };

        license_text = g_strconcat (license[0], "\n\n", license[1], "\n\n",
                                    license[2], "\n\n", NULL);

        gtk_show_about_dialog (GTK_WINDOW (window),
        		       "name", _("CD Player"),
                               "version", VERSION,
                               "copyright", _("Copyright \xc2\xa9 2004-2007 Free Software Foundation, Inc."),
                               "comments", _("Play CDs and save the tracks to disk as files"),
                               "authors", authors,
                               /*"documenters", documenters,*/
                               "translator_credits", strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
                               "logo-icon-name", "goobox",
                               "license", license_text,
                               "wrap-license", TRUE,
                               NULL);
                               
	g_free (license_text);
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
external_app_watch_func (GPid     pid,
	       	         gint     status,
	                 gpointer data)
{
	GooWindow *window = data;
	
	g_spawn_close_pid (pid);
	goo_window_set_hibernate (window, FALSE);
}


static gboolean
exec_external_app (GdkScreen   *screen,
		   const char  *command_line,
	 	   GError     **error,
	 	   gpointer     data)
{
	gboolean   retval;
	gchar    **argv = NULL;
	GPid       child_pid;

	g_return_val_if_fail (command_line != NULL, FALSE);
	
	if (! g_shell_parse_argv (command_line, NULL, &argv, error))
		return FALSE;

	retval = gdk_spawn_on_screen (screen,
				      NULL,
				      argv,
				      NULL,
				      G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
				      NULL,
				      NULL,
				      &child_pid,
				      error);
	g_strfreev (argv);
	g_child_watch_add (child_pid, external_app_watch_func, data);

	return retval;
}


void
activate_action_copy_disc (GtkAction *action, 
			   gpointer   data)
{
	GooWindow *window = data;
	char      *command;
	GError    *error = NULL;

	command = g_strconcat ("nautilus-cd-burner --source-device=", 
			       goo_player_get_device (goo_window_get_player (window)), 
			       NULL);

	goo_window_set_hibernate (window, TRUE);
	if (! exec_external_app (gtk_widget_get_screen (GTK_WIDGET (window)), 
				 command, 
				 &error, 
				 data)) 
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window), 
						   _("Could not execute command"), 
						   &error);
		goo_window_set_hibernate (window, FALSE);
	}
	g_free (command);
}


void
activate_action_extract (GtkAction *action, 
			 gpointer   data)
{
	GooWindow *window = data;
	GError    *error = NULL;
	
	if (! preferences_get_use_sound_juicer ()) {
		dlg_extract (window);
		return;
	}

	goo_window_set_hibernate (window, TRUE);		

	if (! exec_external_app (gtk_widget_get_screen (GTK_WIDGET (window)), 
				 "sound-juicer", 
				 &error, 
				 data)) 
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window), 
						   _("Could not execute command"), 
						   &error);
		goo_window_set_hibernate (window, FALSE);
	}
}


void
activate_action_extract_selected (GtkAction *action, 
			 	  gpointer   data)
{
	GooWindow *window = data;
	GError    *error = NULL;
	
	if (! preferences_get_use_sound_juicer ()) {
		dlg_extract_selected (window);	
		return;
	}
		
	goo_window_set_hibernate (window, TRUE);		

	if (! exec_external_app (gtk_widget_get_screen (GTK_WIDGET (window)),
				 "sound-juicer", 
				 &error, 
				 data)) 
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window), 
						   _("Could not execute command"), 
						   &error);
		goo_window_set_hibernate (window, FALSE);
	}
}


void
activate_action_preferences (GtkAction *action, 
			     gpointer   data)
{
	dlg_preferences (GOO_WINDOW (data));
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


void
activate_action_properties (GtkAction *action, 
		            gpointer   data)
{
	dlg_properties (GOO_WINDOW (data));
}
