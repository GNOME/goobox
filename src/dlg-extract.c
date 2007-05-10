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
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>
#include <gst/gst.h>
#include "typedefs.h"
#include "main.h"
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "typedefs.h"
#include "goo-stock.h"
#include "goo-window.h"
#include "preferences.h"
#include "track-info.h"
#include "goo-player.h"
#include "dlg-ripper.h"
#include "file-utils.h"

#define GLADE_EXTRACT_FILE "goobox.glade"

typedef struct {
	GooWindow   *window;
	GooPlayer   *player_cd;
	GList       *tracks;
	GList       *selected_tracks;

	GladeXML    *gui;

	GtkWidget   *dialog;
	GtkWidget   *e_alltrack_radiobutton;
	GtkWidget   *e_selected_radiobutton;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	track_list_free (data->selected_tracks);
	track_list_free (data->tracks);
	g_object_unref (data->player_cd);
	g_object_unref (data->gui);
	g_free (data);
}


static gboolean
dlg_extract__start_ripping (gpointer callback_data)
{
	GtkWidget  *dialog = callback_data;
	DialogData *data;
	GList      *tracks_to_rip;

	data = g_object_get_data (G_OBJECT (dialog), "dialog_data");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_selected_radiobutton)))
		tracks_to_rip = track_list_dup (data->selected_tracks);
	else
		tracks_to_rip = track_list_dup (goo_player_get_album (data->player_cd)->tracks);

	dlg_ripper (data->window, tracks_to_rip);
	
	track_list_free (tracks_to_rip);
	gtk_widget_destroy (data->dialog);
	
	return FALSE;
}


/* called when the "ok" button is clicked. */
static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_hide (data->dialog); 

	g_idle_add (dlg_extract__start_ripping, data->dialog);		
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("goobox", "extract", &err);
	
	if (err != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
						 GTK_DIALOG_MODAL,
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


/* create the main dialog. */
void
dlg_extract_ask (GooWindow *window)
{
	GstElement       *encoder;
	gboolean          ogg_encoder, flac_encoder, wave_encoder;
	DialogData       *data;
	GtkWidget        *btn_ok;
	GtkWidget        *btn_cancel;
	GtkWidget        *btn_help;
	int               selected;

	encoder = gst_element_factory_make (OGG_ENCODER, "encoder");
	ogg_encoder = encoder != NULL;
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (FLAC_ENCODER, "encoder");
	flac_encoder = encoder != NULL;
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (WAVE_ENCODER, "encoder");
	wave_encoder = encoder != NULL;
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	if (!ogg_encoder && !flac_encoder && !wave_encoder) {
		GtkWidget *d;
		char      *msg;
		
		msg = g_strdup_printf (_("You need at least one of the following GStreamer plugins in order to extract CD tracks:\n\n"
					 "\342\200\242 %s \342\206\222 Ogg Vorbis\n"
					 "\342\200\242 %s \342\206\222 FLAC\n"
					 "\342\200\242 %s \342\206\222 Waveform PCM"),
				       OGG_ENCODER,
				       FLAC_ENCODER,
				       WAVE_ENCODER);

		d = _gtk_message_dialog_new (GTK_WINDOW (window),
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_DIALOG_ERROR,
					     _("No encoder available."),
					     msg,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     NULL);
		g_free (msg);

		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		gtk_widget_show (d);

		return;
	}

	/**/

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_EXTRACT_FILE, NULL, NULL);
        if (!data->gui) {
		g_warning ("Could not find " GLADE_EXTRACT_FILE "\n");
		g_free (data);
                return;
        }

	data->tracks = goo_window_get_tracks (window, FALSE);
	data->selected_tracks = goo_window_get_tracks (window, TRUE);
	data->player_cd = goo_window_get_player (window);
	g_object_ref (data->player_cd);

	eel_gconf_preload_cache ("/apps/goobox/dialogs/extract", GCONF_CLIENT_PRELOAD_ONELEVEL);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "extract_dialog");
	data->e_alltrack_radiobutton = glade_xml_get_widget (data->gui, "e_alltrack_radiobutton");
	data->e_selected_radiobutton = glade_xml_get_widget (data->gui, "e_selected_radiobutton");

	btn_ok = glade_xml_get_widget (data->gui, "e_okbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "e_cancelbutton");
	btn_help = glade_xml_get_widget (data->gui, "e_helpbutton");

	gtk_button_set_use_stock (GTK_BUTTON (btn_ok), TRUE);
	gtk_button_set_label (GTK_BUTTON (btn_ok), GOO_STOCK_EXTRACT);

	/* Set widgets data. */

	g_object_set_data (G_OBJECT (data->dialog), "dialog_data", data);

	/* FIXME: use this property or delete it. */
	if (eel_gconf_get_boolean (PREF_EXTRACT_FIRST_TIME, TRUE)) {
		eel_gconf_set_boolean (PREF_EXTRACT_FIRST_TIME, FALSE);
	} 

	selected = g_list_length (data->selected_tracks);
	gtk_widget_set_sensitive (data->e_selected_radiobutton, selected > 0);
	if (selected <= 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_alltrack_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_selected_radiobutton), TRUE);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}


void
dlg_extract_selected (GooWindow *window)
{
	GList *tracks_to_rip;
	
	tracks_to_rip = goo_window_get_tracks (window, TRUE);
	dlg_ripper (window, tracks_to_rip);
	track_list_free (tracks_to_rip);
}


void
dlg_extract (GooWindow *window)
{
	/* FIXME
	GList *selected_tracks;
	int    n_selected_tracks;
	
	selected_tracks = goo_window_get_tracks (window, TRUE);
	n_selected_tracks = g_list_length (selected_tracks);
	track_list_free (selected_tracks);
	
	if (n_selected_tracks <= 1)
		dlg_ripper (window, NULL);
	else*/
	
	dlg_extract_ask (window);
}
