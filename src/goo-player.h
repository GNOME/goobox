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

#ifndef GOO_PLAYER_H
#define GOO_PLAYER_H

#include <glib.h>
#include "cd-drive.h"
#include "track-info.h"
#include "album-info.h"

#define GOO_TYPE_PLAYER              (goo_player_get_type ())
#define GOO_PLAYER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_PLAYER, GooPlayer))
#define GOO_PLAYER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_PLAYER, GooPlayerClass))
#define GOO_IS_PLAYER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_PLAYER))
#define GOO_IS_PLAYER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_PLAYER))
#define GOO_PLAYER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_PLAYER, GooPlayerClass))

typedef struct _GooPlayer            GooPlayer;
typedef struct _GooPlayerClass       GooPlayerClass;
typedef struct _GooPlayerPrivateData GooPlayerPrivateData;

typedef enum {
	GOO_PLAYER_ACTION_NONE,
	GOO_PLAYER_ACTION_LIST,
	GOO_PLAYER_ACTION_SEEK_SONG,
	GOO_PLAYER_ACTION_SEEK,
	GOO_PLAYER_ACTION_PLAY,
	GOO_PLAYER_ACTION_PAUSE,
	GOO_PLAYER_ACTION_STOP,
	GOO_PLAYER_ACTION_EJECT,
	GOO_PLAYER_ACTION_UPDATE,
	GOO_PLAYER_ACTION_METADATA
} GooPlayerAction;

typedef enum {
	GOO_PLAYER_STATE_ERROR,
	GOO_PLAYER_STATE_NO_DISC,
	GOO_PLAYER_STATE_DATA_DISC,
	GOO_PLAYER_STATE_STOPPED,
	GOO_PLAYER_STATE_PLAYING,
	GOO_PLAYER_STATE_PAUSED,
	GOO_PLAYER_STATE_SEEKING,
	GOO_PLAYER_STATE_LISTING,
	GOO_PLAYER_STATE_UPDATING,
	GOO_PLAYER_STATE_EJECTING
} GooPlayerState;

struct _GooPlayer
{
	GObject __parent;
	GooPlayerPrivateData *priv;
};

struct _GooPlayerClass
{
	GObjectClass __parent_class;

	/*<signals>*/

	void        (*start)             (GooPlayer       *player,
					  GooPlayerAction  action);
	void        (*done)              (GooPlayer       *player,
					  GooPlayerAction  action,
					  GError          *error);
        void        (*progress)          (GooPlayer       *player,
					  double           fraction);
        void        (*message)           (GooPlayer       *player,
					  const char      *msg);
	void        (*state_changed)     (GooPlayer       *player);	
};

GType            goo_player_get_type            (void);
GooPlayer *      goo_player_new                 (const char      *device);
CDDrive *        goo_player_get_drive           (GooPlayer       *player);
void             goo_player_update              (GooPlayer       *player);
void             goo_player_list                (GooPlayer       *player);
void             goo_player_seek_track          (GooPlayer       *player,
						 int              n);
int              goo_player_get_current_track   (GooPlayer       *player);
void             goo_player_skip_to             (GooPlayer       *player,
						 guint            seconds);
void             goo_player_play                (GooPlayer       *player);
void             goo_player_pause               (GooPlayer       *player);
void             goo_player_stop                (GooPlayer       *player);
gboolean         goo_player_eject               (GooPlayer       *player);
GError *         goo_player_get_error           (GooPlayer       *player);
GooPlayerAction  goo_player_get_action          (GooPlayer       *player);
GooPlayerState   goo_player_get_state           (GooPlayer       *player);
gboolean         goo_player_set_device          (GooPlayer       *player,
						 const char      *device);
const char *     goo_player_get_device          (GooPlayer       *player);
const char *     goo_player_get_discid          (GooPlayer       *player);
AlbumInfo *      goo_player_get_album           (GooPlayer       *player);
double           goo_player_get_volume          (GooPlayer       *player);
void             goo_player_set_volume          (GooPlayer       *player,
						 double           vol);
gboolean         goo_player_get_is_busy         (GooPlayer       *player);

#endif /* GOO_PLAYER_H */
