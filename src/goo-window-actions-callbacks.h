/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

#ifndef GOO_WINDOW_ACTIONS_CALLBACKS_H
#define GOO_WINDOW_ACTIONS_CALLBACKS_H

#include <gtk/gtk.h>
#include "glib-utils.h"

DEF_ACTION_CALLBACK (goo_window_activate_play)
DEF_ACTION_CALLBACK (goo_window_activate_play_selected)
DEF_ACTION_CALLBACK (goo_window_activate_pause)
DEF_ACTION_CALLBACK (goo_window_activate_toggle_play)
DEF_ACTION_CALLBACK (goo_window_activate_stop)
DEF_ACTION_CALLBACK (goo_window_activate_next_track)
DEF_ACTION_CALLBACK (goo_window_activate_previous_track)
DEF_ACTION_CALLBACK (goo_window_activate_eject)
DEF_ACTION_CALLBACK (goo_window_activate_reload)
DEF_ACTION_CALLBACK (goo_window_activate_close)
DEF_ACTION_CALLBACK (goo_window_activate_play_all)
DEF_ACTION_CALLBACK (goo_window_activate_repeat)
DEF_ACTION_CALLBACK (goo_window_activate_shuffle)
DEF_ACTION_CALLBACK (goo_window_activate_copy_disc)
DEF_ACTION_CALLBACK (goo_window_activate_extract)
DEF_ACTION_CALLBACK (goo_window_activate_extract_selected)
DEF_ACTION_CALLBACK (goo_window_activate_preferences)
DEF_ACTION_CALLBACK (goo_window_activate_pick_cover_from_disk)
DEF_ACTION_CALLBACK (goo_window_activate_search_cover)
DEF_ACTION_CALLBACK (goo_window_activate_remove_cover)
DEF_ACTION_CALLBACK (goo_window_activate_toggle_visibility)
DEF_ACTION_CALLBACK (goo_window_activate_properties)

#endif /* GOO_WINDOW_ACTIONS_CALLBACKS_H */
