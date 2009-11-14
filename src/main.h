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

#include <brasero/brasero-drive.h>
#include "goo-window.h"

extern GtkWidget *main_window;
extern GList     *window_list;
extern int        AutoPlay;
extern int        HideShow;

GtkWidget *     main_get_window_from_device (const char *device);
BraseroDrive *  main_get_most_likely_drive  (void);
BraseroDrive *  main_get_drive_for_device   (const char *device);
void            system_notify               (GooWindow  *window,
	       			             const char *title,
	       			             const char *msg);

#endif /* MAIN_H */
