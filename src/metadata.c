/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2007-2011 Free Software Foundation, Inc.
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
#include <stdio.h>
#include <string.h>
#include <discid/discid.h>
#include <musicbrainz3/mb_c.h>
#include "album-info.h"
#include "glib-utils.h"
#include "metadata.h"


static TrackInfo *
get_track_info (MbTrack mb_track,
		int     n_track)
{
	TrackInfo *track;
	char       data[1024];
	char       data2[1024];
	MbArtist   mb_artist;

	track = track_info_new (n_track, 0, 0);

	mb_track_get_title (mb_track, data, 1024);
	track_info_set_title (track, data);

	debug (DEBUG_INFO, "==> [MB] TRACK %d: %s\n", n_track, data);

	mb_artist = mb_track_get_artist (mb_track);
	if (mb_artist != NULL) {
		mb_artist_get_unique_name (mb_artist, data, 1024);
		mb_artist_get_id (mb_artist, data2, 1024);
		track_info_set_artist (track, data, data2);
	}

	return track;
}


static AlbumInfo *
get_album_info (MbRelease release)
{
	AlbumInfo *album;
	char       data[1024];
	int        i;
	MbArtist   artist;
	char       data2[1024];
	GList     *tracks = NULL;
	int        n_tracks;

	album = album_info_new ();

	mb_release_get_id (release, data, 1024);
	debug (DEBUG_INFO, "==> [MB] ALBUM_ID: %s\n", data);
	album_info_set_id (album, strrchr (data, '/') + 1);

	mb_release_get_title (release, data, 1024);
	debug (DEBUG_INFO, "==> [MB] ALBUM NAME: %s\n", data);
	album_info_set_title (album, data);

	mb_release_get_asin (release, data, 1024);
	debug (DEBUG_INFO, "==> [MB] ASIN: %s\n", data);
	album_info_set_asin (album, data);

	for (i = 0; i < mb_release_get_num_release_events (release); i++) {
		MbReleaseEvent event;
		int            y = 0, m = 0, d = 0;

		event = mb_release_get_release_event (release, i);
		mb_release_event_get_date (event, data, 1024);
		debug (DEBUG_INFO, "==> [MB] RELEASE DATE: %s\n", data);
		if (sscanf (data, "%d-%d-%d", &y, &m, &d) > 0) {
			GDate *date;

			date = g_date_new_dmy ((d > 0) ? d : 1, (m > 0) ? m : 1, (y > 0) ? y : 1);
			album_info_set_release_date (album, date);
			g_date_free (date);
		}
	}

	artist = mb_release_get_artist (release);
	mb_artist_get_unique_name (artist, data, 1024);
	mb_artist_get_id (artist, data2, 1024);
	album_info_set_artist (album, data, data2);

	tracks = NULL;
	n_tracks = mb_release_get_num_tracks (release);
	debug (DEBUG_INFO, "==> [MB] N TRACKS: %d\n", n_tracks);
	for (i = 0; i < n_tracks; i++) {
		MbTrack    mb_track;
		TrackInfo *track;

		mb_track = mb_release_get_track (release, i);
		track = get_track_info (mb_track, i);
		if (album->artist == NULL)
			album_info_set_artist (album, track->artist, KEEP_PREVIOUS_VALUE);
		tracks = g_list_prepend (tracks, track);
	}

	tracks = g_list_reverse (tracks);
	album_info_set_tracks (album, tracks);

	return album;
}


static GList *
get_album_list (MbResultList list)
{
	GList *albums = NULL;
	int    n_albums;
	int    i;

	n_albums = mb_result_list_get_size (list);
	g_print ("[MB] Num Albums: %d\n", n_albums);

	for (i = 0; i < n_albums; i++) {
		MbRelease release;

		release = mb_result_list_get_release (list, i);
		albums = g_list_prepend (albums, get_album_info (release));
	}

	return g_list_reverse (albums);
}


static void
get_track_info_for_album_list (GList *albums)
{
	GList *scan;

	for (scan = albums; scan; scan = scan->next) {
		AlbumInfo     *album = scan->data;
		MbTrackFilter  filter;
		GList         *tracks;
		MbQuery        query;
		MbResultList   list;
		int            i;

		filter = mb_track_filter_new ();
		mb_track_filter_release_id (filter, album->id);
		query = mb_query_new (NULL, NULL);
		list = mb_query_get_tracks (query, filter);

		tracks = NULL;
		for (i = 0; i < mb_result_list_get_size (list); i++) {
			MbTrack    mb_track;
			TrackInfo *track;

			mb_track = mb_result_list_get_track (list, i);
			track = get_track_info (mb_track, i);
			if ((album->artist == NULL) && (track->artist != NULL))
				album_info_set_artist (album, track->artist, KEEP_PREVIOUS_VALUE);
			tracks = g_list_prepend (tracks, track);
		}

		tracks = g_list_reverse (tracks);
		album_info_set_tracks (album, tracks);

		mb_result_list_free (list);
		mb_query_free (query);
		mb_track_filter_free (filter);
	}
}


/* -- metadata_get_cd_info_from_device -- */


typedef struct {
	char      *device;
	char      *disc_id;
	AlbumInfo *album_info;
} GetCDInfoData;


static void
get_cd_info_data_free (GetCDInfoData *data)
{
	g_free (data->device);
	g_free (data->disc_id);
	album_info_unref (data->album_info);
	g_free (data);
}


static void
get_cd_info_from_device_thread (GSimpleAsyncResult *result,
				GObject            *object,
				GCancellable       *cancellable)
{
	GetCDInfoData *data;
	GList         *tracks;
	DiscId        *disc;

	data = g_simple_async_result_get_op_res_gpointer (result);

	data->album_info = album_info_new ();
	tracks = NULL;
	disc = discid_new ();
	if (discid_read (disc, data->disc_id)) {
		int first_track;
		int last_track;
		int i;

		data->disc_id = g_strdup (discid_get_id (disc));
		debug (DEBUG_INFO, "==> [MB] DISC ID: %s\n", data->disc_id);

		first_track = discid_get_first_track_num (disc);
		debug (DEBUG_INFO, "==> [MB] FIRST TRACK: %d\n", first_track);

		last_track = discid_get_last_track_num (disc);
		debug (DEBUG_INFO, "==> [MB] LAST TRACK: %d\n", last_track);

		for (i = first_track; i <= last_track; i++) {
			gint64 from_sector;
			gint64 n_sectors;

			from_sector = discid_get_track_offset (disc, i);
			n_sectors = discid_get_track_length (disc, i);

			debug (DEBUG_INFO, "==> [MB] Track %d: [%"G_GINT64_FORMAT", %"G_GINT64_FORMAT"]\n", i, from_sector, from_sector + n_sectors);

			tracks = g_list_prepend (tracks, track_info_new (i - first_track, from_sector, from_sector + n_sectors));
		}
	}
	tracks = g_list_reverse (tracks);
	album_info_set_tracks (data->album_info, tracks);

	track_list_free (tracks);
	discid_free (disc);
}


void
metadata_get_cd_info_from_device (const char          *device,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
	GetCDInfoData      *data;
	GSimpleAsyncResult *result;

	result = g_simple_async_result_new (NULL,
	                                    callback,
	                                    user_data,
	                                    metadata_get_cd_info_from_device);

	data = g_new0 (GetCDInfoData, 1);
	data->device = g_strdup (device);
	g_simple_async_result_set_op_res_gpointer (result,
                                                   data,
                                                   (GDestroyNotify) get_cd_info_data_free);

	g_simple_async_result_run_in_thread (result,
					     get_cd_info_from_device_thread,
					     G_PRIORITY_DEFAULT,
					     cancellable);

	g_object_unref (result);
}


gboolean
metadata_get_cd_info_from_device_finish (GAsyncResult  *result,
					 char         **disc_id,
					 AlbumInfo    **album_info,
					 GError       **error)
{
	GSimpleAsyncResult *simple;
	GetCDInfoData      *data;

	g_return_val_if_fail (g_simple_async_result_is_valid (result, NULL, metadata_get_cd_info_from_device), FALSE);

        simple = G_SIMPLE_ASYNC_RESULT (result);

        if (g_simple_async_result_propagate_error (simple, error))
                return FALSE;

        data = g_simple_async_result_get_op_res_gpointer (simple);
        if (disc_id != NULL)
        	*disc_id = g_strdup (data->disc_id);
        if (album_info != NULL)
        	*album_info = album_info_ref (data->album_info);

        return TRUE;
}


/* -- metadata_get_album_info_from_disc_id -- */


typedef struct {
	char  *disc_id;
	GList *albums;
} AlbumFromIDData;


static void
album_from_id_data_free (AlbumFromIDData *data)
{
	g_free (data->disc_id);
	album_list_free (data->albums);
	g_free (data);
}


static void
get_album_info_from_disc_id_thread (GSimpleAsyncResult *result,
				    GObject            *object,
				    GCancellable       *cancellable)
{
	AlbumFromIDData *data;
	MbReleaseFilter  filter;
	MbQuery          query;
	MbResultList     list;

	data = g_simple_async_result_get_op_res_gpointer (result);

	filter = mb_release_filter_new ();
	mb_release_filter_disc_id (filter, data->disc_id);

	query = mb_query_new (NULL, NULL);
	list = mb_query_get_releases (query, filter);
	data->albums = get_album_list (list);

	mb_result_list_free (list);
	mb_query_free (query);
	mb_release_filter_free (filter);
}


void
metadata_get_album_info_from_disc_id (const char          *disc_id,
				      GCancellable        *cancellable,
				      GAsyncReadyCallback  callback,
				      gpointer             user_data)
{
	AlbumFromIDData    *data;
	GSimpleAsyncResult *result;

	result = g_simple_async_result_new (NULL,
	                                    callback,
	                                    user_data,
	                                    metadata_get_album_info_from_disc_id);

	data = g_new0 (AlbumFromIDData, 1);
	data->disc_id = g_strdup (disc_id);
	g_simple_async_result_set_op_res_gpointer (result,
                                                   data,
                                                   (GDestroyNotify) album_from_id_data_free);

	g_simple_async_result_run_in_thread (result,
					     get_album_info_from_disc_id_thread,
					     G_PRIORITY_DEFAULT,
					     cancellable);

	g_object_unref (result);
}


GList *
metadata_get_album_info_from_disc_id_finish (GAsyncResult  *result,
					     GError       **error)
{
	GSimpleAsyncResult *simple;
	AlbumFromIDData    *data;

	g_return_val_if_fail (g_simple_async_result_is_valid (result, NULL, metadata_get_album_info_from_disc_id), NULL);

        simple = G_SIMPLE_ASYNC_RESULT (result);

        if (g_simple_async_result_propagate_error (simple, error))
                return NULL;

        data = g_simple_async_result_get_op_res_gpointer (simple);

        return album_list_dup (data->albums);
}


/* -- metadata_search_album_by_title -- */


typedef struct {
	char  *title;
	GList *albums;
} SearchByTitleData;


static void
search_by_tile_data_free (SearchByTitleData *data)
{
	g_free (data->title);
	album_list_free (data->albums);
	g_free (data);
}


static void
search_album_by_title_thread (GSimpleAsyncResult *result,
			      GObject            *object,
			      GCancellable       *cancellable)
{
	SearchByTitleData *data;
	MbReleaseFilter    filter;
	MbQuery            query;
	MbResultList       list;

	data = g_simple_async_result_get_op_res_gpointer (result);

	filter = mb_release_filter_new ();
	mb_release_filter_title (filter, data->title);

	query = mb_query_new (NULL, NULL);
	list = mb_query_get_releases (query, filter);

	data->albums = get_album_list (list);
	get_track_info_for_album_list (data->albums);

	mb_result_list_free (list);
	mb_query_free (query);
	mb_release_filter_free (filter);
}


void
metadata_search_album_by_title (const char          *title,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
	SearchByTitleData  *data;
	GSimpleAsyncResult *result;

	result = g_simple_async_result_new (NULL,
	                                    callback,
	                                    user_data,
	                                    metadata_search_album_by_title);

	data = g_new0 (SearchByTitleData, 1);
	data->title = g_strdup (title);
	g_simple_async_result_set_op_res_gpointer (result,
                                                   data,
                                                   (GDestroyNotify) search_by_tile_data_free);

	g_simple_async_result_run_in_thread (result,
					     search_album_by_title_thread,
					     G_PRIORITY_DEFAULT,
					     cancellable);

	g_object_unref (result);
}


GList *
metadata_search_album_by_title_finish (GAsyncResult  *result,
				       GError       **error)
{
	GSimpleAsyncResult *simple;
	AlbumFromIDData    *data;

	g_return_val_if_fail (g_simple_async_result_is_valid (result, NULL, metadata_search_album_by_title), NULL);

        simple = G_SIMPLE_ASYNC_RESULT (result);

        if (g_simple_async_result_propagate_error (simple, error))
                return NULL;

        data = g_simple_async_result_get_op_res_gpointer (simple);

        return album_list_dup (data->albums);
}
