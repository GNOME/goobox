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

#ifndef GOO_APPLICATION_H
#define GOO_APPLICATION_H

#include <gtk/gtk.h>
#include <bonobo/bonobo-object.h>
#include "GNOME_Goobox.h"

#define GOO_TYPE_APPLICATION              (goo_application_get_type ())
#define GOO_APPLICATION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_APPLICATION, GooApplication))
#define GOO_APPLICATION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_APPLICATION_TYPE, GooApplicationClass))
#define GOO_IS_APPLICATION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_APPLICATION))
#define GOO_IS_APPLICATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_APPLICATION))
#define GOO_APPLICATION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_APPLICATION, GooApplicationClass))

typedef struct _GooApplication            GooApplication;
typedef struct _GooApplicationClass       GooApplicationClass;

struct _GooApplication
{
	BonoboObject __parent;
};

struct _GooApplicationClass
{
	BonoboObjectClass __parent_class;
	POA_GNOME_Goobox_Application__epv epv;
};

GType          goo_application_get_type          (void);
BonoboObject * goo_application_new               (GdkScreen *screen);

#endif /* GOO_APPLICATION_H */
