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
#include "dlg-preferences.h"
#include "dlg-properties.h"
#include "gtk-utils.h"
#include "goo-application.h"
#include "goo-application-actions-callbacks.h"
#include "goo-window.h"
#include "preferences.h"
#include "main.h"


void
update_actions_sensitivity (GApplication *application)
{
	GVariant *state;
	gboolean  play_all;

	state = g_action_get_state (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_PLAYALL));
	play_all = g_variant_get_boolean (state);
	g_variant_unref (state);

	g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_REPEAT)), play_all);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_SHUFFLE)), play_all);
}


void
goo_application_activate_about (GSimpleAction *action,
				GVariant      *parameter,
				gpointer       user_data)
{
        const char *authors[] = {
                "Paolo Bacchilega <paobac@src.gnome.org>",
                NULL
        };
        const char *documenters [] = {
                "Paolo Bacchilega <paobac@src.gnome.org>",
                NULL
        };
        const char *translator_credits = _("translator_credits");

        gtk_show_about_dialog (GTK_WINDOW (_gtk_application_get_current_window (GTK_APPLICATION (user_data))),
                               "name", _("CD Player"),
                               "version", PACKAGE_VERSION,
                               "copyright", _("Copyright \xc2\xa9 2004-2011 Free Software Foundation, Inc."),
                               "comments", _("Play CDs and save the tracks to disk as files"),
                               "authors", authors,
                               "documenters", documenters,
                               "translator_credits", strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
                               "logo-icon-name", "goobox",
                               "license-type", GTK_LICENSE_GPL_2_0,
                               "wrap-license", TRUE,
                               NULL);
}


void
goo_application_activate_help (GSimpleAction *action,
			       GVariant      *parameter,
			       gpointer       user_data)
{
	GApplication *application = user_data;
	GtkWidget    *window;

	window = _gtk_application_get_current_window (GTK_APPLICATION (application));
	show_help_dialog (GTK_WINDOW (window), NULL);
}


void
goo_application_activate_play_all (GSimpleAction *action,
				   GVariant      *parameter,
				   gpointer       user_data)
{
	GApplication *application = user_data;
	GSettings    *settings;

	g_simple_action_set_state (action, parameter);
	settings = g_settings_new (GOOBOX_SCHEMA_PLAYLIST);
	g_settings_set_boolean (settings, PREF_PLAYLIST_PLAYALL, g_variant_get_boolean (parameter));
	update_actions_sensitivity (application);

	g_object_unref (settings);
}


void
goo_application_activate_preferences (GSimpleAction *action,
				      GVariant      *parameter,
				      gpointer       user_data)
{
	GApplication *application = user_data;
	GtkWidget    *window;

	window = _gtk_application_get_current_window (GTK_APPLICATION (application));
	dlg_preferences (GOO_WINDOW (window));
}


void
goo_application_activate_quit (GSimpleAction *action,
			       GVariant      *parameter,
			       gpointer       user_data)
{
	GApplication *application = user_data;
	GList        *windows;

	while ((windows = gtk_application_get_windows (GTK_APPLICATION (application))) != NULL)
		goo_window_close (GOO_WINDOW (windows->data));
}


void
goo_application_activate_repeat (GSimpleAction *action,
				 GVariant      *parameter,
				 gpointer       user_data)
{
	GApplication *application = user_data;
	GSettings    *settings;

	g_simple_action_set_state (action, parameter);
	settings = g_settings_new (GOOBOX_SCHEMA_PLAYLIST);
	g_settings_set_boolean (settings, PREF_PLAYLIST_REPEAT, g_variant_get_boolean (parameter));
	update_actions_sensitivity (application);

	g_object_unref (settings);
}


void
goo_application_activate_shuffle (GSimpleAction *action,
				  GVariant      *parameter,
				  gpointer       user_data)
{
	GApplication *application = user_data;
	GSettings    *settings;

	g_simple_action_set_state (action, parameter);
	settings = g_settings_new (GOOBOX_SCHEMA_PLAYLIST);
	g_settings_set_boolean (settings, PREF_PLAYLIST_SHUFFLE, g_variant_get_boolean (parameter));
	update_actions_sensitivity (application);

	g_object_unref (settings);
}


void
goo_application_activate_pause (GSimpleAction *action,
				GVariant      *parameter,
				gpointer       user_data)
{
	const char *device_id;
	GtkWidget  *window;

	device_id = g_variant_get_string (parameter, NULL);
	window = main_get_window_from_device (device_id);
	if (window != NULL)
		goo_window_pause (GOO_WINDOW (window));
}


void
goo_application_activate_play (GSimpleAction *action,
			       GVariant      *parameter,
			       gpointer       user_data)
{
	const char *device_id;
	GtkWidget  *window;

	device_id = g_variant_get_string (parameter, NULL);
	window = main_get_window_from_device (device_id);
	if (window != NULL)
		goo_window_play (GOO_WINDOW (window));
}


void
goo_application_activate_play_next (GSimpleAction *action,
				    GVariant      *parameter,
				    gpointer       user_data)
{
	const char *device_id;
	GtkWidget  *window;

	device_id = g_variant_get_string (parameter, NULL);
	window = main_get_window_from_device (device_id);
	if (window != NULL)
		goo_window_next (GOO_WINDOW (window));
}
