/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-propertybox.h>
#include <libgnomeui/gnome-pixmap.h>
#include <glade/glade.h>
#include "main.h"
#include "gconf-utils.h"
#include "typedefs.h"
#include "goo-window.h"
#include "preferences.h"
#include "bacon-cd-selection.h"

#define GLADE_PREF_FILE "goobox.glade"

typedef struct {
	GooWindow *window;

	GladeXML  *gui;

	GtkWidget *dialog;
	GtkWidget *drive_selector;
} DialogData;


/* called when the "apply" button is clicked. */
static void
apply_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	const char *device;

	device = bacon_cd_selection_get_device (BACON_CD_SELECTION (data->drive_selector));
	if (device != NULL) 
		eel_gconf_set_string (PREF_GENERAL_DEVICE, device);
	goo_window_set_location (data->window, device);
	goo_window_update (data->window);
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	apply_cb (widget, data);
	g_object_unref (G_OBJECT (data->gui));
	g_free (data);
}


/* called when the "close" button is clicked. */
static void
close_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	apply_cb (widget, data);
	gtk_widget_destroy (data->dialog);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("goo", "preferences", &err);
	
	if (err != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
						 0,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Could not display help: %s"),
						 err->message);
		
		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		
		gtk_widget_show (dialog);
		
		g_error_free (err);
	}
}


static void
drive_selector_device_changed_cb (GtkOptionMenu *option_menu,
				  const char    *device_path,
				  DialogData    *data)
{
	apply_cb (NULL, data);
}





/* create the main dialog. */
void
dlg_preferences (GooWindow *window)
{
	DialogData       *data;
	GtkWidget        *btn_close;
	GtkWidget        *btn_help;
	GtkWidget        *box;
	char             *device = NULL;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_PREF_FILE, NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_PREF_FILE "\n");
		g_free (data);
                return;
        }

	eel_gconf_preload_cache ("/apps/goo/general", GCONF_CLIENT_PRELOAD_ONELEVEL);

	goo_window_set_location (data->window, NULL);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "preferences_dialog");

	box = glade_xml_get_widget (data->gui, "p_drive_selector_box");
	btn_close  = glade_xml_get_widget (data->gui, "p_closebutton");
	btn_help   = glade_xml_get_widget (data->gui, "p_helpbutton");

	/* Set widgets data. */

	data->drive_selector = bacon_cd_selection_new ();
	gtk_box_pack_start (GTK_BOX (box), data->drive_selector, TRUE, TRUE, 0);
	
	device = eel_gconf_get_string (PREF_GENERAL_DEVICE, bacon_cd_selection_get_default_device (BACON_CD_SELECTION (data->drive_selector)));
	bacon_cd_selection_set_device (BACON_CD_SELECTION (data->drive_selector), device);
	g_free (device);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (btn_close), 
			  "clicked",
			  G_CALLBACK (close_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);

	g_signal_connect (G_OBJECT (data->drive_selector), 
			  "device_changed",
			  G_CALLBACK (drive_selector_device_changed_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
}
