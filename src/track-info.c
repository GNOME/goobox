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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gst/gst.h>
#include "track-info.h"

#define SECTORS_PER_SEC 75
#define TO_SECTOR(t) ((t) / GST_SECOND * SECTORS_PER_SEC)
#define TO_TIME(s) (((s) / SECTORS_PER_SEC) * GST_SECOND)


TrackInfo*
track_info_new (int    number,
		gint64 from_sector,
		gint64 to_sector)
{
	TrackInfo *track;

	track = g_new0 (TrackInfo, 1);

	track->ref = 1;
	track->number = number;

	track_info_set_title (track, NULL);
	track_info_set_artist (track, NULL, NULL);

	track->from_sector = from_sector;
	track->to_sector = to_sector;
	track->sectors = track->to_sector - track->from_sector;

	track->from_time = TO_TIME (from_sector);
	track->to_time = TO_TIME (to_sector);
	track->time = track->to_time - track->from_time;
	track->length = (track->to_time - track->from_time) / GST_SECOND;
	track->min = track->length / 60;
	track->sec = track->length % 60;

	return track;
}


static void
track_info_free (TrackInfo *track)
{
	g_free (track->title);
	g_free (track->artist);
	g_free (track->artist_id);
	g_free (track);
}


GType
track_info_get_type (void)
{
	static GType type = 0;

	if (type == 0)
		type = g_boxed_type_register_static ("TrackInfo",
						     (GBoxedCopyFunc) track_info_copy,
						     (GBoxedFreeFunc) track_info_free);

	return type;
}


void
track_info_ref (TrackInfo *track)
{
	track->ref++;
}


void
track_info_unref (TrackInfo *track)
{
	if (track == NULL)
		return;
	track->ref--;
	if (track->ref == 0)
		track_info_free (track);
}


TrackInfo *
track_info_copy (TrackInfo *src)
{
	TrackInfo *dest;

	dest = track_info_new (src->number, src->from_sector, src->to_sector);
	track_info_set_title (dest, src->title);
	track_info_set_artist (dest, src->artist, src->artist_id);

	return dest;
}


void
track_info_set_title (TrackInfo  *track,
		      const char *title)
{
	if (title == NULL) {
		g_free (track->title);
		track->title = g_strdup_printf (_("Track %u"), track->number + 1);
		return;
	}

	if (title == track->title)
		return;

	g_free (track->title);
	track->title = g_strdup (title);
}


void
track_info_set_artist (TrackInfo  *track,
		       const char *artist,
		       const char *artist_id)
{
	if (artist == NULL) {
		g_free (track->artist);
		g_free (track->artist_id);
		track->artist = NULL /*g_strdup (_("Unknown Artist"))*/;
		track->artist_id = NULL;
		return;
	}

	if (artist == track->artist)
		return;

	g_free (track->artist);
	track->artist = g_strdup (artist);

	g_free (track->artist_id);
	if (artist_id != NULL)
		track->artist_id = g_strdup (artist_id);
	else
		track->artist_id = NULL;
}


void
track_info_copy_metadata (TrackInfo  *to_track,
			  TrackInfo  *from_track)
{
	track_info_set_title (to_track, from_track->title);
	track_info_set_artist (to_track, from_track->artist, from_track->artist_id);
}


GList *
track_list_dup (GList *track_list)
{
	if (track_list == NULL)
		return NULL;
	return g_list_copy_deep (track_list, (GCopyFunc) track_info_copy, NULL);
}


void
track_list_free (GList *track_list)
{
	if (track_list == NULL)
		return;
	g_list_foreach (track_list, (GFunc) track_info_unref, NULL);
	g_list_free (track_list);
}
