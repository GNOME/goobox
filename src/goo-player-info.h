/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004, 2007 Free Software Foundation, Inc.
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

#ifndef GOO_PLAYER_INFO_H
#define GOO_PLAYER_INFO_H

#include <gtk/gtk.h>
#include "goo-window.h"

#define GOO_TYPE_PLAYER_INFO              (goo_player_info_get_type ())
#define GOO_PLAYER_INFO(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_PLAYER_INFO, GooPlayerInfo))
#define GOO_PLAYER_INFO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_PLAYER_INFO, GooPlayerInfoClass))
#define GOO_IS_PLAYER_INFO(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_PLAYER_INFO))
#define GOO_IS_PLAYER_INFO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_PLAYER_INFO))
#define GOO_PLAYER_INFO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_PLAYER_INFO, GooPlayerInfoClass))

typedef struct _GooPlayerInfo        GooPlayerInfo;
typedef struct _GooPlayerInfoClass   GooPlayerInfoClass;
typedef struct _GooPlayerInfoPrivate GooPlayerInfoPrivate;

struct _GooPlayerInfo
{
	GtkBox __parent;
	GooPlayerInfoPrivate *priv;
};

struct _GooPlayerInfoClass
{
	GtkBoxClass __parent_class;

	/*<signals>*/

	void (*cover_clicked) (GooPlayerInfo *info);
	void (*update_status) (GooPlayerInfo *info);
};

GType		goo_player_info_get_type	(void);
GtkWidget *	goo_player_info_new		(GooWindow	*window);
GdkPixbuf *	goo_player_info_get_cover	(GooPlayerInfo	*info);
const char *	goo_player_info_get_cover_file	(GooPlayerInfo	*info);

#endif /* GOO_PLAYER_INFO_H */
