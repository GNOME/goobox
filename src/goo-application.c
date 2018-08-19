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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gst/gst.h>
#include "goo-application.h"
#include "goo-application-actions-callbacks.h"
#include "goo-application-actions-entries.h"
#include "goo-window.h"
#include "gtk-utils.h"
#include "main.h"
#include "preferences.h"


/* -- command line arguments -- */

static const char     *program_argv0 = NULL;
static int             arg_toggle_play = FALSE;
static int             arg_stop = FALSE;
static int             arg_next = FALSE;
static int             arg_prev = FALSE;
static int             arg_eject = FALSE;
static int             arg_quit = FALSE;
static gboolean        arg_version = FALSE;
static char           *arg_device = NULL;


static const GOptionEntry options[] = {
	{ "device", 'd',  0, G_OPTION_ARG_STRING, &arg_device,
	  N_("CD device to be used"),
	  N_("DEVICE_PATH") },
	{ "play", '\0', 0, G_OPTION_ARG_NONE, &arg_auto_play,
          N_("Play the CD on startup"),
          0 },
	{ "toggle-play", '\0', 0, G_OPTION_ARG_NONE, &arg_toggle_play,
          N_("Toggle play"),
          0 },
        { "stop", '\0', 0, G_OPTION_ARG_NONE, &arg_stop,
          N_("Stop playing"),
          0 },
	{ "next", '\0', 0, G_OPTION_ARG_NONE, &arg_next,
          N_("Play the next track"),
          0 },
	{ "previous", '\0', 0, G_OPTION_ARG_NONE, &arg_prev,
          N_("Play the previous track"),
          0 },
	{ "eject", '\0', 0, G_OPTION_ARG_NONE, &arg_eject,
          N_("Eject the CD"),
          0 },
	{ "toggle-visibility", '\0', 0, G_OPTION_ARG_NONE, &arg_toggle_visibility,
          N_("Toggle the main window visibility"),
          0 },
	{ "quit", '\0', 0, G_OPTION_ARG_NONE, &arg_quit,
          N_("Quit the application"),
          0 },
        { "version", 'v', 0, G_OPTION_ARG_NONE, &arg_version,
    	  N_("Show version"),
    	  0 },
	{ NULL }
};


/* -- GooApplication --  */


G_DEFINE_TYPE (GooApplication, goo_application, GTK_TYPE_APPLICATION)


struct _GooApplicationPrivate {
        GSettings *settings;
};


static void
goo_application_finalize (GObject *object)
{
	GooApplication *self = (GooApplication *) object;

	g_object_unref (self->priv->settings);
	G_OBJECT_CLASS (goo_application_parent_class)->finalize (object);
}


static gboolean
required_gstreamer_plugins_available (void)
{
	char *required_plugins[] = { "cdparanoiasrc", "audioconvert", "volume", "giosink" };
	int   i;

	for (i = 0; i < G_N_ELEMENTS (required_plugins); i++) {
		GstElement *element;
		gboolean    present;

		element = gst_element_factory_make (required_plugins[i], NULL);
		present = (element != NULL);
		if (element != NULL)
			gst_object_unref (GST_OBJECT (element));

		if (! present)
			return FALSE;
	}

	return TRUE;
}


static int
goo_application_command_line_finished (GApplication *application,
                                       int           status)
{
        if (status == EXIT_SUCCESS)
                gdk_notify_startup_complete ();

        /* reset the arguments */

	arg_auto_play = FALSE;
	arg_toggle_play = FALSE;
	arg_stop = FALSE;
	arg_next = FALSE;
	arg_prev = FALSE;
	arg_eject = FALSE;
	arg_toggle_visibility = FALSE;
	arg_quit = FALSE;
	g_free (arg_device);
	arg_device = NULL;

        return status;
}


static GOptionContext *
goo_application_create_option_context (void)
{
	GOptionContext *options_context;
	static gsize    initialized = FALSE;

	options_context = g_option_context_new (N_("Play CDs and save the tracks to disk as files"));
	g_option_context_set_translation_domain (options_context, GETTEXT_PACKAGE);
	g_option_context_set_ignore_unknown_options (options_context, TRUE);
	g_option_context_add_main_entries (options_context, options, GETTEXT_PACKAGE);

	if (g_once_init_enter (&initialized)) {
		g_option_context_add_group (options_context, gtk_get_option_group (TRUE));
		g_option_context_add_group (options_context, gst_init_get_option_group ());

		g_once_init_leave (&initialized, TRUE);
	}

	return options_context;
}


static int
goo_application_command_line (GApplication            *application,
			      GApplicationCommandLine *command_line)
{
	char           **argv;
	int              argc;
	GOptionContext  *options_context;
	GError          *error = NULL;
	GtkWidget       *window;

	argv = g_application_command_line_get_arguments (command_line, &argc);
	options_context = goo_application_create_option_context ();
	if (! g_option_context_parse (options_context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		g_option_context_free (options_context);
		return goo_application_command_line_finished (application, EXIT_FAILURE);
	}
	g_option_context_free (options_context);

	/* check the gstreamer plugins */

	if (! required_gstreamer_plugins_available ()) {
		GtkWidget *d;
		d = _gtk_message_dialog_new (NULL,
					     0,
					     _GTK_ICON_NAME_DIALOG_ERROR,
					     _("Cannot start the CD player"),
					     _("In order to read CDs you have to install the GStreamer base plugins"),
					     _GTK_LABEL_OK, GTK_RESPONSE_OK,
					     NULL);
		g_signal_connect_swapped (G_OBJECT (d), "response",
					  G_CALLBACK (gtk_widget_destroy),
					  d);
		gtk_window_set_application (GTK_WINDOW (d), GTK_APPLICATION (application));
		gtk_widget_show (d);

		return goo_application_command_line_finished (application, EXIT_FAILURE);
	}

	/* execute the command line */

	window = _gtk_application_get_current_window (GTK_APPLICATION (application));
	if (window == NULL)
		window = goo_window_new (NULL);
	gtk_window_present (GTK_WINDOW (window));

	if (arg_auto_play) {
		goo_window_play (GOO_WINDOW (window));
	}
	else if (arg_toggle_play) {
		goo_window_toggle_play (GOO_WINDOW (window));
	}
	else if (arg_stop) {
		goo_window_stop (GOO_WINDOW (window));
	}
	else if (arg_next) {
		goo_window_next (GOO_WINDOW (window));
	}
	else if (arg_prev) {
		goo_window_prev (GOO_WINDOW (window));
	}
	else if (arg_eject) {
		goo_window_eject (GOO_WINDOW (window));
	}
	else if (arg_toggle_visibility) {
		goo_window_toggle_visibility (GOO_WINDOW (window));
	}
	else if (arg_quit) {
		goo_window_close (GOO_WINDOW (window));
	}
	else if (arg_device != NULL) {
		BraseroDrive *drive;

		drive = main_get_drive_for_device (arg_device);
		window = main_get_window_from_device (arg_device);
		if (window == NULL) {
			window = goo_window_new (drive);
			gtk_widget_show (window);
		}
		else
			goo_window_set_drive (GOO_WINDOW (window), drive);

		g_object_unref (drive);
		g_free (arg_device);
		arg_device = NULL;
	}

	return goo_application_command_line_finished (application, EXIT_SUCCESS);
}


static gboolean
goo_application_local_command_line (GApplication   *application,
				    char         ***arguments,
				    int            *exit_status)
{
	char           **local_argv;
	int              local_argc;
	GOptionContext  *options_context;
	GError          *error = NULL;
	gboolean         handled_locally = FALSE;

	local_argv = g_strdupv (*arguments);
	local_argc = g_strv_length (local_argv);

	program_argv0 = g_strdup (local_argv[0]);
	*exit_status = EXIT_SUCCESS;

	options_context = goo_application_create_option_context ();
	if (! g_option_context_parse (options_context, &local_argc, &local_argv, &error)) {
		*exit_status = EXIT_FAILURE;
		g_critical ("Failed to parse arguments: %s", error->message);
		g_clear_error (&error);
		handled_locally = TRUE;
	}

	if (arg_version) {
		g_printf ("%s %s, Copyright © 2001-2013 Free Software Foundation, Inc.\n", PACKAGE_NAME, PACKAGE_VERSION);
		handled_locally = TRUE;
	}

	g_option_context_free (options_context);
	g_strfreev (local_argv);

	return handled_locally;
}


/* -- goo_application_startup -- */


static void
pref_playlist_playall_changed (GSettings  *settings,
	  	 	       const char *key,
	  	 	       gpointer    user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_PLAYALL)),
				   g_variant_new_boolean (g_settings_get_boolean (application->priv->settings, PREF_PLAYLIST_PLAYALL)));
	update_actions_sensitivity (G_APPLICATION (application));
}


static void
pref_playlist_repeat_changed (GSettings  *settings,
	  	 	      const char *key,
	  	 	      gpointer    user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_REPEAT)),
				   g_variant_new_boolean (g_settings_get_boolean (application->priv->settings, PREF_PLAYLIST_REPEAT)));
	update_actions_sensitivity (G_APPLICATION (application));
}


static void
pref_playlist_shuffle_changed (GSettings  *settings,
	  	 	       const char *key,
	  	 	       gpointer    user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_SHUFFLE)),
				   g_variant_new_boolean (g_settings_get_boolean (application->priv->settings, PREF_PLAYLIST_SHUFFLE)));
	update_actions_sensitivity (G_APPLICATION (application));
}


static void
initialize_app_menu (GApplication *application)
{
	GooApplication *self = (GooApplication *) application;
	GtkBuilder     *builder;

	g_action_map_add_action_entries (G_ACTION_MAP (application),
					 goo_application_actions,
					 G_N_ELEMENTS (goo_application_actions),
					 application);

	builder = _gtk_builder_new_from_resource ("app-menu.ui");
	gtk_application_set_app_menu (GTK_APPLICATION (application),
				      G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));

	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_PLAYALL)),
				   g_variant_new_boolean (g_settings_get_boolean (self->priv->settings, PREF_PLAYLIST_PLAYALL)));
	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_REPEAT)),
				   g_variant_new_boolean (g_settings_get_boolean (self->priv->settings, PREF_PLAYLIST_REPEAT)));
	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_SHUFFLE)),
				   g_variant_new_boolean (g_settings_get_boolean (self->priv->settings, PREF_PLAYLIST_SHUFFLE)));

	g_signal_connect (self->priv->settings,
			  "changed::" PREF_PLAYLIST_PLAYALL,
			  G_CALLBACK (pref_playlist_playall_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_PLAYLIST_SHUFFLE,
			  G_CALLBACK (pref_playlist_shuffle_changed),
			  self);
	g_signal_connect (self->priv->settings,
			  "changed::" PREF_PLAYLIST_REPEAT,
			  G_CALLBACK (pref_playlist_repeat_changed),
			  self);

	g_object_unref (builder);
}


static void
goo_application_startup (GApplication *application)
{
	G_APPLICATION_CLASS (goo_application_parent_class)->startup (application);

	g_set_application_name (_("CD Player"));
	gtk_window_set_default_icon_name ("goobox");

	initialize_app_menu (application);
}


static void
goo_application_class_init (GooApplicationClass *klass)
{
	GObjectClass      *object_class;
	GApplicationClass *application_class;

	g_type_class_add_private (klass, sizeof (GooApplicationPrivate));

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = goo_application_finalize;

	application_class = G_APPLICATION_CLASS (klass);
	application_class->command_line = goo_application_command_line;
	application_class->local_command_line = goo_application_local_command_line;
	application_class->startup = goo_application_startup;
}


static void
goo_application_init (GooApplication *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GOO_TYPE_APPLICATION, GooApplicationPrivate);
	self->priv->settings = g_settings_new (GOOBOX_SCHEMA_PLAYLIST);
}


GtkApplication *
goo_application_new (void)
{
	return g_object_new (GOO_TYPE_APPLICATION,
	                     "application-id", "org.gnome.Goobox",
	                     "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
	                     NULL);
}
