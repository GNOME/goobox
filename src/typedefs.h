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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <glib.h>
#include <glib-object.h>

#define OGG_ENCODER "vorbisenc"
#define FLAC_ENCODER "flacenc"
#define MP3_ENCODER "lamemp3enc"
#define WAVE_ENCODER "wavenc"

#define OGG_DESCRIPTION N_("Vorbis is an open source, lossy audio codec with high quality output at a lower file size than MP3.")
#define FLAC_DESCRIPTION N_("FLAC is an open source codec that compresses but does not degrade audio quality.")
#define MP3_DESCRIPTION N_("MP3 is a patented lossy audio codec, supported by most digital audio players.")
#define WAVE_DESCRIPTION N_("WAV+PCM is a lossless format that holds uncompressed audio.")

/* same order shown in the preferences dialog */

typedef enum {
	GOO_FILE_FORMAT_OGG,
	GOO_FILE_FORMAT_FLAC,
	GOO_FILE_FORMAT_MP3,
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

