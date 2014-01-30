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

#ifndef GOO_WINDOW_ACTIONS_ENTRIES_H
#define GOO_WINDOW_ACTIONS_ENTRIES_H

#include <config.h>
#include "gtk-utils.h"
#include "goo-window-actions-callbacks.h"


static const GActionEntry goo_window_actions[] = {
	{ "play", goo_window_activate_play },
	{ "play-selected", goo_window_activate_play_selected },
	{ "pause", goo_window_activate_pause },
	{ "toggle-play", goo_window_activate_toggle_play },
	{ "stop", goo_window_activate_stop },
	{ "next-track", goo_window_activate_next_track },
	{ "previous-track", goo_window_activate_previous_track },
	{ "eject", goo_window_activate_eject },
	{ "reload", goo_window_activate_reload },
	{ "close", goo_window_activate_close },
	{ "play-all", goo_window_activate_play_all },
	{ "repeat", goo_window_activate_repeat },
	{ "shuffle", goo_window_activate_shuffle },
	{ "copy-disc", goo_window_activate_copy_disc },
	{ "extract", goo_window_activate_extract },
	{ "extract-selected", goo_window_activate_extract_selected },
	{ "preferences", goo_window_activate_preferences },
	{ "pick-cover-from-disk", goo_window_activate_pick_cover_from_disk },
	{ "search-cover", goo_window_activate_search_cover },
	{ "remove-cover", goo_window_activate_remove_cover },
	{ "toggle-visibility", goo_window_activate_toggle_visibility },
	{ "properties", goo_window_activate_properties }
};


static const _GtkAccelerator goo_window_accelerators[] = {
        { "close", "<Control>w" },
        { "toggle-play", "space" },
        { "stop", "Escape" },
        { "next-track", "n" },
        { "previous-track", "p" },
        { "eject", "j" },
        { "extract", "e" },
        { "close", "<Control>w" },
        { "properties", "<Control>Return" }
};


#endif /* GOO_WINDOW_ACTIONS_ENTRIES_H */
