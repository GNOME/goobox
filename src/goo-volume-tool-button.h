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
 *
 *  Author: Paolo Bacchilega
 *
 */

/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GOO_VOLUME_TOOL_BUTTON_H__
#define __GOO_VOLUME_TOOL_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GOO_TYPE_VOLUME_TOOL_BUTTON         (goo_volume_tool_button_get_type ())
#define GOO_VOLUME_TOOL_BUTTON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GOO_TYPE_VOLUME_TOOL_BUTTON, GooVolumeToolButton))
#define GOO_VOLUME_TOOL_BUTTON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GOO_TYPE_VOLUME_TOOL_BUTTON, GooVolumeToolButtonClass))
#define GOO_IS_VOLUME_TOOL_BUTTON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOO_TYPE_VOLUME_TOOL_BUTTON))
#define GOO_IS_VOLUME_TOOL_BUTTON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GOO_TYPE_VOLUME_TOOL_BUTTON))
#define GOO_VOLUME_TOOL_BUTTON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GOO_TYPE_VOLUME_TOOL_BUTTON, GooVolumeToolButtonClass))

typedef struct _GooVolumeToolButtonClass   GooVolumeToolButtonClass;
typedef struct _GooVolumeToolButton        GooVolumeToolButton;
typedef struct _GooVolumeToolButtonPrivate GooVolumeToolButtonPrivate;

struct _GooVolumeToolButton {
	GtkToolButton parent;
	GooVolumeToolButtonPrivate *priv;
};

struct _GooVolumeToolButtonClass {
	GtkToolButtonClass parent_class;

	/*<signals>*/

	void (*changed) (GooVolumeToolButton *button);
};

GType         goo_volume_tool_button_get_type     (void) G_GNUC_CONST;
GtkToolItem  *goo_volume_tool_button_new          (void);
double        goo_volume_tool_button_get_volume   (GooVolumeToolButton *button);
void          goo_volume_tool_button_set_volume   (GooVolumeToolButton *button,
						   double           value,
						   gboolean         notify);

G_END_DECLS

#endif /* __GOO_VOLUME_TOOL_BUTTON_H__ */
