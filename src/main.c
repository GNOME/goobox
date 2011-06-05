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
#include <brasero3/brasero-medium-monitor.h>
#include <gst/gst.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "eggsmclient.h"
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
#endif /* ENABLE_NOTIFICATION */


#define VOLUME_STEP 0.10


/* -- command line arguments -- */


GtkApplication        *main_application = NULL;
int                    arg_auto_play = FALSE;
int                    arg_toggle_visibility = FALSE;
static int             arg_toggle_play = FALSE;
static int             arg_stop = FALSE;
static int             arg_next = FALSE;
static int             arg_prev = FALSE;
static int             arg_eject = FALSE;
static int             arg_volume_up = FALSE;
static int             arg_volume_down = FALSE;
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
	{ "volume-up", '\0', 0, G_OPTION_ARG_NONE, &arg_volume_up,
          N_("Volume Up"),
          0 },
	{ "volume-down", '\0', 0, G_OPTION_ARG_NONE, &arg_volume_down,
          N_("Volume Down"),
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


typedef GtkApplication      GooApplication;
typedef GtkApplicationClass GooApplicationClass;


G_DEFINE_TYPE (GooApplication, goo_application, GTK_TYPE_APPLICATION)


static void
goo_application_finalize (GObject *object)
{
	G_OBJECT_CLASS (goo_application_parent_class)->finalize (object);
}


static void
goo_application_init (GooApplication *app)
{
	g_set_application_name (_("CD Player"));
}


static void
goo_application_activate (GApplication *application)
{
	GtkWidget *window;

	window = goo_window_new (NULL);
	gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (application));
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
	GtkSettings *gtk_settings;
	EggSMClient *client = NULL;

        gtk_settings = gtk_settings_get_default ();
        g_object_set (G_OBJECT (gtk_settings), "gtk-application-prefer-dark-theme", TRUE, NULL);

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
		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (gtk_main_quit),
				  NULL);
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
	else if (arg_volume_up) {
		double volume;

		volume = goo_window_get_volume (GOO_WINDOW (window));
		goo_window_set_volume (GOO_WINDOW (window), volume + VOLUME_STEP);
	}
	else if (arg_volume_down) {
		double volume;

		volume = goo_window_get_volume (GOO_WINDOW (window));
		goo_window_set_volume (GOO_WINDOW (window), volume - VOLUME_STEP);
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
	arg_volume_up = FALSE;
	arg_volume_down = FALSE;
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

	if (! g_thread_supported ()) {
		g_thread_init (NULL);
		gdk_threads_init ();
	}

	/* text domain */

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* run the main application */

	application = main_application = goo_application_new ();
	gdk_threads_enter ();
	status = g_application_run (G_APPLICATION (application), argc, argv);
	gdk_threads_leave ();
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

	for (scan = gtk_application_get_windows (main_application); scan; scan = scan->next) {
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

	notify_notification_close (notification, NULL);
	g_idle_add (play_next, window);
}


static void
notify_action_stop_cb (NotifyNotification *notification,
                       char               *action,
                       gpointer            user_data)
{
	GooWindow *window = user_data;

	goo_window_stop (window);
	notify_notification_close (notification, NULL);
}


#endif /* ENABLE_NOTIFICATION */


void
system_notify (GooWindow  *window,
	       const char *title,
	       const char *msg)
{
#ifdef ENABLE_NOTIFICATION

	GtkStatusIcon *status_icon;
	GdkScreen     *screen = NULL;
	int            x = -1, y = -1;

	if (! notify_is_initted ())
		return;

	status_icon = goo_window_get_status_icon (window);
	if (status_icon != NULL) {
		GdkRectangle area;

		if (gtk_status_icon_get_geometry (status_icon, &screen, &area, NULL)) {
			y = area.y + area.height;
			x = area.x + (area.width / 2);
		}
	}

	if (notification == NULL) {
		gboolean  supports_actions;
		GList    *caps;

		supports_actions = FALSE;
		caps = notify_get_server_caps ();
		if (caps != NULL) {
			GList *c;

			for (c = caps; c != NULL; c = c->next) {
				if (strcmp ((char*)c->data, "actions") == 0) {
					supports_actions = TRUE;
					break;
				}
			}

			g_list_foreach (caps, (GFunc)g_free, NULL);
			g_list_free (caps);
		}

#if NOTIFY_CHECK_VERSION (0, 7, 0)
		notification = notify_notification_new (title, msg, "goobox");
#else
		notification = notify_notification_new_with_status_icon (title, msg, "goobox", status_icon);
#endif
		notify_notification_set_urgency (notification, NOTIFY_URGENCY_LOW);

		if (supports_actions) {
			notify_notification_add_action (notification,
							GTK_STOCK_MEDIA_NEXT,
							_("Next"),
							notify_action_next_cb,
							window,
							NULL);
			notify_notification_add_action (notification,
							GTK_STOCK_MEDIA_STOP,
							_("Stop"),
							notify_action_stop_cb,
							window,
							NULL);
		}
	}
	else
		notify_notification_update (notification, title, msg, "goobox");

	/*if ((x >= 0) && (y >= 0))
		notify_notification_set_geometry_hints (notification,
							screen,
							x, y);*/

	notify_notification_show (notification, NULL);

#endif /* ENABLE_NOTIFICATION */
}
