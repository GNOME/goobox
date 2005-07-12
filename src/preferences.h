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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gtk/gtkwindow.h>
#include <gtk/gtkenums.h>
#include "typedefs.h"
#include "goo-window.h"

#define PREF_GENERAL_DEVICE           "/apps/goobox/general/device"
#define PREF_GENERAL_VOLUME           "/apps/goobox/general/volume"
#define PREF_GENERAL_COVER_PATH       "/apps/goobox/general/cover_path"
#define PREF_GENERAL_USE_SJ           "/apps/goobox/general/use_sound_juicer"

#define PREF_UI_TOOLBAR               "/apps/goobox/ui/toolbar_visible"
#define PREF_UI_STATUSBAR             "/apps/goobox/ui/statusbar_visible"
#define PREF_UI_PLAYLIST              "/apps/goobox/ui/playlist_visible"
#define PREF_UI_WINDOW_WIDTH          "/apps/goobox/ui/window_width"
#define PREF_UI_WINDOW_HEIGHT         "/apps/goobox/ui/window_height"

#define PREF_PLAYLIST_PLAYALL         "/apps/goobox/playlist/play_all"
#define PREF_PLAYLIST_SHUFFLE         "/apps/goobox/playlist/shuffle"
#define PREF_PLAYLIST_REPEAT          "/apps/goobox/playlist/repeat"
#define PREF_PLAYLIST_SORT_METHOD     "/apps/goobox/playlist/sort_method"
#define PREF_PLAYLIST_SORT_TYPE       "/apps/goobox/playlist/sort_type"

#define PREF_EXTRACT_DESTINATION      "/apps/goobox/dialogs/extract/destination"
#define PREF_EXTRACT_FILETYPE         "/apps/goobox/dialogs/extract/file_type"
#define PREF_EXTRACT_SAVE_PLAYLIST    "/apps/goobox/dialogs/extract/save_playlist"
#define PREF_EXTRACT_FIRST_TIME       "/apps/goobox/dialogs/extract/first_time"
#define PREF_RIPPER_VIEW_DISTINATION  "/apps/goobox/dialogs/ripper/view_destination"

#define PREF_ENCODER_OGG_QUALITY      "/apps/goobox/encoder/ogg_quality"
#define PREF_ENCODER_FLAC_COMPRESSION "/apps/goobox/encoder/flac_compression"
#define PREF_ENCODER_MP3_QUALITY      "/apps/goobox/encoder/mp3_quality"

#define PREF_DESKTOP_ICON_THEME         "/desktop/gnome/file_views/icon_theme"
#define PREF_DESKTOP_MENUS_HAVE_TEAROFF "/desktop/gnome/interface/menus_have_tearoff"
#define PREF_DESKTOP_MENUBAR_DETACHABLE "/desktop/gnome/interface/menubar_detachable"
#define PREF_DESKTOP_TOOLBAR_DETACHABLE "/desktop/gnome/interface/toolbar_detachable"

void                pref_util_save_window_geometry    (GtkWindow  *window,
						       const char *dialog);

void                pref_util_restore_window_geometry (GtkWindow  *window,
						       const char *dialog);

GooFileFormat       pref_get_file_format              (void);
void                pref_set_file_format              (GooFileFormat value);


WindowSortMethod    preferences_get_sort_method       (void);
void                preferences_set_sort_method       (WindowSortMethod i_value);

GtkSortType         preferences_get_sort_type         (void);
void                preferences_set_sort_type         (GtkSortType i_value);

gboolean            preferences_get_use_sound_juicer  (void);

#endif /* PREFERENCES_H */
