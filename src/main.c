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


static void     prepare_app         (poptContext pctx);
static void     initialize_data     (void);
static void     release_data        (void);

static void     init_session        (char **argv);
static gboolean session_is_restored (void);
static gboolean load_session        (void);
static BonoboObject *goo_application = NULL;

GtkWindow *main_window = NULL;

static char *default_device = NULL;


struct poptOption options[] = {
	{ "device", 'd', POPT_ARG_STRING, &default_device, 0,
	  N_("CD device to be used"),
	  N_("DEVICE_PATH") },
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
	char *icon_path = PIXMAPSDIR "/goobox.png";

	if (! path_is_file (icon_path))
                g_warning ("Could not find %s", icon_path);
	else
		gnome_window_icon_set_default_from_file (icon_path);

	eel_gconf_monitor_add ("/apps/goobox");
}


static void 
release_data ()
{
	if (goo_application != NULL)
		bonobo_object_unref (goo_application);
	eel_global_client_free ();
}


/* Create the windows. */


static gboolean
check_plugins (void)
{
	GstElement *cdparanoia;
	gboolean    result;

	cdparanoia = gst_element_factory_make ("cdparanoia", "cdreader");
	result = (cdparanoia != NULL);
	if (cdparanoia != NULL) 
		gst_object_unref (GST_OBJECT (cdparanoia));

	return result;
}


static void 
prepare_app (poptContext pctx)
{
	if (!check_plugins()) {
		GtkWidget *d;
		d = _gtk_message_dialog_new (NULL,
					     0,
					     GTK_STOCK_DIALOG_ERROR,
					     _("Cannot start the CD player"),
					     _("In order to read CDs you have to install the cdparanoia gstreamer plugin"),
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					     NULL);
		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (bonobo_main_quit),
				  NULL);
		gtk_widget_show (d);

		return;
	}

	if (session_is_restored ()) 
		load_session ();
	else 
		main_window = goo_window_new (default_device);
	gtk_widget_show (GTK_WIDGET (main_window));

	goo_application = goo_application_new (gdk_screen_get_default ());
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
