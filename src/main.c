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
#include <libbonobo.h>
#include <libgnomeui/gnome-window-icon.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>
#include <gst/gst.h>
#include "file-utils.h"
#include "goo-stock.h"
#include "gconf-utils.h"
#include "goo-window.h"
#include "typedefs.h"
#include "preferences.h"
#include "main.h"
#include "goo-application.h"
#include "gtk-utils.h"
#include "glib-utils.h"

#ifdef HAVE_MMKEYS
#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <gdk/gdkx.h>
#endif /* HAVE_MMKEYS */

#ifdef HAVE_LIBNOTIFY

#include <libnotify/notify.h>
static NotifyNotification *notification = NULL;

#endif /* HAVE_LIBNOTIFY */

GtkWindow *main_window = NULL;
int        AutoPlay = FALSE;
int        PlayPause = FALSE;
int        Next = FALSE;
int        Prev = FALSE;
int        Eject = FALSE;
int        HideShow = FALSE;
int        VolumeUp = FALSE;
int        VolumeDown = FALSE;
int        Quit = FALSE;

static void     prepare_app         (poptContext pctx);
static void     initialize_data     (void);
static void     release_data        (void);

static void     init_session        (char **argv);
static gboolean session_is_restored (void);
static gboolean load_session        (void);
static void     init_mmkeys         (void);

static char         *default_device = NULL;
static BonoboObject *goo_application = NULL;

struct poptOption options[] = {
	{ "device", 'd', POPT_ARG_STRING, &default_device, 0,
	  N_("CD device to be used"),
	  N_("DEVICE_PATH") },
	{ "play", '\0', POPT_ARG_NONE, &AutoPlay, 0,
          N_("Play the CD on startup"),
          0 },
	{ "play-pause", '\0', POPT_ARG_NONE, &PlayPause, 0,
          N_("Play/Pause"),
          0 },
	{ "next", '\0', POPT_ARG_NONE, &Next, 0,
          N_("Play the next track"),
          0 },
	{ "previous", '\0', POPT_ARG_NONE, &Prev, 0,
          N_("Play the previous track"),
          0 },
	{ "eject", '\0', POPT_ARG_NONE, &Eject, 0,
          N_("Eject the CD"),
          0 },
	{ "hide-show", '\0', POPT_ARG_NONE, &HideShow, 0,
          N_("Hide/Show the main window"),
          0 },
	{ "volume-up", '\0', POPT_ARG_NONE, &VolumeUp, 0,
          N_("Volume Up"),
          0 },
	{ "volume-down", '\0', POPT_ARG_NONE, &VolumeDown, 0,
          N_("Volume Down"),
          0 },
	{ "quit", '\0', POPT_ARG_NONE, &Quit, 0,
          N_("Quit the application"),
          0 },
	{ NULL, '\0', 0, NULL, 0 }
};


/* -- Main -- */

int main (int argc, char **argv)
{
	GnomeProgram *program;
	GValue value = { 0 };
	poptContext pctx;
	CORBA_Object factory;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

 	program = gnome_program_init ("goobox", VERSION,
				      LIBGNOMEUI_MODULE, argc, argv,
				      GNOME_PARAM_POPT_TABLE, options,
				      GNOME_PARAM_HUMAN_READABLE_NAME, _("CD player and ripper"),
				      GNOME_PARAM_APP_PREFIX, GOO_PREFIX,
                                      GNOME_PARAM_APP_SYSCONFDIR, GOO_SYSCONFDIR,
                                      GNOME_PARAM_APP_DATADIR, GOO_DATADIR,
                                      GNOME_PARAM_APP_LIBDIR, GOO_LIBDIR,
				      NULL);

	g_object_get_property (G_OBJECT (program),
			       GNOME_PARAM_POPT_CONTEXT,
			       g_value_init (&value, G_TYPE_POINTER));
	pctx = g_value_get_pointer (&value);

	factory = bonobo_activation_activate_from_id ("OAFIID:GNOME_Goobox_Application_Factory",
						      Bonobo_ACTIVATION_FLAG_EXISTING_ONLY,
						      NULL, NULL);

	if (factory != NULL) {
		CORBA_Environment     env;
		GNOME_Goobox_Application app;

		CORBA_exception_init (&env);

		app = bonobo_activation_activate_from_id ("OAFIID:GNOME_Goobox_Application", 0, NULL, &env);

                if (AutoPlay)
                	GNOME_Goobox_Application_play (app, &env);
		else if (PlayPause)
                	GNOME_Goobox_Application_play_pause (app, &env);
		else if (Next)
                	GNOME_Goobox_Application_next (app, &env);
		else if (Prev)
                	GNOME_Goobox_Application_prev (app, &env);
		else if (Eject)
                	GNOME_Goobox_Application_eject (app, &env);
		else if (HideShow)
                	GNOME_Goobox_Application_hide_show (app, &env);
		else if (VolumeUp)
                	GNOME_Goobox_Application_volume_up (app, &env);
		else if (VolumeDown)
                	GNOME_Goobox_Application_volume_down (app, &env);
		else if (Quit)
                	GNOME_Goobox_Application_quit (app, &env);
		else
			GNOME_Goobox_Application_present (app, &env);

		bonobo_object_release_unref (app, &env);
		CORBA_exception_free (&env);

		gdk_notify_startup_complete ();

		exit (0);
	}

	if (! g_thread_supported ()) {
		g_thread_init (NULL);
		gdk_threads_init ();
	}

	if (! gnome_vfs_init ()) 
                g_error ("Cannot initialize the Virtual File System.");

	glade_gnome_init ();
	
	gst_init (NULL, NULL);
	/*gst_use_threads (TRUE);*/


#ifdef HAVE_LIBNOTIFY
	if (! notify_init ("goobox")) 
                g_warning ("Cannot initialize notification system.");
#endif /* HAVE_LIBNOTIFY */

	goo_stock_init ();
	init_session (argv);
	initialize_data ();
	prepare_app (pctx);
	poptFreeContext (pctx);

	bonobo_main ();

	release_data ();

	return 0;
}


/* Initialize application data. */


static void 
initialize_data ()
{
        g_set_application_name (_("Goobox"));
        gtk_window_set_default_icon_name ("goobox");

	eel_gconf_monitor_add ("/apps/goobox");
}


static void 
release_data (void)
{
	if (goo_application != NULL)
		bonobo_object_unref (goo_application);
	eel_global_client_free ();
}


/* Create the windows. */


static void 
prepare_app (poptContext pctx)
{
	if (session_is_restored ()) 
		load_session ();
	else 
		main_window = goo_window_new (default_device);
	gtk_widget_show (GTK_WIDGET (main_window));

	goo_application = goo_application_new (gdk_screen_get_default ());

#ifdef HAVE_MMKEYS
	init_mmkeys ();
#endif /* HAVE_MMKEYS */
}


/* SM support */


/* The master client we use for SM */
static GnomeClient *master_client = NULL;

/* argv[0] from main(); used as the command to restart the program */
static const char *program_argv0 = NULL;


static void
save_session (GnomeClient *client)
{
	const char  *prefix;

	prefix = gnome_client_get_config_prefix (client);
	gnome_config_push_prefix (prefix);

	gnome_config_set_string ("Session/location", goo_player_get_location (goo_window_get_player (GOO_WINDOW (main_window))));

	gnome_config_pop_prefix ();
	gnome_config_sync ();
}


/* save_yourself handler for the master client */
static gboolean
client_save_yourself_cb (GnomeClient *client,
			 gint phase,
			 GnomeSaveStyle save_style,
			 gboolean shutdown,
			 GnomeInteractStyle interact_style,
			 gboolean fast,
			 gpointer data)
{
	const char *prefix;
	char       *argv[4] = { NULL };

	save_session (client);

	prefix = gnome_client_get_config_prefix (client);

	/* Tell the session manager how to discard this save */

	argv[0] = "rm";
	argv[1] = "-rf";
	argv[2] = gnome_config_get_real_path (prefix);
	argv[3] = NULL;
	gnome_client_set_discard_command (client, 3, argv);

	/* Tell the session manager how to clone or restart this instance */

	argv[0] = (char *) program_argv0;
	argv[1] = NULL; /* "--debug-session"; */
	
	gnome_client_set_clone_command (client, 1, argv);
	gnome_client_set_restart_command (client, 1, argv);

	return TRUE;
}

/* die handler for the master client */
static void
client_die_cb (GnomeClient *client, gpointer data)
{
	if (! client->save_yourself_emitted)
		save_session (client);
	if (goo_application != NULL)
		bonobo_object_unref (goo_application);
	bonobo_main_quit ();
}


static void
init_session (char **argv)
{
	if (master_client != NULL)
		return;

	program_argv0 = argv[0];

	master_client = gnome_master_client ();

	g_signal_connect (master_client, "save_yourself",
			  G_CALLBACK (client_save_yourself_cb),
			  NULL);

	g_signal_connect (master_client, "die",
			  G_CALLBACK (client_die_cb),
			  NULL);
}


gboolean
session_is_restored (void)
{
	gboolean restored;
	
	if (! master_client)
		return FALSE;

	restored = (gnome_client_get_flags (master_client) & GNOME_CLIENT_RESTORED) != 0;

	return restored;
}


gboolean
load_session (void)
{
	char *location;

	gnome_config_push_prefix (gnome_client_get_config_prefix (master_client));

	location = gnome_config_get_string ("Session/location");
	main_window = goo_window_new (location);
	g_free (location);

	gnome_config_pop_prefix ();

	return TRUE;
}


/* From rhythmbox/shell/rb-shell-player.c
 *
 *  Copyright (C) 2002, 2003 Jorn Baayen <jorn@nl.linux.org>
 *  Copyright (C) 2002,2003 Colin Walters <walters@debian.org>
 *
 *  Modified by Paolo Bacchilega for Goobox
 *
 *  Copyright (C) 2005 Paolo Bacchilega <paobac@cvs.gnome.org>
 */

#ifdef HAVE_MMKEYS

static void
grab_mmkey (int        key_code, 
	    GdkWindow *root)
{
	gdk_error_trap_push ();

	XGrabKey (GDK_DISPLAY (), key_code,
		  0,
		  GDK_WINDOW_XID (root), True,
		  GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
		  Mod2Mask,
		  GDK_WINDOW_XID (root), True,
		  GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
		  Mod5Mask,
		  GDK_WINDOW_XID (root), True,
		  GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
		  LockMask,
		  GDK_WINDOW_XID (root), True,
		  GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
		  Mod2Mask | Mod5Mask,
		  GDK_WINDOW_XID (root), True,
		  GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
		  Mod2Mask | LockMask,
		  GDK_WINDOW_XID (root), True,
		  GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
		  Mod5Mask | LockMask,
		  GDK_WINDOW_XID (root), True,
		  GrabModeAsync, GrabModeAsync);
	XGrabKey (GDK_DISPLAY (), key_code,
		  Mod2Mask | Mod5Mask | LockMask,
		  GDK_WINDOW_XID (root), True,
		  GrabModeAsync, GrabModeAsync);
	
	gdk_flush ();
        if (gdk_error_trap_pop ()) 
		debug (DEBUG_INFO, "Error grabbing key");
}


static GdkFilterReturn
filter_mmkeys (GdkXEvent *xevent, 
	       GdkEvent  *event, 
	       gpointer   data)
{
	XEvent    *xev;
	XKeyEvent *key;

	xev = (XEvent *) xevent;
	if (xev->type != KeyPress) 
		return GDK_FILTER_CONTINUE;

	key = (XKeyEvent *) xevent;

	if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPlay) == key->keycode) {	
		goo_window_toggle_play (GOO_WINDOW (main_window));
		return GDK_FILTER_REMOVE;
	} 
	else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPause) == key->keycode) {	
		goo_window_pause (GOO_WINDOW (main_window));
		return GDK_FILTER_REMOVE;
	} 
	else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop) == key->keycode) {
		goo_window_stop (GOO_WINDOW (main_window));
		return GDK_FILTER_REMOVE;		
	} 
	else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev) == key->keycode) {
		goo_window_prev (GOO_WINDOW (main_window));
		return GDK_FILTER_REMOVE;		
	} 
	else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext) == key->keycode) {
		goo_window_next (GOO_WINDOW (main_window));
		return GDK_FILTER_REMOVE;
	} 
	else if (XKeysymToKeycode (GDK_DISPLAY (), XF86XK_Eject) == key->keycode) {
		goo_window_eject (GOO_WINDOW (main_window));
		return GDK_FILTER_REMOVE;
	} 
	else 
		return GDK_FILTER_CONTINUE;
}


static void
init_mmkeys (void)
{
	gint        keycodes[] = {0, 0, 0, 0, 0, 0};
	GdkDisplay *display;
	GdkScreen  *screen;
	GdkWindow  *root;
	guint       i, j;

	keycodes[0] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPlay);
	keycodes[1] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioStop);
	keycodes[2] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPrev);
	keycodes[3] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioNext);
	keycodes[4] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_AudioPause);
	keycodes[5] = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_Eject);

	display = gdk_display_get_default ();

	for (i = 0; i < gdk_display_get_n_screens (display); i++) {
		screen = gdk_display_get_screen (display, i);

		if (screen != NULL) {
			root = gdk_screen_get_root_window (screen);

			for (j = 0; j < G_N_ELEMENTS (keycodes) ; j++) {
				if (keycodes[j] != 0)
					grab_mmkey (keycodes[j], root);
			}

			gdk_window_add_filter (root, filter_mmkeys, NULL);
		}
	}
}

#endif /* HAVE_MMKEYS */


void 
system_notify (const char *title,
	       const char *msg,
	       int         x,
	       int         y)
{
#ifdef HAVE_LIBNOTIFY

	if (! notify_is_initted())
		return;

	if (notification == NULL) 
		notification = 	notify_notification_new (title, msg, "goobox", NULL);
	else
		notify_notification_update (notification, title, msg, "goobox");
	
	if ((x >= 0) && (y >= 0)) 
		notify_notification_set_geometry_hints (notification,
							NULL,
							x, y);

	notify_notification_show (notification, NULL);
	
#endif /* HAVE_LIBNOTIFY */
}
