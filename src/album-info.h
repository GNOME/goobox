/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2007 The Free Software Foundation, Inc.
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

#ifndef ALBUM_INFO_H
#define ALBUM_INFO_H

#include <glib.h>
#include <glib-object.h>
#include <time.h>
#include "track-info.h"

#define VARIUOS_ARTIST_ID "XYZ#!@$%"
#define KEEP_PREVIOUS_VALUE "(keep)"

typedef struct {
	int       ref;
	char     *id;
	char     *title;
	char     *artist;
	char     *artist_id;
	gboolean  various_artist;
	char     *genre;
	char     *asin;
	GDate    *release_date;
	GList    *tracks; /* TrackInfo */
	int       n_tracks;
	gint64    total_length;
} AlbumInfo;

#define GOO_TYPE_ALBUM_INFO (album_info_get_type ())

GType           album_info_get_type         (void);
AlbumInfo *     album_info_new              (void);
AlbumInfo *     album_info_ref              (AlbumInfo  *album);
void            album_info_unref            (AlbumInfo  *album);
AlbumInfo *     album_info_copy             (AlbumInfo  *album);
void            album_info_set_id           (AlbumInfo  *album,
					     const char *id);
void            album_info_set_title        (AlbumInfo  *album,
					     const char *title);
void            album_info_set_artist       (AlbumInfo  *album,
					     const char *artist,
					     const char *artist_id);
void            album_info_set_genre        (AlbumInfo  *album,
					     const char *genre);
void            album_info_set_asin         (AlbumInfo  *album,
					     const char *asin);
void            album_info_set_release_date (AlbumInfo  *album,
					     GDate      *date);
void            album_info_set_tracks       (AlbumInfo  *album,
					     GList      *tracks);
TrackInfo *     album_info_get_track        (AlbumInfo  *album,
					     int         track_number);
void            album_info_copy_metadata    (AlbumInfo  *to_album,
					     AlbumInfo  *from_album);
gboolean        album_info_load_from_cache  (AlbumInfo  *album,
			    		     const char *disc_id);
void            album_info_save_to_cache    (AlbumInfo  *to_album,
			  		     const char *disc_id);

GList *         album_list_dup              (GList      *album_list);
void            album_list_free             (GList      *album_list);

#endif /* GOO_ALBUM_INFO_H */
