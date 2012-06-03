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

#ifndef ACTIONS_H
#define ACTIONS_H

#include <gtk/gtk.h>

void activate_action_play (GtkAction *action, gpointer data);
void activate_action_play_selected (GtkAction *action, gpointer data);
void activate_action_pause (GtkAction *action, gpointer data);
void activate_action_toggle_play (GtkAction *action, gpointer data);
void activate_action_stop (GtkAction *action, gpointer data);
void activate_action_next (GtkAction *action, gpointer data);
void activate_action_prev (GtkAction *action, gpointer data);
void activate_action_eject (GtkAction *action, gpointer data);
void activate_action_reload (GtkAction *action, gpointer data);
void activate_action_manual (GtkAction *action, gpointer data);
void activate_action_shortcuts (GtkAction *action, gpointer data);
void activate_action_about (GtkAction *action, gpointer data);
void activate_action_close (GtkAction *action, gpointer data);
void activate_action_quit (GtkAction *action, gpointer data);
void activate_action_play_all (GtkAction *action, gpointer data);
void activate_action_repeat (GtkAction *action, gpointer data);
void activate_action_shuffle (GtkAction *action, gpointer data);
void activate_action_copy_disc (GtkAction *action, gpointer data);
void activate_action_extract (GtkAction *action, gpointer data);
void activate_action_extract_selected (GtkAction *action, gpointer data);
void activate_action_preferences (GtkAction *action, gpointer data);
void activate_action_pick_cover_from_disk (GtkAction *action, gpointer data);
void activate_action_search_cover_on_internet (GtkAction *action, gpointer data);
void activate_action_remove_cover (GtkAction *action, gpointer data);
void activate_action_toggle_visibility (GtkAction *action, gpointer data);
void activate_action_properties (GtkAction *action, gpointer data);

#endif /* ACTIONS_H */
