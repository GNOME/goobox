/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004-2009 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either arg_version 2 of the License, or
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
#include <stdlib.h>
#include <brasero3/brasero-medium-monitor.h>
#include <gst/gst.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "goo-application.h"
#include "goo-player-info.h"
#include "goo-window.h"
#include "gtk-utils.h"
#include "main.h"
#include "preferences.h"
#include "typedefs.h"
#ifdef ENABLE_NOTIFICATION
#include <libnotify/notify.h>
#ifndef NOTIFY_CHECK_VERSION
#define NOTIFY_CHECK_VERSION(x,y,z) 0
#endif
static NotifyNotification *notification = NULL;
static gboolean            notification_supports_persistence = FALSE;
static gboolean            notification_supports_actions = FALSE;
#endif /* ENABLE_NOTIFICATION */


GtkApplication *Main_Application = NULL;
int	        arg_auto_play = FALSE;
int	        arg_toggle_visibility = FALSE;


int
main (int argc, char *argv[])
{
	int status;

	/* text domain */

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* run the main application */

	Main_Application = goo_application_new ();

#ifdef ENABLE_NOTIFICATION
	if (! notify_init (g_get_application_name ()))
                g_warning ("Cannot initialize notification system.");
#endif /* ENABLE_NOTIFICATION */

	status = g_application_run (G_APPLICATION (Main_Application), argc, argv);

	g_object_unref (Main_Application);

	return status;
}


/* -- utilities -- */


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
		gboolean rtl;

		notify_notification_clear_actions (notification);

		rtl = gtk_widget_get_direction (GTK_WIDGET (window)) == GTK_TEXT_DIR_RTL;

		if (goo_player_get_state (goo_window_get_player (window)) == GOO_PLAYER_STATE_PLAYING)
			notify_notification_add_action (notification,
							GOO_ICON_NAME_PAUSE,
							_("Pause"),
							notify_action_toggle_play_cb,
							window,
							NULL);
		else
			notify_notification_add_action (notification,
							rtl ? GOO_ICON_NAME_PLAY : GOO_ICON_NAME_PLAY,
							_("Play"),
							notify_action_toggle_play_cb,
							window,
							NULL);

		notify_notification_add_action (notification,
						rtl ? GOO_ICON_NAME_NEXT_RTL : GOO_ICON_NAME_NEXT,
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
