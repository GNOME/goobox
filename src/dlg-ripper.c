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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <math.h>
#include <brasero3/brasero-drive.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "dlg-ripper.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "main.h"
#include "preferences.h"
#include "typedefs.h"


#define TOC_OFFSET 150
#define DESTINATION_PERMISSIONS 0755
#define PLS_PERMISSIONS 0644
#define UPDATE_DELAY 400
#define BUFFER_SIZE 1024
#define GET_WIDGET(x) _gtk_builder_get_widget (data->builder, (x))
#define _GTK_RESPONSE_VIEW 10


typedef struct {
	GooWindow     *window;
	char          *destination;
	GooFileFormat  format;
	GList         *tracks;
	int            n_tracks;
	GList         *current_track;
	int            current_track_n;
	char          *ext;
	BraseroDrive  *drive;
	AlbumInfo     *album;

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
	GFile         *current_file;
	gboolean       ripping;
	GTimer        *timer;
	double         prev_remaining_time;

	GtkBuilder    *builder;
	GSettings     *settings_ripper;
	GSettings     *settings_encoder;
	GtkWidget     *dialog;
} DialogData;


static void
dialog_destroy_cb (GtkWidget  *widget,
		   DialogData *data)
{
	if (data->update_handle != 0) {
		g_source_remove (data->update_handle);
		data->update_handle = 0;
	}

	if (data->ripping && (data->current_file != NULL))
		g_file_delete (data->current_file, NULL, NULL);

	if (data->pipeline != NULL) {
		gst_element_set_state (data->pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (data->pipeline));
	}

	if (data->timer != NULL)
		g_timer_destroy (data->timer);

	brasero_drive_unlock (data->drive);
	g_object_unref (data->drive);
	g_free (data->destination);
	_g_object_unref (data->current_file);
	track_list_free (data->tracks);
	_g_object_unref (data->builder);
	_g_object_unref (data->settings_encoder);
	_g_object_unref (data->settings_ripper);
	g_free (data);
}


static void rip_current_track (DialogData *data);


static gboolean
rip_next_track (gpointer callback_data)
{
	DialogData *data = callback_data;
	TrackInfo  *track;

	_g_object_unref (data->current_file);
	data->current_file = NULL;

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
	int         ripped_tracks;
	double      fraction;
	char       *msg, *time_left;

	ripped_tracks = data->current_track_sectors + data->prev_tracks_sectors;
	fraction = (double) ripped_tracks / data->total_sectors;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (GET_WIDGET ("progress_progressbar")), fraction);

	/* wait a bit before predicting the remaining time. */
	if ((fraction < 10e-3) || (ripped_tracks < 250)) {
		time_left = NULL;
		data->prev_remaining_time = -1.0;
	}
	else {
		double elapsed, remaining_time;
		int    minutes, seconds;

		elapsed = g_timer_elapsed (data->timer, NULL);
		remaining_time = (elapsed / fraction) - elapsed;
		minutes = (int) (floor (remaining_time + 0.5)) / 60;
		seconds = (int) (floor (remaining_time + 0.5)) % 60;

		if ((data->prev_remaining_time > 0.0)
		    && ( fabs (data->prev_remaining_time - remaining_time) > 20.0))
			time_left = NULL;
		else if (minutes > 59)
			time_left = g_strdup_printf (_("(%d∶%02d∶%02d Remaining)"), minutes / 60, minutes % 60, seconds);
		else
			time_left = g_strdup_printf (_("(%d∶%02d Remaining)"), minutes, seconds);

		data->prev_remaining_time = remaining_time;
	}

	msg = g_strdup_printf (_("Extracting track: %d of %d %s"), data->current_track_n, data->n_tracks, (time_left != NULL ? time_left : ""));
	gtk_progress_bar_set_text  (GTK_PROGRESS_BAR (GET_WIDGET ("progress_progressbar")), msg);

	g_free (msg);
	g_free (time_left);

	return FALSE;
}


static void
pipeline_eos_cb (GstBus     *bus,
		 GstMessage *message,
		 gpointer    callback_data)
{
	DialogData *data = callback_data;

	if (data->update_handle != 0) {
		g_source_remove (data->update_handle);
		data->update_handle = 0;
	}
	g_idle_add (rip_next_track, data);
}


static void
pipeline_error_cb (GstBus     *bus,
		   GstMessage *message,
		   gpointer    callback_data)
{
	DialogData *data = callback_data;
	GError     *error;

	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_hide (data->dialog);

	gst_message_parse_error (message, &error, NULL);
	_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->window), _("Could not extract the tracks"), &error);

	gtk_widget_destroy (data->dialog);
}


static gboolean
update_progress_cb (gpointer callback_data)
{
	DialogData *data = callback_data;
	gint64      sector = 0;

	if (data->update_handle != 0) {
		g_source_remove (data->update_handle);
		data->update_handle = 0;
	}

	if (! gst_pad_query_position (data->source_pad, data->sector_format, &sector))
		return FALSE;

	data->current_track_sectors = sector;
	g_idle_add (update_ui, data);

	data->update_handle = g_timeout_add (UPDATE_DELAY, update_progress_cb, data);

	return FALSE;
}


static gboolean
create_pipeline (DialogData *data)
{
	GstBus     *bus;
	GstElement *audioconvert;
	GstElement *audioresample;
	GstElement *queue;
	float       quality;
	int         flac_compression;
	gboolean    result;

	data->pipeline = gst_pipeline_new ("pipeline");

	data->source = gst_element_factory_make ("cdparanoiasrc", "source");
	g_object_set (G_OBJECT (data->source),
		      "device", brasero_drive_get_device (data->drive),
		      /*"read-speed", G_MAXINT,*/
		      NULL);

	audioconvert = gst_element_factory_make ("audioconvert", "convert");
    	audioresample = gst_element_factory_make ("audioresample", "resample");

	queue = gst_element_factory_make ("queue", "queue");
	g_object_set (queue, "max-size-time", 120 * GST_SECOND, NULL);

	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:
		data->ext = "ogg";
		data->encoder = gst_element_factory_make (OGG_ENCODER, "encoder");
		quality = g_settings_get_double (data->settings_encoder, PREF_ENCODER_OGG_QUALITY);
		g_object_set (data->encoder,
			      "quality", quality,
			      NULL);
		data->container = gst_element_factory_make ("oggmux", "container");
		break;

	case GOO_FILE_FORMAT_FLAC:
		data->ext = "flac";
		data->encoder = gst_element_factory_make (FLAC_ENCODER, "encoder");
		flac_compression = g_settings_get_int (data->settings_encoder, PREF_ENCODER_FLAC_COMPRESSION);
		g_object_set (data->encoder,
			      "quality", flac_compression,
			      NULL);
		data->container = NULL;
		break;

	case GOO_FILE_FORMAT_WAVE:
		data->ext = "wav";
		data->encoder = gst_element_factory_make (WAVE_ENCODER, "encoder");
		data->container = NULL;
		break;

	case GOO_FILE_FORMAT_MP3:
		data->ext = "mp3";
		data->encoder = gst_element_factory_make (MP3_ENCODER, "encoder");
		quality = g_settings_get_int (data->settings_encoder, PREF_ENCODER_MP3_QUALITY);
		g_object_set (data->encoder,
			      "quality", quality,
			      NULL);
		data->container = NULL;
		break;
	}

	data->sink = gst_element_factory_make ("giosink", "sink");

	if (data->container != NULL) {
		gst_bin_add_many (GST_BIN (data->pipeline), data->source, queue, audioconvert, audioresample, data->encoder, data->container, data->sink, NULL);
		result = gst_element_link_many (data->source, queue, audioconvert, audioresample, data->encoder, data->container, data->sink, NULL);
	}
	else {
		gst_bin_add_many (GST_BIN (data->pipeline), data->source, queue, audioconvert, audioresample, data->encoder, data->sink, NULL);
		result = gst_element_link_many (data->source, queue, audioconvert, audioresample, data->encoder, data->sink, NULL);
	}

	if (! result) {
		GError *error;

		g_clear_object (&data->pipeline);

		error = g_error_new (G_IO_ERROR, G_IO_ERROR_INVAL, "Could not link the pipeline");
		_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->window), _("Could not extract the tracks"), &error);

		return FALSE;
	}

	data->track_format = gst_format_get_by_nick ("track");
	data->sector_format = gst_format_get_by_nick ("sector");
	data->source_pad = gst_element_get_static_pad (data->source, "src");

	bus = gst_element_get_bus (data->pipeline);
	gst_bus_add_signal_watch (bus);

	g_signal_connect (G_OBJECT (bus),
			  "message::error",
			  G_CALLBACK (pipeline_error_cb),
			  data);
	g_signal_connect (G_OBJECT (bus),
			  "message::eos",
			  G_CALLBACK (pipeline_eos_cb),
			  data);

	return TRUE;
}


static gboolean
valid_filename_char (char c)
{
	/* "$\'`\"\\!?* ()[]<>&|@#:;" */
	return strchr ("/\\!?*:;'`\"", c) == NULL;
}


/* Remove special characters from a track title in order to make it a
 * valid filename. */
static char *
tracktitle_to_filename (const char *trackname,
			gboolean    escape)
{
	char       *filename, *f, *f2;
	const char *t;
	gboolean    add_space;

	if (trackname == NULL)
		return NULL;

	filename = g_new (char, strlen (trackname) + 1);

	/* Substitute invalid characters with spaces. */

	f = filename;
	t = trackname;
	while (*t != 0) {
		gboolean invalid_char = FALSE;

		while ((*t != 0) && ! valid_filename_char (*t)) {
			invalid_char = TRUE;
			t++;
		}

		if (invalid_char)
			*f++ = ' ';

		*f = *t;

		if (*t != 0) {
			f++;
			t++;
		}
	}
	*f = 0;

	/* Remove double spaces. */
	add_space = FALSE;
	for (f = f2 = filename; *f != 0; f++)
		if (*f != ' ') {
			if (add_space) {
				*f2++ = ' ';
				add_space = FALSE;
			}
			*f2 = *f;
			f2++;
		} else
			add_space = TRUE;
	*f2 = 0;

	if (escape) {
		char *escaped;

		escaped = g_uri_escape_string (filename, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);
		g_free (filename);
		filename = escaped;
	}

	return filename;
}


static GFile *
get_destination_folder (DialogData  *data,
		 	GError     **error)
{
	char  *artist_filename;
	char  *album_filename;
	char  *uri;
	GFile *folder;

	artist_filename = tracktitle_to_filename (data->album->artist, TRUE);
	album_filename = tracktitle_to_filename (data->album->title, TRUE);
	uri = g_strconcat (data->destination,
			   G_DIR_SEPARATOR_S,
			   artist_filename,
			   G_DIR_SEPARATOR_S,
			   album_filename,
			   NULL);
	folder = g_file_new_for_uri (uri);
	if (folder == NULL) {
		if (error != NULL)
			*error = g_error_new (G_IO_ERROR, G_IO_ERROR_INVALID_FILENAME, _("Invalid destination folder: %s"), uri);
	}

	g_free (uri);
	g_free (artist_filename);
	g_free (album_filename);

	return folder;
}


static void
done_dialog_response_cb (GtkDialog  *dialog,
			 int         response,
			 gpointer    userdata)
{
	DialogData *data = (DialogData*) userdata;

	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (response == _GTK_RESPONSE_VIEW) {
		GFile  *folder;
		GError *error = NULL;

		folder = get_destination_folder (data, &error);
		if (folder != NULL) {
			char *uri;

			uri = g_file_get_uri (folder);
			if (! gtk_show_uri_on_window (GTK_WINDOW (data->window), uri, GDK_CURRENT_TIME, &error))
				_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->window), _("Could not display the destination folder"), &error);

			g_free (uri);
		}
		else
			_gtk_error_dialog_from_gerror_run (GTK_WINDOW (data->window), _("Could not display the destination folder"), &error);

		g_object_unref (folder);
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

	filename = tracktitle_to_filename (track->title, FALSE);
	track_filename = g_strdup_printf ("%s. %s.%s", zero_padded (track->number + 1), filename, ext);
	g_free (filename);

	return track_filename;
}


static void
save_playlist (DialogData *data)
{
	GFile         *folder;
	char          *album_filename;
	char          *playlist_filename;
	GFile         *file;
	GOutputStream *stream;

	folder = get_destination_folder (data, NULL);
	if (folder == NULL)
		return;

	album_filename = tracktitle_to_filename (data->album->title, FALSE);
	playlist_filename = g_strconcat (album_filename, ".pls", NULL);
	file = g_file_get_child (folder, playlist_filename);

	g_file_delete (file, NULL, NULL);

	stream = (GOutputStream *) g_file_create (file, G_FILE_CREATE_NONE, NULL, NULL);
	if (stream != NULL) {
		char   buffer[BUFFER_SIZE];
		GList *scan;
		int    n = 0;

		strcpy (buffer, "[playlist]\n");
		g_output_stream_write (stream, buffer, strlen (buffer), NULL, NULL);

		sprintf (buffer, "NumberOfEntries=%d\n", data->n_tracks);
		g_output_stream_write (stream, buffer, strlen (buffer), NULL, NULL);

		strcpy (buffer, "Version=2\n");
		g_output_stream_write (stream, buffer, strlen (buffer), NULL, NULL);

		for (scan = data->tracks; scan; scan = scan->next) {
			TrackInfo *track = scan->data;
			char      *track_filename;
			char      *unescaped;

			n++;

			track_filename = get_track_filename (track, data->ext);
			unescaped = g_uri_unescape_string (track_filename, "");

			sprintf (buffer, "File%d=%s\n", n, unescaped);
			g_output_stream_write (stream, buffer, strlen (buffer), NULL, NULL);

			g_free (unescaped);
			g_free (track_filename);

			sprintf (buffer, "Title%d=%s - %s\n", n, data->album->artist, track->title);
			g_output_stream_write (stream, buffer, strlen (buffer), NULL, NULL);

			sprintf (buffer, "Length%d=%d\n", n, track->min * 60 + track->sec);
			g_output_stream_write (stream, buffer, strlen (buffer), NULL, NULL);
		}

		g_object_unref (stream);
	}

	g_object_unref (file);
	g_free (playlist_filename);
	g_free (album_filename);
	g_object_unref (folder);
}


static void
rip_current_track (DialogData *data)
{
	TrackInfo *track;
	char      *escaped;
	GFile     *folder;
	GError    *error = NULL;
	char      *filename;

	if (data->current_track == NULL) {
		GtkWidget *d;

		if (g_settings_get_boolean (data->settings_ripper, PREF_RIPPER_SAVE_PLAYLIST))
			save_playlist (data);

		data->ripping = FALSE;
		gtk_widget_hide (data->dialog);

		d = _gtk_message_dialog_new (GTK_WINDOW (data->window),
					     GTK_DIALOG_MODAL,
					     _("Tracks extracted successfully"),
					     NULL,
					     _GTK_LABEL_CLOSE, GTK_RESPONSE_CLOSE,
					     _("_View"), _GTK_RESPONSE_VIEW,
					     NULL);

		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (done_dialog_response_cb),
				  data);
		gtk_window_present (GTK_WINDOW (d));

		return;
	}

	track = data->current_track->data;

	escaped = g_markup_printf_escaped ("<i>%s</i>", track->title);
	gtk_label_set_markup (GTK_LABEL (GET_WIDGET ("track_label")), escaped);
	g_free (escaped);

	/* Set the destination file */

	folder = get_destination_folder (data, &error);
	if (folder == NULL) {
		_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->window),
						    _("Could not extract tracks"),
						    &error);
		gtk_widget_destroy (data->dialog);
		return;
	}

	if (! g_file_make_directory_with_parents (folder, NULL, &error)) {
		if (! g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
			_gtk_error_dialog_from_gerror_show (GTK_WINDOW (data->window),
							    _("Could not extract tracks"),
							    &error);
			gtk_widget_destroy (data->dialog);
			return;
		}
	}

	_g_object_unref (data->current_file);
	filename = get_track_filename (track, data->ext);
	data->current_file = g_file_get_child (folder, filename);

	g_free (filename);
	g_object_unref (folder);

	g_file_delete (data->current_file, NULL, NULL);

	gst_element_set_state (data->pipeline, GST_STATE_NULL);
	g_object_set (G_OBJECT (data->sink), "file", data->current_file, NULL);
	g_object_set (G_OBJECT (data->source), "track", track->number + 1, NULL);

	/* Set track tags. */

	if (GST_IS_TAG_SETTER (data->encoder)) {
		gst_tag_setter_add_tags (GST_TAG_SETTER (data->encoder),
				         GST_TAG_MERGE_REPLACE,
				         GST_TAG_TITLE, track->title,
				         GST_TAG_ARTIST, data->album->artist,
				         GST_TAG_ALBUM, data->album->title,
				         GST_TAG_TRACK_NUMBER, (guint) track->number + 1,
				         GST_TAG_TRACK_COUNT, (guint) data->n_tracks,
				         GST_TAG_DURATION, (guint64) track->length * GST_SECOND,
				         GST_TAG_COMMENT, _("Ripped with CD Player"),
				         GST_TAG_ENCODER, PACKAGE_NAME,
				         GST_TAG_ENCODER_VERSION, PACKAGE_VERSION,
				         NULL);
		if (data->album->genre != NULL)
			gst_tag_setter_add_tags (GST_TAG_SETTER (data->encoder),
					         GST_TAG_MERGE_REPLACE,
					         GST_TAG_GENRE, data->album->genre,
					         NULL);
		if (g_date_valid (data->album->release_date))
			gst_tag_setter_add_tags (GST_TAG_SETTER (data->encoder),
					         GST_TAG_MERGE_REPLACE,
					         GST_TAG_DATE, data->album->release_date,
					         NULL);
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

	if (! create_pipeline (data))
		return;

	data->ripping = TRUE;
	brasero_drive_lock (data->drive, _("Extracting disc tracks"), NULL);

	data->prev_tracks_sectors = 0;
	data->total_sectors = 0;
	for (scan = data->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		data->total_sectors += track->sectors;
	}
	data->current_track_n = 1;
	data->current_track = data->tracks;

	g_timer_start (data->timer);
	rip_current_track (data);
}


static void
dialog_response_cb (GtkWidget  *dialog,
		    int         response_id,
		    DialogData *data)
{
	gtk_widget_destroy (dialog);
}


void
dlg_ripper (GooWindow *window,
	    GList     *tracks)
{
	GooPlayer  *player;
	DialogData *data;

	goo_window_stop (window);
	player = goo_window_get_player (window);

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->settings_ripper = g_settings_new (GOOBOX_SCHEMA_RIPPER);
	data->settings_encoder = g_settings_new (GOOBOX_SCHEMA_ENCODER);
	data->builder = _gtk_builder_new_from_resource ("ripper.ui");

	data->dialog = g_object_new (GTK_TYPE_DIALOG,
				     "title", _("Extracting Tracks"),
				     "transient-for", GTK_WINDOW (window),
				     "modal", TRUE,
				     "use-header-bar", _gtk_settings_get_dialogs_use_header (),
				     "resizable", TRUE,
				     NULL);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (data->dialog))),
			   GET_WIDGET ("ripper_dialog"));
	gtk_dialog_add_buttons (GTK_DIALOG (data->dialog),
				_GTK_LABEL_CANCEL, GTK_RESPONSE_CANCEL,
				NULL);

	data->destination = g_settings_get_string (data->settings_ripper, PREF_RIPPER_DESTINATION);
	if ((data->destination == NULL) || (strcmp (data->destination, "") == 0))
		data->destination = g_filename_to_uri (g_get_user_special_dir (G_USER_DIRECTORY_MUSIC), NULL, NULL);
	if (data->destination == NULL)
		data->destination = g_filename_to_uri (g_get_home_dir (), NULL, NULL);
	data->drive = g_object_ref (goo_player_get_drive (player));
	data->format = g_settings_get_enum (data->settings_ripper, PREF_RIPPER_FILETYPE);
	data->album = goo_player_get_album (player);
	if (tracks != NULL)
		data->tracks = track_list_dup (tracks);
	else
		data->tracks = track_list_dup (data->album->tracks);
	data->n_tracks = g_list_length (data->tracks);
	data->update_handle = 0;
	data->timer = g_timer_new ();

	/* Set widgets data. */

	gtk_label_set_ellipsize (GTK_LABEL (GET_WIDGET ("track_label")), PANGO_ELLIPSIZE_END);

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "destroy",
			  G_CALLBACK (dialog_destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (data->dialog),
			  "response",
			  G_CALLBACK (dialog_response_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);

	start_ripper (data);
}
