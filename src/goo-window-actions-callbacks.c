/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "dlg-extract.h"
#include "dlg-preferences.h"
#include "dlg-properties.h"
#include "gtk-utils.h"
#include "goo-window.h"
#include "goo-window-actions-callbacks.h"
#include "preferences.h"


void
goo_window_activate_play (GSimpleAction *action,
			  GVariant      *parameter,
			  gpointer       user_data)
{
	goo_window_play (GOO_WINDOW (user_data));
}


void
goo_window_activate_play_selected (GSimpleAction *action,
				   GVariant      *parameter,
				   gpointer       user_data)
{
	goo_window_play_selected (GOO_WINDOW (user_data));
}



void
goo_window_activate_pause (GSimpleAction *action,
			   GVariant      *parameter,
			   gpointer       user_data)
{
	goo_window_pause (GOO_WINDOW (user_data));
}


void
goo_window_activate_toggle_play (GSimpleAction *action,
				 GVariant      *parameter,
				 gpointer       user_data)
{
	goo_window_toggle_play (GOO_WINDOW (user_data));
}


void
goo_window_activate_stop (GSimpleAction *action,
			  GVariant      *parameter,
			  gpointer       user_data)
{
	goo_window_stop (GOO_WINDOW (user_data));
}


void
goo_window_activate_next_track (GSimpleAction *action,
				GVariant      *parameter,
				gpointer       user_data)
{
	goo_window_next (GOO_WINDOW (user_data));
}


void
goo_window_activate_previous_track (GSimpleAction *action,
				    GVariant      *parameter,
				    gpointer       user_data)
{
	goo_window_prev (GOO_WINDOW (user_data));
}


void
goo_window_activate_eject (GSimpleAction *action,
			   GVariant      *parameter,
			   gpointer       user_data)
{
	goo_window_eject (GOO_WINDOW (user_data));
}


void
goo_window_activate_reload (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       user_data)
{
	goo_window_update (GOO_WINDOW (user_data));
}


void
goo_window_activate_close (GSimpleAction *action,
			   GVariant      *parameter,
			   gpointer       user_data)
{
	goo_window_close (GOO_WINDOW (user_data));
}


void
goo_window_activate_play_all (GSimpleAction *action,
			      GVariant      *parameter,
			      gpointer       user_data)
{
	GSettings *settings;

	g_simple_action_set_state (action, parameter);
	settings = g_settings_new (GOOBOX_SCHEMA_PLAYLIST);
	g_settings_set_boolean (settings, PREF_PLAYLIST_PLAYALL, g_variant_get_boolean (parameter));

	g_object_unref (settings);
}


void
goo_window_activate_repeat (GSimpleAction *action,
			    GVariant      *parameter,
			    gpointer       user_data)
{
	GSettings *settings;

	g_simple_action_set_state (action, parameter);
	settings = g_settings_new (GOOBOX_SCHEMA_PLAYLIST);
	g_settings_set_boolean (settings, PREF_PLAYLIST_REPEAT, g_variant_get_boolean (parameter));

	g_object_unref (settings);
}


void
goo_window_activate_shuffle (GSimpleAction *action,
			     GVariant      *parameter,
			     gpointer       user_data)
{
	GSettings *settings;

	g_simple_action_set_state (action, parameter);
	settings = g_settings_new (GOOBOX_SCHEMA_PLAYLIST);
	g_settings_set_boolean (settings, PREF_PLAYLIST_SHUFFLE, g_variant_get_boolean (parameter));

	g_object_unref (settings);
}


/* -- goo_window_activate_copy_disc -- */


static void
external_app_watch_func (GPid     pid,
	       	         gint     status,
	                 gpointer data)
{
	g_spawn_close_pid (pid);
	goo_window_set_hibernate (GOO_WINDOW (data), FALSE);
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

	retval = g_spawn_async (NULL,
				argv,
				NULL,
				G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
				NULL,
				NULL,
				&child_pid,
				error);
	g_child_watch_add (child_pid, external_app_watch_func, data);

	g_strfreev (argv);

	return retval;
}


void
goo_window_activate_copy_disc (GSimpleAction *action,
			       GVariant      *parameter,
			       gpointer       user_data)
{
	GooWindow *window = user_data;
	char      *command;
	GError    *error = NULL;

	command = g_strconcat ("brasero --copy=",
			       goo_player_get_device (goo_window_get_player (window)),
			       NULL);

	goo_window_set_hibernate (window, TRUE);
	if (! exec_external_app (gtk_widget_get_screen (GTK_WIDGET (window)),
				 command,
				 &error,
				 window))
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window),
						   _("Could not execute command"),
						   &error);
		goo_window_set_hibernate (window, FALSE);
	}

	g_free (command);
}


void
goo_window_activate_extract (GSimpleAction *action,
			     GVariant      *parameter,
			     gpointer       user_data)
{
	GooWindow *window = user_data;
	GSettings *settings;
	gboolean   use_sound_juicer;
	GError    *error = NULL;

	settings = g_settings_new (GOOBOX_SCHEMA_GENERAL);
	use_sound_juicer = g_settings_get_boolean (settings, PREF_GENERAL_USE_SJ);
	g_object_unref (settings);

	if (! use_sound_juicer) {
		dlg_extract (window);
		return;
	}

	goo_window_set_hibernate (window, TRUE);

	if (! exec_external_app (gtk_widget_get_screen (GTK_WIDGET (window)),
				 "sound-juicer",
				 &error,
				 window))
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window),
						   _("Could not execute command"),
						   &error);
		goo_window_set_hibernate (window, FALSE);
	}
}


void
goo_window_activate_extract_selected (GSimpleAction *action,
				      GVariant      *parameter,
				      gpointer       user_data)
{
	GooWindow *window = user_data;
	GSettings *settings;
	gboolean   use_sound_juicer;
	GError    *error = NULL;

	settings = g_settings_new (GOOBOX_SCHEMA_GENERAL);
	use_sound_juicer = g_settings_get_boolean (settings, PREF_GENERAL_USE_SJ);
	g_object_unref (settings);

	if (! use_sound_juicer) {
		dlg_extract_selected (window);
		return;
	}

	goo_window_set_hibernate (window, TRUE);

	if (! exec_external_app (gtk_widget_get_screen (GTK_WIDGET (window)),
				 "sound-juicer",
				 &error,
				 window))
	{
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (window),
						   _("Could not execute command"),
						   &error);
		goo_window_set_hibernate (window, FALSE);
	}
}


void
goo_window_activate_preferences (GSimpleAction *action,
				 GVariant      *parameter,
				 gpointer       user_data)
{
	dlg_preferences (GOO_WINDOW (user_data));
}


void
goo_window_activate_pick_cover_from_disk (GSimpleAction *action,
					  GVariant      *parameter,
					  gpointer       user_data)
{
	goo_window_pick_cover_from_disk (GOO_WINDOW (user_data));
}


void
goo_window_activate_search_cover (GSimpleAction *action,
				  GVariant      *parameter,
				  gpointer       user_data)
{
	goo_window_search_cover_on_internet (GOO_WINDOW (user_data));
}


void
goo_window_activate_remove_cover (GSimpleAction *action,
				  GVariant      *parameter,
				  gpointer       user_data)
{
	goo_window_remove_cover (GOO_WINDOW (user_data));
}


void
goo_window_activate_toggle_visibility (GSimpleAction *action,
				       GVariant      *parameter,
				       gpointer       user_data)
{
	goo_window_toggle_visibility (GOO_WINDOW (user_data));
}


void
goo_window_activate_properties (GSimpleAction *action,
				GVariant      *parameter,
				gpointer       user_data)
{
	dlg_properties (GOO_WINDOW (user_data));
}
