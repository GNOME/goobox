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

#ifndef SONG_INFO_H
#define SONG_INFO_H

#include <glib.h>
#include <glib-object.h>
#include <time.h>

typedef struct {
	guint     ref : 8;
	guint     number;
	char     *title;
	char     *artist;
	gint64    time;
} SongInfo;

#define GOO_TYPE_SONG_INFO (song_info_get_type ())

GType           song_info_get_type    (void);
SongInfo *      song_info_new         (void);
void            song_info_ref         (SongInfo *song);
void            song_info_unref       (SongInfo *song);
SongInfo *      song_info_copy        (SongInfo *song);

GList *         song_list_dup         (GList    *song_list);
void            song_list_free        (GList    *song_list);

#endif /* SONG_INFO_H */
