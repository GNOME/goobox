/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2013 Free Software Foundation, Inc.
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

#ifndef GOO_APPLICATION_ACTIONS_CALLBACKS_H
#define GOO_APPLICATION_ACTIONS_CALLBACKS_H

#include <gtk/gtk.h>
#include "glib-utils.h"

void update_actions_sensitivity (GApplication *application);

DEF_ACTION_CALLBACK (goo_application_activate_about)
DEF_ACTION_CALLBACK (goo_application_activate_help)
DEF_ACTION_CALLBACK (goo_application_activate_play_all)
DEF_ACTION_CALLBACK (goo_application_activate_preferences)
DEF_ACTION_CALLBACK (goo_application_activate_quit)
DEF_ACTION_CALLBACK (goo_application_activate_repeat)
DEF_ACTION_CALLBACK (goo_application_activate_shuffle)

#endif /* GOO_APPLICATION_ACTIONS_CALLBACKS_H */
