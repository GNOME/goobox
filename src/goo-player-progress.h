/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2018 Free Software Foundation, Inc.
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

#ifndef GOO_PLAYER_PROGRESS_H
#define GOO_PLAYER_PROGRESS_H

#include <gtk/gtk.h>
#include "goo-player.h"

#define GOO_TYPE_PLAYER_PROGRESS              (goo_player_progress_get_type ())
#define GOO_PLAYER_PROGRESS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_PLAYER_PROGRESS, GooPlayerProgress))
#define GOO_PLAYER_PROGRESS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_PLAYER_PROGRESS, GooPlayerProgressClass))
#define GOO_IS_PLAYER_PROGRESS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_PLAYER_PROGRESS))
#define GOO_IS_PLAYER_PROGRESS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_PLAYER_PROGRESS))
#define GOO_PLAYER_PROGRESS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_PLAYER_PROGRESS, GooPlayerProgressClass))

typedef struct _GooPlayerProgress            GooPlayerProgress;
typedef struct _GooPlayerProgressClass       GooPlayerProgressClass;
typedef struct _GooPlayerProgressPrivate     GooPlayerProgressPrivate;

struct _GooPlayerProgress
{
	GtkBox __parent;
	GooPlayerProgressPrivate *priv;
};

struct _GooPlayerProgressClass
{
	GtkBoxClass __parent_class;

	/*<signals>*/

	void (*skip_to) (GooPlayerProgress	*info,
			 int			 seconds);

};

GType		goo_player_progress_get_type	(void);
GtkWidget *	goo_player_progress_new		(GooPlayer		*player);
void		goo_player_progress_set_title	(GooPlayerProgress	*progress,
						 const char 		*title);
double		goo_player_progress_get_progress
						(GooPlayerProgress	*progress);

#endif /* GOO_PLAYER_PROGRESS_H */
