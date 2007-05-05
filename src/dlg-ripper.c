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
#include <math.h>

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-propertybox.h>
#include <libgnomeui/gnome-pixmap.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
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
#include "goo-player.h"
#include "goo-cdrom.h"

#define TOC_OFFSET 150
#define GLADE_RIPPER_FILE "goobox.glade"
#define DESTINATION_PERMISSIONS 0755
#define PLS_PERMISSIONS 0644
#define UPDATE_DELAY 400
#define DEFAULT_OGG_QUALITY 0.5
#define DEFAULT_FLAC_COMPRESSION 5
#define BUFFER_SIZE 1024

typedef struct {
	GooWindow     *window;
	char          *location;
	char          *destination;
	GooFileFormat  format;
	char          *album;
	char          *artist;
	int            year;
	char          *genre;
	GList         *tracks;
	int            tracks_n;
	int            total_tracks;
	GList         *current_track;
	int            current_track_n;
	char          *ext;
	GooCdrom      *cdrom;

	GstElement    *pipeline;
	GstElement    *source;
	GstElement    *encoder;
	GstElement    *container;
	GstElement    *sink;
	GstPad        *source_pad;
	GstFormat      track_format, sector_format;
	guint          update_handle;
	int            total_sectors;
	int            current_track_sectors;
	int            prev_tracks_sectors;
	char          *current_file;
	gboolean       ripping;
	GTimer        *timer;

	GladeXML      *gui;

	GtkWidget     *dialog;
	GtkWidget     *r_progress_progressbar;
	GtkWidget     *r_track_label;
} DialogData;


/* From lame.h */
typedef enum vbr_mode_e {
  vbr_off=0,
  vbr_mt,               /* obsolete, same as vbr_mtrh */
  vbr_rh,
  vbr_abr,
  vbr_mtrh,
  vbr_max_indicator,    /* Don't use this! It's used for sanity checks.       */  
  vbr_default=vbr_rh    /* change this to change the default VBR mode of LAME */
} vbr_mode;


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

	if (data->pipeline != NULL) {
		gst_element_set_state (data->pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (data->pipeline));
	}

	if (data->timer != NULL)
		g_timer_destroy (data->timer);

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
	TrackInfo  *track;

	g_free (data->current_file);
	data->current_file = NULL;

	gst_element_set_state (data->encoder, GST_STATE_NULL);
	gst_element_set_state (data->pipeline, GST_STATE_NULL);

	track = data->current_track->data;
	data->prev_tracks_sectors += track->sectors;

	data->current_track_n++;
	data->current_track = data->current_track->next;
	rip_current_track (data);

	return FALSE;
}


static gboolean
update_ui (gpointer callback_data)
{
	DialogData *data = callback_data;
	double      fraction;
	double      elapsed, remaining;
	int         minutes;
	char       *msg, *time_left;

	fraction = ((double) (data->current_track_sectors + data->prev_tracks_sectors)) / (double) data->total_sectors;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (data->r_progress_progressbar), fraction);

	elapsed = g_timer_elapsed (data->timer, NULL);
	remaining = (elapsed / fraction) - elapsed;	
	minutes = (int) (floor (remaining + 0.5)) / 60;
	if (minutes == 0)
		time_left = g_strdup (_("less than a minute left"));
	else
		time_left = g_strdup_printf (ngettext ("about %d minute left", "about %d minutes left", minutes), minutes);
	msg = g_strdup_printf (_("%d of %d tracks extracted, %s"), data->current_track_n - 1, data->tracks_n, time_left);

	gtk_progress_bar_set_text  (GTK_PROGRESS_BAR (data->r_progress_progressbar), msg);

	g_free (msg);
	g_free (time_left);

	return FALSE;
}

static gboolean
pipeline_bus_message_cb (GstBus     *bus,
		         GstMessage *message,
		         gpointer    callback_data)
{
	DialogData *data = callback_data;
	
	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_EOS:
		if (data->update_handle != 0) {
			g_source_remove (data->update_handle);
			data->update_handle = 0;
		}
		g_idle_add (rip_next_track, data);	
		break;
		
	default:
		break;
	}
	
	return TRUE;
}


static gboolean
update_progress_cb (gpointer callback_data)
{
	DialogData  *data = callback_data;
	gint64       sector = 0;

	if (data->update_handle != 0) {
		g_source_remove (data->update_handle);
		data->update_handle = 0;
	}
	
	if (! gst_pad_query_position (data->source_pad, &data->sector_format, &sector))
		return FALSE;
	
	data->current_track_sectors = sector;
	g_idle_add (update_ui, data);

	data->update_handle = g_timeout_add (UPDATE_DELAY, update_progress_cb, data);

	return FALSE;
}


static void
create_pipeline (DialogData *data)
{
	GstElement *audioconvert;
	GstElement *audioresample;	
	GstElement *queue;
	float       ogg_quality;
	int         flac_compression;
	
	data->pipeline = gst_pipeline_new ("pipeline");
		
	data->source = gst_element_make_from_uri (GST_URI_SRC, "cdda://", "source");
	g_object_set (G_OBJECT (data->source), 
		      "device", data->location, 
		      NULL);
	
	audioconvert = gst_element_factory_make ("audioconvert", "convert");
    	audioresample = gst_element_factory_make ("audioresample", "resample");
	queue = gst_element_factory_make ("queue", "queue");
	g_object_set (queue, "max-size-time", 120 * GST_SECOND, NULL);

	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:	
		data->encoder = gst_element_factory_make (OGG_ENCODER, "encoder");
		data->ext = "ogg";
		ogg_quality = eel_gconf_get_float (PREF_ENCODER_OGG_QUALITY, DEFAULT_OGG_QUALITY);
		g_object_set (data->encoder,
			      "quality", ogg_quality,
			      NULL);
		data->container = gst_element_factory_make ("oggmux", "tagger");
		break;

	case GOO_FILE_FORMAT_FLAC:
		data->encoder = data->container = gst_element_factory_make (FLAC_ENCODER, "encoder");
		data->ext = "flac";
		flac_compression = eel_gconf_get_integer (PREF_ENCODER_FLAC_COMPRESSION, DEFAULT_FLAC_COMPRESSION);
		g_object_set (data->encoder,
			      "quality", flac_compression,
			      NULL);
		break;

	case GOO_FILE_FORMAT_WAVE:
		data->encoder = data->container = gst_element_factory_make (WAVE_ENCODER, "encoder");
		data->ext = "wav";
		break;
	}

	data->sink = gst_element_factory_make ("gnomevfssink", "filesink");

	if (data->encoder == data->container) {
		gst_bin_add_many (GST_BIN (data->pipeline), data->source, queue, 
				  audioconvert, audioresample, data->encoder, 
				  data->sink, NULL);
		gst_element_link_many (data->source, queue, audioconvert, 
				       audioresample, data->encoder, 
				       data->sink, NULL);
	}
	else {
		gst_bin_add_many (GST_BIN (data->pipeline), data->source, queue, 
				  audioconvert, audioresample, data->encoder,
				  data->container, data->sink, NULL);
		gst_element_link_many (data->source, queue, audioconvert, 
				       audioresample, data->encoder, 
				       data->container, data->sink, NULL);
	}

	data->track_format = gst_format_get_by_nick ("track");
	data->sector_format = gst_format_get_by_nick ("sector");
	data->source_pad = gst_element_get_pad (data->source, "src");
	  
	gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (data->pipeline)),
			   pipeline_bus_message_cb, 
			   data);
}


static char*
get_destination_folder (DialogData *data)
{
	char *artist_filename;
	char *album_filename;
	char *result;

	artist_filename = tracktitle_to_filename (data->artist);
	album_filename = tracktitle_to_filename (data->album);
	
	result = g_build_filename (data->destination,
				   artist_filename,
				   album_filename,
				   NULL);

	g_free (artist_filename);
	g_free (album_filename);

	return result;
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
		char   *scheme = NULL;
		char   *url    = NULL;
		
		scheme = gnome_vfs_get_uri_scheme (folder);
		if ((scheme == NULL) || (scheme == ""))
			url = gnome_vfs_get_uri_from_local_path (folder);
		else
			url = g_strdup (folder);

		if (! gnome_url_show (url, &error))
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->window), _("Could not display the destination folder"), &error);

		g_free (scheme);
		g_free (url);
		g_free (folder);
	}

	gtk_widget_destroy (data->dialog);
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


static char *
get_track_filename (TrackInfo  *track,
		    const char *ext)
{
	char *filename, *track_filename;

	filename = tracktitle_to_filename (track->title);
	track_filename = g_strdup_printf ("%s. %s.%s", zero_padded (track->number + 1), filename, ext);
	g_free (filename);

	return track_filename;
}


static void
save_playlist (DialogData *data)
{
	char           *folder;
	char           *filename, *album_filename;
	GnomeVFSResult  result;
	GnomeVFSHandle *handle;

	folder = get_destination_folder (data);
	album_filename = tracktitle_to_filename (data->album);
	filename = g_strconcat ("file://", folder, "/", album_filename, ".pls", NULL);
	g_free (album_filename);

	gnome_vfs_unlink (filename);

	result = gnome_vfs_create (&handle,
				   filename,
				   GNOME_VFS_OPEN_WRITE,
				   TRUE,
				   PLS_PERMISSIONS);

	if (result == GNOME_VFS_OK) {
		GList *scan;
		char   buffer[BUFFER_SIZE];
		int    n = 0;

		strcpy (buffer, "[playlist]\n");
		gnome_vfs_write (handle,
				 buffer,
				 strlen (buffer),
				 NULL);

		sprintf (buffer, "NumberOfEntries=%d\n", data->tracks_n);
		gnome_vfs_write (handle,
				 buffer,
				 strlen (buffer),
				 NULL);

		strcpy (buffer, "Version=2\n");
		gnome_vfs_write (handle,
				 buffer,
				 strlen (buffer),
				 NULL);
		
		for (scan = data->tracks; scan; scan = scan->next) {
			TrackInfo *track = scan->data;
			char      *track_filename;

			n++;

			track_filename = get_track_filename (track, data->ext);
			
			sprintf (buffer, "File%d=%s\n", n, track_filename);
			gnome_vfs_write (handle,
					 buffer,
					 strlen (buffer),
					 NULL);
			g_free (track_filename);

			sprintf (buffer, "Title%d=%s - %s\n", n, data->artist, track->title);
			gnome_vfs_write (handle,
					 buffer,
					 strlen (buffer),
					 NULL);

			sprintf (buffer, "Length%d=%d\n", n, track->min * 60 + track->sec);
			gnome_vfs_write (handle,
					 buffer,
					 strlen (buffer),
					 NULL);
		}

		gnome_vfs_close (handle);
	}

	g_free (filename);
	g_free (folder);
}


static void
rip_current_track (DialogData *data)
{
	TrackInfo      *track;
	char           *msg;
	char           *filename;
	char           *folder;
	GstEvent       *event;
	GnomeVFSResult  result;

	if (data->current_track == NULL) {
		GtkWidget *d;

		if (eel_gconf_get_boolean (PREF_EXTRACT_SAVE_PLAYLIST, TRUE))
			save_playlist (data);

		data->ripping = FALSE;
		gtk_widget_hide (data->dialog);

		d = _gtk_ok_dialog_with_checkbutton_new (GTK_WINDOW (data->window),
							 GTK_DIALOG_MODAL,
							 _("Tracks extracted successfully"),
							 GTK_STOCK_OK,
							 _("_View destination folder"),
							 PREF_RIPPER_VIEW_DISTINATION);
							    
		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (done_dialog_response_cb),
				  data);
		gtk_window_set_resizable (GTK_WINDOW (d), FALSE);
		gtk_widget_show (d);

		return;
	}
		
	track = data->current_track->data;

	msg = g_markup_printf_escaped (_("<i>Extracting \"%s\"</i>"), track->title); 
	gtk_label_set_markup (GTK_LABEL (data->r_track_label), msg);
	g_free (msg);

	/* Set the filename */

	folder = get_destination_folder (data);
	
	if ((result = ensure_dir_exists (folder, DESTINATION_PERMISSIONS)) != GNOME_VFS_OK) {
		char *utf8_folder;

		utf8_folder = g_locale_to_utf8 (folder, -1, 0, 0, 0);
		_gtk_error_dialog_run (GTK_WINDOW (data->window),
				       _("Could not extract tracks"),
				       _("Could not create folder \"%s\"\n\n%s"),
				       utf8_folder,
				       gnome_vfs_result_to_string (result));
		g_free (utf8_folder);
		gtk_widget_destroy (data->dialog);
		return;
	}

	filename = get_track_filename (track, data->ext);
	g_free (data->current_file);
	data->current_file = g_build_filename (folder, filename, NULL);
	g_free (filename);
	g_free (folder);

	gnome_vfs_unlink (data->current_file);

	gst_element_set_state (data->sink, GST_STATE_NULL);
	g_object_set (G_OBJECT (data->sink), "location", data->current_file, NULL);

	/* Set song tags. */

	if (GST_IS_TAG_SETTER (data->encoder)) {
		gst_tag_setter_add_tags (GST_TAG_SETTER (data->encoder),   
				         GST_TAG_MERGE_REPLACE,
				         GST_TAG_TITLE, track->title,
				         GST_TAG_ARTIST, data->artist,
				         GST_TAG_ALBUM, data->album,
				         GST_TAG_TRACK_NUMBER, (guint) track->number + 1,
				         GST_TAG_TRACK_COUNT, (guint) data->total_tracks,
				         GST_TAG_DURATION, (guint64) track->length * GST_SECOND, 
				         GST_TAG_COMMENT, _("Ripped with Goobox"),
				         GST_TAG_ENCODER, PACKAGE_NAME,
				         GST_TAG_ENCODER_VERSION, PACKAGE_VERSION,
				         NULL);
		if (data->genre != NULL)
			gst_tag_setter_add_tags (GST_TAG_SETTER (data->encoder),   
					         GST_TAG_MERGE_REPLACE,
					         GST_TAG_GENRE, data->genre,
					         NULL);
		if (data->year != 0) {
			GDate *d = g_date_new_dmy (1, 1, data->year);
			gst_tag_setter_add_tags (GST_TAG_SETTER (data->encoder),   
					         GST_TAG_MERGE_REPLACE,
					         GST_TAG_DATE,  g_date_get_julian (d),
					         NULL);
			g_date_free (d);
		}
	}
		
	/* Seek to track. */

	gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
	event = gst_event_new_seek (1.0, 
				    data->track_format,
				    GST_SEEK_FLAG_FLUSH,
				    GST_SEEK_TYPE_SET,
				    track->number,
				    GST_SEEK_TYPE_SET,
				    track->number + 1);
	if (! gst_pad_send_event (data->source_pad, event)) {
		g_warning ("seek failed");
		return;
	}

	/* Start ripping. */

	data->current_track_sectors = 0;
	data->update_handle = g_timeout_add (UPDATE_DELAY, update_progress_cb, data);

	gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
}


static void
start_ripper (DialogData *data)
{
	GList *scan;

	data->ripping = TRUE;

	goo_cdrom_lock_tray (data->cdrom);

	data->prev_tracks_sectors = 0;
	data->total_sectors = 0;
	for (scan = data->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		data->total_sectors += track->sectors;
	}
	create_pipeline (data);
	data->current_track_n = 1;
	data->current_track = data->tracks;

	g_timer_start (data->timer);

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
	    int            year,
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
	data->year = year;
	if (genre != NULL)
		data->genre = g_strdup (genre);
	data->tracks = track_list_dup (tracks);
	data->tracks_n = g_list_length (data->tracks);
	data->total_tracks = total_tracks;
	data->update_handle = 0;
	data->timer = g_timer_new ();
	data->cdrom = goo_cdrom_new (location);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "ripper_dialog");
	data->r_progress_progressbar = glade_xml_get_widget (data->gui, "r_progress_progressbar");
	data->r_track_label = glade_xml_get_widget (data->gui, "r_track_label");
	btn_cancel = glade_xml_get_widget (data->gui, "r_cancelbutton");

	/* Set widgets data. */

	gtk_label_set_ellipsize (GTK_LABEL (data->r_track_label),
				 PANGO_ELLIPSIZE_END);

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
