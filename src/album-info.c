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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <gst/gst.h>
#include "album-info.h"
#include "glib-utils.h"
#include "gth-user-dir.h"
#include "track-info.h"


#define MBI_VARIOUS_ARTIST_ID  "89ad4ac3-39f7-470e-963a-56509c546377"


AlbumInfo*
album_info_new (void)
{
	AlbumInfo *album;

	album = g_new0 (AlbumInfo, 1);

	album->ref = 1;
	album->release_date = g_date_new ();
	album_info_set_id (album, NULL);
	album_info_set_title (album, NULL);
	album_info_set_artist (album, NULL, NULL);
	album_info_set_tracks (album, NULL);

	return album;
}


static void
album_info_free (AlbumInfo *album)
{
	g_free (album->id);
	g_free (album->title);
	g_free (album->artist);
	g_free (album->artist_id);
	g_free (album->genre);
	g_free (album->asin);
	g_date_free (album->release_date);
	track_list_free (album->tracks);
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


AlbumInfo *
album_info_ref (AlbumInfo *album)
{
	album->ref++;
	return album;
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
	album_info_set_id (dest, src->id);
	album_info_set_title (dest, src->title);
	album_info_set_artist (dest, src->artist, src->artist_id);
	album_info_set_genre (dest, src->genre);
	album_info_set_asin (dest, src->asin);
	album_info_set_release_date (dest, src->release_date);
	dest->various_artist = src->various_artist;
	album_info_set_tracks (dest, src->tracks);

	return dest;
}


void
album_info_set_id (AlbumInfo  *album,
		   const char *id)
{
	if (album->id == id)
		return;

	g_free (album->id);
	if (id != NULL)
		album->id = g_strdup (id);
	else
		album->id = NULL;
}


void
album_info_set_title (AlbumInfo  *album,
		      const char *title)
{
	if (title == NULL) {
		g_free (album->title);
		album->title = NULL /*g_strdup (_("Unknown Album"))*/;
		return;
	}

	if (album->title == title)
		return;

	g_free (album->title);
	album->title = g_strdup (title);
}


void
album_info_set_artist (AlbumInfo  *album,
		       const char *artist,
		       const char *artist_id)
{
	if ((artist == NULL) || (artist[0] == 0)) {
		g_free (album->artist);
		album->artist = NULL /*g_strdup (_("Unknown Artist"))*/;
		g_free (album->artist_id);
		album->artist_id = NULL;
		return;
	}

	if (album->artist == artist)
		return;

	g_free (album->artist);
	g_free (album->artist_id);

	album->various_artist = (artist_id != NULL) && (strcmp (artist_id, MBI_VARIOUS_ARTIST_ID) == 0);
	if (artist != NULL)
		album->artist = g_strdup (artist);
	else if (album->various_artist)
		album->artist = g_strdup (_("Various"));
	else
		album->artist = NULL;

	if ((artist_id != NULL) && (strcmp (artist_id, KEEP_PREVIOUS_VALUE) != 0))
		album->artist_id = g_strdup (artist_id);
}


void
album_info_set_genre (AlbumInfo  *album,
		      const char *genre)
{
	if (album->genre == genre)
		return;

	g_free (album->genre);
	album->genre = NULL;

	if (genre != NULL)
		album->genre = g_strdup (genre);
}


void
album_info_set_asin (AlbumInfo  *album,
		     const char *asin)
{
	if (album->asin == asin)
		return;

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

	if (album->tracks == tracks)
		return;

	track_list_free (album->tracks);
	album->tracks = track_list_dup (tracks);

	album->n_tracks = 0;
	album->total_length = 0;
	for (scan = album->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;

		if ((album->artist != NULL) && (track->artist == NULL))
			track_info_set_artist (track, album->artist, album->artist_id);

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


void
album_info_copy_metadata (AlbumInfo *to_album,
			  AlbumInfo *from_album)
{
	GList *scan_to, *scan_from;

	album_info_set_id (to_album, from_album->id);
	album_info_set_title (to_album, from_album->title);
	album_info_set_artist (to_album, from_album->artist, from_album->artist_id);
	to_album->various_artist = from_album->various_artist;
	album_info_set_genre (to_album, from_album->genre);
	album_info_set_asin (to_album, from_album->asin);
	album_info_set_release_date (to_album, from_album->release_date);

	for (scan_to = to_album->tracks, scan_from = from_album->tracks;
	     scan_to && scan_from;
	     scan_to = scan_to->next, scan_from = scan_from->next)
	{
		TrackInfo *to_track = scan_to->data;
		TrackInfo *from_track = scan_from->data;

		track_info_copy_metadata (to_track, from_track);
	}
}


static char *
get_cache_path (AlbumInfo  *album,
		const char *disc_id)
{
	if (disc_id == NULL)
		return NULL;
	else
		return gth_user_dir_get_file (GTH_DIR_DATA, "goobox", "albums", disc_id, NULL);
}


gboolean
album_info_load_from_cache (AlbumInfo  *album,
			    const char *disc_id)
{
	GKeyFile *f;
	char     *path;
	char     *s;
	int       i;
	GList    *scan;

	path = get_cache_path (album, disc_id);
	if (path == NULL)
		return FALSE;

	f = g_key_file_new ();
	if (! g_key_file_load_from_file (f, path, G_KEY_FILE_NONE, NULL)) {
		g_free (path);
		g_key_file_free (f);
		return FALSE;
	}
	g_free (path);

	s = g_key_file_get_string (f, "Album", "ID", NULL);
	if (s != NULL) {
		album_info_set_id (album, s);
		g_free (s);
	}

	s = g_key_file_get_string (f, "Album", "Title", NULL);
	if (s != NULL) {
		album_info_set_title (album, s);
		g_free (s);
	}

	s = g_key_file_get_string (f, "Album", "Artist", NULL);
	if (s != NULL) {
		album_info_set_artist (album, s, "");
		g_free (s);
	}
	album->various_artist = g_key_file_get_boolean (f, "Album", "VariousArtists", NULL);

	s = g_key_file_get_string (f, "Album", "Genre", NULL);
	if (s != NULL) {
		album_info_set_genre (album, s);
		g_free (s);
	}

	s = g_key_file_get_string (f, "Album", "ReleaseDate", NULL);
	if (s != NULL) {
		int y = 0, m = 0, d = 0;

		if (sscanf (s, "%d/%d/%d", &d, &m, &y) > 0) {
			GDate *date;

			date = g_date_new_dmy ((d > 0) ? d : 1, (m > 0) ? m : 1, (y > 0) ? y : 1);
			album_info_set_release_date (album, date);
			g_date_free (date);
		}
		g_free (s);
	}

	i = 1;
	for (scan = album->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		char      *group;

		group = g_strdup_printf ("Track %d", i);
		s = g_key_file_get_string (f, group, "Title", NULL);
		if (s != NULL) {
			track_info_set_title (track, s);
			g_free (s);
		}
		s = g_key_file_get_string (f, group, "Artist", NULL);
		if (s != NULL) {
			track_info_set_artist (track, s, "");
			g_free (s);
		}
		g_free (group);

		i++;
	}

	g_key_file_free (f);

	return TRUE;
}


void
album_info_save_to_cache (AlbumInfo  *album,
			  const char *disc_id)
{
	GKeyFile *f;
	char     *data;
	gsize     length;
	GError   *error = NULL;
	int       i;
	GList    *scan;

	f = g_key_file_new ();

	if (album->id != NULL)
		g_key_file_set_string (f, "Album", "ID", album->id);
	if (album->title != NULL)
		g_key_file_set_string (f, "Album", "Title", album->title);
	if (album->artist != NULL)
		g_key_file_set_string (f, "Album", "Artist", album->artist);
	g_key_file_set_boolean (f, "Album", "VariousArtists", album->various_artist);
	if (album->genre != NULL)
		g_key_file_set_string (f, "Album", "Genre", album->genre);
	if (album->asin != NULL)
		g_key_file_set_string (f, "Album", "Asin", album->asin);
	if (g_date_valid (album->release_date)) {
		char s[64];

		g_date_strftime (s, sizeof(s), "%d/%m/%Y", album->release_date);
		g_key_file_set_string (f, "Album", "ReleaseDate", s);
	}

	i = 1;
	for (scan = album->tracks; scan; scan = scan->next) {
		TrackInfo *track = scan->data;
		char      *group;

		group = g_strdup_printf ("Track %d", i);
		g_key_file_set_string (f, group, "Title", track->title);
		if (track->artist != NULL)
			g_key_file_set_string (f, group, "Artist", track->artist);
		g_free (group);

		i++;
	}

	data = g_key_file_to_data (f, &length, &error);
	if (data == NULL) {
		debug (DEBUG_INFO, "%s\n", error->message);
		g_clear_error (&error);
	}
	else {
		char *path;

		path = get_cache_path (album, disc_id);
		if (path != NULL) {
			char *dir;

			dir = g_path_get_dirname (path);
			g_mkdir_with_parents (dir, 0700);

			if (! g_file_set_contents (path, data, length, &error)) {
				debug (DEBUG_INFO, "%s\n", error->message);
				g_clear_error (&error);
			}

			g_free (dir);
			g_free (path);
		}
		g_free (data);
	}

	g_key_file_free (f);
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
