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

#ifndef GOO_WINDOW_H
#define GOO_WINDOW_H

#include <gtk/gtk.h>
#include <brasero3/brasero-drive.h>
#include "album-info.h"
#include "goo-player.h"

#define GOO_TYPE_WINDOW              (goo_window_get_type ())
#define GOO_WINDOW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_WINDOW, GooWindow))
#define GOO_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_WINDOW_TYPE, GooWindowClass))
#define GOO_IS_WINDOW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_WINDOW))
#define GOO_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_WINDOW))
#define GOO_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_WINDOW, GooWindowClass))

typedef struct _GooWindow        GooWindow;
typedef struct _GooWindowClass   GooWindowClass;
typedef struct _GooWindowPrivate GooWindowPrivate;

struct _GooWindow
{
	GtkApplicationWindow __parent;
	GtkWidget *preferences_dialog;
	GtkWidget *properties_dialog;
	GooWindowPrivate *priv;
};

struct _GooWindowClass
{
	GtkApplicationWindowClass __parent_class;

	/*<signals>*/

	void (*update_cover) (GooWindow *window);
};

GType       goo_window_get_type                  (void);
GtkWidget * goo_window_new                       (BraseroDrive *drive);
void        goo_window_close                     (GooWindow    *window);
void        goo_window_update                    (GooWindow    *window);
void        goo_window_play                      (GooWindow    *window);
void        goo_window_play_selected             (GooWindow    *window);
void        goo_window_toggle_play               (GooWindow    *window);
void        goo_window_stop                      (GooWindow    *window);
void        goo_window_pause                     (GooWindow    *window);
void        goo_window_prev                      (GooWindow    *window);
void        goo_window_next                      (GooWindow    *window);
void        goo_window_eject                     (GooWindow    *window);
void        goo_window_set_drive                 (GooWindow    *window,
						  BraseroDrive *drive);
AlbumInfo * goo_window_get_album                 (GooWindow    *window);
GList *     goo_window_get_tracks                (GooWindow    *window,
						  gboolean      selection);
GooPlayer * goo_window_get_player                (GooWindow    *window);
GtkWidget * goo_window_get_player_info           (GooWindow    *window);
void        goo_window_update_cover              (GooWindow    *window);
gboolean    goo_window_set_cover_image_from_pixbuf
						 (GooWindow  *window,
					          GdkPixbuf    *image);
gboolean    goo_window_set_cover_image           (GooWindow    *window,
						  const char   *filename);
gboolean    goo_window_set_cover_image_from_data (GooWindow    *window,
						  void         *buffer,
						  gsize         count);
char *      goo_window_get_cover_filename        (GooWindow    *window);
void        goo_window_pick_cover_from_disk      (GooWindow    *window);
void        goo_window_search_cover_on_internet  (GooWindow    *window);
void        goo_window_remove_cover              (GooWindow    *window);
void        goo_window_toggle_visibility         (GooWindow    *window);
void        goo_window_set_hibernate             (GooWindow    *window,
						  gboolean      hibernate);
void        goo_window_set_current_cd_autofetch  (GooWindow    *window,
						  gboolean      autofetch);
gboolean    goo_window_get_current_cd_autofetch  (GooWindow    *window);

#endif /* GOO_WINDOW_H */
