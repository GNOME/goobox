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
#include <errno.h>

#include <gnome.h>
#include "goo-marshal.h"
#include "goo-cdrom.h"
#include "goo-cdrom-bsd.h"
#include "goo-cdrom-linux.h"
#include "goo-cdrom-solaris.h"


struct _GooCdromPrivateData {
	char          *device;
	char          *default_device;
	GError        *error;
	GooCdromState  state;
};

enum {
	STATE_CHANGED,
        LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_DEFAULT_DEVICE
};

static GObjectClass *parent_class = NULL;
static guint goo_cdrom_signals[LAST_SIGNAL] = { 0 };

static void goo_cdrom_class_init  (GooCdromClass *class);
static void goo_cdrom_init        (GooCdrom *cdrom);
static void goo_cdrom_finalize    (GObject *object);


GType
goo_cdrom_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GooCdromClass),
			NULL,
			NULL,
			(GClassInitFunc) goo_cdrom_class_init,
			NULL,
			NULL,
			sizeof (GooCdrom),
			0,
			(GInstanceInitFunc) goo_cdrom_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "GooCdrom",
					       &type_info,
					       0);
	}

        return type;
}


static gboolean
base_goo_cdrom_eject (GooCdrom *cdrom)
{
	return TRUE;
}


static gboolean
base_goo_cdrom_close_tray (GooCdrom *cdrom)
{
	return TRUE;
}


static gboolean
base_goo_cdrom_lock_tray (GooCdrom *cdrom)
{
	return TRUE;
}


static gboolean
base_goo_cdrom_unlock_tray (GooCdrom *cdrom)
{
	return TRUE;
}


static gboolean
base_goo_cdrom_is_cdrom_device (GooCdrom   *cdrom,
				const char *device)
{
	return TRUE;
}


static gboolean
base_goo_cdrom_update_state (GooCdrom *cdrom)
{
	return TRUE;
}


static void
goo_cdrom_set_property (GObject      *object, 
			guint         property_id,
			const GValue *value, 
			GParamSpec   *pspec)
{
	GooCdrom *cdrom = GOO_CDROM (object);
	GooCdromPrivateData *priv = cdrom->priv;

	switch (property_id) {
	case PROP_DEFAULT_DEVICE:
		g_free (priv->default_device);
		priv->default_device = g_strdup (g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
goo_cdrom_get_property (GObject    *object, 
			guint       property_id,
			GValue     *value, 
			GParamSpec *pspec)
{
	GooCdrom *cdrom = GOO_CDROM (object);
	GooCdromPrivateData *priv = cdrom->priv;

	switch (property_id) { 
	case PROP_DEFAULT_DEVICE:
		g_value_set_string (value, priv->default_device);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void 
goo_cdrom_class_init (GooCdromClass *class)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (class);

        parent_class = g_type_class_peek_parent (class);

	goo_cdrom_signals[STATE_CHANGED] =
                g_signal_new ("state_changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooCdromClass, state_changed),
			      NULL, NULL,
			      goo_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

        gobject_class->finalize = goo_cdrom_finalize;
	gobject_class->set_property = goo_cdrom_set_property;
	gobject_class->get_property = goo_cdrom_get_property;

	g_object_class_install_property (gobject_class, PROP_DEFAULT_DEVICE,
			g_param_spec_string ("default_device", 
					     NULL, NULL,
					     NULL, G_PARAM_READWRITE));

	class->eject            = base_goo_cdrom_eject;
	class->close_tray       = base_goo_cdrom_close_tray;
	class->lock_tray        = base_goo_cdrom_lock_tray;
	class->unlock_tray      = base_goo_cdrom_unlock_tray;
	class->update_state     = base_goo_cdrom_update_state;
	class->is_cdrom_device  = base_goo_cdrom_is_cdrom_device;

	class->state_changed = NULL;
}


static void 
goo_cdrom_init (GooCdrom *cdrom)
{
	GooCdromPrivateData *priv;

	cdrom->priv = g_new0 (GooCdromPrivateData, 1);
	priv = cdrom->priv;

	priv->error = NULL;
	priv->device = NULL;
	priv->default_device = NULL;
	priv->state = GOO_CDROM_STATE_UNKNOWN;
}


static void 
goo_cdrom_finalize (GObject *object)
{
        GooCdrom *cdrom;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_CDROM (object));

	cdrom = GOO_CDROM (object);
	if (cdrom->priv != NULL) {
		GooCdromPrivateData *priv = cdrom->priv;

		if (priv->error != NULL) 
			g_error_free (priv->error);
		g_free (priv->device);
		g_free (priv->default_device);

		g_free (cdrom->priv);
		cdrom->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


GQuark 
goo_cdrom_error_quark (void)
{
	static GQuark quark;
        if (!quark)
                quark = g_quark_from_static_string ("goo_cdrom_error");
        return quark;
}


GooCdrom *
goo_cdrom_new (const char *device)
{
#if defined(HAVE_BSD)
	return goo_cdrom_bsd_new (device);
#elif defined(HAVE_LINUX)
	return goo_cdrom_linux_new (device);
#elif defined(HAVE_SOLARIS)
	return goo_cdrom_solaris_new (device);
#endif
}


void
goo_cdrom_construct (GooCdrom   *cdrom,
		     const char *device)
{
	if (device != NULL)
		cdrom->priv->device = g_strdup (device);
}


gboolean
goo_cdrom_eject (GooCdrom *cdrom)
{
	return GOO_CDROM_GET_CLASS (G_OBJECT (cdrom))->eject (cdrom);
}


gboolean
goo_cdrom_close_tray (GooCdrom *cdrom)
{
	return GOO_CDROM_GET_CLASS (G_OBJECT (cdrom))->close_tray (cdrom);
}


gboolean
goo_cdrom_lock_tray (GooCdrom *cdrom)
{
	return GOO_CDROM_GET_CLASS (G_OBJECT (cdrom))->lock_tray (cdrom);
}


gboolean
goo_cdrom_unlock_tray (GooCdrom *cdrom)
{
	return GOO_CDROM_GET_CLASS (G_OBJECT (cdrom))->unlock_tray (cdrom);
}


gboolean
goo_cdrom_is_cdrom_device (GooCdrom   *cdrom,
			   const char *device)
{
	return GOO_CDROM_GET_CLASS (G_OBJECT (cdrom))->is_cdrom_device (cdrom, device);
}


gboolean
goo_cdrom_set_device (GooCdrom   *cdrom,
		      const char *device)
{
	GooCdromPrivateData *priv = cdrom->priv;

	if (! goo_cdrom_is_cdrom_device (cdrom, device)) {
		goo_cdrom_set_error_from_string (cdrom, _("The specified device is not valid"));
		goo_cdrom_set_state (cdrom, GOO_CDROM_STATE_ERROR);
		return FALSE;
	}

	g_free (priv->device);
	priv->device = NULL;

	if (device != NULL)
		priv->device = g_strdup (device);

	cdrom->priv->state = GOO_CDROM_STATE_UNKNOWN;

	return TRUE;
}


const char *
goo_cdrom_get_device (GooCdrom *cdrom)
{
	GooCdromPrivateData *priv = cdrom->priv;
	
	if (priv->device == NULL)
		return priv->default_device;
	else
		return priv->device;
}


GooCdromState
goo_cdrom_get_state (GooCdrom *cdrom)
{
	return cdrom->priv->state; 
}


gboolean
goo_cdrom_update_state (GooCdrom *cdrom)
{
	return GOO_CDROM_GET_CLASS (G_OBJECT (cdrom))->update_state (cdrom);
}


GError *
goo_cdrom_get_error (GooCdrom *cdrom)
{
	if (cdrom->priv->error != NULL)
		return g_error_copy (cdrom->priv->error);
	else
		return NULL;
}


void
goo_cdrom_set_state (GooCdrom      *cdrom,
		     GooCdromState  state)
{
	if (state == cdrom->priv->state) 
		return;
	cdrom->priv->state = state;
	g_signal_emit (cdrom, goo_cdrom_signals[STATE_CHANGED], 0);
}


void
goo_cdrom_set_error (GooCdrom *cdrom,
		     GError   *error)
{
	if (cdrom->priv->error != NULL) {
		g_error_free (cdrom->priv->error);
		cdrom->priv->error = NULL;
	}
	cdrom->priv->error = error;
}


void
goo_cdrom_set_error_from_string (GooCdrom   *cdrom,
				 const char *value)
{
	GError *error;
	error = g_error_new (GOO_CDROM_ERROR, 0, "%s", value);
	goo_cdrom_set_error (cdrom, error);
}


void
goo_cdrom_set_error_from_errno (GooCdrom *cdrom)
{
	GError *error;
	error = g_error_new (GOO_CDROM_ERROR, 0, "%s", g_strerror (errno));
	goo_cdrom_set_error (cdrom, error);
}
