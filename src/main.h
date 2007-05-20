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

#ifndef MAIN_H
#define MAIN_H

#include <gtk/gtkwindow.h>

#include "cd-drive.h"

extern GtkWindow *main_window;
extern GList     *window_list;
extern int        AutoPlay;
extern int        HideShow;
extern GList     *Drives;

void        system_notify          (const char *title,
		    		    const char *msg,
		    		    int         x,
		    		    int         y);
CDDrive *   get_drive_from_device  (const char *device);
GtkWindow * get_window_from_device (const char *device);

#endif /* MAIN_H */
