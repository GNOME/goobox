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
#include <math.h>
#include <string.h>
#include <gnome.h>
#include <gst/gst.h>
#include <musicbrainz/queries.h>
#include <musicbrainz/mb_c.h>

#include "goo-player-cd.h"
#include "goo-cdrom.h"
#include "song-info.h"
#include "track-info.h"
#include "glib-utils.h"
#include "GNOME_Media_CDDBSlave2.h"
#include "cddb-slave-client.h"

#define TOC_OFFSET 150
#define SECTORS_PER_SEC 75
#define POLL_TIMEOUT 1000
#define REFRESH_RATE 5
#define PROGRESS_DELAY 400
#define QUEUE_SIZE 16384U /*131072U*/

struct _GooPlayerCDPrivateData {
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
	
	CDDBSlaveClient *cddb_client;
};

static GooPlayerClass *parent_class = NULL;

static void goo_player_cd_class_init  (GooPlayerCDClass *class);
static void goo_player_cd_init        (GooPlayerCD *player);
static void goo_player_cd_finalize    (GObject *object);


GType
goo_player_cd_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GooPlayerCDClass),
			NULL,
			NULL,
			(GClassInitFunc) goo_player_cd_class_init,
			NULL,
			NULL,
			sizeof (GooPlayerCD),
			0,
			(GInstanceInitFunc) goo_player_cd_init
		};

		type = g_type_register_static (GOO_TYPE_PLAYER,
					       "GooPlayerCD",
					       &type_info,
					       0);
	}

        return type;
}


static void
action_done (GooPlayer       *player, 
	     GooPlayerAction  action)
{
	g_signal_emit_by_name (G_OBJECT (player), "done", action, NULL);
}


static gboolean
update_state_cb (gpointer data)
{
	GooPlayerCD *player = data;

	g_source_remove (player->priv->update_state_id);
	goo_cdrom_update_state (player->priv->cdrom);
	player->priv->update_state_id = g_timeout_add (POLL_TIMEOUT,
					               update_state_cb,
						       player);
						       
	return FALSE;
}


static void
add_state_polling (GooPlayerCD *player)
{
	GooPlayerCDPrivateData *priv = player->priv;

	if (priv->update_state_id != 0)
		return;

	debug (DEBUG_INFO, "ADD POLLING\n");

	priv->update_state_id = g_timeout_add (POLL_TIMEOUT,
					       update_state_cb,
					       player);
}


static void
remove_state_polling (GooPlayerCD *player)
{
	if (player->priv->update_state_id == 0) 
		return;

	debug (DEBUG_INFO, "REMOVE POLLING\n");

	g_source_remove (player->priv->update_state_id);
	player->priv->update_state_id = 0;
}


static void
destroy_pipeline (GooPlayerCD *player,
		  gboolean     poll)
{
	GooPlayerCDPrivateData *priv = player->priv;

	goo_cdrom_unlock_tray (priv->cdrom);

	if (priv->pipeline != NULL) {
		gst_element_set_state (priv->pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (priv->pipeline));
		priv->pipeline = NULL;
	}

	if (priv->update_progress_id != 0) {
		g_source_remove (priv->update_progress_id);
		priv->update_progress_id = 0;
	}

	priv->source = NULL;
	priv->source_pad = NULL;
	priv->volume = NULL;

	if (poll)
		add_state_polling (player);
}


static gboolean
player_done_cb (gpointer callback_data)
{
	GooPlayer *player = GOO_PLAYER (callback_data);
	
	action_done (player, GOO_PLAYER_ACTION_PLAY);
	
	return FALSE;
}


static gboolean
pipeline_bus_message_cb (GstBus     *bus,
		         GstMessage *message,
		         gpointer    data)
{
	GooPlayerCD *player_cd = (GooPlayerCD *) data;

	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_EOS:
		if (player_cd->priv->update_progress_id != 0) {
			g_source_remove (player_cd->priv->update_progress_id);
			player_cd->priv->update_progress_id = 0;
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
	GooPlayerCD *player_cd = callback_data;
	GooPlayer   *player = GOO_PLAYER (player_cd);
	GooPlayerCDPrivateData *priv = player_cd->priv;
	gboolean     ret;
	gint64       sector = 0;
	double       fraction;

	if (priv->update_progress_id != 0) {
		g_source_remove (priv->update_progress_id);
		priv->update_progress_id = 0;
	}
	
	ret = gst_pad_query_position (priv->source_pad, &priv->sector_format, &sector);
	if (!ret)
		return FALSE;

	fraction = ((double) sector) / (double) priv->current_track->sectors;
	g_signal_emit_by_name (G_OBJECT (player), 
			       "progress", 
			       fraction,
			       NULL);

	priv->update_progress_id = g_timeout_add (PROGRESS_DELAY, update_progress_cb, callback_data);

	return FALSE;
}


static void
create_pipeline (GooPlayerCD *player)
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

	player->priv->source = gst_element_make_from_uri (GST_URI_SRC, "cdda://", "source");
	device = goo_cdrom_get_device (player->priv->cdrom);
	debug (DEBUG_INFO, "DEVICE: %s\n", device);
	g_object_set (G_OBJECT (player->priv->source), 
		      "device", device, 
		      NULL);

	audioconvert = gst_element_factory_make ("audioconvert", "convert");
    	audioresample = gst_element_factory_make ("audioresample", "resample");
	player->priv->volume = gst_element_factory_make ("volume", "volume");
	queue = gst_element_factory_make ("queue", "queue");
	sink = gst_element_factory_make ("gconfaudiosink", "sink");
	
	gst_bin_add_many (GST_BIN (player->priv->pipeline), 
			  player->priv->source,
			  queue,
			  audioconvert,
			  audioresample, 
			  player->priv->volume, 
			  sink, 
			  NULL);
	gst_element_link_many (player->priv->source,
			       queue,
			       audioconvert,
			       audioresample, 
			       player->priv->volume, 
			       sink, 
			       NULL);

	player->priv->track_format = gst_format_get_by_nick ("track");
	player->priv->sector_format = gst_format_get_by_nick ("sector");
	player->priv->source_pad = gst_element_get_pad (player->priv->source, "src");
	
	gst_bus_add_watch (gst_pipeline_get_bus (GST_PIPELINE (player->priv->pipeline)),
			   pipeline_bus_message_cb, 
			   player);
}


static void
goo_player_cd_empty_list (GooPlayer *player)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	GooPlayerCDPrivateData *priv = player_cd->priv;

	track_list_free (priv->tracks);
	priv->tracks = NULL;
	priv->n_tracks = 0;
	priv->total_time = 0;
	priv->current_track = NULL;
	g_free (priv->discid);
	priv->discid = NULL;
	g_free (priv->artist);
	priv->artist = NULL;
	g_free (priv->title);
	priv->title = NULL;
	g_free (priv->genre);
	priv->genre = NULL;

	goo_player_set_current_song (player, -1);
	goo_player_set_title (player, NULL);
	goo_player_set_subtitle (player, NULL);
	goo_player_set_year (player, 0);
}


static void
cd_player_update (GooPlayer *player)
{
	GooCdrom *cdrom = GOO_PLAYER_CD (player)->priv->cdrom;
	goo_cdrom_set_state (cdrom, GOO_CDROM_STATE_UNKNOWN);
	goo_cdrom_update_state (cdrom);
}


static void
set_cd_metadata_from_rdf (GooPlayer *player, 
			  char      *rdf)
{
	GooPlayerCD   *player_cd = GOO_PLAYER_CD (player);
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

 	g_free (player_cd->priv->title);
 	player_cd->priv->title = NULL;
 		
 	g_free (player_cd->priv->artist);
 	player_cd->priv->artist = NULL;
 		
 	g_free (player_cd->priv->genre);
 	player_cd->priv->genre = NULL;
 		
 	player_cd->priv->year = 0;
 		
 	if (mb_GetResultData (mb, MBE_AlbumGetAlbumName, data, sizeof (data))) 
		player_cd->priv->title = g_strdup (data);
	else
		player_cd->priv->title = g_strdup (_("Unknown Title"));
 
	if (mb_GetResultData (mb, MBE_AlbumGetAlbumArtistId, data, sizeof (data))) 
		if (g_ascii_strncasecmp (MBI_VARIOUS_ARTIST_ID, data, 64) == 0)
			player_cd->priv->artist = g_strdup (_("Various"));
		
	if (mb_GetResultInt (mb, MBE_AlbumGetNumTracks) != player_cd->priv->n_tracks)
		goto set_cd_metadata_end;	
		
	for (scan = player_cd->priv->tracks, n_track = 1; scan; scan = scan->next, n_track++) {
		TrackInfo *track = scan->data;
			
		if (mb_GetResultData1 (mb, MBE_AlbumGetTrackName, data, sizeof (data), n_track))
			track_info_set_title (track, data);
				
		if ((player_cd->priv->artist == NULL) && (mb_GetResultData1 (mb, MBE_AlbumGetArtistName, data, sizeof (data), n_track)))
			player_cd->priv->artist = g_strdup (data);
	}
		
	goo_player_set_title (player, player_cd->priv->title);
	goo_player_set_subtitle (player, player_cd->priv->artist);
	goo_player_set_year (player, player_cd->priv->year);
	
	action_done (player, GOO_PLAYER_ACTION_METADATA);		

set_cd_metadata_end:
	
	mb_Delete (mb);
}


static int
check_get_cd_metadata (gpointer data)
{
	GooPlayer     *player = data;
	GooPlayerCD   *player_cd = GOO_PLAYER_CD (player);
	gboolean       done, exiting;
	char          *rdf;
				
	/* Remove the check. */

        g_source_remove (player_cd->priv->check_id);
        player_cd->priv->check_id = 0;
	
	/**/

	g_mutex_lock (player_cd->priv->yes_or_no);
	done = player_cd->priv->thread == NULL;
	exiting = player_cd->priv->exiting;
        g_mutex_unlock (player_cd->priv->yes_or_no);

	if (exiting) {
		goo_player_set_is_busy (player, FALSE);
		destroy_pipeline (player_cd, TRUE);
		return FALSE;
	}

	if (!done) {
		player_cd->priv->check_id = g_timeout_add (REFRESH_RATE, 
						           check_get_cd_metadata, 
					 	           player);
		return FALSE;
	}
	
	/**/

	g_mutex_lock (player_cd->priv->yes_or_no);
	rdf = player_cd->priv->rdf;
	player_cd->priv->rdf = NULL;
	g_mutex_unlock (player_cd->priv->yes_or_no);
	
	if (rdf != NULL) { 
		set_cd_metadata_from_rdf (player, rdf);
		g_free (rdf);
	}
	
	return FALSE;			
}


static void * 
get_cd_metadata (void *thread_data)
{
	GooPlayer     *player = thread_data;
	GooPlayerCD   *player_cd = GOO_PLAYER_CD (player);
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

	g_mutex_lock (player_cd->priv->yes_or_no);
	g_free (player_cd->priv->rdf);
	player_cd->priv->rdf = rdf;
	player_cd->priv->thread = NULL;
	g_mutex_unlock (player_cd->priv->yes_or_no);

	g_thread_exit (NULL);

	return NULL;
}

static int
check_get_cd_tracks (gpointer data)
{
	GooPlayer   *player = data;
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	gboolean     done;
	gboolean     exiting;

	/* Remove check. */

        g_source_remove (player_cd->priv->check_id);
        player_cd->priv->check_id = 0;
	
	/**/

	g_mutex_lock (player_cd->priv->yes_or_no);
	done = player_cd->priv->thread == NULL;
	exiting = player_cd->priv->exiting;
        g_mutex_unlock (player_cd->priv->yes_or_no);

	if (exiting) {
		goo_player_set_is_busy (player, FALSE);
		destroy_pipeline (player_cd, TRUE);
		return FALSE;
	}

	if (!done) {
		player_cd->priv->check_id = g_timeout_add (REFRESH_RATE, 
							   check_get_cd_tracks, 
							   player);
		return FALSE;
	}

	/**/

	goo_player_set_is_busy (player, FALSE);
	gst_element_set_state (player_cd->priv->pipeline, GST_STATE_NULL);
	goo_player_set_state (player, GOO_PLAYER_STATE_STOPPED, TRUE);
	action_done (player, GOO_PLAYER_ACTION_LIST);
	destroy_pipeline (player_cd, TRUE);

	/**/

	g_mutex_lock (player_cd->priv->yes_or_no);
	player_cd->priv->thread = g_thread_create (get_cd_metadata, player, FALSE, NULL);
	g_mutex_unlock (player_cd->priv->yes_or_no);
	
	player_cd->priv->check_id = g_timeout_add (REFRESH_RATE, check_get_cd_metadata, player);

	return FALSE;
}


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
	GooPlayer   *player = thread_data;
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);

	if (player_cd->priv->pipeline != NULL)
		gst_element_set_state (player_cd->priv->pipeline, GST_STATE_PAUSED);

	/**/

	musicbrainz_t mb;
	
	mb = mb_New ();
	mb_UseUTF8 (mb, TRUE);
	mb_SetDevice (mb, (char*) goo_player_get_location (player));
	if (mb_Query (mb, MBQ_GetCDTOC)) {
		char data[256];
		int  i;
		
		mb_GetResultData(mb, MBE_TOCGetCDIndexId, data, sizeof (data));
		g_print ("==> [MB] DISC ID: %s\n", data);
		g_print ("==> [MB] FIRST TRACK: %d\n", mb_GetResultInt (mb, MBE_TOCGetFirstTrack));
		player_cd->priv->n_tracks = mb_GetResultInt (mb, MBE_TOCGetLastTrack);
		g_print ("==> [MB] LAST TRACK: %d\n", player_cd->priv->n_tracks);
		
		for (i = 0; i < player_cd->priv->n_tracks; i++) {
			TrackInfo *track;
			int        from_sector;
			int        n_sectors;
			
			from_sector = mb_GetResultInt1 (mb, MBE_TOCGetTrackSectorOffset, i + 2);
			n_sectors = mb_GetResultInt1 (mb, MBE_TOCGetTrackNumSectors, i + 2);
			
			g_print ("==> [MB] Track %d: [%d, %d]\n", i, from_sector, from_sector + n_sectors);
			
			track = track_info_new (i, from_sector, from_sector + n_sectors);
			player_cd->priv->tracks = g_list_prepend (player_cd->priv->tracks, track);
		}
	}
	mb_Delete (mb);

	player_cd->priv->tracks = g_list_reverse (player_cd->priv->tracks);
	player_cd->priv->total_time = get_total_time (player_cd->priv->tracks);

	g_free (player_cd->priv->discid);
	player_cd->priv->discid = get_disc_id (player_cd->priv->tracks);
	debug (DEBUG_INFO, "DISC ID: %s\n", player_cd->priv->discid);

	g_mutex_lock (player_cd->priv->yes_or_no);
	player_cd->priv->thread = NULL;
	g_mutex_unlock (player_cd->priv->yes_or_no);

	g_thread_exit (NULL);

	return NULL;
}


static void
cd_player_list (GooPlayer *player)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);

	if (goo_player_get_is_busy (player))
		return;

	goo_player_cd_empty_list (player);

	if (goo_cdrom_get_state (player_cd->priv->cdrom) != GOO_CDROM_STATE_OK) {
		debug (DEBUG_INFO, "NOT OK\n");
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, TRUE);
		action_done (player, GOO_PLAYER_ACTION_LIST);
		return;
	}

	goo_player_set_is_busy (player, TRUE);
	create_pipeline (player_cd);
	
	g_mutex_lock (player_cd->priv->yes_or_no);
	player_cd->priv->thread = g_thread_create (get_cd_tracks, player, FALSE, NULL);
	g_mutex_unlock (player_cd->priv->yes_or_no);
	
	player_cd->priv->check_id = g_timeout_add (REFRESH_RATE, check_get_cd_tracks, player);
}


static void
cd_player_seek_song (GooPlayer *player,
		     gint       n)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	GooPlayerCDPrivateData *priv = player_cd->priv;
	GstEvent    *event;
	int          track_n;

	if (goo_player_get_is_busy (player))
		return;

	if (priv->n_tracks == 0) {
		action_done (player, GOO_PLAYER_ACTION_SEEK_SONG);
		return;
	}

	goo_player_stop (player);
	create_pipeline (player_cd);
	gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);

	/* seek to track */

	goo_player_set_state (player, GOO_PLAYER_STATE_SEEKING, TRUE);

	track_n = CLAMP (n, 0, priv->n_tracks - 1);
	goo_player_set_current_song (player, track_n);
	priv->current_track = goo_player_cd_get_track (player_cd, track_n);

	debug (DEBUG_INFO, "seek to track %d\n", track_n); /* FIXME */

	g_return_if_fail (priv->current_track != NULL);

	event = gst_event_new_seek (1.0, 
				    priv->track_format,
				    GST_SEEK_FLAG_FLUSH,
				    GST_SEEK_TYPE_SET,
				    track_n, 
				    GST_SEEK_TYPE_SET,
				    track_n + 1);
	if (!gst_pad_send_event (priv->source_pad, event))
		g_warning ("seek failed");

	action_done (player, GOO_PLAYER_ACTION_SEEK_SONG);
	goo_player_play (player);
}


static SongInfo*
create_song_info (GooPlayerCD *player_cd,
		  TrackInfo   *track) 
{
	SongInfo *song;

	song = song_info_new ();
	song->number = track->number;
	song->title = g_strdup (track->title);
	song->artist = g_strdup (player_cd->priv->artist);
	song->time = track->length;

	return song;
}


static SongInfo *
cd_player_get_song (GooPlayer *player,
		    gint       n)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	TrackInfo   *track;
	
	track = goo_player_cd_get_track (player_cd, n);
	if (track == NULL)
		return NULL;
	else
		return create_song_info (player_cd, track);
}


static void
cd_player_skip_to (GooPlayer *player,
		   guint      seconds)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	GstEvent    *event;

	if (goo_player_get_is_busy (player))
		return;

	if (player_cd->priv->pipeline == NULL)
		return;

	event = gst_event_new_seek (1.0, 
				    GST_FORMAT_TIME,
				    GST_SEEK_FLAG_FLUSH,
				    GST_SEEK_TYPE_SET, 
				    G_GINT64_CONSTANT (1000000000) * seconds,
				    GST_SEEK_TYPE_NONE, 
				    0);
	if (! gst_pad_send_event (player_cd->priv->source_pad, event))
		g_warning ("seek failed");
}


static void
cd_player_play (GooPlayer *player)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	GooPlayerCDPrivateData *priv = player_cd->priv;
	int vol;

	if (goo_player_get_is_busy (player))
		return;

	if (priv->n_tracks == 0) {
		action_done (player, GOO_PLAYER_ACTION_PLAY);
		return;
	}

	if (! ((priv->pipeline != NULL)
	       && ((goo_player_get_state (player) == GOO_PLAYER_STATE_PAUSED)
		   || (goo_player_get_state (player) == GOO_PLAYER_STATE_SEEKING))))
		create_pipeline (player_cd);

	vol = goo_player_get_volume (player);
	g_object_set (G_OBJECT (priv->volume), "volume", (double) vol, NULL);

	gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
	goo_player_set_state (player, GOO_PLAYER_STATE_PLAYING, TRUE);

	priv->update_progress_id = g_timeout_add (PROGRESS_DELAY, update_progress_cb, player);
}


static void
cd_player_pause (GooPlayer *player)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	GooPlayerCDPrivateData *priv = player_cd->priv;

	if (goo_player_get_is_busy (player))
		return;

	if (priv->pipeline == NULL)
		return;

	gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);
	goo_player_set_state (GOO_PLAYER (player), GOO_PLAYER_STATE_PAUSED, TRUE);
	action_done (player, GOO_PLAYER_ACTION_PAUSE);
}


static void
cd_player_stop (GooPlayer *player)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	GooPlayerCDPrivateData *priv = player_cd->priv;

	if (goo_player_get_is_busy (player))
		return;

	if (priv->pipeline == NULL)
		return;

	destroy_pipeline (player_cd, TRUE);

	goo_player_set_state (GOO_PLAYER (player), GOO_PLAYER_STATE_STOPPED, TRUE);
	action_done (player, GOO_PLAYER_ACTION_STOP);
}


static GList*
cd_player_get_song_list (GooPlayer *player)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	GooPlayerCDPrivateData *priv = player_cd->priv;
	GList *list = NULL, *scan;

	if (goo_player_get_is_busy (player))
		return NULL;

	for (scan = priv->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		list = g_list_prepend (list, create_song_info (player_cd, track));
	}

	return list;
}


static gboolean
cd_player_eject (GooPlayer *player)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);
	GooPlayerCDPrivateData *priv = player_cd->priv;
	gboolean      result;
	GooCdromState cdrom_state;

	if (goo_player_get_is_busy (player))
		return FALSE;

	destroy_pipeline (player_cd, TRUE);

	cdrom_state = goo_cdrom_get_state (priv->cdrom);
	result = goo_cdrom_eject (priv->cdrom);

	if (!result)
		goo_player_set_error (player, goo_cdrom_get_error (priv->cdrom));

	action_done (player, GOO_PLAYER_ACTION_EJECT);
	
	return result;
}


gboolean
cd_player_set_location (GooPlayer  *player,
			const char *location)
{
	GooPlayerCD *player_cd = GOO_PLAYER_CD (player);

	if (goo_player_get_is_busy (player))
		return TRUE;

	debug (DEBUG_INFO, "LOCATION: %s\n", location);

	destroy_pipeline (player_cd, FALSE);
	remove_state_polling (player_cd);

	goo_cdrom_set_device (player_cd->priv->cdrom, location);

	if (location == NULL) 
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, FALSE);

	GOO_PLAYER_CLASS (parent_class)->set_location (player, location);

	return TRUE;
}


void
cd_player_set_volume (GooPlayer *player,
		      int        vol)
{
	GooPlayerCDPrivateData *priv = GOO_PLAYER_CD (player)->priv;

	if (goo_player_get_is_busy (player))
		return;

	if (priv->volume != NULL)
		g_object_set (G_OBJECT (priv->volume), "volume", (double) vol / 100.0, NULL);
}


static void 
goo_player_cd_class_init (GooPlayerCDClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	GooPlayerClass *player_class = GOO_PLAYER_CLASS (class);

        parent_class = g_type_class_peek_parent (class);

        gobject_class->finalize = goo_player_cd_finalize;

	player_class->update           = cd_player_update;
	player_class->list             = cd_player_list;
	player_class->seek_song        = cd_player_seek_song;
	player_class->get_song         = cd_player_get_song;
	player_class->skip_to          = cd_player_skip_to;
	player_class->play             = cd_player_play;
	player_class->pause            = cd_player_pause;
	player_class->stop             = cd_player_stop;
	player_class->get_song_list    = cd_player_get_song_list;
	player_class->eject            = cd_player_eject;
	player_class->set_location     = cd_player_set_location;
	player_class->set_volume       = cd_player_set_volume;
}


static void
cdrom_state_changed_cb (GooCdrom  *cdrom,
			GooPlayer *player)
{
	GooPlayerCDPrivateData *priv = GOO_PLAYER_CD (player)->priv;
	GooCdromState  cdrom_state = goo_cdrom_get_state (cdrom);
	char          *state;
	char          *message = "";

	if (goo_player_get_location (player) == NULL)
		return;

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
		message = _("Data CD");
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
		GError *error = goo_cdrom_get_error (priv->cdrom);
		
		goo_player_set_error (player, error);
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, TRUE);
		goo_player_cd_empty_list (player);
		action_done (player, GOO_PLAYER_ACTION_LIST);
	} 
	else {
		goo_player_set_error (player, g_error_new (GOO_CDROM_ERROR, 0, "%s", message));
		goo_player_set_state (player, GOO_PLAYER_STATE_ERROR, TRUE);
		goo_player_cd_empty_list (player);
		action_done (player, GOO_PLAYER_ACTION_LIST);
		if (cdrom_state == GOO_CDROM_STATE_TRAY_OPEN) 
			add_state_polling (GOO_PLAYER_CD (player));
	}
}


static int
count_tracks (CDDBSlaveClientTrackInfo **info)
{
        int i = 0;
	while (info[i] != NULL) 
		i++;
        return i;
}


static void
cddb_slave_listener_event_cb (BonoboListener    *listener,
			      const char        *name,
			      const BonoboArg   *arg,
			      CORBA_Environment *ev,
			      gpointer           data)
{
	GooPlayer                           *player = data;
	GooPlayerCD                         *player_cd = data;
	GooPlayerCDPrivateData              *priv = player_cd->priv;
        GNOME_Media_CDDBSlave2_QueryResult  *query = arg->_value;
	CDDBSlaveClientTrackInfo           **track_info = NULL;
	int                                  ntracks, i;
	GList                               *scan;

	if (priv->discid == NULL)
		return;
	if (strcmp (query->discid, priv->discid) != 0)
		return;

	if (query->result != GNOME_Media_CDDBSlave2_OK)
		return;

	if (! cddb_slave_client_is_valid (priv->cddb_client, priv->discid)) {
		action_done (player, GOO_PLAYER_ACTION_METADATA);
		return;
	}

	track_info = cddb_slave_client_get_tracks (priv->cddb_client, priv->discid);
	ntracks = count_tracks (track_info);
	if (ntracks != priv->n_tracks) 
		return;
	
	/* Update CD properties */

	g_free (priv->title);
	priv->title = cddb_slave_client_get_disc_title (priv->cddb_client, priv->discid);

	g_free (priv->artist);
	priv->artist = cddb_slave_client_get_artist (priv->cddb_client, priv->discid);

	g_free (priv->genre);
	priv->genre = cddb_slave_client_get_genre (priv->cddb_client, priv->discid);

	priv->year = cddb_slave_client_get_year (priv->cddb_client, priv->discid);

	goo_player_set_title (player, priv->title);
	goo_player_set_subtitle (player, priv->artist);
	goo_player_set_year (player, priv->year);

	for (scan = priv->tracks, i = 0; scan; scan = scan->next, i++) {
		TrackInfo *track = scan->data;
		track_info_set_title (track, track_info[i]->name);
	}
	cddb_slave_client_free_track_info (track_info);

	action_done (player, GOO_PLAYER_ACTION_METADATA);
}


static void 
goo_player_cd_init (GooPlayerCD *player)
{
	GooPlayerCDPrivateData *priv;
	BonoboListener         *listener = NULL;

	player->priv = g_new0 (GooPlayerCDPrivateData, 1);
	priv = player->priv;

	priv->yes_or_no = g_mutex_new ();
	priv->check_id = 0;
	priv->exiting = FALSE,

	priv->discid = NULL;
	priv->n_tracks = 0;
	priv->tracks = NULL;
	goo_player_set_current_song (GOO_PLAYER (player), -1);
	goo_player_set_volume_protected (GOO_PLAYER (player), 100);

	priv->update_progress_id = 0;

	priv->cddb_client = cddb_slave_client_new ();
	listener = bonobo_listener_new (NULL, NULL);
	g_signal_connect (G_OBJECT (listener), 
			  "event-notify",
			  G_CALLBACK (cddb_slave_listener_event_cb), 
			  player);
	cddb_slave_client_add_listener (priv->cddb_client, listener);
}


static void 
goo_player_cd_finalize (GObject *object)
{
        GooPlayerCD *player_cd;
        GooPlayer   *player;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_PLAYER_CD (object));
  
	player_cd = GOO_PLAYER_CD (object);
	player = GOO_PLAYER (object);
	
	g_mutex_lock (player_cd->priv->yes_or_no);
	player_cd->priv->exiting = TRUE;
        g_mutex_unlock (player_cd->priv->yes_or_no);

 	if (player_cd->priv != NULL) {
		GooPlayerCDPrivateData *priv = player_cd->priv;
		
		if (priv->check_id != 0) {
			g_source_remove (priv->check_id);
			priv->check_id = 0;
		}

		destroy_pipeline (player_cd, FALSE);

		g_mutex_free (priv->yes_or_no);

		remove_state_polling (player_cd);
		destroy_pipeline (player_cd, FALSE);

		g_free (priv->discid);
		g_free (priv->artist);
		g_free (priv->title);
		track_list_free (priv->tracks);

		g_object_unref (priv->cddb_client);

		g_free (player_cd->priv);
		player_cd->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


GooPlayer *
goo_player_cd_new (const char *location)
{
	GooPlayer   *player;
	GooPlayerCD *player_cd;
	const char  *device;

	player = GOO_PLAYER (g_object_new (GOO_TYPE_PLAYER_CD, NULL));
	goo_player_construct (player, location);

	player_cd = GOO_PLAYER_CD (player);

	device = goo_player_get_location (GOO_PLAYER (player));
	player_cd->priv->cdrom = goo_cdrom_new (device);
	g_signal_connect (player_cd->priv->cdrom, 
			  "state_changed",
			  G_CALLBACK (cdrom_state_changed_cb), 
			  GOO_PLAYER (player));

	return player;
}


const char *
goo_player_cd_get_discid (GooPlayerCD *player)
{
	return player->priv->discid;
}


TrackInfo*
goo_player_cd_get_track (GooPlayerCD *player,
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
goo_player_cd_get_tracks (GooPlayerCD *player)
{
	return track_list_dup (player->priv->tracks);
}


const char *
goo_player_cd_get_artist (GooPlayerCD *player)
{
	return player->priv->artist;
}


const char *
goo_player_cd_get_album (GooPlayerCD *player)
{
	return player->priv->title;
}


const char *
goo_player_cd_get_genre (GooPlayerCD *player)
{
	return player->priv->genre;
}
