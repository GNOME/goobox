/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004 The Free Software Foundation, Inc.
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

#ifndef TRACK_INFO_H
#define TRACK_INFO_H

#include <glib.h>
#include <glib-object.h>
#include <time.h>

typedef struct {
	int     ref;
	guint   number;
	gint64  from_time, to_time;
	gint64  from_sector, to_sector;
	gint64  time, length, sectors;
	int     min, sec;
	char   *title;
	char   *artist;
	char   *artist_id;
} TrackInfo;

#define GOO_TYPE_TRACK_INFO (track_info_get_type ())

GType           track_info_get_type       (void);
TrackInfo *     track_info_new            (int         number,
					   gint64      from_sector,
					   gint64      to_sector);
void            track_info_ref            (TrackInfo  *track);
void            track_info_unref          (TrackInfo  *track);
TrackInfo *     track_info_copy           (TrackInfo  *track);
void            track_info_set_title      (TrackInfo  *track,
					   const char *title);
void            track_info_set_artist     (TrackInfo  *track,
					   const char *artist,
					   const char *artist_id);
void            track_info_copy_metadata  (TrackInfo  *to_info,
					   TrackInfo  *from_info);

GList *         track_list_dup            (GList      *track_list);
void            track_list_free           (GList      *track_list);


#endif /* TRACK_INFO_H */
