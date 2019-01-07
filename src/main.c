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


GtkApplication *Main_Application = NULL;
int	        arg_auto_play = FALSE;
int	        arg_toggle_visibility = FALSE;


int
main (int argc, char *argv[])
{
	int status;

	/* text domain */

	bindtextdomain (GETTEXT_PACKAGE, GOO_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* run the main application */

	Main_Application = goo_application_new ();
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


void
system_notify (GooWindow       *window,
	       const char      *id,
	       const char      *summary,
	       const char      *body)
{
	GNotification *notification;
	const char    *cover_path;

	notification = g_notification_new (summary);
	g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_LOW);
	if (body != NULL)
		g_notification_set_body (G_NOTIFICATION (notification), body);

	/* cover */

	cover_path = goo_player_info_get_cover_file (GOO_PLAYER_INFO (goo_window_get_player_info (window)));
	if (cover_path != NULL) {
		GFile *cover_file;
		GIcon *cover_icon;

		cover_file = g_file_new_for_path (cover_path);
		cover_icon = g_file_icon_new (cover_file);
		g_notification_set_icon (G_NOTIFICATION (notification), cover_icon);

		g_object_unref (cover_icon);
		g_object_unref (cover_file);
	}

	/* actions */

	if (g_strcmp0 (id, "new-track") == 0) {
		const char *device_id;

		device_id = goo_player_get_device (goo_window_get_player (window));
		if (goo_player_get_state (goo_window_get_player (window)) == GOO_PLAYER_STATE_PLAYING)
			g_notification_add_button_with_target (G_NOTIFICATION (notification), _("Pause"), "app.pause", "s", device_id);
		else
			g_notification_add_button_with_target (G_NOTIFICATION (notification), _("Play"), "app.play", "s", device_id);
		g_notification_add_button_with_target (G_NOTIFICATION (notification), _("Next"), "app.play-next", "s", device_id);
	}
	else if (g_strcmp0 (id, "new-album") == 0) {
		const char *device_id;

		device_id = goo_player_get_device (goo_window_get_player (window));
		g_notification_add_button_with_target (G_NOTIFICATION (notification), _("Play"), "app.play-next", "s", device_id);
	}

	/* send */

	g_application_send_notification (G_APPLICATION (gtk_window_get_application (GTK_WINDOW (window))), "cd-info", notification);

	g_object_unref (notification);
}
