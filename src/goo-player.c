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
#include <gnome.h>
#include <gst/gst.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <musicbrainz/queries.h>
#include <musicbrainz/mb_c.h>

#include "goo-player.h"
#include "goo-marshal.h"
#include "goo-cdrom.h"
#include "glib-utils.h"
#include "file-utils.h"
#include "main.h"

#define TOC_OFFSET 150
#define SECTORS_PER_SEC 75
#define POLL_TIMEOUT 1000
#define REFRESH_RATE 5
#define PROGRESS_DELAY 400
#define QUEUE_SIZE 16384U /*131072U*/

struct _GooPlayerPrivateData {
	GooPlayerState   state;
	GooPlayerAction  action;
	GError          *error;
	int              current_song;
	double           volume_value;
	char            *location;
	gboolean         is_busy;
	
	CDDrive         *drive;
	GooCdrom        *cdrom;
	GstElement      *pipeline;
	GstElement      *source;
	GstPad          *source_pad;
	GstElement      *volume;
	gint64           total_time;
	int              n_tracks;
	GstFormat        track_format;
	GstFormat        sector_format;
	
	GList           *tracks;
	TrackInfo       *current_track;
	char            *discid;
	char            *asin;
	char            *artist;
	char            *title;
	char            *genre;
	int              year;
	guint            update_state_id;

	guint            update_progress_id;

	GThread         *thread;
	GMutex          *yes_or_no;
	guint            check_id;
	gboolean         exiting;

	char            *rdf;
};

enum {
	START,
        DONE,
	PROGRESS,
	MESSAGE,
	STATE_CHANGED,
        LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint goo_player_signals[LAST_SIGNAL] = { 0 };

static void goo_player_class_init  (GooPlayerClass *class);
static void goo_player_init        (GooPlayer *player);
static void goo_player_finalize    (GObject *object);


GType
goo_player_get_type (void)
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GooPlayerClass),
			NULL,
			NULL,
			(GClassInitFunc) goo_player_class_init,
			NULL,
			NULL,
			sizeof (GooPlayer),
			0,
			(GInstanceInitFunc) goo_player_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GooPlayer",
					       &type_info,
					       0);
	}

        return type;
}


static void
remove_state_polling (GooPlayer *player)
{
	if (player->priv->update_state_id == 0) 
		return;

	debug (DEBUG_INFO, "REMOVE POLLING\n");

	g_source_remove (player->priv->update_state_id);
	player->priv->update_state_id = 0;
}


static gboolean
update_state_cb (gpointer data)
{
	GooPlayer *player = data;

	g_source_remove (player->priv->update_state_id);
	goo_cdrom_update_state (player->priv->cdrom);
	player->priv->update_state_id = g_timeout_add (POLL_TIMEOUT,
					               update_state_cb,
						       player);
						       
	return FALSE;
}


static void
add_state_polling (GooPlayer *player)
{
	if (player->priv->update_state_id != 0)
		return;

	debug (DEBUG_INFO, "ADD POLLING\n");

	player->priv->update_state_id = g_timeout_add (POLL_TIMEOUT,
						       update_state_cb,
						       player);
}


static void
destroy_pipeline (GooPlayer *player,
		  gboolean     poll)
{
	if (player->priv->cdrom != NULL)
		goo_cdrom_unlock_tray (player->priv->cdrom);

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
	player->priv->volume = NULL;

	if (poll)
		add_state_polling (player);
}


static void
action_done (GooPlayer       *player, 
	     GooPlayerAction  action)
{
	g_signal_emit_by_name (G_OBJECT (player), "done", action, NULL);
}


static gboolean
player_done_cb (gpointer callback_data)
{
	GooPlayer *player = callback_data;
	
	action_done (player, GOO_PLAYER_ACTION_PLAY);
	
	return FALSE;
}


static gboolean
pipeline_bus_message_cb (GstBus     *bus,
		         GstMessage *message,
		         gpointer    data)
{
	GooPlayer *player = (GooPlayer *) data;

	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_EOS:
		if (player->priv->update_progress_id != 0) {
			g_source_remove (player->priv->update_progress_id);
			player->priv->update_progress_id = 0;
		}
		g_idle_add (player_done_cb, data);	
		break;
		
	default:
		break;
	}
	
	return TRUE;
}


static gboolean
update_progress_cb (gpointer callback_data)
{
	GooPlayer *player = callback_data;
	gboolean   ret;
	gint64     sector = 0;
	double     fraction;

	if (player->priv->update_progress_id != 0) {
		g_source_remove (player->priv->update_progress_id);
		player->priv->update_progress_id = 0;
	}
	
	ret = gst_pad_query_position (player->priv->source_pad, &(player->priv->sector_format), &sector);
	if (!ret)
		return FALSE;

	fraction = ((double) sector) / (double) player->priv->current_track->sectors;
	g_signal_emit_by_name (G_OBJECT (player), 
			       "progress", 
			       fraction,
			       NULL);

	player->priv->update_progress_id = g_timeout_add (PROGRESS_DELAY, update_progress_cb, callback_data);

	return FALSE;
}


static void
create_pipeline (GooPlayer *player)
{
	const char *device;
	GstElement *audioconvert;
	GstElement *audioresample;
	GstElement *queue;
	GstElement *sink;

	destroy_pipeline (player, FALSE);
	remove_state_polling (player);
	goo_cdrom_lock_tray (player->priv->cdrom);

	player->priv->pipeline = gst_pipeline_new ("pipeline");

	/*player->priv->source = gst_element_make_from_uri (GST_URI_SRC, "cdda://1", "source");*/
	player->priv->source = gst_element_factory_make ("cdparanoiasrc", "source");
	device = goo_cdrom_get_device (player->priv->cdrom);
	debug (DEBUG_INFO, "DEVICE: %s\n", device);
	g_object_set (G_OBJECT (player->priv->source), 
		      "device", device, 
		      NULL);

	audioconvert = gst_element_factory_make ("audioconvert", "convert");
    	audioresample = gst_element_factory_make ("audioresample", "resample");
	player->priv->volume = gst_element_factory_make ("volume", "volume");
	
	queue = gst_element_factory_make ("queue", "queue");
	g_object_set (queue,
		      "min-threshold-time", (guint64) 200 * GST_MSECOND,
		      "max-size-time", (guint64) 2 * GST_SECOND,
		      NULL);
	
	sink = gst_element_factory_make ("gconfaudiosink", "sink");
	
	gst_bin_add_many (GST_BIN (player->priv->pipeline), 
			  player->priv->source, queue, audioconvert,
			  audioresample, player->priv->volume, sink, NULL);
	gst_element_link_many (player->priv->source, queue,
			       audioconvert, audioresample, 
			       player->priv->volume, sink, NULL);

	player->priv->track_format = gst_format_get_by_nick ("track");
	player->priv->sector_format = gst_format_get_by_nick ("sector");
	player->priv->source_pad = gst_element_get_pad (player->priv->source, "src");
	
	gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (player->priv->pipeline)),
			   pipeline_bus_message_cb, 
			   player);
}


static void
goo_player_empty_list (GooPlayer *player)
{
	track_list_free (player->priv->tracks);
	player->priv->tracks = NULL;
	
	player->priv->n_tracks = 0;
	player->priv->total_time = 0;
	player->priv->current_track = NULL;
	
	g_free (player->priv->discid);
	player->priv->discid = NULL;
	
	g_free (player->priv->artist);
	player->priv->artist = NULL;
	
	g_free (player->priv->title);
	player->priv->title = NULL;
	
	g_free (player->priv->genre);
	player->priv->genre = NULL;

	player->priv->current_song = -1;
	player->priv->year = 0;
}


static SongInfo*
create_song_info (GooPlayer *player,
		  TrackInfo *track) 
{
	SongInfo *song;

	song = song_info_new ();
	song->number = track->number;
	song->title = g_strdup (track->title);
	song->artist = g_strdup (player->priv->artist);
	song->time = track->length;

	return song;
}


static void
goo_player_set_state (GooPlayer       *player,
		      GooPlayerState   state,
		      gboolean         notify)
{
	player->priv->state = state;
	if (notify)
		g_signal_emit (G_OBJECT (player), 
			       goo_player_signals[STATE_CHANGED], 
			       0,
			       NULL);
}


gboolean
cd_player_set_location (GooPlayer  *player,
			const char *location)
{
	if (goo_player_get_is_busy (player))
		return TRUE;

	debug (DEBUG_INFO, "LOCATION: %s\n", location);

	destroy_pipeline (player, FALSE);
	remove_state_polling (player);

	goo_cdrom_set_device (player->priv->cdrom, location);

	if (location == NULL) 
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, FALSE);

	g_free (player->priv->location);
	player->priv->location = NULL;
	
	if (location != NULL)
		player->priv->location = g_strdup (location);

	return TRUE;
}


static void 
goo_player_class_init (GooPlayerClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);

        parent_class = g_type_class_peek_parent (class);

	goo_player_signals[START] =
                g_signal_new ("start",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, start),
			      NULL, NULL,
			      goo_marshal_VOID__INT,
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
			      goo_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);
	goo_player_signals[MESSAGE] =
		g_signal_new ("message",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, message),
			      NULL, NULL,
			      goo_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
	goo_player_signals[STATE_CHANGED] =
		g_signal_new ("state_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, state_changed),
			      NULL, NULL,
			      goo_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

        gobject_class->finalize = goo_player_finalize;
}


static void 
goo_player_init (GooPlayer *player)
{
	GooPlayerPrivateData *priv;

	player->priv = g_new0 (GooPlayerPrivateData, 1);
	priv = player->priv;

	priv->state = GOO_PLAYER_STATE_STOPPED;
	priv->action = GOO_PLAYER_ACTION_NONE;
	priv->error = NULL;
	priv->current_song = 0;
	priv->location = NULL;
	priv->title = NULL;
	priv->year = 0;
	priv->is_busy = FALSE;

	priv->yes_or_no = g_mutex_new ();
	priv->check_id = 0;
	priv->exiting = FALSE,

	priv->discid = NULL;
	priv->n_tracks = 0;
	priv->tracks = NULL;
	priv->current_song = -1;
	priv->volume_value = 1.0;

	priv->update_progress_id = 0;
}


static void 
goo_player_finalize (GObject *object)
{
        GooPlayer *player;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_PLAYER (object));
  
	player = GOO_PLAYER (object);
	
	g_mutex_lock (player->priv->yes_or_no);
	player->priv->exiting = TRUE;
        g_mutex_unlock (player->priv->yes_or_no);

 	if (player->priv != NULL) {
		GooPlayerPrivateData *priv = player->priv;
		
		if (priv->check_id != 0) {
			g_source_remove (priv->check_id);
			priv->check_id = 0;
		}

		destroy_pipeline (player, FALSE);

		g_mutex_free (priv->yes_or_no);

		remove_state_polling (player);
		destroy_pipeline (player, FALSE);

		g_free (priv->discid);
		g_free (priv->artist);
		g_free (priv->title);
		track_list_free (priv->tracks);

		g_free (player->priv);
		player->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
goo_player_set_error (GooPlayer  *player,
		      GError     *error)
{
     if (player->priv->error != NULL)
	     g_error_free (player->priv->error);
     player->priv->error = error;
}


static void
cdrom_state_changed_cb (GooCdrom  *cdrom,
			GooPlayer *player)
{
	GooCdromState  cdrom_state;
	char          *state;
	char          *message = "";

	if (goo_player_get_location (player) == NULL)
		return;

	cdrom_state = goo_cdrom_get_state (cdrom);

	switch (cdrom_state) {
	case GOO_CDROM_STATE_ERROR:
		state = "ERROR";
		break;
	case GOO_CDROM_STATE_UNKNOWN:
		state = "UNKNOWN";
		break;
	case GOO_CDROM_STATE_DRIVE_NOT_READY:
		state = "DRIVE_NOT_READY";
		message = _("Drive not ready");
		break;
	case GOO_CDROM_STATE_TRAY_OPEN:
		state = "TRAY_OPEN";
		message = _("Tray open");
		message = _("No disc");
		break;
	case GOO_CDROM_STATE_NO_DISC:
		state = "NO_DISC";
		message = _("No disc");
		break;
	case GOO_CDROM_STATE_DATA_CD:
		state = "DATA_CD";
		message = _("Data Disc");
		break;
	case GOO_CDROM_STATE_OK:
		state = "OK";
		break;
	default:
		break;
	}

	debug (DEBUG_INFO, "STATE CHANGED [%s]\n", state);

	if (cdrom_state == GOO_CDROM_STATE_UNKNOWN)
		return;

	else if (cdrom_state == GOO_CDROM_STATE_OK) {
		goo_player_set_state (player, GOO_PLAYER_STATE_STOPPED, TRUE);
		goo_player_list (player);
	} 
	else if (cdrom_state == GOO_CDROM_STATE_ERROR) {
		GError *error = goo_cdrom_get_error (player->priv->cdrom);
		
		goo_player_set_error (player, error);
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, TRUE);
		goo_player_empty_list (player);
		action_done (player, GOO_PLAYER_ACTION_LIST);
	} 
	else if (cdrom_state == GOO_CDROM_STATE_NO_DISC) {
		goo_player_set_error (player, g_error_new (GOO_CDROM_ERROR, 0, "%s", message));
		goo_player_set_state (player, GOO_PLAYER_STATE_NO_DISC, TRUE);
		goo_player_empty_list (player);
		action_done (player, GOO_PLAYER_ACTION_LIST);
	}
	else if (cdrom_state == GOO_CDROM_STATE_DATA_CD) {
		goo_player_set_error (player, g_error_new (GOO_CDROM_ERROR, 0, "%s", message));
		goo_player_set_state (player, GOO_PLAYER_STATE_DATA_DISC, TRUE);
		goo_player_empty_list (player);
		action_done (player, GOO_PLAYER_ACTION_LIST);
	}
	else {
		goo_player_set_error (player, g_error_new (GOO_CDROM_ERROR, 0, "%s", message));
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, TRUE);
		goo_player_empty_list (player);
		action_done (player, GOO_PLAYER_ACTION_LIST);
		if (cdrom_state == GOO_CDROM_STATE_TRAY_OPEN) 
			add_state_polling (GOO_PLAYER (player));
	}
}


GooPlayer *
goo_player_new (const char *location)
{
	GooPlayer  *player;
	const char *device;

	player = GOO_PLAYER (g_object_new (GOO_TYPE_PLAYER, NULL));
	player->priv->location = g_strdup (location);

	device = goo_player_get_location (GOO_PLAYER (player));
	player->priv->drive = get_drive_from_device (device);
	player->priv->cdrom = goo_cdrom_new (device);
	
	g_signal_connect (player->priv->cdrom, 
			  "state_changed",
			  G_CALLBACK (cdrom_state_changed_cb), 
			  GOO_PLAYER (player));

	return player;
}


CDDrive *
goo_player_get_drive (GooPlayer *player)
{
	return player->priv->drive;
}


static void
notify_action_start (GooPlayer *player)
{
	g_signal_emit (G_OBJECT (player), 
		       goo_player_signals[START], 
		       0,
		       player->priv->action, 
		       NULL);
}


void
goo_player_update (GooPlayer *player)
{
	player->priv->action = GOO_PLAYER_ACTION_UPDATE;
	player->priv->state = GOO_PLAYER_STATE_UPDATING;
	notify_action_start (player);
	
	goo_cdrom_set_state (player->priv->cdrom, GOO_CDROM_STATE_UNKNOWN);
	goo_cdrom_update_state (player->priv->cdrom);
}


/* -- goo_player_list -- */


static void
set_cd_metadata_from_rdf (GooPlayer *player, 
			  char      *rdf)
{
	musicbrainz_t  mb;
	int            n_albums;
	char           data[1024];
	int            n_track;
	GList         *scan;

	if (rdf == NULL)
		return;

	mb = mb_New ();
	mb_UseUTF8 (mb, TRUE);
	mb_SetResultRDF (mb, rdf);
	
	n_albums = mb_GetResultInt(mb, MBE_GetNumAlbums);
	g_print ("NumAlbums: %d\n", n_albums);
	
	if (n_albums != 1) 
		goto set_cd_metadata_end;
		
	mb_Select1 (mb, MBS_SelectAlbum, 1);

 	g_free (player->priv->title);
 	player->priv->title = NULL;
 		
 	g_free (player->priv->artist);
 	player->priv->artist = NULL;
 		
 	g_free (player->priv->genre);
 	player->priv->genre = NULL;
 		
 	player->priv->year = 0;

	if (mb_GetResultData(mb, MBE_AlbumGetAmazonAsin, data, sizeof (data))) {
		player->priv->asin = g_strdup (data);
		g_print ("==> [MB] ASIN: %s\n", player->priv->asin);
	}
 		
 	if (mb_GetResultData (mb, MBE_AlbumGetAlbumName, data, sizeof (data))) 
		player->priv->title = g_strdup (data);
	else
		player->priv->title = g_strdup (_("Unknown Title"));
 
	if (mb_GetResultData (mb, MBE_AlbumGetAlbumArtistId, data, sizeof (data))) 
		if (g_ascii_strncasecmp (MBI_VARIOUS_ARTIST_ID, data, 64) == 0)
			player->priv->artist = g_strdup (_("Various"));
		
	if (mb_GetResultInt (mb, MBE_AlbumGetNumTracks) != player->priv->n_tracks)
		goto set_cd_metadata_end;	
		
	for (scan = player->priv->tracks, n_track = 1; scan; scan = scan->next, n_track++) {
		TrackInfo *track = scan->data;
			
		if (mb_GetResultData1 (mb, MBE_AlbumGetTrackName, data, sizeof (data), n_track))
			track_info_set_title (track, data);
				
		if ((player->priv->artist == NULL) && (mb_GetResultData1 (mb, MBE_AlbumGetArtistName, data, sizeof (data), n_track)))
			player->priv->artist = g_strdup (data);
	}

	action_done (player, GOO_PLAYER_ACTION_METADATA);		

set_cd_metadata_end:
	
	mb_Delete (mb);
}


static void
goo_player_set_is_busy (GooPlayer *player,
			gboolean   is_busy)
{
	player->priv->is_busy = is_busy;
}


static char *
get_cached_rdf_path (GooPlayer *player)
{
	char *path = NULL;
	char *dir;
	
	if (player->priv->discid == NULL)
		return NULL;
		
	dir = g_build_filename (g_get_home_dir (), ".gnome2", "sound-juicer", "cache", NULL);
	if (ensure_dir_exists (dir, 0700) == GNOME_VFS_OK) 
		path = g_build_filename (dir, player->priv->discid, NULL);
	g_free (dir);
	
	return path;
}


static void
save_rdf_to_cache (GooPlayer  *player, 
	           const char *rdf)
{
	char   *path;
	GError *error = NULL;
	 
	if (rdf == NULL)
		return;
	
	path = get_cached_rdf_path (player);
	if (path == NULL)
		return;
	
	if (g_file_test (path, G_FILE_TEST_EXISTS)) {
		g_free (path);
    		return;
	}
  
	if (! g_file_set_contents (path, rdf, strlen (rdf), &error)) {
		g_print ("%s\n", error->message);
		g_clear_error (&error);
	}
	
	g_free (path);
}


static char *
read_cached_rdf (GooPlayer *player)
{
	char   *path;
	char   *rdf = NULL;
	GError *error = NULL;
		
	path = get_cached_rdf_path (player);
	if (path == NULL)
		return NULL;
	
	if (! g_file_get_contents (path, &rdf, NULL, &error)) {
		g_print ("%s\n", error->message);
		g_clear_error (&error);
		rdf = NULL;
	}
	
	g_free (path);
		
	return rdf;
}


static int
check_get_cd_metadata (gpointer data)
{
	GooPlayer *player = data;
	gboolean   done, exiting;
	char      *rdf;
				
	/* Remove the check. */

        g_source_remove (player->priv->check_id);
        player->priv->check_id = 0;
	
	/**/

	g_mutex_lock (player->priv->yes_or_no);
	done = player->priv->thread == NULL;
	exiting = player->priv->exiting;
        g_mutex_unlock (player->priv->yes_or_no);

	if (exiting) {
		goo_player_set_is_busy (player, FALSE);
		destroy_pipeline (player, TRUE);
		return FALSE;
	}

	if (!done) {
		player->priv->check_id = g_timeout_add (REFRESH_RATE, 
						        check_get_cd_metadata, 
					 	        player);
		return FALSE;
	}
	
	/**/

	g_mutex_lock (player->priv->yes_or_no);
	rdf = player->priv->rdf;
	player->priv->rdf = NULL;
	g_mutex_unlock (player->priv->yes_or_no);
	
	if (rdf != NULL) { 
		set_cd_metadata_from_rdf (player, rdf);
		save_rdf_to_cache (player, rdf);
		g_free (rdf);
	}
	
	return FALSE;			
}


static void * 
get_cd_metadata (void *thread_data)
{
	GooPlayer     *player = thread_data;
	musicbrainz_t  mb;
	char          *rdf = NULL;
	
	mb = mb_New ();
	mb_UseUTF8 (mb, TRUE);
	if (mb_Query (mb, MBQ_GetCDInfo)) {
		int rdf_len;
	
		rdf_len = mb_GetResultRDFLen (mb);
		rdf = g_malloc (rdf_len + 1);
		mb_GetResultRDF (mb, rdf, rdf_len);
	}
	mb_Delete (mb);

	g_mutex_lock (player->priv->yes_or_no);
	g_free (player->priv->rdf);
	player->priv->rdf = rdf;
	player->priv->thread = NULL;
	g_mutex_unlock (player->priv->yes_or_no);

	g_thread_exit (NULL);

	return NULL;
}


static int
check_get_cd_tracks (gpointer data)
{
	GooPlayer *player = data;
	gboolean   done;
	gboolean   exiting;
	char      *rdf;
	
	/* Remove the check. */

        g_source_remove (player->priv->check_id);
        player->priv->check_id = 0;
	
	/**/

	g_mutex_lock (player->priv->yes_or_no);
	done = player->priv->thread == NULL;
	exiting = player->priv->exiting;
        g_mutex_unlock (player->priv->yes_or_no);

	if (exiting) {
		goo_player_set_is_busy (player, FALSE);
		destroy_pipeline (player, TRUE);
		return FALSE;
	}

	if (!done) {
		player->priv->check_id = g_timeout_add (REFRESH_RATE, 
							check_get_cd_tracks, 
							player);
		return FALSE;
	}

	/**/

	goo_player_set_is_busy (player, FALSE);
	gst_element_set_state (player->priv->pipeline, GST_STATE_NULL);
	goo_player_set_state (player, GOO_PLAYER_STATE_STOPPED, TRUE);
	
	action_done (player, GOO_PLAYER_ACTION_LIST);
	destroy_pipeline (player, TRUE);

	/**/
	
	rdf = read_cached_rdf (player); 
	if (rdf != NULL) {
		set_cd_metadata_from_rdf (player, rdf);
		g_free (rdf);
		return FALSE;
	}

	g_mutex_lock (player->priv->yes_or_no);
	player->priv->thread = g_thread_create (get_cd_metadata, player, FALSE, NULL);
	g_mutex_unlock (player->priv->yes_or_no);
	
	player->priv->check_id = g_timeout_add (REFRESH_RATE, check_get_cd_metadata, player);

	return FALSE;
}

/*
static int 
sum_of_digits (int n) 
{
	int sum = 0;
	while (n > 0) {
		sum = sum + (n % 10);
		n = n / 10;
	}
	return sum;
}


static char *
get_disc_id (GList *tracks)
{
	GList *scan;
	int    checksum = 0;
	int    n_tracks = 0;
	int    first_sector;
	int    last_sector;
	int    disc_id;
	
	for (scan = tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		
		checksum += sum_of_digits (track->from_sector /  SECTORS_PER_SEC);
		n_tracks++;
		
		if (scan == tracks)
			first_sector = track->from_sector;
		if (scan->next == NULL)
			last_sector = track->to_sector;
	}
	
	disc_id = ((checksum % 255) << 24) | (((last_sector - first_sector) / SECTORS_PER_SEC) << 8) | n_tracks;
	
	return g_strdup_printf ("%x", disc_id);
}
*/

static int
get_total_time (GList *tracks)
{
	int first_sector;
	int last_sector;
	
	first_sector = ((TrackInfo*)tracks->data)->from_sector;
	last_sector = ((TrackInfo*)tracks->data)->to_sector;
	
	return (last_sector - first_sector) / SECTORS_PER_SEC;
}


static void * 
get_cd_tracks (void *thread_data)
{
	GooPlayer     *player = thread_data;
	musicbrainz_t  mb;
	
	if (player->priv->pipeline != NULL)
		gst_element_set_state (player->priv->pipeline, GST_STATE_PAUSED);

	g_free (player->priv->discid);
	player->priv->discid = NULL;

	g_free (player->priv->asin);
	player->priv->asin = NULL;

	mb = mb_New ();
	mb_UseUTF8 (mb, TRUE);
	mb_SetDevice (mb, (char*) goo_player_get_location (player));
	if (mb_Query (mb, MBQ_GetCDTOC)) {
		char data[256];
		int  i;
		
		mb_GetResultData(mb, MBE_TOCGetCDIndexId, data, sizeof (data));
		player->priv->discid = g_strdup (data);
		
		g_print ("==> [MB] DISC ID: %s\n", player->priv->discid);
			
		g_print ("==> [MB] FIRST TRACK: %d\n", mb_GetResultInt (mb, MBE_TOCGetFirstTrack));
		player->priv->n_tracks = mb_GetResultInt (mb, MBE_TOCGetLastTrack);
		g_print ("==> [MB] LAST TRACK: %d\n", player->priv->n_tracks);
		
		for (i = 0; i < player->priv->n_tracks; i++) {
			TrackInfo *track;
			int        from_sector;
			int        n_sectors;
			
			from_sector = mb_GetResultInt1 (mb, MBE_TOCGetTrackSectorOffset, i + 2);
			n_sectors = mb_GetResultInt1 (mb, MBE_TOCGetTrackNumSectors, i + 2);
			
			g_print ("==> [MB] Track %d: [%d, %d]\n", i, from_sector, from_sector + n_sectors);
			
			track = track_info_new (i, from_sector, from_sector + n_sectors);
			player->priv->tracks = g_list_prepend (player->priv->tracks, track);
		}
	}
	mb_Delete (mb);

	player->priv->tracks = g_list_reverse (player->priv->tracks);
	player->priv->total_time = get_total_time (player->priv->tracks);

	/*player->priv->discid = get_disc_id (player->priv->tracks);
	debug (DEBUG_INFO, "DISC ID: %s\n", player->priv->discid);*/

	g_mutex_lock (player->priv->yes_or_no);
	player->priv->thread = NULL;
	g_mutex_unlock (player->priv->yes_or_no);

	g_thread_exit (NULL);

	return NULL;
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

	if (goo_cdrom_get_state (player->priv->cdrom) != GOO_CDROM_STATE_OK) {
		debug (DEBUG_INFO, "NOT OK\n");
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, TRUE);
		action_done (player, GOO_PLAYER_ACTION_LIST);
		return;
	}

	goo_player_set_is_busy (player, TRUE);
	create_pipeline (player);
	
	g_mutex_lock (player->priv->yes_or_no);
	player->priv->thread = g_thread_create (get_cd_tracks, player, FALSE, NULL);
	g_mutex_unlock (player->priv->yes_or_no);
	
	player->priv->check_id = g_timeout_add (REFRESH_RATE, check_get_cd_tracks, player);
}


void
goo_player_seek_song (GooPlayer *player,
		      int        n)
{
	GstEvent *event;
	int       track_n;
		
	if (goo_player_get_is_busy (player))
		return;

	player->priv->action = GOO_PLAYER_ACTION_SEEK_SONG;
	player->priv->state = GOO_PLAYER_STATE_SEEKING;
	notify_action_start (player);

	if (player->priv->n_tracks == 0) {
		action_done (player, GOO_PLAYER_ACTION_SEEK_SONG);
		return;
	}

	goo_player_stop (player);
	create_pipeline (player);
	gst_element_set_state (player->priv->pipeline, GST_STATE_PAUSED);

	/* seek to track */

	goo_player_set_state (player, GOO_PLAYER_STATE_SEEKING, TRUE);

	track_n = CLAMP (n, 0, player->priv->n_tracks - 1);
	player->priv->current_song = track_n;
	player->priv->current_track = goo_player_get_track (player, track_n);

	debug (DEBUG_INFO, "seek to track %d\n", track_n); /* FIXME */

	g_return_if_fail (player->priv->current_track != NULL);

	event = gst_event_new_seek (1.0, 
				    player->priv->track_format,
				    GST_SEEK_FLAG_FLUSH,
				    GST_SEEK_TYPE_SET,
				    track_n, 
				    GST_SEEK_TYPE_SET,
				    track_n + 1);
	if (!gst_pad_send_event (player->priv->source_pad, event))
		g_warning ("seek failed");

	action_done (player, GOO_PLAYER_ACTION_SEEK_SONG);
	goo_player_play (player);
}


SongInfo *
goo_player_get_song (GooPlayer *player,
		     int        n)
{
	TrackInfo *track;
	
	if (n < 0)
		return NULL;
	
	track = goo_player_get_track (player, n);
	if (track == NULL)
		return NULL;
	
	return create_song_info (player, track);	
}


int
goo_player_get_current_song (GooPlayer *player)
{
	return player->priv->current_song;
}


void
goo_player_skip_to (GooPlayer *player,
		    guint      seconds)
{
	GstEvent *event;

	if (goo_player_get_is_busy (player))
		return;

	if (player->priv->pipeline == NULL)
		return;

	event = gst_event_new_seek (1.0, 
				    GST_FORMAT_TIME,
				    GST_SEEK_FLAG_FLUSH,
				    GST_SEEK_TYPE_SET, 
				    G_GINT64_CONSTANT (1000000000) * seconds,
				    GST_SEEK_TYPE_NONE, 
				    0);
	if (! gst_pad_send_event (player->priv->source_pad, event))
		g_warning ("seek failed");
}


gboolean
goo_player_set_location (GooPlayer  *player,
		         const char *location)
{
	if (goo_player_get_is_busy (player))
		return TRUE;

	debug (DEBUG_INFO, "LOCATION: %s\n", location);

	destroy_pipeline (player, FALSE);
	remove_state_polling (player);

	goo_cdrom_set_device (player->priv->cdrom, location);

	if (location == NULL) 
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, FALSE);

	g_free (player->priv->location);
	player->priv->location = NULL;
	if (location != NULL)
		player->priv->location = g_strdup (location);

	return TRUE;
}


const char *
goo_player_get_location (GooPlayer *player)
{
	return player->priv->location;
}


const char *
goo_player_get_title (GooPlayer *player)
{
	return player->priv->title;
}


int
goo_player_get_year (GooPlayer *player)
{
	return player->priv->year;
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

	if (player->priv->n_tracks == 0) {
		action_done (player, GOO_PLAYER_ACTION_PLAY);
		return;
	}

	if (! ((player->priv->pipeline != NULL)
	       && ((goo_player_get_state (player) == GOO_PLAYER_STATE_PAUSED)
		   || (goo_player_get_state (player) == GOO_PLAYER_STATE_SEEKING))))
		create_pipeline (player);

	g_object_set (G_OBJECT (player->priv->volume), "volume", goo_player_get_volume (player), NULL);

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


gboolean
goo_player_eject (GooPlayer *player)
{
	GooCdromState cdrom_state;
	gboolean      result;

	if (goo_player_get_is_busy (player))
		return FALSE;

	player->priv->action = GOO_PLAYER_ACTION_EJECT;
	player->priv->state = GOO_PLAYER_STATE_EJECTING;
	notify_action_start (player);

	destroy_pipeline (player, TRUE);

	cdrom_state = goo_cdrom_get_state (player->priv->cdrom);
	result = goo_cdrom_eject (player->priv->cdrom);

	if (!result)
		goo_player_set_error (player, goo_cdrom_get_error (player->priv->cdrom));

	action_done (player, GOO_PLAYER_ACTION_EJECT);
	
	return result;	
}


GError *
goo_player_get_error (GooPlayer *player)
{
     if (player->priv->error != NULL)
	     return g_error_copy (player->priv->error);
     else
	     return NULL;
}


GList *
goo_player_get_song_list (GooPlayer *player)
{
	GList *list = NULL, *scan;

	if (goo_player_get_is_busy (player))
		return NULL;

	for (scan = player->priv->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		
		list = g_list_prepend (list, create_song_info (player, track));
	}

	return list;
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
goo_player_get_volume (GooPlayer *player)
{
	return player->priv->volume_value;
}


void
goo_player_set_volume (GooPlayer *player,
		       double     vol)
{
	if (goo_player_get_is_busy (player))
		return;

	player->priv->volume_value = vol;
	if (player->priv->volume != NULL)
		g_object_set (G_OBJECT (player->priv->volume), "volume", vol, NULL);	
}


gboolean
goo_player_get_is_busy (GooPlayer *player)
{
	return player->priv->is_busy;
}


const char *
goo_player_get_discid (GooPlayer *player)
{
	return player->priv->discid;
}


const char *
goo_player_get_asin (GooPlayer *player)
{
	return player->priv->asin;
}


TrackInfo*
goo_player_get_track (GooPlayer *player,
			 guint        n)
{
	GList *scan;

	for (scan = player->priv->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		if (track->number == n)
			return track;
	}

	return NULL;
}


GList *
goo_player_get_tracks (GooPlayer *player)
{
	return track_list_dup (player->priv->tracks);
}


int
goo_player_get_n_tracks (GooPlayer *player)
{
	return player->priv->n_tracks;
}


const char *
goo_player_get_artist (GooPlayer *player)
{
	return player->priv->artist;
}


const char *
goo_player_get_album (GooPlayer *player)
{
	return player->priv->title;
}


const char *
goo_player_get_genre (GooPlayer *player)
{
	return player->priv->genre;
}
