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
#include <bonobo/bonobo-generic-factory.h>
#include "goo-application.h"
#include "goo-window.h"
#include "main.h"

#define VOLUME_STEP 25


static BonoboObject *
goo_application_factory (BonoboGenericFactory *this_factory,
			 const char           *iid,
			 gpointer              user_data)
{
	if (strcmp (iid, "OAFIID:GNOME_Goobox_Application") != 0)
		return NULL;
	else
		return BONOBO_OBJECT (g_object_new (GOO_TYPE_APPLICATION, NULL));
}


BonoboObject *
goo_application_new (GdkScreen *screen)
{
        BonoboGenericFactory *factory;
        char                 *display_name;
        char                 *registration_id;

        display_name = gdk_screen_make_display_name (screen);
        registration_id = bonobo_activation_make_registration_id ("OAFIID:GNOME_Goobox_Application_Factory", display_name);

        factory = bonobo_generic_factory_new (registration_id,
                                              goo_application_factory,
                                              NULL);
	g_free (display_name);
        g_free (registration_id);

        return BONOBO_OBJECT (factory);
}


static void
impl_goo_application_present (PortableServer_Servant  _servant,
			      CORBA_Environment      *ev)
{
	GooPlayer *player;

	if (GTK_WIDGET_VISIBLE (main_window))
		gtk_window_present (main_window);
	else
		goo_window_toggle_visibility (GOO_WINDOW (main_window));

	player = goo_window_get_player (GOO_WINDOW (main_window));
	if (goo_player_get_state (player) != GOO_PLAYER_STATE_PLAYING) 
		goo_window_update (GOO_WINDOW (main_window));
}


static void
impl_goo_application_play (PortableServer_Servant  _servant,
			   CORBA_Environment      *ev)
{
	goo_window_play (GOO_WINDOW (main_window));
}


static void
impl_goo_application_play_pause (PortableServer_Servant  _servant,
				 CORBA_Environment      *ev)
{
	goo_window_toggle_play (GOO_WINDOW (main_window));
}


static void
impl_goo_application_next (PortableServer_Servant  _servant,
			   CORBA_Environment      *ev)
{
	goo_window_next (GOO_WINDOW (main_window));
}


static void
impl_goo_application_prev (PortableServer_Servant  _servant,
			   CORBA_Environment      *ev)
{
	goo_window_prev (GOO_WINDOW (main_window));
}


static void
impl_goo_application_eject (PortableServer_Servant  _servant,
			    CORBA_Environment      *ev)
{
	goo_window_eject (GOO_WINDOW (main_window));
}


static void
impl_goo_application_hide_show (PortableServer_Servant  _servant,
				CORBA_Environment      *ev)
{
	goo_window_toggle_visibility (GOO_WINDOW (main_window));
}


static void
impl_goo_application_volume_up (PortableServer_Servant  _servant,
				CORBA_Environment      *ev)
{
	GooWindow *window = GOO_WINDOW (main_window);
	int        volume;

	volume = goo_window_get_volume (window);
	goo_window_set_volume (window, volume + VOLUME_STEP);
}


static void
impl_goo_application_volume_down (PortableServer_Servant  _servant,
				  CORBA_Environment      *ev)
{
	GooWindow *window = GOO_WINDOW (main_window);
	int        volume;

	volume = goo_window_get_volume (window);
	goo_window_set_volume (window, volume - VOLUME_STEP);
}


static void
impl_goo_application_quit (PortableServer_Servant  _servant,
			   CORBA_Environment      *ev)
{
	goo_window_close (GOO_WINDOW (main_window));
}


static void
goo_application_class_init (GooApplicationClass *klass)
{
        POA_GNOME_Goobox_Application__epv *epv = &klass->epv;
        epv->present     = impl_goo_application_present;
        epv->play        = impl_goo_application_play;
        epv->play_pause  = impl_goo_application_play_pause;
        epv->next        = impl_goo_application_next;
        epv->prev        = impl_goo_application_prev;
        epv->eject       = impl_goo_application_eject;
        epv->hide_show   = impl_goo_application_hide_show;
        epv->volume_up   = impl_goo_application_volume_up;
        epv->volume_down = impl_goo_application_volume_down;
        epv->quit        = impl_goo_application_quit;
}


static void
goo_application_init (GooApplication *c)
{
}


BONOBO_TYPE_FUNC_FULL (
        GooApplication,
        GNOME_Goobox_Application,
        BONOBO_TYPE_OBJECT,
        goo_application);
