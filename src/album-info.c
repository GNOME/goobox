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
#include <gnome.h>
#include <gst/gst.h>
#include "album-info.h"


AlbumInfo*
album_info_new (void)
{
	AlbumInfo *album;

	album = g_new0 (AlbumInfo, 1);

	album->ref = 1;
	album->release_date = g_date_new ();
	album_info_set_title (album, NULL);
	album_info_set_artist (album, NULL);
	album_info_set_tracks (album, NULL);
	
	return album;
}


static void
album_info_free (AlbumInfo *album)
{
	g_free (album->title);
	g_free (album->artist);
	g_free (album->genre);
	g_free (album->asin);
	g_date_free (album->release_date);
	g_free (album);
}


GType
album_info_get_type (void)
{
	static GType type = 0;
  
	if (type == 0)
		type = g_boxed_type_register_static ("AlbumInfo", 
						     (GBoxedCopyFunc) album_info_copy,
						     (GBoxedFreeFunc) album_info_free);
  
	return type;
}


void
album_info_ref (AlbumInfo *album)
{
	album->ref++;
}


void
album_info_unref (AlbumInfo *album)
{
	if (album == NULL)
		return;
	album->ref--;
	if (album->ref == 0)
		album_info_free (album);
}


AlbumInfo *
album_info_copy (AlbumInfo *src)
{
	AlbumInfo *dest;

	dest = album_info_new ();
	album_info_set_title (dest, src->title);
	album_info_set_artist (dest, src->artist);
	album_info_set_genre (dest, src->genre);
	album_info_set_asin (dest, src->asin);
	album_info_set_release_date (dest, src->release_date);
	dest->various_artist = src->various_artist; 
	album_info_set_tracks (dest, src->tracks);
	
	return dest;
}


void
album_info_set_title (AlbumInfo  *album,
		      const char *title)
{
	g_free (album->title);
	if (title != NULL)
		album->title = g_strdup (title);
	else
		album->title = NULL /*g_strdup (_("Unknown Album"))*/;
}


void
album_info_set_artist (AlbumInfo  *album,
		       const char *artist)
{
	g_free (album->artist);
	
	album->various_artist = (artist != NULL) && (strcmp (artist, VARIUOS_ARTIST_ID) == 0);
	if (album->various_artist)
		album->artist = g_strdup (_("Various"));
	else if (artist != NULL) 
		album->artist = g_strdup (artist);
	else
		album->artist = NULL /*g_strdup (_("Unknown Artist"))*/;
}


void 
album_info_set_genre (AlbumInfo  *album,
		      const char *genre)
{
	g_free (album->genre);
	album->genre = NULL;
	
	if (genre != NULL)
		album->genre = g_strdup (genre);	
}


void
album_info_set_asin (AlbumInfo  *album,
		     const char *asin)
{
	g_free (album->asin);
	album->asin = NULL;
	
	if (asin != NULL)
		album->asin = g_strdup (asin);
}


void 
album_info_set_release_date (AlbumInfo *album,
			     GDate     *date)
{
	if ((date != NULL) && (g_date_valid (date))) 
		g_date_set_julian (album->release_date, g_date_get_julian (date));
	else
		g_date_clear (album->release_date, 1);
}


void
album_info_set_tracks (AlbumInfo  *album,
		       GList      *tracks)
{
	GList *scan;
	
	track_list_free (album->tracks);
	album->tracks = track_list_dup (tracks);
	
	album->n_tracks = 0;
	album->total_length = 0;
	for (scan = album->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		
		album->n_tracks++;
		album->total_length += track->length;
	}
}


TrackInfo *
album_info_get_track (AlbumInfo  *album,
		      int         track_number)
{
	GList *scan;

	for (scan = album->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		
		if (track->number == track_number)
			return track_info_copy (track);
	}

	return NULL;	
}
				     

/* -- */


GList *
album_list_dup (GList *album_list)
{
	GList *new_list;

	if (album_list == NULL)
		return NULL;

	new_list = g_list_copy (album_list);
	g_list_foreach (new_list, (GFunc) album_info_ref, NULL);

	return new_list;
}


void
album_list_free (GList *album_list)
{
	if (album_list == NULL)
		return;
	g_list_foreach (album_list, (GFunc) album_info_unref, NULL);
	g_list_free (album_list);
}
