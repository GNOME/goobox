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
#include <gnome.h>
#include <gdk/gdkkeysyms.h>
#include "glib-utils.h"
#include "goo-volume-button.h"
#include "goo-marshal.h"
#include "goo-stock.h"

#define SCALE_HEIGHT 145

struct _GooVolumeButtonPrivateData {
	GtkWidget   *popup_win;
	GtkWidget   *volume_scale;
	GtkWidget   *volume_label;
	GtkTooltips *tips;
	double       value;
	double       from_value;
	double       to_value;
	double       step;
};

enum {
	CHANGED,
        LAST_SIGNAL
};

static GtkToggleButtonClass *parent_class = NULL;
static guint goo_volume_button_signals[LAST_SIGNAL] = { 0 };


static void
update_volume_label (GooVolumeButton *button)
{
	double     value = button->priv->value;
	char      *text;
	char      *icon;
	GtkWidget *image;

	if ((value - 0.0) < 10e-3)
		icon = GOO_STOCK_VOLUME_ZERO;
	else if (value < 25.0)
		icon = GOO_STOCK_VOLUME_MIN;
	else if (value < 75.0)
		icon = GOO_STOCK_VOLUME_MED;
	else
		icon = GOO_STOCK_VOLUME_MAX;

	image = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_BUTTON);
	if (image != NULL) 
		g_object_set (button, 
			      "image", image, 
			      NULL);

	text = g_strdup_printf ("%3.0f%%", value);
	gtk_label_set_text (GTK_LABEL (button->priv->volume_label), text);
	g_free (text);

	text = g_strdup_printf (_("Volume level: %3.0f%%"), button->priv->value);
	gtk_tooltips_set_tip (button->priv->tips,
			      GTK_WIDGET (button),
			      text,
			      NULL);
	g_free (text);
}


static void 
goo_volume_button_finalize (GObject *object)
{
        GooVolumeButton *button;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_VOLUME_BUTTON (object));

	button = GOO_VOLUME_BUTTON (object);
	if (button->priv != NULL) {
		GooVolumeButtonPrivateData *priv = button->priv;

		gtk_object_unref (GTK_OBJECT (priv->tips));

		g_free (button->priv);
		button->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void 
goo_volume_button_class_init (GooVolumeButtonClass *klass)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

	goo_volume_button_signals[CHANGED] =
                g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooVolumeButtonClass, changed),
			      NULL, NULL,
			      goo_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

        gobject_class->finalize = goo_volume_button_finalize;
}


static void 
goo_volume_button_init (GooVolumeButton *button)
{
	button->priv = g_new0 (GooVolumeButtonPrivateData, 1);
	button->priv->value = 0.0;
}


GType
goo_volume_button_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GooVolumeButtonClass),
			NULL,
			NULL,
			(GClassInitFunc) goo_volume_button_class_init,
			NULL,
			NULL,
			sizeof (GooVolumeButton),
			0,
			(GInstanceInitFunc) goo_volume_button_init
		};

		type = g_type_register_static (GTK_TYPE_TOGGLE_BUTTON,
					       "GooVolumeButton",
					       &type_info,
					       0);
	}

        return type;
}


static void
button_toggled_cb (GtkToggleButton *toggle_button,
		   GooVolumeButton *button)
{
	GooVolumeButtonPrivateData *priv = button->priv;

	if (gtk_toggle_button_get_active (toggle_button)) {
		GtkWidget     *widget = GTK_WIDGET (button);
		GtkAllocation  allocation = widget->allocation;
		int            root_x, root_y;

		gdk_window_get_origin (widget->window, &root_x, &root_y);
		gtk_window_move (GTK_WINDOW (priv->popup_win),
				 root_x + allocation.x,
				 root_y + allocation.y + allocation.height);
		update_volume_label (button);

		gtk_widget_show_all (priv->popup_win);
		gtk_window_resize (GTK_WINDOW (priv->popup_win),
				   allocation.width,
				   SCALE_HEIGHT);

		gdk_pointer_grab (priv->popup_win->window, 
				  TRUE,
				  (GDK_POINTER_MOTION_MASK 
				   | GDK_BUTTON_PRESS_MASK 
				   | GDK_BUTTON_RELEASE_MASK),
				  NULL, 
				  NULL, 
				  GDK_CURRENT_TIME);
		gdk_keyboard_grab (priv->popup_win->window, TRUE, GDK_CURRENT_TIME);
		gtk_widget_grab_focus (priv->volume_scale);
		gtk_grab_add (priv->popup_win);
	} 
}


static void
ungrab (GooVolumeButton *button)
{
	gdk_pointer_ungrab (GDK_CURRENT_TIME);
	gdk_keyboard_ungrab (GDK_CURRENT_TIME);
	gtk_grab_remove (button->priv->popup_win);
	gtk_widget_hide (button->priv->popup_win);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}


static void
volume_scale_value_changed_cb (GtkRange        *range,
			       GooVolumeButton *button)
{
	button->priv->value = gtk_range_get_value (range);
	update_volume_label (button);
	g_signal_emit (G_OBJECT (button), 
		       goo_volume_button_signals[CHANGED],
		       0,
		       NULL);
}


static int
popup_win_event_cb (GtkWidget       *widget, 
		    GdkEvent        *event, 
		    GooVolumeButton *button)
{
	GooVolumeButtonPrivateData *priv = button->priv;
	GtkWidget *event_widget;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		event_widget = gtk_get_event_widget ((GdkEvent *)event);

		if (event_widget == (GtkWidget*)button) {
			ungrab (button);
			return TRUE;
		} else {
			int x, y, w, h;
			gdk_window_get_geometry (priv->popup_win->window, 
						 &x, &y, &w, &h, NULL);
			if ((event->button.x < 0) 
			    || (event->button.x > w)
			    || (event->button.y < 0) 
			    || (event->button.y > h)) {
				ungrab (button);
				return TRUE;
			}
		}
		break;

	case GDK_KEY_PRESS:
		switch (event->key.keyval) {
		case GDK_Escape:
			ungrab (button);
			return TRUE;
		default:
			break;
		}
		break;

	default:
		break;
	}

	return FALSE;
}


static gboolean
button_scroll_event_cb (GtkWidget       *widget,
			GdkEventScroll  *event,
			GooVolumeButton *button)
{
	GooVolumeButtonPrivateData *priv = button->priv;
	double direction = 1.0;

	if (event->direction == GDK_SCROLL_UP) 
		direction = 1.0;
	else if (event->direction == GDK_SCROLL_DOWN) 
		direction = -1.0;
	else
		return FALSE;

	goo_volume_button_set_volume (button, 
				      priv->value + (direction * priv->step),
				      TRUE);

	return TRUE;

}


static void 
goo_volume_button_construct (GooVolumeButton *button,
			     double           from_value,
			     double           to_value,
			     double           step)
{
	GooVolumeButtonPrivateData *priv;
	GtkWidget *out_frame;
	GtkWidget *volume_vbox;
	GtkWidget *label;

	priv = button->priv;

	priv->from_value = from_value;
	priv->to_value = to_value;
	priv->step = step;

	priv->tips = gtk_tooltips_new ();
	gtk_object_ref (GTK_OBJECT (priv->tips));
	gtk_object_sink (GTK_OBJECT (priv->tips));

	priv->popup_win = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_wmclass (GTK_WINDOW (priv->popup_win), "", "goo_volume_button");

	g_signal_connect (G_OBJECT (priv->popup_win),
			  "event",
			  G_CALLBACK (popup_win_event_cb), 
			  button);

	out_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (out_frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (priv->popup_win), out_frame);

	/**/

	volume_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (volume_vbox), 0);
	gtk_container_add (GTK_CONTAINER (out_frame), volume_vbox);

	label = gtk_label_new (_("+"));
	gtk_box_pack_start (GTK_BOX (volume_vbox), label, FALSE, FALSE, 0);

	priv->volume_scale = gtk_vscale_new_with_range (from_value, to_value, step);
	gtk_range_set_inverted (GTK_RANGE (priv->volume_scale), TRUE);
	gtk_scale_set_draw_value (GTK_SCALE (priv->volume_scale), FALSE);
	/*gtk_range_set_update_policy (GTK_RANGE (priv->volume_scale), GTK_UPDATE_DELAYED);*/
	gtk_range_set_increments (GTK_RANGE (priv->volume_scale), step, step);

	gtk_box_pack_start (GTK_BOX (volume_vbox), priv->volume_scale, TRUE, TRUE, 0);

	g_signal_connect (priv->volume_scale, 
			  "value_changed",
			  G_CALLBACK (volume_scale_value_changed_cb), 
			  button);

	label = gtk_label_new (_("-"));
	gtk_box_pack_start (GTK_BOX (volume_vbox), label, FALSE, FALSE, 0);

	/* */

	gtk_box_pack_start (GTK_BOX (volume_vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
	priv->volume_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (volume_vbox), priv->volume_label, FALSE, FALSE, 3);

	/**/

	g_signal_connect (G_OBJECT (button),
			  "toggled",
			  G_CALLBACK (button_toggled_cb), 
			  button);
	g_signal_connect (G_OBJECT (button),
			  "scroll_event",
			  G_CALLBACK (button_scroll_event_cb), 
			  button);
}


GtkWidget *
goo_volume_button_new (double from_value,
		       double to_value,
		       double step)
{
	GtkWidget *widget;

	widget = (GtkWidget*) g_object_new (GOO_TYPE_VOLUME_BUTTON, 
					    "label", GOO_STOCK_VOLUME_MAX,
					    "use_stock", TRUE,
					    "use_underline", TRUE,
					    "relief", GTK_RELIEF_NONE,
					    NULL);
	goo_volume_button_construct (GOO_VOLUME_BUTTON (widget), from_value, to_value, step);

	return widget;
}


double
goo_volume_button_get_volume (GooVolumeButton *button)
{
	return button->priv->value;
}


void
goo_volume_button_set_volume (GooVolumeButton *button,
			      double           vol,
			      gboolean         notify)
{
	vol = CLAMP (vol, 0.0, 100.0);
	button->priv->value = vol;

	g_signal_handlers_block_by_data (button->priv->volume_scale, button);
	gtk_range_set_value (GTK_RANGE (button->priv->volume_scale), vol);
	g_signal_handlers_unblock_by_data (button->priv->volume_scale, button);

	update_volume_label (button);
	if (notify) 
		g_signal_emit (G_OBJECT (button), 
			       goo_volume_button_signals[CHANGED],
			       0,
			       NULL);
}

