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
#include <glade/glade.h>
#include <gst/gst.h>
#include "typedefs.h"
#include "main.h"
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "typedefs.h"
#include "goo-window.h"
#include "preferences.h"
#include "song-info.h"
#include "track-info.h"
#include "goo-player-cd.h"
#include "dlg-ripper.h"

#define GLADE_EXTRACT_FILE "goobox.glade"

typedef struct {
	GooWindow   *window;
	GooPlayerCD *player_cd;
	GList       *songs;
	GList       *selected_songs;

	GladeXML  *gui;

	GtkWidget *dialog;
	GtkWidget *e_alltrack_radiobutton;
	GtkWidget *e_selected_radiobutton;
	GtkWidget *e_destination_entry;
	GtkWidget *e_destination_button;
	GtkWidget *e_ogg_radiobutton;
	GtkWidget *e_flac_radiobutton;
	GtkWidget *e_mp3_radiobutton;
	GtkWidget *e_wave_radiobutton;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	song_list_free (data->selected_songs);
	song_list_free (data->songs);
	g_object_unref (data->player_cd);
	g_object_unref (data->gui);
	g_free (data);
}


static GList*
get_tracks (DialogData *data,
	    gboolean    selected)
{
	GList *tracks = NULL;
	GList *scan;

	if (!selected)
		return goo_player_cd_get_tracks (data->player_cd);

	for (scan = data->selected_songs; scan; scan = scan->next) {
		SongInfo *song = scan->data;
		TrackInfo *track = goo_player_cd_get_track (data->player_cd, song->number);
		track_info_ref (track);
		tracks = g_list_prepend (tracks, track);
	}

	return g_list_reverse (tracks);
}


/* called when the "ok" button is clicked. */
static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	const char *destination;
	GooFileFormat file_format = GOO_FILE_FORMAT_OGG;
	GList *tracks_to_rip;

	/* save preferences */

	destination = gtk_entry_get_text (GTK_ENTRY (data->e_destination_entry));
	eel_gconf_set_path (PREF_EXTRACT_DESTINATION, destination);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_ogg_radiobutton)))
		file_format = GOO_FILE_FORMAT_OGG;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_flac_radiobutton)))
		file_format = GOO_FILE_FORMAT_FLAC;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_mp3_radiobutton)))
		file_format = GOO_FILE_FORMAT_MP3;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_wave_radiobutton)))
		file_format = GOO_FILE_FORMAT_WAVE;
	pref_set_file_format (file_format);

	/**/

	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_hide (data->dialog);

	tracks_to_rip = get_tracks (data, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_selected_radiobutton)));

	dlg_ripper (data->window,
		    goo_player_get_location (GOO_PLAYER (data->player_cd)),
		    destination,
		    file_format,
		    goo_player_cd_get_album (data->player_cd),
		    goo_player_cd_get_artist (data->player_cd),
		    goo_player_cd_get_genre (data->player_cd),
		    g_list_length (data->songs),
		    tracks_to_rip);

	track_list_free (tracks_to_rip);

	/**/

	gtk_widget_destroy (data->dialog);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("goo", "extract", &err);
	
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
open_response_cb (GtkDialog  *file_sel,
		  int         button_number,
		  gpointer    userdata)
{
	DialogData  *data = (DialogData   * )userdata;

	if (button_number == GTK_RESPONSE_ACCEPT) {
		_gtk_entry_set_filename_text (GTK_ENTRY (data->e_destination_entry),
					      gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_sel)));
	}
	gtk_widget_destroy (GTK_WIDGET (file_sel));
}


static void
destination_button_clicked_cb (GtkWidget  *button,
			       DialogData *data)
{
	GtkWidget *file_sel;
	
	file_sel = gtk_file_chooser_dialog_new (_("Choose destination folder"),
						NULL,
						GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, 
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
						GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						NULL);
	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (file_sel), TRUE);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_sel), gtk_entry_get_text (GTK_ENTRY (data->e_destination_entry)));

	g_signal_connect (G_OBJECT (file_sel),
			  "response",
			  G_CALLBACK (open_response_cb),
			  data);
	
	g_signal_connect_swapped (GTK_DIALOG (file_sel),
				  "close",
				  G_CALLBACK (gtk_widget_destroy),
				  GTK_WIDGET (file_sel));
	
	gtk_widget_show_all (GTK_WIDGET (file_sel));
}





/* create the main dialog. */
void
dlg_extract (GooWindow *window)
{
	DialogData       *data;
	GtkWidget        *btn_ok;
	GtkWidget        *btn_cancel;
	GtkWidget        *btn_help;
	GtkWidget        *filetype_btn;
	char             *path = NULL;
	GooFileFormat     file_format;
	int               selected;
	GstElement       *encoder;
	gboolean          ogg_encoder, flac_encoder, mp3_encoder, wave_encoder;
	gboolean          find_first_available;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_EXTRACT_FILE, NULL, NULL);
        if (!data->gui) {
		g_warning ("Could not find " GLADE_EXTRACT_FILE "\n");
		g_free (data);
                return;
        }

	data->songs = goo_window_get_song_list (window, FALSE);
	data->selected_songs = goo_window_get_song_list (window, TRUE);
	data->player_cd = GOO_PLAYER_CD (goo_window_get_player (window));
	g_object_ref (data->player_cd);

	eel_gconf_preload_cache ("/apps/goo/dialogs/extract", GCONF_CLIENT_PRELOAD_ONELEVEL);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "extract_dialog");

	data->e_alltrack_radiobutton = glade_xml_get_widget (data->gui, "e_alltrack_radiobutton");
	data->e_selected_radiobutton = glade_xml_get_widget (data->gui, "e_selected_radiobutton");
	data->e_destination_entry = glade_xml_get_widget (data->gui, "e_destination_entry");
	data->e_destination_button = glade_xml_get_widget (data->gui, "e_destination_button");
	data->e_ogg_radiobutton = glade_xml_get_widget (data->gui, "e_ogg_radiobutton");
	data->e_flac_radiobutton = glade_xml_get_widget (data->gui, "e_flac_radiobutton");
	data->e_mp3_radiobutton = glade_xml_get_widget (data->gui, "e_mp3_radiobutton");
	data->e_wave_radiobutton = glade_xml_get_widget (data->gui, "e_wave_radiobutton");

	btn_ok = glade_xml_get_widget (data->gui, "e_okbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "e_cancelbutton");
	btn_help = glade_xml_get_widget (data->gui, "e_helpbutton");

	/* Set widgets data. */

	selected = g_list_length (data->selected_songs);
	if (selected == 0) {
		gtk_widget_set_sensitive (data->e_selected_radiobutton, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_alltrack_radiobutton), TRUE);
	} else {
		gtk_widget_set_sensitive (data->e_selected_radiobutton, TRUE);
		if (selected > 1)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_selected_radiobutton), TRUE);
		else
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_alltrack_radiobutton), TRUE);
	}

	path = eel_gconf_get_path (PREF_EXTRACT_DESTINATION, "~");
	gtk_entry_set_text (GTK_ENTRY (data->e_destination_entry), path);
	g_free (path);

	encoder = gst_element_factory_make (OGG_ENCODER, "encoder");
	ogg_encoder = encoder != NULL;
	gtk_widget_set_sensitive (data->e_ogg_radiobutton, ogg_encoder);
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (FLAC_ENCODER, "encoder");
	flac_encoder = encoder != NULL;
	gtk_widget_set_sensitive (data->e_flac_radiobutton, flac_encoder);
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (MP3_ENCODER, "encoder");
	mp3_encoder = encoder != NULL;
	gtk_widget_set_sensitive (data->e_mp3_radiobutton, mp3_encoder);
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (WAVE_ENCODER, "encoder");
	wave_encoder = encoder != NULL;
	gtk_widget_set_sensitive (data->e_wave_radiobutton, wave_encoder);
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	if (!ogg_encoder && !flac_encoder && !mp3_encoder && !wave_encoder) {
		GtkWidget *d;
		char *msg;
		
		msg = g_strdup_printf (_("You need at least one of the following GStreamer plugins in order to extract CD tracks:\n\n"
					 "\342\200\242 %s \342\206\222 Ogg Vorbis\n"
					 "\342\200\242 %s \342\206\222 FLAC\n"
					 "\342\200\242 %s \342\206\222 Mp3\n"
					 "\342\200\242 %s \342\206\222 Wave"),
				       OGG_ENCODER,
				       FLAC_ENCODER,
				       MP3_ENCODER,
				       WAVE_ENCODER);

		d = _gtk_message_dialog_new (GTK_WINDOW (window),
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_DIALOG_ERROR,
					     _("No encoder available."),
					     msg,
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					     NULL);
		g_free (msg);

		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		gtk_widget_show (d);

		destroy_cb (NULL, data);
		return;
	}

	file_format = pref_get_file_format ();

	find_first_available = (((file_format == GOO_FILE_FORMAT_OGG) && !ogg_encoder)
				|| ((file_format == GOO_FILE_FORMAT_FLAC) && !flac_encoder)
				|| ((file_format == GOO_FILE_FORMAT_MP3) && !mp3_encoder)
				|| ((file_format == GOO_FILE_FORMAT_WAVE) && !wave_encoder));

	if (find_first_available) {
		if (ogg_encoder)
			file_format = GOO_FILE_FORMAT_OGG;
		else if (flac_encoder)
			file_format = GOO_FILE_FORMAT_FLAC;
		else if (mp3_encoder)
			file_format = GOO_FILE_FORMAT_MP3;
		else if (wave_encoder)
			file_format = GOO_FILE_FORMAT_WAVE;
	}

	switch (file_format) {
	case GOO_FILE_FORMAT_OGG:
		filetype_btn = data->e_ogg_radiobutton;
		break;
	case GOO_FILE_FORMAT_FLAC:
		filetype_btn = data->e_flac_radiobutton;
		break;
	case GOO_FILE_FORMAT_MP3:
		filetype_btn = data->e_mp3_radiobutton;
		break;
	case GOO_FILE_FORMAT_WAVE:
		filetype_btn = data->e_wave_radiobutton;
		break;
	}

	switch (file_format) {
	case GOO_FILE_FORMAT_OGG:
		filetype_btn = data->e_ogg_radiobutton;
		break;
	case GOO_FILE_FORMAT_FLAC:
		filetype_btn = data->e_flac_radiobutton;
		break;
	case GOO_FILE_FORMAT_MP3:
		filetype_btn = data->e_mp3_radiobutton;
		break;
	case GOO_FILE_FORMAT_WAVE:
		filetype_btn = data->e_wave_radiobutton;
		break;
	}
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filetype_btn), TRUE);

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
	g_signal_connect (G_OBJECT (data->e_destination_button), 
			  "clicked",
			  G_CALLBACK (destination_button_clicked_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
}
