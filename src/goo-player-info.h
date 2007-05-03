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

#ifndef GOO_PLAYER_INFO_H
#define GOO_PLAYER_INFO_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkhbox.h>
#include "goo-window.h"
#include "goo-player-cd.h"
#include "song-info.h"

#define GOO_TYPE_PLAYER_INFO              (goo_player_info_get_type ())
#define GOO_PLAYER_INFO(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_PLAYER_INFO, GooPlayerInfo))
#define GOO_PLAYER_INFO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_PLAYER_INFO, GooPlayerInfoClass))
#define GOO_IS_PLAYER_INFO(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_PLAYER_INFO))
#define GOO_IS_PLAYER_INFO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_PLAYER_INFO))
#define GOO_PLAYER_INFO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_PLAYER_INFO, GooPlayerInfoClass))

typedef struct _GooPlayerInfo            GooPlayerInfo;
typedef struct _GooPlayerInfoClass       GooPlayerInfoClass;
typedef struct _GooPlayerInfoPrivateData GooPlayerInfoPrivateData;

struct _GooPlayerInfo
{
	GtkHBox __parent;
	GooPlayerInfoPrivateData *priv;
};

struct _GooPlayerInfoClass
{
	GtkHBoxClass __parent_class;

	/*<signals>*/

        void        (*cover_clicked)    (GooPlayerInfo   *info);
        void        (*skip_to)          (GooPlayerInfo   *info,
					 int              seconds);

};

GType            goo_player_info_get_type         (void);
GtkWidget *      goo_player_info_new              (GooWindow      *window,
						   GooPlayer      *player);
void             goo_player_info_set_player       (GooPlayerInfo  *info,
						   GooPlayer      *player);
void             goo_player_info_update_state     (GooPlayerInfo  *info);
void             goo_player_info_set_total_time   (GooPlayerInfo  *info,
						   gint64          current_time);
void             goo_player_info_set_time         (GooPlayerInfo  *info,
						   gint64          current_time);
void             goo_player_info_set_sensitive    (GooPlayerInfo  *info,
						   gboolean        value);
void             goo_player_info_set_cover        (GooPlayerInfo  *info,
						   const char     *cover);

#endif /* GOO_PLAYER_INFO_H */
