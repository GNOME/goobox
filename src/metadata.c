/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2007 Free Software Foundation, Inc.
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
#include <musicbrainz3/mb_c.h>
#include "glib-utils.h"
#include "metadata.h"
#include "album-info.h"


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


GList *
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


void
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
