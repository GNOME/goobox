/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004-2009 Free Software Foundation, Inc.
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
#include <brasero/brasero-medium-monitor.h>
#include <gst/gst.h>
#include <glib.h>
#include <unique/unique.h>
#include "eggsmclient.h"
#include "goo-stock.h"
#include "gconf-utils.h"
#include "goo-window.h"
#include "typedefs.h"
#include "preferences.h"
#include "main.h"
#include "gtk-utils.h"
#include "glib-utils.h"

#ifdef ENABLE_NOTIFICATION
#include <libnotify/notify.h>
static NotifyNotification *notification = NULL;
#endif /* ENABLE_NOTIFICATION */

#define VOLUME_STEP 0.10

enum {
	COMMAND_UNUSED,
	COMMAND_PLAY,
	COMMAND_PLAY_PAUSE,
	COMMAND_STOP,
	COMMAND_NEXT_TRACK,
	COMMAND_PREVIOUS_TRACK,
	COMMAND_EJECT_DISK,
	COMMAND_HIDE_SHOW,
	COMMAND_VOLUME_UP,
	COMMAND_vOLUME_DOWN,
	COMMAND_QUIT,
	COMMAND_PRESENT,
	COMMAND_SET_DEVICE
};

GtkWidget *main_window = NULL;
int        AutoPlay = FALSE;
int        PlayPause = FALSE;
int        Stop = FALSE;
int        Next = FALSE;
int        Prev = FALSE;
int        Eject = FALSE;
int        HideShow = FALSE;
int        VolumeUp = FALSE;
int        VolumeDown = FALSE;
int        Quit = FALSE;

static void release_data (void);

static UniqueApp  *application = NULL;
static const char *program_argv0; /* argv[0] from main(); used as the command to restart the program */
static char       *default_device = NULL;
static gboolean    version = FALSE;

static const GOptionEntry options[] = {
	{ "device", 'd',  0, G_OPTION_ARG_STRING, &default_device, 
	  N_("CD device to be used"),
	  N_("DEVICE_PATH") },
	{ "play", '\0', 0, G_OPTION_ARG_NONE, &AutoPlay,
          N_("Play the CD on startup"),
          0 },
	{ "play-pause", '\0', 0, G_OPTION_ARG_NONE, &PlayPause, 
          N_("Play/Pause"),
          0 },
        { "stop", '\0', 0, G_OPTION_ARG_NONE, &Stop, 
          N_("Stop playing"),
          0 },
	{ "next", '\0', 0, G_OPTION_ARG_NONE, &Next,
          N_("Play the next track"),
          0 },
	{ "previous", '\0', 0, G_OPTION_ARG_NONE, &Prev,
          N_("Play the previous track"),
          0 },
	{ "eject", '\0', 0, G_OPTION_ARG_NONE, &Eject,
          N_("Eject the CD"),
          0 },
	{ "hide-show", '\0', 0, G_OPTION_ARG_NONE, &HideShow,
          N_("Hide/Show the main window"),
          0 },
	{ "volume-up", '\0', 0, G_OPTION_ARG_NONE, &VolumeUp,
          N_("Volume Up"),
          0 },
	{ "volume-down", '\0', 0, G_OPTION_ARG_NONE, &VolumeDown,
          N_("Volume Down"),
          0 },
	{ "quit", '\0', 0, G_OPTION_ARG_NONE, &Quit,
          N_("Quit the application"),
          0 },

          { "version", 'v', 0, G_OPTION_ARG_NONE, &version,
  	  N_("Show version"), NULL },

	{ NULL }
};


/* session management */


static void
goo_save_state (EggSMClient *client,
		GKeyFile    *state,
		gpointer     user_data)
{
	const char *argv[2] = { NULL };

	argv[0] = program_argv0;
	argv[1] = NULL;
	egg_sm_client_set_restart_command (client, 1, argv);

	g_key_file_set_string (state, "Session", "/device", goo_player_get_device (goo_window_get_player (GOO_WINDOW (main_window))));
}


static void
goo_session_manager_init (void)
{
	EggSMClient *client = NULL;

	client = egg_sm_client_get ();
	g_signal_connect (client, "save-state", G_CALLBACK (goo_save_state), NULL);
}


static void
goo_restore_session (EggSMClient *client)
{
	GKeyFile     *state = NULL;
	char         *device;
	BraseroDrive *drive;

	state = egg_sm_client_get_state_file (client);

	device = g_key_file_get_string (state, "Session", "device", NULL);
	drive = main_get_drive_for_device (device);
	main_window = goo_window_new (drive);
	gtk_widget_show (main_window);

	g_object_unref (drive);
	g_free (device);
}


static UniqueResponse
unique_app_message_received_cb (UniqueApp         *unique_app,
				UniqueCommand      command,
				UniqueMessageData *message,
				guint              time_,
				gpointer           user_data)
{
	UniqueResponse  res;

	res = UNIQUE_RESPONSE_OK;

	switch (command) {
	case UNIQUE_OPEN:
	case UNIQUE_NEW:
		/* FIXME */
		break;

	case COMMAND_PLAY:
		goo_window_play (GOO_WINDOW (main_window));
		break;

	case COMMAND_PLAY_PAUSE:
		goo_window_toggle_play (GOO_WINDOW (main_window));
		break;

	case COMMAND_STOP:
		goo_window_stop (GOO_WINDOW (main_window));
		break;

	case COMMAND_NEXT_TRACK:
		goo_window_next (GOO_WINDOW (main_window));
		break;

	case COMMAND_PREVIOUS_TRACK:
		goo_window_prev (GOO_WINDOW (main_window));
		break;

	case COMMAND_EJECT_DISK:
		goo_window_eject (GOO_WINDOW (main_window));
		break;

	case COMMAND_HIDE_SHOW:
		goo_window_toggle_visibility (GOO_WINDOW (main_window));
		break;

	case COMMAND_VOLUME_UP:
		{
			double volume;

			volume = goo_window_get_volume (GOO_WINDOW (main_window));
			goo_window_set_volume (GOO_WINDOW (main_window), volume + VOLUME_STEP);
		}
		break;

	case COMMAND_vOLUME_DOWN:
		{
			double volume;

			volume = goo_window_get_volume (GOO_WINDOW (main_window));
			goo_window_set_volume (GOO_WINDOW (main_window), volume - VOLUME_STEP);
		}
		break;

	case COMMAND_QUIT:
		goo_window_close (GOO_WINDOW (main_window));
		break;

	case COMMAND_PRESENT:
		if (gtk_widget_get_visible (GTK_WIDGET (main_window)))
			gtk_window_present (GTK_WINDOW (main_window));
		else
			goo_window_toggle_visibility (GOO_WINDOW (main_window));
		break;

	case COMMAND_SET_DEVICE:
		{
			char *device;

			device = unique_message_data_get_text (message);
			if (*device == '\0')
				device = NULL;
			if (device != NULL) {
				BraseroDrive *drive;

				drive = main_get_drive_for_device (device);
				main_window = main_get_window_from_device (device);
				if (main_window == NULL)
					main_window = goo_window_new (drive);
				else
					goo_window_set_drive (GOO_WINDOW (main_window), drive);

				g_object_unref (drive);
			}
		}
		break;

	default:
		res = UNIQUE_RESPONSE_PASSTHROUGH;
		break;
	}

	return res;
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


static void
release_data (void)
{
	_g_object_unref (application);
	eel_global_client_free ();
}


static void
prepare_application (void)
{
	EggSMClient *client = NULL;

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

		return;
	}

	application = unique_app_new_with_commands ("org.gnome.goobox", NULL,
						    "auto-play", COMMAND_PLAY,
						    "play-pause", COMMAND_PLAY_PAUSE,
						    "stop", COMMAND_STOP,
						    "next", COMMAND_NEXT_TRACK,
						    "prev", COMMAND_PREVIOUS_TRACK,
						    "eject", COMMAND_EJECT_DISK,
						    "hide-show", COMMAND_HIDE_SHOW,
						    "volume-up", COMMAND_VOLUME_UP,
						    "volume-down", COMMAND_vOLUME_DOWN,
						    "quit", COMMAND_QUIT,
						    "present", COMMAND_PRESENT,
						    "set-device", COMMAND_SET_DEVICE,
						    NULL);

	if (unique_app_is_running (application)) {
		if (default_device != NULL) {
			UniqueMessageData *data;

			data = unique_message_data_new ();
			unique_message_data_set_text (data, default_device, -1);
			unique_app_send_message (application, COMMAND_SET_DEVICE, data);

			unique_message_data_free (data);
		}

		if (AutoPlay)
			unique_app_send_message (application, COMMAND_PLAY, NULL);
		else if (PlayPause)
			unique_app_send_message (application, COMMAND_PLAY_PAUSE, NULL);
		else if (Stop)
			unique_app_send_message (application, COMMAND_STOP, NULL);
		else if (Next)
			unique_app_send_message (application, COMMAND_NEXT_TRACK, NULL);
		else if (Prev)
			unique_app_send_message (application, COMMAND_PREVIOUS_TRACK, NULL);
		else if (Eject)
			unique_app_send_message (application, COMMAND_EJECT_DISK, NULL);
		else if (HideShow)
			unique_app_send_message (application, COMMAND_HIDE_SHOW, NULL);
		else if (VolumeUp)
			unique_app_send_message (application, COMMAND_VOLUME_UP, NULL);
		else if (VolumeDown)
			unique_app_send_message (application, COMMAND_vOLUME_DOWN, NULL);
		else if (Quit)
			unique_app_send_message (application, COMMAND_QUIT, NULL);

		return;
	}

	if (! unique_app_is_running (application)) {
	        g_set_application_name (_("CD Player"));
	        gtk_window_set_default_icon_name ("goobox");
	        goo_stock_init ();
		eel_gconf_monitor_add ("/apps/goobox");
		g_signal_connect (application,
				  "message-received",
				  G_CALLBACK (unique_app_message_received_cb),
				  NULL);
	}

	client = egg_sm_client_get ();
	if (egg_sm_client_is_resumed (client)) {
		goo_restore_session (client);
		return;
	}

	main_window = goo_window_new (NULL);
	gtk_widget_show (main_window);
}


int main (int argc, char **argv)
{
	char           *description;
	GOptionContext *context = NULL;
	GError         *error = NULL;

	program_argv0 = argv[0];

	if (! g_thread_supported ()) {
		g_thread_init (NULL);
		gdk_threads_init ();
	}

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	
        description = g_strdup_printf ("- %s", _("Play CDs and save the tracks to disk as files"));
        context = g_option_context_new (description);
        g_free (description);
        g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
        g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context, egg_sm_client_get_option_group ());
	g_option_context_add_group (context, gst_init_get_option_group ());
  	g_option_context_set_ignore_unknown_options (context, TRUE);
	if (! g_option_context_parse (context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}
	g_option_context_free (context);

	if (version) {
		g_print ("%s %s, Copyright (C) 2004-2009 Free Software Foundation, Inc.\n", PACKAGE_NAME, PACKAGE_VERSION);
		return 0;
	}

#ifdef ENABLE_NOTIFICATION
	if (! notify_init ("goobox"))
                g_warning ("Cannot initialize notification system.");
#endif /* ENABLE_NOTIFICATION */

	goo_session_manager_init ();
	prepare_application ();

	if ((application == NULL) || ! unique_app_is_running (application)) {
		gdk_threads_enter ();
		gtk_main ();
		gdk_threads_leave ();
	}

	release_data ();
	
	return 0;
}


GtkWidget *
main_get_window_from_device (const char *device)
{
	GList *scan;
	
	if (device == NULL)
		return NULL;
		
	for (scan = window_list; scan; scan = scan->next) {
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

		notification = notify_notification_new_with_status_icon (title, msg, "goobox", status_icon);
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
