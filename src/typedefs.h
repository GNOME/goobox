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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#define OGG_ENCODER "vorbisenc"
#define FLAC_ENCODER "flacenc"
#define WAVE_ENCODER "wavenc"

typedef enum {
	GOO_FILE_FORMAT_OGG,
	GOO_FILE_FORMAT_FLAC,
	GOO_FILE_FORMAT_WAVE
} GooFileFormat;


typedef enum {
	WINDOW_SORT_BY_NUMBER = 0,
	WINDOW_SORT_BY_TIME = 1,
	WINDOW_SORT_BY_TITLE = 2
} WindowSortMethod;


typedef void (*DataFunc)         (gpointer    user_data);
typedef void (*ReadyFunc)        (GError     *error,
			 	  gpointer    user_data);
typedef void (*ReadyCallback)    (GObject    *object,
				  GError     *error,
			   	  gpointer    user_data);

#endif /* TYPEDEFS_H */

