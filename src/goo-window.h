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

#ifndef GOO_WINDOW_H
#define GOO_WINDOW_H

#include <libgnomeui/gnome-app.h>
#include "goo-player.h"

#define GOO_TYPE_WINDOW              (goo_window_get_type ())
#define GOO_WINDOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_WINDOW, GooWindow))
#define GOO_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_WINDOW_TYPE, GooWindowClass))
#define GOO_IS_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_WINDOW))
#define GOO_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_WINDOW))
#define GOO_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_WINDOW, GooWindowClass))

typedef struct _GooWindow            GooWindow;
typedef struct _GooWindowClass       GooWindowClass;
typedef struct _GooWindowPrivateData GooWindowPrivateData;

struct _GooWindow
{
	GnomeApp __parent;
	GooWindowPrivateData *priv;
};

struct _GooWindowClass
{
	GnomeAppClass __parent_class;
};

GType       goo_window_get_type                  (void);
GtkWindow * goo_window_new                       (const char  *location);
void        goo_window_close                     (GooWindow   *window);
void        goo_window_set_toolbar_visibility    (GooWindow   *window,
						  gboolean     visible);
void        goo_window_set_statusbar_visibility  (GooWindow   *window,
						  gboolean     visible);
void        goo_window_update                    (GooWindow   *window);
void        goo_window_play                      (GooWindow   *window);
void        goo_window_play_selected             (GooWindow   *window);
void        goo_window_toggle_play               (GooWindow   *window);
void        goo_window_stop                      (GooWindow   *window);
void        goo_window_pause                     (GooWindow   *window);
void        goo_window_prev                      (GooWindow   *window);
void        goo_window_next                      (GooWindow   *window);
void        goo_window_eject                     (GooWindow   *window);
void        goo_window_set_location              (GooWindow   *window,
						  const char  *location);
GList      *goo_window_get_song_list             (GooWindow   *window,
						  gboolean     selection);
void        goo_window_edit_cddata               (GooWindow   *window);
GooPlayer  *goo_window_get_player                (GooWindow   *window);
void        goo_window_pick_cover_from_disk      (GooWindow   *window);
void        goo_window_search_cover_on_internet  (GooWindow   *window);

#endif /* GOO_WINDOW_H */
