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
#include "main.h"


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
	gtk_window_present (main_window);
}


static void
goo_application_class_init (GooApplicationClass *klass)
{
        POA_GNOME_Goobox_Application__epv *epv = &klass->epv;
        epv->present = impl_goo_application_present;
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
