/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004-2011 Free Software Foundation, Inc.
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gtk/gtk.h>

#define GOOBOX_SCHEMA                "org.gnome.Goobox"
#define GOOBOX_SCHEMA_GENERAL        GOOBOX_SCHEMA ".General"
#define GOOBOX_SCHEMA_UI             GOOBOX_SCHEMA ".UI"
#define GOOBOX_SCHEMA_PLAYLIST       GOOBOX_SCHEMA ".Playlist"
#define GOOBOX_SCHEMA_RIPPER         GOOBOX_SCHEMA ".Ripper"
#define GOOBOX_SCHEMA_ENCODER        GOOBOX_SCHEMA ".Encoder"

#define PREF_GENERAL_DEVICE           "device"
#define PREF_GENERAL_VOLUME           "volume"
#define PREF_GENERAL_COVER_PATH       "cover-path"
#define PREF_GENERAL_USE_SJ           "use-sound-juicer"
#define PREF_GENERAL_AUTOPLAY         "autoplay"

#define PREF_UI_TOOLBAR               "toolbar-visible"
#define PREF_UI_STATUSBAR             "statusbar-visible"
#define PREF_UI_PLAYLIST              "playlist-visible"
#define PREF_UI_WINDOW_WIDTH          "window-width"
#define PREF_UI_WINDOW_HEIGHT         "window-height"

#define PREF_PLAYLIST_PLAYALL         "play-all"
#define PREF_PLAYLIST_SHUFFLE         "shuffle"
#define PREF_PLAYLIST_REPEAT          "repeat"
#define PREF_PLAYLIST_SORT_METHOD     "sort-method"
#define PREF_PLAYLIST_SORT_TYPE       "sort-type"

#define PREF_RIPPER_DESTINATION       "destination"
#define PREF_RIPPER_FILETYPE          "file-type"
#define PREF_RIPPER_SAVE_PLAYLIST     "save-playlist"
#define PREF_RIPPER_VIEW_DISTINATION  "view-destination"

#define PREF_ENCODER_OGG_QUALITY      "ogg-quality"
#define PREF_ENCODER_FLAC_COMPRESSION "flac-compression"
#define PREF_ENCODER_MP3_QUALITY      "mp3-quality"

#endif /* PREFERENCES_H */
