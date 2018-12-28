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
#include "glib-utils.h"
#include "goo-application-actions-callbacks.h"
#include "preferences.h"


static const GActionEntry goo_application_actions[] = {
	{ "preferences",  goo_application_activate_preferences },
	{ PREF_PLAYLIST_PLAYALL, _g_toggle_action_activated, NULL, "true", goo_application_activate_play_all },
	{ PREF_PLAYLIST_REPEAT, _g_toggle_action_activated, NULL, "false", goo_application_activate_repeat },
	{ PREF_PLAYLIST_SHUFFLE, _g_toggle_action_activated, NULL, "true", goo_application_activate_shuffle },
	{ "help",  goo_application_activate_help },
	{ "about", goo_application_activate_about },
	{ "quit",  goo_application_activate_quit },
	{ "pause",  goo_application_activate_pause, "s", NULL, NULL },
	{ "play",  goo_application_activate_play, "s", NULL, NULL },
	{ "play-next",  goo_application_activate_play_next, "s", NULL, NULL },
};


#endif /* GOO_WINDOW_ACTIONS_ENTRIES_H */
