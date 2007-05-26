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
#include <musicbrainz/queries.h>
#include <musicbrainz/mb_c.h>
#include "glib-utils.h"
#include "metadata.h"
#include "album-info.h"


static AlbumInfo* 
get_album_info (musicbrainz_t mb,
		int           n_album)
{
	AlbumInfo *album;
	char       data[1024];
	int        n_track, n_tracks;   
	GList     *tracks = NULL;       
	
	/*mb_Select (mb, MBS_Rewind);*/  
	if (! mb_Select1 (mb, MBS_SelectAlbum, n_album))
		return NULL;

	album = album_info_new ();
	
	if (mb_GetResultData (mb, MBE_AlbumGetAlbumId, data, sizeof (data))) {
		char data2[1024];
		mb_GetIDFromURL (mb, data, data2, sizeof (data2));
		debug (DEBUG_INFO, "==> [MB] ALBUM_ID: %s (%s)\n", data, data2);
		album_info_set_id (album, data2);
	}
	else
		return album;
	
 	if (mb_GetResultData (mb, MBE_AlbumGetAlbumName, data, sizeof (data))) {
		album_info_set_title (album, data);
		debug (DEBUG_INFO, "==> [MB] ALBUM NAME: %s\n", data);
 	}

	if (mb_GetResultData (mb, MBE_AlbumGetAmazonAsin, data, sizeof (data))) {
		album_info_set_asin (album, data);
		debug (DEBUG_INFO, "==> [MB] ASIN: %s\n", data);
	}
		
 	if (mb_GetResultInt (mb, MBE_AlbumGetNumReleaseDates) >= 1) {
 		int y = 0, m = 0, d = 0;
 		
		mb_Select1 (mb, MBS_SelectReleaseDate, 1);
 		
		mb_GetResultData (mb, MBE_ReleaseGetDate, data, sizeof (data));
		debug (DEBUG_INFO, "==> [MB] RELEASE DATE: %s\n", data);
		if (sscanf (data, "%d-%d-%d", &y, &m, &d) > 0) {
			GDate *date;
		
			date = g_date_new_dmy ((d > 0) ? d : 1, (m > 0) ? m : 1, (y > 0) ? y : 1);
			album_info_set_release_date (album, date);
			g_date_free (date);
		}
 		
		mb_GetResultData (mb, MBE_ReleaseGetCountry, data, sizeof (data));
		debug (DEBUG_INFO, "==> [MB] RELEASE COUNTRY: %s\n", data);
 		
		mb_Select (mb, MBS_Back);
 	}
 
	if (mb_GetResultData (mb, MBE_AlbumGetAlbumArtistName, data, sizeof (data))) {
		char data2[1024], data3[1024];
		
		mb_GetResultData (mb, MBE_AlbumGetArtistId, data2, sizeof (data2));
		mb_GetIDFromURL (mb, data2, data3, sizeof (data3));
		
		debug (DEBUG_INFO, "==> [MB] ARTIST_ID: %s (%s)\n", data2, data3);
		
		album_info_set_artist (album, data, data3);
	}
	
	tracks = NULL;
	n_tracks = mb_GetResultInt (mb, MBE_AlbumGetNumTracks);
	debug (DEBUG_INFO, "==> [MB] N TRACKS: %d\n", n_tracks);
	for (n_track = 1; n_track <= n_tracks; n_track++) {
		TrackInfo *track;
		
		track = track_info_new (n_track - 1, 0, 0);
		tracks = g_list_prepend (tracks, track);
		
		if (mb_GetResultData1 (mb, MBE_AlbumGetTrackName, data, sizeof (data), n_track))
			track_info_set_title (track, data);
		
		debug (DEBUG_INFO, "==> [MB] TRACK %d: %s\n", n_track, data);
			
		if (mb_GetResultData1 (mb, MBE_AlbumGetArtistName, data, sizeof (data), n_track)) {
		    	char data2[1024], data3[1024];
		
			mb_GetResultData1 (mb, MBE_AlbumGetArtistId, data2, sizeof (data2), n_track);
			mb_GetIDFromURL (mb, data2, data3, sizeof (data3));
			track_info_set_artist (track, data, data3);
			
			if (album->artist == NULL)
				album_info_set_artist (album, data, KEEP_PREVIOUS_VALUE);
		}
	}
	mb_Select (mb, MBS_Back);
	
	tracks = g_list_reverse (tracks);
	album_info_set_tracks (album, tracks);
	
	return album;
}


GList* 
get_album_list (musicbrainz_t mb)
{
	GList *albums = NULL;
	int    n_albums, i;
		
	n_albums = mb_GetResultInt (mb, MBE_GetNumAlbums);
	g_print ("[MB] Num Albums: %d\n", n_albums);
	
	for (i = 1; i <= n_albums; i++)
		albums = g_list_prepend (albums, get_album_info (mb, i));
	
	return g_list_reverse (albums);
}
