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

#ifndef GOO_PLAYER_CD_H
#define GOO_PLAYER_CD_H

#include <glib.h>
#include "goo-player.h"
#include "track-info.h"

#define GOO_TYPE_PLAYER_CD              (goo_player_cd_get_type ())
#define GOO_PLAYER_CD(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_PLAYER_CD, GooPlayerCD))
#define GOO_PLAYER_CD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_PLAYER_CD, GooPlayerCDClass))
#define GOO_IS_PLAYER_CD(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_PLAYER_CD))
#define GOO_IS_PLAYER_CD_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_PLAYER_CD))
#define GOO_PLAYER_CD_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_PLAYER_CD, GooPlayerCDClass))

typedef struct _GooPlayerCD            GooPlayerCD;
typedef struct _GooPlayerCDClass       GooPlayerCDClass;
typedef struct _GooPlayerCDPrivateData GooPlayerCDPrivateData;

struct _GooPlayerCD
{
	GooPlayer __parent;
	GooPlayerCDPrivateData *priv;
};

struct _GooPlayerCDClass
{
	GooPlayerClass __parent_class;
};

GType       goo_player_cd_get_type   (void);
GooPlayer  *goo_player_cd_new        (const char *location);
const char *goo_player_cd_get_discid (GooPlayerCD *player);
TrackInfo  *goo_player_cd_get_track  (GooPlayerCD *player,
				      guint        n);
GList      *goo_player_cd_get_tracks (GooPlayerCD *player);
const char *goo_player_cd_get_artist (GooPlayerCD *player);
const char *goo_player_cd_get_album  (GooPlayerCD *player);
const char *goo_player_cd_get_genre  (GooPlayerCD *player);

#endif /* GOO_PLAYER_CD_H */
