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

#ifndef GOO_CDROM_LINUX_CD_H
#define GOO_CDROM_LINUX_CD_H

#include <glib.h>
#include "goo-cdrom.h"

#define GOO_TYPE_CDROM_LINUX              (goo_cdrom_linux_get_type ())
#define GOO_CDROM_LINUX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CDROM_LINUX, GooCdromLinux))
#define GOO_CDROM_LINUX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CDROM_LINUX, GooCdromLinuxClass))
#define GOO_IS_CDROM_LINUX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CDROM_LINUX))
#define GOO_IS_CDROM_LINUX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CDROM_LINUX))
#define GOO_CDROM_LINUX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_CDROM_LINUX, GooCdromLinuxClass))

typedef struct _GooCdromLinux            GooCdromLinux;
typedef struct _GooCdromLinuxClass       GooCdromLinuxClass;

struct _GooCdromLinux
{
	GooCdrom __parent;
};

struct _GooCdromLinuxClass
{
	GooCdromClass __parent_class;
};

GType      goo_cdrom_linux_get_type   (void);
GooCdrom  *goo_cdrom_linux_new        (const char *device);

#endif /* GOO_CDROM_LINUX_H */
