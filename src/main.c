/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004-2009 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either arg_version 2 of the License, or
 *  (at your option) any later arg_version.
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
#include <stdlib.h>
#include <brasero3/brasero-medium-monitor.h>
#include <gst/gst.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "actions.h"
#include "eggsmclient.h"
#include "goo-player-info.h"
#include "goo-stock.h"
#include "goo-window.h"
#include "typedefs.h"
#include "preferences.h"
#include "main.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#ifdef ENABLE_NOTIFICATION
#include <libnotify/notify.h>
#ifndef NOTIFY_CHECK_VERSION
#define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif
static NotifyNotification *notification = NULL;
gboolean                   notification_supports_persistence = FALSE;
gboolean                   notification_supports_actions = FALSE;
#endif /* ENABLE_NOTIFICATION */


#define VOLUME_STEP 0.10


/* -- command line arguments -- */


int                    arg_auto_play = FALSE;
int                    arg_toggle_visibility = FALSE;
static int             arg_toggle_play = FALSE;
static int             arg_stop = FALSE;
static int             arg_next = FALSE;
static int             arg_prev = FALSE;
static int             arg_eject = FALSE;
static int             arg_quit = FALSE;
static gboolean        arg_version = FALSE;
static char           *arg_device = NULL;
static const char     *program_argv0; /* argv[0] from main(); used as the command to restart the program */


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


/* -- session management -- */


static void
client_save_state (EggSMClient *client,
		   GKeyFile    *state,
		   gpointer     user_data)
{
	GApplication *application = user_data;
	const char   *argv[2] = { NULL };
	guint         i;
	GList        *scan;

	argv[0] = program_argv0;
	argv[1] = NULL;
	egg_sm_client_set_restart_command (client, 1, argv);

	i = 0;
	for (scan = gtk_application_get_windows (GTK_APPLICATION (application)); scan; scan = scan->next) {
		GtkWidget *window = scan->data;
		char      *key;

		key = g_strdup_printf ("device%d", ++i);
		g_key_file_set_string (state,
				       "Session",
				       key,
				       goo_player_get_device (goo_window_get_player (GOO_WINDOW (window))));

		g_free (key);
	}

	g_key_file_set_integer (state, "Session", "devices", i);
}


static void
client_quit_requested_cb (EggSMClient *client,
			  gpointer     data)
{
	egg_sm_client_will_quit (client, TRUE);
}


static void
client_quit_cb (EggSMClient *client,
		gpointer     data)
{
	gtk_main_quit ();
}


static void
goo_session_manager_init (GApplication *application)
{
	EggSMClient *client = NULL;

	client = egg_sm_client_get ();
	g_signal_connect (client,
			  "save_state",
			  G_CALLBACK (client_save_state),
			  application);
	g_signal_connect (client,
			  "quit_requested",
			  G_CALLBACK (client_quit_requested_cb),
			  application);
	g_signal_connect (client,
			  "quit",
			  G_CALLBACK (client_quit_cb),
			  application);
}


static void
goo_restore_session (EggSMClient  *client,
		     GApplication *application)
{
	GKeyFile *state = NULL;
	guint     i;

	state = egg_sm_client_get_state_file (client);
	i = g_key_file_get_integer (state, "Session", "devices", NULL);
	g_assert (i > 0);
	for (/* void */; i > 0; i--) {
		char         *key;
		char         *device;
		BraseroDrive *drive;
		GtkWidget    *window;

		key = g_strdup_printf ("device%d", i);
		device = g_key_file_get_string (state, "Session", key, NULL);
		g_free (key);

		g_assert (device != NULL);

		drive = main_get_drive_for_device (device);
		window = goo_window_new (drive);
		gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (application));
		gtk_widget_show (window);

		g_object_unref (drive);
		g_free (device);
	}
}


/* -- main application -- */


GtkApplication *Main_Application = NULL;


typedef struct {
	GtkApplication __parent;
	GSettings *settings;
} GooApplication;


typedef GtkApplicationClass GooApplicationClass;


G_DEFINE_TYPE (GooApplication, goo_application, GTK_TYPE_APPLICATION)


static void
goo_application_finalize (GObject *object)
{
	GooApplication *self = (GooApplication *) object;

	g_object_unref (self->settings);
	G_OBJECT_CLASS (goo_application_parent_class)->finalize (object);
}


static void
goo_application_init (GooApplication *self)
{
	g_set_application_name (_("CD Player"));

	self->settings = g_settings_new (GOOBOX_SCHEMA_PLAYLIST);
}


static void
goo_application_activate (GApplication *application)
{
	GtkWidget *window;

	window = goo_window_new (NULL);
	gtk_widget_show (window);
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


static gboolean
init_application (GApplication *application)
{
	EggSMClient *client = NULL;

        gtk_window_set_default_icon_name ("goobox");
        goo_stock_init ();

	if (! required_gstreamer_plugins_available ()) {
		GtkWidget *d;
		d = _gtk_message_dialog_new (NULL,
					     0,
					     GTK_STOCK_DIALOG_ERROR,
					     _("Cannot start the CD player"),
					     _("In order to read CDs you have to install the gstreamer base plugins"),
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     NULL);
		g_signal_connect_swapped (G_OBJECT (d), "response",
					  G_CALLBACK (gtk_widget_destroy),
					  d);
		gtk_window_set_application (GTK_WINDOW (d), GTK_APPLICATION (application));
		gtk_widget_show (d);

		return FALSE;
	}

#ifdef ENABLE_NOTIFICATION
	if (! notify_init (g_get_application_name ()))
                g_warning ("Cannot initialize notification system.");
#endif /* ENABLE_NOTIFICATION */

	goo_session_manager_init (application);

	client = egg_sm_client_get ();
	if (egg_sm_client_is_resumed (client)) {
		goo_restore_session (client, application);
		return FALSE;
	}

	return TRUE;
}


static int
goo_application_command_line (GApplication            *application,
			      GApplicationCommandLine *command_line)
{
	char           **argv;
	int              argc;
	GOptionContext  *options_context;
	GError          *error = NULL;
	GList           *window_list;
	GtkWidget       *window;

	argv = g_application_command_line_get_arguments (command_line, &argc);

	options_context = g_option_context_new (N_("Play CDs and save the tracks to disk as files"));
	g_option_context_set_translation_domain (options_context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (options_context, options, GETTEXT_PACKAGE);
	g_option_context_add_group (options_context, gtk_get_option_group (TRUE));
	g_option_context_add_group (options_context, egg_sm_client_get_option_group ());
	g_option_context_add_group (options_context, gst_init_get_option_group ());
	g_option_context_set_ignore_unknown_options (options_context, TRUE);
	if (! g_option_context_parse (options_context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	window_list = gtk_application_get_windows (GTK_APPLICATION (application));
	if (window_list == NULL) {
		if (! init_application (application))
			return 0;
		window = goo_window_new (NULL);
		gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (application));
		gtk_widget_show (window);
	}
	else
		window = window_list->data;

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
			gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (application));
			gtk_widget_show (window);
		}
		else
			goo_window_set_drive (GOO_WINDOW (window), drive);

		g_object_unref (drive);
		g_free (arg_device);
		arg_device = NULL;
	}

	/* reset arguments */

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

	return 0;
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

	*exit_status = 0;

	options_context = g_option_context_new (N_("Play CDs and save the tracks to disk as files"));
	g_option_context_set_translation_domain (options_context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (options_context, options, GETTEXT_PACKAGE);
	g_option_context_add_group (options_context, gtk_get_option_group (TRUE));
	g_option_context_add_group (options_context, egg_sm_client_get_option_group ());
	g_option_context_add_group (options_context, gst_init_get_option_group ());
	g_option_context_set_ignore_unknown_options (options_context, TRUE);
	if (! g_option_context_parse (options_context, &local_argc, &local_argv, &error)) {
		*exit_status = EXIT_FAILURE;
		g_critical ("Failed to parse arguments: %s", error->message);
		g_clear_error (&error);
		handled_locally = TRUE;
	}

	if (arg_version) {
		g_printf ("%s %s, Copyright © 2001-2011 Free Software Foundation, Inc.\n", PACKAGE_NAME, PACKAGE_VERSION);
		handled_locally = TRUE;
	}

	g_strfreev (local_argv);

	return handled_locally;
}


/* -- application menu  -- */


static void
toggle_action_activated (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       data)
{
	GVariant *state;

	state = g_action_get_state (G_ACTION (action));
	g_action_change_state (G_ACTION (action), g_variant_new_boolean (! g_variant_get_boolean (state)));

	g_variant_unref (state);
}


static void
update_app_menu_sensitivity (GooApplication *application)
{
	GVariant *state;
	gboolean  play_all;

	state = g_action_get_state (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_PLAYALL));
	play_all = g_variant_get_boolean (state);
	g_variant_unref (state);

	g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_REPEAT)), play_all);
	g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_SHUFFLE)), play_all);
}


static void
activate_play_all (GSimpleAction *action,
		   GVariant      *parameter,
		   gpointer       user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (action, parameter);
	g_settings_set_boolean (application->settings, PREF_PLAYLIST_PLAYALL, g_variant_get_boolean (parameter));
	update_app_menu_sensitivity (application);
}


static void
activate_repeat (GSimpleAction *action,
		 GVariant      *parameter,
		 gpointer       user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (action, parameter);
	g_settings_set_boolean (application->settings, PREF_PLAYLIST_REPEAT, g_variant_get_boolean (parameter));
	update_app_menu_sensitivity (application);
}


static void
activate_shuffle (GSimpleAction *action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (action, parameter);
	g_settings_set_boolean (application->settings, PREF_PLAYLIST_SHUFFLE, g_variant_get_boolean (parameter));
	update_app_menu_sensitivity (application);
}


static void
activate_preferences (GSimpleAction *action,
		      GVariant      *parameter,
		      gpointer       user_data)
{
	GApplication *application = user_data;
	GList        *windows;

	windows = gtk_application_get_windows (GTK_APPLICATION (application));
	if (windows != NULL)
		activate_action_preferences (NULL, windows->data);
}


static void
activate_help (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
	GApplication *application = user_data;
	GList        *windows;

	windows = gtk_application_get_windows (GTK_APPLICATION (application));
	if (windows != NULL)
		activate_action_manual (NULL, windows->data);
}


static void
activate_about (GSimpleAction *action,
		GVariant      *parameter,
		gpointer       user_data)
{
	GApplication *application = user_data;
	GList        *windows;

	windows = gtk_application_get_windows (GTK_APPLICATION (application));
	if (windows != NULL)
		activate_action_about (NULL, windows->data);
}


static void
activate_quit (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
	GApplication *application = user_data;
	GList        *windows;

	windows = gtk_application_get_windows (GTK_APPLICATION (application));
	if (windows != NULL)
		activate_action_quit (NULL, windows->data);
}


static void
pref_playlist_playall_changed (GSettings  *settings,
	  	 	       const char *key,
	  	 	       gpointer    user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_PLAYALL)),
				   g_variant_new_boolean (g_settings_get_boolean (application->settings, PREF_PLAYLIST_PLAYALL)));
	update_app_menu_sensitivity (application);
}


static void
pref_playlist_repeat_changed (GSettings  *settings,
	  	 	      const char *key,
	  	 	      gpointer    user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_REPEAT)),
				   g_variant_new_boolean (g_settings_get_boolean (application->settings, PREF_PLAYLIST_REPEAT)));
	update_app_menu_sensitivity (application);
}


static void
pref_playlist_shuffle_changed (GSettings  *settings,
	  	 	       const char *key,
	  	 	       gpointer    user_data)
{
	GooApplication *application = user_data;

	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_SHUFFLE)),
				   g_variant_new_boolean (g_settings_get_boolean (application->settings, PREF_PLAYLIST_SHUFFLE)));
	update_app_menu_sensitivity (application);
}


static const GActionEntry app_menu_entries[] = {
	{ "preferences",  activate_preferences },
	{ PREF_PLAYLIST_PLAYALL, toggle_action_activated, NULL, "true", activate_play_all },
	{ PREF_PLAYLIST_REPEAT, toggle_action_activated, NULL, "false", activate_repeat },
	{ PREF_PLAYLIST_SHUFFLE, toggle_action_activated, NULL, "true", activate_shuffle },
	{ "help",  activate_help },
	{ "about", activate_about },
	{ "quit",  activate_quit }
};


static void
initialize_app_menu (GApplication *application)
{
	GooApplication *self = (GooApplication *) application;
	GtkBuilder     *builder;

	g_action_map_add_action_entries (G_ACTION_MAP (application),
					 app_menu_entries,
					 G_N_ELEMENTS (app_menu_entries),
					 application);

	builder = _gtk_builder_new_from_resource ("app-menu.ui");
	gtk_application_set_app_menu (GTK_APPLICATION (application),
				      G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));

	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_PLAYALL)),
				   g_variant_new_boolean (g_settings_get_boolean (self->settings, PREF_PLAYLIST_PLAYALL)));
	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_REPEAT)),
				   g_variant_new_boolean (g_settings_get_boolean (self->settings, PREF_PLAYLIST_REPEAT)));
	g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (application), PREF_PLAYLIST_SHUFFLE)),
				   g_variant_new_boolean (g_settings_get_boolean (self->settings, PREF_PLAYLIST_SHUFFLE)));

	g_signal_connect (self->settings,
			  "changed::" PREF_PLAYLIST_PLAYALL,
			  G_CALLBACK (pref_playlist_playall_changed),
			  self);
	g_signal_connect (self->settings,
			  "changed::" PREF_PLAYLIST_SHUFFLE,
			  G_CALLBACK (pref_playlist_shuffle_changed),
			  self);
	g_signal_connect (self->settings,
			  "changed::" PREF_PLAYLIST_REPEAT,
			  G_CALLBACK (pref_playlist_repeat_changed),
			  self);

	g_object_unref (builder);
}


static void
goo_application_startup (GApplication *application)
{
	G_APPLICATION_CLASS (goo_application_parent_class)->startup (application);
	initialize_app_menu (application);
}


static void
goo_application_class_init (GooApplicationClass *klass)
{
	GObjectClass      *object_class;
	GApplicationClass *application_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = goo_application_finalize;

	application_class = G_APPLICATION_CLASS (klass);
	application_class->activate = goo_application_activate;
	application_class->command_line = goo_application_command_line;
	application_class->local_command_line = goo_application_local_command_line;
	application_class->startup = goo_application_startup;
}


static GtkApplication *
goo_application_new (void)
{
	g_type_init ();

	return g_object_new (goo_application_get_type (),
	                     "application-id", "org.gnome.Goobox",
	                     "flags", 0,
	                     NULL);
}


int
main (int argc, char *argv[])
{
	GtkApplication *application;
	int             status;

	program_argv0 = argv[0];

	/* text domain */

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* run the main application */

	application = Main_Application = goo_application_new ();
	status = g_application_run (G_APPLICATION (application), argc, argv);

	g_object_unref (application);

	return status;
}


/* -- utility functions -- */


GtkWidget *
main_get_window_from_device (const char *device)
{
	GList *scan;

	if (device == NULL)
		return NULL;

	for (scan = gtk_application_get_windows (GTK_APPLICATION (g_application_get_default ())); scan; scan = scan->next) {
		GooWindow *window = scan->data;

		if (g_strcmp0 (goo_player_get_device (goo_window_get_player (window)), device) == 0)
			return (GtkWidget *) window;
	}

	return NULL;
}


BraseroDrive *
main_get_most_likely_drive (void)
{
	BraseroDrive         *result;
	BraseroMediumMonitor *monitor;
	GSList               *drivers;

	monitor = brasero_medium_monitor_get_default ();
	drivers = brasero_medium_monitor_get_drives (monitor, BRASERO_MEDIA_TYPE_AUDIO | BRASERO_MEDIA_TYPE_CD);
	if (drivers != NULL)
		result = g_object_ref ((BraseroDrive *) drivers->data);
	else
		result = NULL;

	g_slist_foreach (drivers, (GFunc) g_object_unref, NULL);
	g_slist_free (drivers);
	g_object_unref (monitor);

	return result;
}


BraseroDrive *
main_get_drive_for_device (const char *device)
{
	BraseroDrive         *result = NULL;
	BraseroMediumMonitor *monitor;

	monitor = brasero_medium_monitor_get_default ();
	result = brasero_medium_monitor_get_drive (monitor, device);
	g_object_unref (monitor);

	return result;
}


#ifdef ENABLE_NOTIFICATION


static gboolean
play_next (gpointer user_data)
{
	GooWindow *window = user_data;

	goo_window_next (window);

	return FALSE;
}


static void
notify_action_next_cb (NotifyNotification *notification,
                       char               *action,
                       gpointer            user_data)
{
	GooWindow *window = user_data;

	if (! notification_supports_persistence)
		notify_notification_close (notification, NULL);

	g_idle_add (play_next, window);
}


static gboolean
toggle_play (gpointer user_data)
{
	GooWindow *window = user_data;

	goo_window_toggle_play (window);

	return FALSE;
}


static void
notify_action_toggle_play_cb (NotifyNotification *notification,
			      char               *action,
			      gpointer            user_data)
{
	GooWindow *window = user_data;

	if (! notification_supports_persistence)
		notify_notification_close (notification, NULL);

	g_idle_add (toggle_play, window);
}


#endif /* ENABLE_NOTIFICATION */


gboolean
notification_has_persistence (void)
{
#ifdef ENABLE_NOTIFICATION

	gboolean  supports_persistence = FALSE;
	GList    *caps;

	caps = notify_get_server_caps ();
	if (caps != NULL) {
		supports_persistence = g_list_find_custom (caps, "persistence", (GCompareFunc) strcmp) != NULL;

		g_list_foreach (caps, (GFunc)g_free, NULL);
		g_list_free (caps);
	}

	return supports_persistence;

#else

	return FALSE;

#endif /* ENABLE_NOTIFICATION */
}


void
system_notify (GooWindow       *window,
	       const char      *summary,
	       const char      *body)
{
#ifdef ENABLE_NOTIFICATION

	GdkPixbuf *cover;

	if (! notify_is_initted ())
		return;

	if (notification == NULL) {
		GList *caps;

		notification_supports_actions = FALSE;
		notification_supports_persistence = FALSE;

		caps = notify_get_server_caps ();
		if (caps != NULL) {
			notification_supports_actions = g_list_find_custom (caps, "actions", (GCompareFunc) strcmp) != NULL;
			notification_supports_persistence = g_list_find_custom (caps, "persistence", (GCompareFunc) strcmp) != NULL;

			g_list_foreach (caps, (GFunc)g_free, NULL);
			g_list_free (caps);
		}

#if NOTIFY_CHECK_VERSION (0, 7, 0)
		notification = notify_notification_new (summary, body, "goobox");
#else
		notification = notify_notification_new_with_status_icon (summary, body, "goobox", status_icon);
#endif
		notify_notification_set_hint_string (notification, "desktop-entry", "goobox");
		notify_notification_set_urgency (notification, NOTIFY_URGENCY_LOW);
	}
	else
		notify_notification_update (notification, summary, body, "goobox");

	cover = goo_player_info_get_cover (GOO_PLAYER_INFO (goo_window_get_player_info (window)));
	notify_notification_set_image_from_pixbuf (notification, cover);

	if (notification_supports_actions) {
		notify_notification_clear_actions (notification);

		if (goo_player_get_state (goo_window_get_player (window)) == GOO_PLAYER_STATE_PLAYING)
			notify_notification_add_action (notification,
							GOO_STOCK_PAUSE,
							_("Pause"),
							notify_action_toggle_play_cb,
							window,
							NULL);
		else
			notify_notification_add_action (notification,
							GOO_STOCK_PLAY,
							_("Play"),
							notify_action_toggle_play_cb,
							window,
							NULL);

		notify_notification_add_action (notification,
						GOO_STOCK_NEXT,
						_("Next"),
						notify_action_next_cb,
						window,
						NULL);

		notify_notification_set_hint (notification,
					      "action-icons",
					      g_variant_new_boolean (TRUE));
	}

	if (notification_supports_persistence)
		notify_notification_set_hint (notification,
					      "resident" /* "transient" */,
					      g_variant_new_boolean (TRUE));

	notify_notification_show (notification, NULL);

#endif /* ENABLE_NOTIFICATION */
}
