/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004, 2007 Free Software Foundation, Inc.
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
#include <math.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gst/gst.h>
#include "goo-player.h"
#include "goo-marshal.h"
#include "glib-utils.h"
#include "gth-user-dir.h"
#include "main.h"
#include "metadata.h"

#define TOC_OFFSET 150
#define SECTORS_PER_SEC 75
#define POLL_TIMEOUT 1000
#define REFRESH_RATE 5
#define PROGRESS_DELAY 400
#define QUEUE_SIZE 16384U /*131072U*/

struct _GooPlayerPrivate {
	BraseroDrive    *drive;
	gulong           medium_added_event;
	gulong           medium_removed_event;

	GooPlayerState   state;
	GooPlayerAction  action;
	double           volume_value;
	gboolean         is_busy;
	gboolean         audio_cd;
	gboolean         hibernate;

	GstElement      *pipeline;
	GstElement      *source;
	GstPad          *source_pad;
	GstElement      *audio_volume;
	GstFormat        track_format;
	GstFormat        sector_format;

	char            *discid;
	AlbumInfo       *album;
	TrackInfo       *current_track;
	int              current_track_n;

	guint            update_state_id;
	guint            update_progress_id;

	GMutex           data_mutex;
	gboolean         exiting;
	GCancellable    *cancellable;
	GList           *albums;
};

enum {
	START,
        DONE,
	PROGRESS,
	MESSAGE,
	STATE_CHANGED,
        LAST_SIGNAL
};

static guint goo_player_signals[LAST_SIGNAL] = { 0 };

static void goo_player_finalize    (GObject *object);

#define GOO_PLAYER_GET_PRIVATE_DATA(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), GOO_TYPE_PLAYER, GooPlayerPrivate))

G_DEFINE_TYPE (GooPlayer, goo_player, G_TYPE_OBJECT)


static void
destroy_pipeline (GooPlayer *player,
		  gboolean   poll)
{
	if (player->priv->pipeline != NULL) {
		gst_element_set_state (player->priv->pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (player->priv->pipeline));
		player->priv->pipeline = NULL;
	}

	if (player->priv->update_progress_id != 0) {
		g_source_remove (player->priv->update_progress_id);
		player->priv->update_progress_id = 0;
	}

	player->priv->source = NULL;
	player->priv->source_pad = NULL;
	player->priv->audio_volume = NULL;
}


static void
action_done (GooPlayer       *self,
	     GooPlayerAction  action)
{
	g_signal_emit_by_name (G_OBJECT (self), "done", action, NULL);
}


static gboolean
player_done_cb (gpointer user_data)
{
	action_done ((GooPlayer *) user_data, GOO_PLAYER_ACTION_PLAY);
	return FALSE;
}


static gboolean
pipeline_bus_message_cb (GstBus     *bus,
		         GstMessage *message,
		         gpointer    user_data)
{
	GooPlayer *self = user_data;

	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_EOS:
		if (self->priv->update_progress_id != 0) {
			g_source_remove (self->priv->update_progress_id);
			self->priv->update_progress_id = 0;
		}
		g_idle_add (player_done_cb, user_data);
		break;

	default:
		break;
	}

	return TRUE;
}


static gboolean
update_progress_cb (gpointer user_data)
{
	GooPlayer *self = user_data;
	gint64     sector = 0;

	if (self->priv->update_progress_id != 0) {
		g_source_remove (self->priv->update_progress_id);
		self->priv->update_progress_id = 0;
	}

	if ((self->priv->current_track == NULL)
	    || ! gst_pad_query_position (self->priv->source_pad,
					 &self->priv->sector_format,
					 &sector))
	{
		return FALSE;
	}

	g_signal_emit_by_name (G_OBJECT (self),
			       "progress",
			       ((double) sector) / (double) self->priv->current_track->sectors,
			       NULL);

	self->priv->update_progress_id = g_timeout_add (PROGRESS_DELAY, update_progress_cb, user_data);

	return FALSE;
}


static void
create_pipeline (GooPlayer *self)
{
	GstElement *audioconvert;
	GstElement *audioresample;
	GstElement *queue;
	GstElement *sink;

	destroy_pipeline (self, FALSE);

	self->priv->pipeline = gst_pipeline_new ("pipeline");

	/*self->priv->source = gst_element_make_from_uri (GST_URI_SRC, "cdda://1", "source");*/
	self->priv->source = gst_element_factory_make ("cdparanoiasrc", "source");

	debug (DEBUG_INFO, "DEVICE: %s\n", brasero_drive_get_device (self->priv->drive));
	g_object_set (G_OBJECT (self->priv->source),
		      "device", brasero_drive_get_device (self->priv->drive),
		      NULL);

	audioconvert = gst_element_factory_make ("audioconvert", "convert");
    	audioresample = gst_element_factory_make ("audioresample", "resample");
	self->priv->audio_volume = gst_element_factory_make ("volume", "volume");

	queue = gst_element_factory_make ("queue", "queue");
	g_object_set (queue,
		      "min-threshold-time", (guint64) 200 * GST_MSECOND,
		      "max-size-time", (guint64) 2 * GST_SECOND,
		      NULL);

	sink = gst_element_factory_make ("gsettingsaudiosink", "sink");

	gst_bin_add_many (GST_BIN (self->priv->pipeline),
			  self->priv->source,
			  queue,
			  audioconvert,
			  audioresample,
			  self->priv->audio_volume,
			  sink,
			  NULL);
	gst_element_link_many (self->priv->source,
			       queue,
			       audioconvert,
			       audioresample,
			       self->priv->audio_volume,
			       sink,
			       NULL);

	self->priv->track_format = gst_format_get_by_nick ("track");
	self->priv->sector_format = gst_format_get_by_nick ("sector");
	self->priv->source_pad = gst_element_get_pad (self->priv->source, "src");

	gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (self->priv->pipeline)),
			   pipeline_bus_message_cb,
			   self);
}


static void
goo_player_empty_list (GooPlayer *player)
{
	album_info_unref (player->priv->album);
	player->priv->album = album_info_new ();
	player->priv->current_track = NULL;
	player->priv->current_track_n = -1;
}


static void
goo_player_set_state (GooPlayer       *self,
		      GooPlayerState   state,
		      gboolean         notify)
{
	if (state == GOO_PLAYER_STATE_PLAYING)
		brasero_drive_lock (self->priv->drive, _("Playing CD"), NULL);
	else
		brasero_drive_unlock (self->priv->drive);

	self->priv->state = state;
	if (notify)
		g_signal_emit (G_OBJECT (self),
			       goo_player_signals[STATE_CHANGED],
			       0,
			       NULL);
}


static void
goo_player_class_init (GooPlayerClass *class)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (class);
	gobject_class->finalize = goo_player_finalize;

	g_type_class_add_private (class, sizeof (GooPlayerPrivate));

	goo_player_signals[START] =
                g_signal_new ("start",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, start),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);
	goo_player_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, done),
			      NULL, NULL,
			      goo_marshal_VOID__INT_POINTER,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      G_TYPE_POINTER);
	goo_player_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, progress),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);
	goo_player_signals[MESSAGE] =
		g_signal_new ("message",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, message),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
	goo_player_signals[STATE_CHANGED] =
		g_signal_new ("state_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, state_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}


static void
goo_player_init (GooPlayer *self)
{
	self->priv = GOO_PLAYER_GET_PRIVATE_DATA (self);
	self->priv->state = GOO_PLAYER_STATE_NO_DISC;
	self->priv->action = GOO_PLAYER_ACTION_NONE;
	self->priv->is_busy = FALSE;
	self->priv->hibernate = FALSE;
	g_mutex_init (&self->priv->data_mutex);
	self->priv->exiting = FALSE,
	self->priv->discid = NULL;
	self->priv->album = album_info_new ();
	self->priv->current_track_n = -1;
	self->priv->volume_value = 1.0;
	self->priv->update_progress_id = 0;
	self->priv->albums = NULL;
	self->priv->cancellable = g_cancellable_new ();
}


static void
goo_player_finalize (GObject *object)
{
        GooPlayer *self;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_PLAYER (object));

	self = GOO_PLAYER (object);

	g_mutex_lock (&self->priv->data_mutex);
	self->priv->exiting = TRUE;
        g_mutex_unlock (&self->priv->data_mutex);

        brasero_drive_unlock (self->priv->drive);
        if (self->priv->medium_added_event != 0)
		g_signal_handler_disconnect (self->priv->drive, self->priv->medium_added_event);
	if (self->priv->medium_removed_event != 0)
		g_signal_handler_disconnect (self->priv->drive, self->priv->medium_removed_event);
	g_object_unref (self->priv->drive);

	if (self->priv->update_progress_id != 0) {
		g_source_remove (self->priv->update_progress_id);
		self->priv->update_progress_id = 0;
	}

	destroy_pipeline (self, FALSE);
	g_mutex_clear (&self->priv->data_mutex);
	destroy_pipeline (self, FALSE);
	g_free (self->priv->discid);
	album_info_unref (self->priv->album);
	g_object_unref (self->priv->cancellable);

	G_OBJECT_CLASS (goo_player_parent_class)->finalize (object);
}


static void
drive_medium_added_cb (BraseroDrive  *drive,
		       BraseroMedium *medium,
		       gpointer       user_data)
{
	GooPlayer *self = user_data;

	action_done (self, GOO_PLAYER_ACTION_MEDIUM_ADDED);
	goo_player_update (self);
}


static void
drive_medium_removed_cb (BraseroDrive  *drive,
		         BraseroMedium *medium,
		         gpointer       user_data)
{
	GooPlayer *self = user_data;

	action_done (self, GOO_PLAYER_ACTION_MEDIUM_REMOVED);
	goo_player_update (self);
}


GooPlayer *
goo_player_new (BraseroDrive *drive)
{
	GooPlayer *self;

	g_return_val_if_fail (drive != NULL, NULL);

	self = GOO_PLAYER (g_object_new (GOO_TYPE_PLAYER, NULL));
	goo_player_set_drive (self, drive);

	return self;
}


static void
notify_action_start (GooPlayer *self)
{
	g_signal_emit (G_OBJECT (self),
		       goo_player_signals[START],
		       0,
		       self->priv->action,
		       NULL);
}


static void
goo_player_set_is_busy (GooPlayer *self,
			gboolean   is_busy)
{
	self->priv->is_busy = is_busy;
}


/* -- goo_player_list -- */


void
goo_player_set_album (GooPlayer *self,
		      AlbumInfo *album)
{
	if (self->priv->album == NULL)
		return;
	album_info_copy_metadata (self->priv->album, album);
	album_info_save_to_cache (self->priv->album, self->priv->discid);
	action_done (self, GOO_PLAYER_ACTION_METADATA);
}


gboolean
goo_player_is_audio_cd (GooPlayer *self)
{
	return self->priv->audio_cd;
}


void
goo_player_hibernate (GooPlayer *self,
		      gboolean   hibernate)
{
	self->priv->hibernate = hibernate;
}


gboolean
goo_player_is_hibernate (GooPlayer *self)
{
	return self->priv->hibernate;
}


void
goo_player_update (GooPlayer *self)
{
	BraseroMedium *medium;

	if (self->priv->hibernate)
		return;

	self->priv->audio_cd = FALSE;

	medium = brasero_drive_get_medium (self->priv->drive);
	if (medium == NULL) {
		goo_player_stop (self);
		goo_player_set_state (self, GOO_PLAYER_STATE_NO_DISC, TRUE);
		goo_player_empty_list (self);
		action_done (self, GOO_PLAYER_ACTION_LIST);
	}
	else if ((BRASERO_MEDIUM_IS (brasero_medium_get_status (medium), BRASERO_MEDIUM_CD | BRASERO_MEDIUM_HAS_AUDIO))) {
		self->priv->audio_cd = TRUE;
		goo_player_set_state (self, GOO_PLAYER_STATE_STOPPED, TRUE);
		goo_player_list (self);
	}
	else {
		goo_player_stop (self);
		goo_player_set_state (self, GOO_PLAYER_STATE_DATA_DISC, TRUE);
		goo_player_empty_list (self);
		action_done (self, GOO_PLAYER_ACTION_LIST);
	}
}


static void
album_info_from_disc_id_ready_cb (GObject      *source_object,
				  GAsyncResult *result,
				  gpointer      user_data)
{
	GooPlayer *player = user_data;
	GList     *albums;
	GError    *error = NULL;

	albums = metadata_get_album_info_from_disc_id_finish (result, &error);
	if (albums != NULL) {
		AlbumInfo *first_album = albums->data;

		/* FIXME: ask the user which album to use if the query
		 * returned more than one album. */

		goo_player_set_album (player, first_album);
		album_info_save_to_cache (player->priv->album, player->priv->discid);

		album_list_free (albums);
	}
}


static void
get_cd_info_from_device_ready_cb (GObject      *source_object,
				  GAsyncResult *result,
				  gpointer      user_data)
{
	GooPlayer *player = user_data;
	AlbumInfo *album;
	GError    *error = NULL;

	if (metadata_get_cd_info_from_device_finish (result,
						     &player->priv->discid,
						     &album,
						     &error))
	{
		album_info_set_tracks (player->priv->album, album->tracks);
	}

	goo_player_set_is_busy (player, FALSE);
	gst_element_set_state (player->priv->pipeline, GST_STATE_NULL);
	goo_player_set_state (player, GOO_PLAYER_STATE_STOPPED, TRUE);
	action_done (player, GOO_PLAYER_ACTION_LIST);
	destroy_pipeline (player, TRUE);

	if (player->priv->discid == NULL)
		return;

	if (album_info_load_from_cache (player->priv->album, player->priv->discid)) {
		action_done (player, GOO_PLAYER_ACTION_METADATA);
		return;
	}

	metadata_get_album_info_from_disc_id (player->priv->discid,
					      player->priv->cancellable,
					      album_info_from_disc_id_ready_cb,
					      player);

	album_info_unref (album);
}


void
goo_player_list (GooPlayer *player)
{
	if (goo_player_get_is_busy (player))
		return;

	player->priv->action = GOO_PLAYER_ACTION_LIST;
	player->priv->state = GOO_PLAYER_STATE_LISTING;
	notify_action_start (player);

	goo_player_empty_list (player);
	goo_player_set_is_busy (player, TRUE);
	create_pipeline (player);

	if (player->priv->pipeline != NULL)
		gst_element_set_state (player->priv->pipeline, GST_STATE_PAUSED);

	g_free (player->priv->discid);
	player->priv->discid = NULL;

	metadata_get_cd_info_from_device (goo_player_get_device (player),
					  player->priv->cancellable,
					  get_cd_info_from_device_ready_cb,
					  player);
}


static TrackInfo*
get_track (GooPlayer *player,
           guint      n)
{
	GList *scan;

	for (scan = player->priv->album->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;

		if (track->number == n)
			return track;
	}

	return NULL;
}


void
goo_player_seek_track (GooPlayer *player,
		       int        n)
{
	int track_n;

	if (goo_player_get_is_busy (player))
		return;

	player->priv->action = GOO_PLAYER_ACTION_SEEK_SONG;
	player->priv->state = GOO_PLAYER_STATE_SEEKING;
	notify_action_start (player);

	if (player->priv->album->n_tracks == 0) {
		action_done (player, GOO_PLAYER_ACTION_SEEK_SONG);
		return;
	}

	goo_player_stop (player);
	create_pipeline (player);
	gst_element_set_state (player->priv->pipeline, GST_STATE_PAUSED);

	/* seek to track */

	goo_player_set_state (player, GOO_PLAYER_STATE_SEEKING, TRUE);

	track_n = CLAMP (n, 0, player->priv->album->n_tracks - 1);
	player->priv->current_track_n = track_n;

	debug (DEBUG_INFO, "seek to track %d\n", track_n);

	player->priv->current_track = get_track (player, player->priv->current_track_n);

	g_return_if_fail (player->priv->current_track != NULL);

	if (! gst_element_seek (player->priv->pipeline,
			        1.0,
			        player->priv->track_format,
			        GST_SEEK_FLAG_FLUSH,
			        GST_SEEK_TYPE_SET,
			        track_n,
			        GST_SEEK_TYPE_SET,
			        track_n + 1))
	{
		g_warning ("seek failed");
	}
	gst_element_get_state (player->priv->pipeline, NULL, NULL, -1);

	action_done (player, GOO_PLAYER_ACTION_SEEK_SONG);
	goo_player_play (player);
}


int
goo_player_get_current_track (GooPlayer *player)
{
	return player->priv->current_track_n;
}


void
goo_player_skip_to (GooPlayer *player,
		    guint      seconds)
{
	if (goo_player_get_is_busy (player))
		return;

	if (player->priv->pipeline == NULL)
		return;

	gst_element_set_state (player->priv->pipeline, GST_STATE_PAUSED);
	gst_element_seek (player->priv->pipeline,
			  1.0,
			  GST_FORMAT_TIME,
			  GST_SEEK_FLAG_FLUSH,
			  GST_SEEK_TYPE_SET,
			  G_GINT64_CONSTANT (1000000000) * seconds,
			  GST_SEEK_TYPE_NONE,
			  0);
	gst_element_get_state (player->priv->pipeline, NULL, NULL, -1);
	gst_element_set_state (player->priv->pipeline, GST_STATE_PLAYING);
}


void
goo_player_set_drive (GooPlayer    *self,
		      BraseroDrive *drive)
{
	g_return_if_fail (drive != NULL);

	if (self->priv->drive != NULL) {
		if (self->priv->medium_added_event != 0)
			g_signal_handler_disconnect (self->priv->drive, self->priv->medium_added_event);
		if (self->priv->medium_removed_event != 0)
			g_signal_handler_disconnect (self->priv->drive, self->priv->medium_removed_event);
		g_object_unref (self->priv->drive);
	}

	self->priv->drive = g_object_ref (drive);
	self->priv->medium_added_event =
			g_signal_connect (self->priv->drive,
					  "medium-added",
					  G_CALLBACK (drive_medium_added_cb),
					  self);
	self->priv->medium_removed_event =
			g_signal_connect (self->priv->drive,
					  "medium-removed",
					  G_CALLBACK (drive_medium_removed_cb),
					  self);
}


BraseroDrive *
goo_player_get_drive (GooPlayer *self)
{
	return self->priv->drive;
}


const char *
goo_player_get_device (GooPlayer *self)
{
	return brasero_drive_get_device (self->priv->drive);
}


void
goo_player_play (GooPlayer *player)
{
	if (goo_player_get_is_busy (player))
		return;
	if (player->priv->state == GOO_PLAYER_STATE_PLAYING)
		return;

	player->priv->action = GOO_PLAYER_ACTION_PLAY;
	notify_action_start (player);

	if (player->priv->album->n_tracks == 0) {
		action_done (player, GOO_PLAYER_ACTION_PLAY);
		return;
	}

	if (! ((player->priv->pipeline != NULL)
	       && ((goo_player_get_state (player) == GOO_PLAYER_STATE_PAUSED)
		   || (goo_player_get_state (player) == GOO_PLAYER_STATE_SEEKING))))
	{
		create_pipeline (player);
	}

	g_object_set (G_OBJECT (player->priv->audio_volume), "volume", goo_player_get_audio_volume (player), NULL);

	gst_element_set_state (player->priv->pipeline, GST_STATE_PLAYING);
	goo_player_set_state (player, GOO_PLAYER_STATE_PLAYING, TRUE);

	player->priv->update_progress_id = g_timeout_add (PROGRESS_DELAY, update_progress_cb, player);
}


void
goo_player_pause (GooPlayer *player)
{
	if (goo_player_get_is_busy (player))
		return;
	if (player->priv->state == GOO_PLAYER_STATE_PAUSED)
		return;
	if (player->priv->pipeline == NULL)
		return;

	gst_element_set_state (player->priv->pipeline, GST_STATE_PAUSED);
	goo_player_set_state (GOO_PLAYER (player), GOO_PLAYER_STATE_PAUSED, TRUE);

	action_done (player, GOO_PLAYER_ACTION_PAUSE);
}


void
goo_player_stop (GooPlayer *player)
{
	if (goo_player_get_is_busy (player))
		return;
	if (player->priv->state == GOO_PLAYER_STATE_STOPPED)
		return;
	if (player->priv->pipeline == NULL)
		return;

	destroy_pipeline (player, TRUE);
	goo_player_set_state (GOO_PLAYER (player), GOO_PLAYER_STATE_STOPPED, TRUE);

	action_done (player, GOO_PLAYER_ACTION_STOP);
}



static void
eject_ready_cb (GObject      *source_object,
                GAsyncResult *result,
                gpointer      user_data)
{
	GooPlayer *self = user_data;
	GError    *error = NULL;

	if (! g_drive_eject_with_operation_finish (G_DRIVE (source_object), result, &error))
		g_signal_emit_by_name (G_OBJECT (self), "done", GOO_PLAYER_ACTION_MEDIUM_REMOVED, error);
	else
		g_signal_emit_by_name (G_OBJECT (self), "done", GOO_PLAYER_ACTION_MEDIUM_REMOVED, NULL);

	goo_player_set_state (self, GOO_PLAYER_STATE_STOPPED, TRUE);
}


void
goo_player_eject (GooPlayer *self)
{
	GDrive *gdrive;

	if (self->priv->hibernate)
		return;

	g_signal_emit_by_name (G_OBJECT (self), "start", GOO_PLAYER_ACTION_MEDIUM_REMOVED);

	gdrive = brasero_drive_get_gdrive (self->priv->drive);
	g_drive_eject_with_operation (gdrive,
				      G_MOUNT_UNMOUNT_NONE,
				      NULL,
				      NULL,
				      eject_ready_cb,
				      self);

	g_object_unref (gdrive);
}


GooPlayerAction
goo_player_get_action (GooPlayer *player)
{
	return player->priv->action;
}


GooPlayerState
goo_player_get_state (GooPlayer *player)
{
	return player->priv->state;
}


double
goo_player_get_audio_volume (GooPlayer *player)
{
	return player->priv->volume_value;
}


void
goo_player_set_audio_volume (GooPlayer *player,
		             double     vol)
{
	if (goo_player_get_is_busy (player))
		return;

	player->priv->volume_value = vol;
	if (player->priv->audio_volume != NULL)
		g_object_set (G_OBJECT (player->priv->audio_volume), "volume", vol, NULL);
}


gboolean
goo_player_get_is_busy (GooPlayer *self)
{
	return self->priv->is_busy || self->priv->hibernate;
}


const char *
goo_player_get_discid (GooPlayer *player)
{
	return player->priv->discid;
}


AlbumInfo *
goo_player_get_album (GooPlayer *player)
{
	return player->priv->album;
}
