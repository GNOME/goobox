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

#include <config.h>
#include <string.h>
#include <glib.h>
#include "goo-player.h"
#include "goo-marshal.h"

struct _GooPlayerPrivateData {
	GooPlayerState   state;
	GooPlayerAction  action;
	GError          *error;
	int              current_song;
	int              volume;
	char            *location;
	char            *title;
	char            *subtitle;
};

enum {
	START,
        DONE,
	PROGRESS,
	MESSAGE,
	STATE_CHANGED,
        LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint goo_player_signals[LAST_SIGNAL] = { 0 };

static void goo_player_class_init  (GooPlayerClass *class);
static void goo_player_init        (GooPlayer *player);
static void goo_player_finalize    (GObject *object);


GType
goo_player_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GooPlayerClass),
			NULL,
			NULL,
			(GClassInitFunc) goo_player_class_init,
			NULL,
			NULL,
			sizeof (GooPlayer),
			0,
			(GInstanceInitFunc) goo_player_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GooPlayer",
					       &type_info,
					       0);
	}

        return type;
}


static void
base_goo_player_update (GooPlayer *player)
{
}


static void
base_goo_player_list (GooPlayer *player)
{
}


static void
base_goo_player_seek_song (GooPlayer *player,
			   int        n)
{
}


SongInfo *
base_goo_player_get_song (GooPlayer *player,
			  int        n)
{
	return NULL;
}


gboolean
base_goo_player_set_location (GooPlayer  *player,
			      const char *location)
{
	g_free (player->priv->location);
	player->priv->location = NULL;
	if (location != NULL)
		player->priv->location = g_strdup (location);

	return TRUE;
}


void
base_goo_player_set_volume (GooPlayer *player,
			    int        vol)
{
	player->priv->volume = vol;
}


static void
base_goo_player_skip_to (GooPlayer *player,
			 guint      seconds)
{
}


static void
base_goo_player_play (GooPlayer *player)
{
}


static void
base_goo_player_pause (GooPlayer *player)
{
}


static void
base_goo_player_stop (GooPlayer *player)
{
}


static gboolean
base_goo_player_eject (GooPlayer *player)
{
	return FALSE;
}


static GList*
base_goo_player_get_song_list (GooPlayer *player)
{
	return NULL;
}


static void 
goo_player_class_init (GooPlayerClass *class)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (class);

        parent_class = g_type_class_peek_parent (class);

	goo_player_signals[START] =
                g_signal_new ("start",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, start),
			      NULL, NULL,
			      goo_marshal_VOID__INT,
			      G_TYPE_NONE, 
			      1,
			      G_TYPE_INT);
	goo_player_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, done),
			      NULL, NULL,
			      goo_marshal_VOID__INT_POINTER,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      G_TYPE_POINTER);
	goo_player_signals[PROGRESS] =
		g_signal_new ("progress",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, progress),
			      NULL, NULL,
			      goo_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);
	goo_player_signals[MESSAGE] =
		g_signal_new ("message",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, message),
			      NULL, NULL,
			      goo_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
	goo_player_signals[STATE_CHANGED] =
		g_signal_new ("state_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerClass, state_changed),
			      NULL, NULL,
			      goo_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

        gobject_class->finalize = goo_player_finalize;

	class->update           = base_goo_player_update;
	class->list             = base_goo_player_list;
	class->seek_song        = base_goo_player_seek_song;
	class->get_song         = base_goo_player_get_song;
	class->skip_to          = base_goo_player_skip_to;
	class->play             = base_goo_player_play;
	class->pause            = base_goo_player_pause;
	class->stop             = base_goo_player_stop;
	class->get_song_list    = base_goo_player_get_song_list;
	class->eject            = base_goo_player_eject;
	class->set_location     = base_goo_player_set_location;
	class->set_volume       = base_goo_player_set_volume;

	class->start          = NULL;
	class->done           = NULL;
	class->progress       = NULL;
	class->message        = NULL;
	class->state_changed   = NULL;
}


static void 
goo_player_init (GooPlayer *player)
{
	GooPlayerPrivateData *priv;

	player->priv = g_new0 (GooPlayerPrivateData, 1);
	priv = player->priv;

	priv->state = GOO_PLAYER_STATE_STOPPED;
	priv->action = GOO_PLAYER_ACTION_NONE;
	priv->error = NULL;
	priv->current_song = 0;
	priv->volume = 0;
	priv->location = NULL;
	priv->title = NULL;
	priv->subtitle = NULL;
}


static void 
goo_player_finalize (GObject *object)
{
        GooPlayer *player;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_PLAYER (object));

	player = GOO_PLAYER (object);
	if (player->priv != NULL) {
		GooPlayerPrivateData *priv = player->priv;

		if (priv->error != NULL)
			g_error_free (priv->error);

		g_free (priv->location);
		g_free (priv->title);
		g_free (priv->subtitle);

		g_free (player->priv);
		player->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


void
goo_player_construct (GooPlayer  *player,
		      const char *location)
{
	base_goo_player_set_location (player, location);
}


static void
notify_action_start (GooPlayer *player)
{
	g_signal_emit (G_OBJECT (player), 
		       goo_player_signals[START], 
		       0,
		       player->priv->action, 
		       NULL);
}


void
goo_player_update (GooPlayer *player)
{
	player->priv->action = GOO_PLAYER_ACTION_UPDATE;
	player->priv->state = GOO_PLAYER_STATE_UPDATING;
	notify_action_start (player);
	GOO_PLAYER_GET_CLASS (G_OBJECT (player))->update (player);
}


void
goo_player_list (GooPlayer *player)
{
	player->priv->action = GOO_PLAYER_ACTION_LIST;
	player->priv->state = GOO_PLAYER_STATE_SEEKING;
	notify_action_start (player);
	GOO_PLAYER_GET_CLASS (G_OBJECT (player))->list (player);
}


void
goo_player_seek_song (GooPlayer *player,
		      int        n)
{
	player->priv->action = GOO_PLAYER_ACTION_SEEK_SONG;
	player->priv->state = GOO_PLAYER_STATE_SEEKING;
	notify_action_start (player);
	GOO_PLAYER_GET_CLASS (G_OBJECT (player))->seek_song (player, n);
}


SongInfo *
goo_player_get_song (GooPlayer *player,
		     int        n)
{
	if (n >= 0)
		return GOO_PLAYER_GET_CLASS (G_OBJECT (player))->get_song (player, n);
	else
		return NULL;
}


int
goo_player_get_current_song (GooPlayer *player)
{
	return player->priv->current_song;
}


void
goo_player_set_current_song (GooPlayer *player,
			     int        n)
{
	player->priv->current_song = n;
}


void
goo_player_skip_to (GooPlayer *player,
		    guint      seconds)
{
	GOO_PLAYER_GET_CLASS (G_OBJECT (player))->skip_to (player, seconds);
}


const char *
goo_player_get_location (GooPlayer *player)
{
	return player->priv->location;
}


gboolean
goo_player_set_location (GooPlayer  *player,
			 const char *location)
{
	return GOO_PLAYER_GET_CLASS (G_OBJECT (player))->set_location (player, location);
}


const char *
goo_player_get_title (GooPlayer *player)
{
	return player->priv->title;
}


void
goo_player_set_title (GooPlayer  *player,
		      const char *value)
{
	g_free (player->priv->title);
	player->priv->title = NULL;
	if (value != NULL)
		player->priv->title = g_strdup (value);
}


const char *
goo_player_get_subtitle (GooPlayer *player)
{
	return player->priv->subtitle;
}


void
goo_player_set_subtitle (GooPlayer  *player,
			 const char *value)
{
	g_free (player->priv->subtitle);
	player->priv->subtitle = NULL;
	if (value != NULL)
		player->priv->subtitle = g_strdup (value);
}


void
goo_player_play (GooPlayer *player)
{
	if (player->priv->state == GOO_PLAYER_STATE_PLAYING)
		return;
	player->priv->action = GOO_PLAYER_ACTION_PLAY;
	notify_action_start (player);
	GOO_PLAYER_GET_CLASS (G_OBJECT (player))->play (player);
}


void
goo_player_pause (GooPlayer *player)
{
	if (player->priv->state == GOO_PLAYER_STATE_PAUSED)
		return;
	GOO_PLAYER_GET_CLASS (G_OBJECT (player))->pause (player);
}


void
goo_player_stop (GooPlayer *player)
{
	if (player->priv->state == GOO_PLAYER_STATE_STOPPED)
		return;
	GOO_PLAYER_GET_CLASS (G_OBJECT (player))->stop (player);
}


gboolean
goo_player_eject (GooPlayer *player)
{
	player->priv->action = GOO_PLAYER_ACTION_EJECT;
	player->priv->state = GOO_PLAYER_STATE_EJECTING;
	notify_action_start (player);
	return GOO_PLAYER_GET_CLASS (G_OBJECT (player))->eject (player);
}


void
goo_player_set_error (GooPlayer  *player,
		      GError     *error)
{
     if (player->priv->error != NULL)
	     g_error_free (player->priv->error);
     player->priv->error = error;
}


GError *
goo_player_get_error (GooPlayer *player)
{
     if (player->priv->error != NULL)
	     return g_error_copy (player->priv->error);
     else
	     return NULL;
}


GList *
goo_player_get_song_list (GooPlayer *player)
{
	return GOO_PLAYER_GET_CLASS (G_OBJECT (player))->get_song_list (player);
}


GooPlayerAction
goo_player_get_action (GooPlayer *player)
{
	return player->priv->action;
}


GooPlayerState
goo_player_get_state (GooPlayer *player)
{
	return player->priv->state;
}


void
goo_player_set_state (GooPlayer       *player,
		      GooPlayerState   state,
		      gboolean         notify)
{
	player->priv->state = state;
	if (notify)
		g_signal_emit (G_OBJECT (player), 
			       goo_player_signals[STATE_CHANGED], 
			       0,
			       NULL);
}


int
goo_player_get_volume (GooPlayer *player)
{
	return player->priv->volume;
}


void
goo_player_set_volume_protected (GooPlayer *player,
				 int        vol)
{
	player->priv->volume = vol;
}


void
goo_player_set_volume (GooPlayer *player,
		       int        vol)
{
	player->priv->volume = vol;
	GOO_PLAYER_GET_CLASS (G_OBJECT (player))->set_volume (player, vol);
}

