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

#ifndef GOO_CDROM_H
#define GOO_CDROM_H

#include <glib.h>
#include <glib-object.h>

#define GOO_TYPE_CDROM              (goo_cdrom_get_type ())
#define GOO_CDROM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOO_TYPE_CDROM, GooCdrom))
#define GOO_CDROM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOO_TYPE_CDROM, GooCdromClass))
#define GOO_IS_CDROM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOO_TYPE_CDROM))
#define GOO_IS_CDROM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOO_TYPE_CDROM))
#define GOO_CDROM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GOO_TYPE_CDROM, GooCdromClass))

typedef struct _GooCdrom            GooCdrom;
typedef struct _GooCdromClass       GooCdromClass;
typedef struct _GooCdromPrivateData GooCdromPrivateData;

typedef enum {
	GOO_CDROM_STATE_UNKNOWN,
	GOO_CDROM_STATE_ERROR,
	GOO_CDROM_STATE_DRIVE_NOT_READY,
	GOO_CDROM_STATE_TRAY_OPEN,
	GOO_CDROM_STATE_NO_DISC,
	GOO_CDROM_STATE_DATA_CD,
	GOO_CDROM_STATE_OK,
} GooCdromState;

struct _GooCdrom
{
	GObject __parent;
	GooCdromPrivateData *priv;
};

struct _GooCdromClass
{
	GObjectClass __parent_class;

	/*<virtual functions>*/

	gboolean      (*eject)             (GooCdrom       *cdrom);
	gboolean      (*close_tray)        (GooCdrom       *cdrom);
	gboolean      (*lock_tray)         (GooCdrom       *cdrom);
	gboolean      (*unlock_tray)       (GooCdrom       *cdrom);
	gboolean      (*update_state)      (GooCdrom       *cdrom);
	gboolean      (*is_cdrom_device)   (GooCdrom       *cdrom,
					    const char     *device);

	/*<signals>*/

	void          (*state_changed)     (GooCdrom        *cdrom);
};

#define GOO_CDROM_ERROR (goo_cdrom_error_quark())
GQuark           goo_cdrom_error_quark          (void);

GType            goo_cdrom_get_type             (void);
GooCdrom *       goo_cdrom_new                  (const char     *device);
void             goo_cdrom_construct            (GooCdrom       *cdrom,
						 const char     *device);
gboolean         goo_cdrom_eject                (GooCdrom        *cdrom);
gboolean         goo_cdrom_close_tray           (GooCdrom        *cdrom);
gboolean         goo_cdrom_lock_tray            (GooCdrom        *cdrom);
gboolean         goo_cdrom_unlock_tray          (GooCdrom        *cdrom);
gboolean         goo_cdrom_update_state         (GooCdrom        *cdrom);
gboolean         goo_cdrom_is_cdrom_device      (GooCdrom        *cdrom,
						 const char      *device);
gboolean         goo_cdrom_set_device           (GooCdrom        *cdrom,
						 const char      *device);
const char *     goo_cdrom_get_device           (GooCdrom        *cdrom);
GooCdromState    goo_cdrom_get_state            (GooCdrom        *cdrom);
GError *         goo_cdrom_get_error            (GooCdrom        *cdrom);

/* protected */

void             goo_cdrom_set_state            (GooCdrom        *cdrom,
						 GooCdromState    state);
void             goo_cdrom_set_error            (GooCdrom        *cdrom,
						 GError          *error);
void             goo_cdrom_set_error_from_string(GooCdrom        *cdrom,
						 const char      *value);
void             goo_cdrom_set_error_from_errno (GooCdrom        *cdrom);


#endif /* GOO_CDROM_H */
