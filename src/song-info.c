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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib.h>
#include "song-info.h"


SongInfo *
song_info_new (void)
{
	SongInfo *info;

	info = g_new0 (SongInfo, 1);
	info->ref = 1;
	info->number = 0;
	info->title = NULL;
	info->artist = NULL;
	info->time = 0;

	return info;
}


static void
song_info_free (SongInfo *song)
{
	g_free (song->title);
	g_free (song->artist);
	g_free (song);
}


GType
song_info_get_type (void)
{
	static GType type = 0;
  
	if (type == 0)
		type = g_boxed_type_register_static ("SongInfo", (GBoxedCopyFunc) song_info_copy, (GBoxedFreeFunc) song_info_free);
  
	return type;
}


void
song_info_ref (SongInfo *song)
{
	song->ref++;
}


void
song_info_unref (SongInfo *song)
{
	if (song == NULL)
		return;
	song->ref--;
	if (song->ref == 0)
		song_info_free (song);
}


SongInfo *
song_info_copy (SongInfo *src)
{
	SongInfo *dest;

	dest = song_info_new ();
	if (src->title != NULL)
		dest->title = g_strdup (src->title);
	if (src->artist != NULL)
		dest->artist = g_strdup (src->artist);
	dest->time = src->time;
	dest->number = src->number;

	return dest;
}


GList *
song_list_dup (GList *song_list)
{
	GList *new_list;

	if (song_list == NULL)
		return NULL;

	new_list = g_list_copy (song_list);
	g_list_foreach (song_list, (GFunc) song_info_ref, NULL);

	return new_list;
}


void
song_list_free (GList *song_list)
{
	if (song_list == NULL)
		return;
	g_list_foreach (song_list, (GFunc) song_info_unref, NULL);
	g_list_free (song_list);
}
