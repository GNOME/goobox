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
#include <libgnomevfs/gnome-vfs-ops.h>
#include <glade/glade.h>
#include <gst/gst.h>
#include "typedefs.h"
#include "main.h"
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "file-utils.h"
#include "typedefs.h"
#include "goo-window.h"
#include "preferences.h"
#include "song-info.h"
#include "track-info.h"
#include "goo-player-cd.h"
#include "goo-cdrom.h"

#define TOC_OFFSET 150
#define GLADE_RIPPER_FILE "goobox.glade"
#define DESTINATION_PERMISSIONS 0755
#define UPDATE_DELAY 400
#define DEFAULT_OGG_QUALITY 0.3
#define DEFAULT_FLAC_COMPRESSION 5
#define DEFAULT_MP3_BITRATE 128

typedef struct {
	GooWindow     *window;
	char          *location;
	char          *destination;
	GooFileFormat  format;
	char          *album;
	char          *artist;
	char          *genre;
	GList         *tracks;
	int            total_tracks;
	GList         *current_track;
	char          *ext;
	GooCdrom      *cdrom;

	GstElement    *rip_thread;
	GstElement    *source;
	GstElement    *encoder;
	GstElement    *sink;
	GstPad        *source_pad;
	GstFormat      track_format, sector_format;
	guint          update_handle;
	double         fraction;
	char          *current_file;
	gboolean       ripping;

	GladeXML      *gui;

	GtkWidget     *dialog;
	GtkWidget     *r_progress_progressbar;
	GtkWidget     *r_progress_label;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if (data->update_handle != 0) {
		g_source_remove (data->update_handle);
		data->update_handle = 0;
	}

	if (data->ripping && (data->current_file != NULL))
		gnome_vfs_unlink (data->current_file);

	if (data->rip_thread != NULL) {
		gst_element_set_state (data->rip_thread, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (data->rip_thread));
	}

	goo_cdrom_unlock_tray (data->cdrom);
	g_object_unref (data->cdrom);
	
	g_free (data->location);
	g_free (data->destination);
	g_free (data->album);
	g_free (data->artist);
	g_free (data->genre);
	g_free (data->current_file);
	track_list_free (data->tracks);

	g_object_unref (data->gui);
	g_free (data);
}


static void rip_current_track (DialogData *data);


static gboolean
rip_next_track (gpointer callback_data)
{
	DialogData *data = callback_data;

	g_free (data->current_file);
	data->current_file = NULL;

	gst_element_set_state (data->encoder, GST_STATE_NULL);
	gst_element_set_state (data->rip_thread, GST_STATE_NULL);
	data->current_track = data->current_track->next;
	rip_current_track (data);

	return FALSE;
}


static gboolean
update_ui (gpointer callback_data)
{
	DialogData *data = callback_data;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->r_progress_progressbar), data->fraction);
	return FALSE;
}


static void
encoder_eos_cb (GstBin     *bin, 
		DialogData *data)
{
	if (data->update_handle != 0) {
		g_source_remove (data->update_handle);
		data->update_handle = 0;
	}
	g_idle_add (rip_next_track, data);
}


static gboolean
update_progress_cb (gpointer callback_data)
{
	DialogData  *data = callback_data;
	TrackInfo   *track = data->current_track->data;
	gint64       from_sector;
	guint64      sector = 0;
	gboolean     ret;

	if (data->update_handle != 0) {
		g_source_remove (data->update_handle);
		data->update_handle = 0;
	}
	
	ret = gst_pad_query (data->source_pad, GST_QUERY_POSITION, &data->sector_format, &sector);
	if (!ret)
		return FALSE;
	
	from_sector = track->from_sector - TOC_OFFSET;
	data->fraction = ((double) (sector - from_sector)) / (double)track->sectors;
	g_idle_add (update_ui, data);

	data->update_handle = g_timeout_add (UPDATE_DELAY, update_progress_cb, data);

	return FALSE;
}


static void
create_pipeline (DialogData *data)
{
	float ogg_quality;
	int   flac_compression;
	int   mp3_bitrate;

	data->rip_thread = gst_thread_new ("rip_thread");
		
	data->source = gst_element_factory_make ("cdparanoia", "cdparanoia");
	g_object_set (G_OBJECT (data->source), 
		      "location", data->location,
		      NULL);

	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:	
		data->encoder = gst_element_factory_make (OGG_ENCODER, "encoder");
		data->ext = "ogg";
		ogg_quality = eel_gconf_get_float (PREF_ENCODER_OGG_QUALITY, DEFAULT_OGG_QUALITY) + 0.05;
		g_object_set (data->encoder,
			      "quality", ogg_quality,
			      NULL);
		break;
	case GOO_FILE_FORMAT_FLAC:
		data->encoder = gst_element_factory_make (FLAC_ENCODER, "encoder");
		data->ext = "flac";
		flac_compression = eel_gconf_get_integer (PREF_ENCODER_FLAC_COMPRESSION, DEFAULT_FLAC_COMPRESSION);
		g_object_set (data->encoder,
			      "quality", flac_compression,
			      NULL);
		break;
	case GOO_FILE_FORMAT_MP3:
		data->encoder = gst_element_factory_make (MP3_ENCODER, "encoder");
		data->ext = "mp3";
		mp3_bitrate = eel_gconf_get_integer (PREF_ENCODER_MP3_BITRATE, DEFAULT_MP3_BITRATE);
		g_object_set (data->encoder,
			      "bitrate", mp3_bitrate,
			      NULL);
		break;
	case GOO_FILE_FORMAT_WAVE:
		data->encoder = gst_element_factory_make (WAVE_ENCODER, "encoder");
		data->ext = "wav";
		break;
	}
	g_signal_connect (data->encoder, 
			  "eos", 
			  G_CALLBACK (encoder_eos_cb), 
			  data);

	data->sink = gst_element_factory_make ("filesink", "filesink");

	gst_bin_add_many (GST_BIN (data->rip_thread), data->source, data->encoder, data->sink, NULL);
	gst_element_link_many (data->source, data->encoder, data->sink, NULL);

	data->track_format = gst_format_get_by_nick ("track");
	data->sector_format = gst_format_get_by_nick ("sector");
	data->source_pad = gst_element_get_pad (data->source, "src");
}


static char *
zero_padded (int n)
{
        static char  s[1024];
        char        *t;

        sprintf (s, "%2d", n);
        for (t = s; (t != NULL) && (*t != 0); t++)
                if (*t == ' ')
                        *t = '0';

        return s;
}


static char*
get_destination_folder (DialogData *data)
{
	return g_build_filename (data->destination,
				 data->artist,
				 data->album,
				 NULL);
}


static void
done_dialog_response_cb (GtkDialog  *dialog,
			 int         button_number,
			 gpointer    userdata)
{
	DialogData *data = (DialogData*) userdata;

	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	if ((button_number == GTK_RESPONSE_OK)
	    && eel_gconf_get_boolean (PREF_RIPPER_VIEW_DISTINATION, FALSE)) {
		GError *error  = NULL;
		char   *folder = get_destination_folder (data);
		char   *url    = g_strconcat ("file://", folder, NULL);

		if (! gnome_url_show (url, &error))
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->window), _("Could not display the destination folder"), &error);

		g_free (folder);
		g_free (url);
	}

	gtk_widget_destroy (data->dialog);
}


static void
rip_current_track (DialogData *data)
{
	TrackInfo *track;
	char      *msg;
	char      *filename;
	char      *folder;
	GstEvent  *event;

	if (data->current_track == NULL) {
		GtkWidget *d;

		data->ripping = FALSE;
		gtk_widget_hide (data->dialog);

		d = _gtk_ok_dialog_with_checkbutton_new (GTK_WINDOW (data->window),
							 GTK_DIALOG_MODAL,
							 _("Tracks extracted successfully"),
							 GTK_STOCK_OK,
							 _("View destination folder"),
							 PREF_RIPPER_VIEW_DISTINATION);
							    
		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (done_dialog_response_cb),
				  data);
		gtk_window_set_resizable (GTK_WINDOW (d), FALSE);
		gtk_widget_show (d);

		return;
	}
		
	track = data->current_track->data;
	msg = g_strdup_printf (_("Extracting: %s"), track->title);
	gtk_label_set_text (GTK_LABEL (data->r_progress_label), msg);
	g_free (msg);

	/* Set the filename */

	folder = get_destination_folder (data);
	ensure_dir_exists (folder, DESTINATION_PERMISSIONS);

	filename = g_strdup_printf ("%s - %s.%s", zero_padded (track->number + 1), track->title, data->ext);
	g_free (data->current_file);
	data->current_file = g_build_filename (folder, filename, NULL);
	g_free (filename);
	g_free (folder);

	gnome_vfs_unlink (data->current_file);

	gst_element_set_state (data->sink, GST_STATE_NULL);
	g_object_set (G_OBJECT (data->sink), "location", data->current_file, NULL);

	/* Set song tags. */

	if (GST_IS_TAG_SETTER (data->encoder)) 
		gst_tag_setter_add (GST_TAG_SETTER (data->encoder),   
				    GST_TAG_MERGE_REPLACE_ALL,
				    GST_TAG_TITLE, track->title,
				    GST_TAG_ARTIST, data->artist,
				    GST_TAG_ALBUM, data->album,
				    GST_TAG_GENRE, data->genre,
				    GST_TAG_TRACK_NUMBER, (guint) track->number + 1,
				    GST_TAG_TRACK_COUNT, (guint) data->total_tracks,
				    GST_TAG_DURATION, (guint64) track->length * GST_SECOND, 
				    GST_TAG_COMMENT, _("Ripped with Goobox"),
				    NULL);
		
	/* Seek to track. */

	gst_element_set_state (data->rip_thread, GST_STATE_PAUSED);
	event = gst_event_new_segment_seek (data->track_format | GST_SEEK_METHOD_SET | GST_SEEK_FLAG_FLUSH, 
					    track->number, track->number + 1);
	if (!gst_pad_send_event (data->source_pad, event)) {
		g_warning ("seek failed");
		return;
	}

	/* Start ripping. */

	data->update_handle = g_timeout_add (UPDATE_DELAY, update_progress_cb, data);

	gst_element_set_state (data->rip_thread, GST_STATE_PLAYING);
}


static void
start_ripper (DialogData *data)
{
	goo_cdrom_lock_tray (data->cdrom);
	data->ripping = TRUE;
	create_pipeline (data);
	data->current_track = data->tracks;
	rip_current_track (data);
}





/* create the main dialog. */
void
dlg_ripper (GooWindow     *window,
	    const char    *location,
	    const char    *destination,
	    GooFileFormat  format,
	    const char    *album,
	    const char    *artist,
	    const char    *genre,
	    int            total_tracks,
	    GList         *tracks)
{
	DialogData  *data;
	GtkWidget   *btn_cancel;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_RIPPER_FILE, NULL, NULL);
        if (!data->gui) {
		g_warning ("Could not find " GLADE_RIPPER_FILE "\n");
		g_free (data);
                return;
        }

	data->location = g_strdup (location);
	data->destination = g_strdup (destination);
	data->format = format;
	data->album = g_strdup (album);
	data->artist = g_strdup (artist);
	data->genre = g_strdup (genre);
	data->tracks = track_list_dup (tracks);
	data->total_tracks = total_tracks;
	data->update_handle = 0;

	data->cdrom = goo_cdrom_new (location);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "ripper_dialog");
	data->r_progress_progressbar = glade_xml_get_widget (data->gui, "r_progress_progressbar");
	data->r_progress_label = glade_xml_get_widget (data->gui, "r_progress_label");
	btn_cancel = glade_xml_get_widget (data->gui, "r_cancelbutton");

	/* Set widgets data. */

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  data->dialog);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);

	start_ripper (data);
}
