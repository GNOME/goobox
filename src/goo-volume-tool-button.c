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

#include <config.h>
#include <math.h>
#include <gnome.h>
#include <gdk/gdkkeysyms.h>
#include "glib-utils.h"
#include "goo-volume-tool-button.h"
#include "goo-marshal.h"
#include "goo-stock.h"

#define GOO_VOLUME_TOOL_BUTTON_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GOO_TYPE_VOLUME_TOOL_BUTTON, GooVolumeToolButtonPrivate))
#define FLOAT_EQUAL(a,b) (fabs (a - b) < 1e-6)

#define SCALE_WIDTH 50
#define SCALE_HEIGHT 145
#define DEFAULT_VOLUME 50.0


struct _GooVolumeToolButtonPrivate {
	GtkWidget   *button;
	GtkWidget   *arrow;
	GtkWidget   *arrow_button;
	GtkWidget   *box;

	GtkWidget   *popup_win;
	GtkWidget   *volume_scale;
	GtkWidget   *volume_label;
	GtkTooltips *tips;
	double       value;
	double       from_value;
	double       to_value;
	double       step;
	gboolean     mute;
	double       mute_value;
};

static void goo_volume_tool_button_init       (GooVolumeToolButton      *button);
static void goo_volume_tool_button_class_init (GooVolumeToolButtonClass *klass);
static void goo_volume_tool_button_finalize   (GObject                  *object);

enum {
	CHANGED,
        LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint goo_volume_tool_button_signals[LAST_SIGNAL] = { 0 };

GType
goo_volume_tool_button_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (GooVolumeToolButtonClass),
			(GBaseInitFunc) 0,
			(GBaseFinalizeFunc) 0,
			(GClassInitFunc) goo_volume_tool_button_class_init,
			(GClassFinalizeFunc) 0,
			NULL,
			sizeof (GooVolumeToolButton),
			0, /* n_preallocs */
			(GInstanceInitFunc) goo_volume_tool_button_init
		};
		
		type = g_type_register_static (GTK_TYPE_TOOL_BUTTON,
					       "GooVolumeToolButton",
					       &info, 0);
	}
	
	return type;
}


static gboolean
goo_volume_tool_button_set_tooltip (GtkToolItem *tool_item,
				    GtkTooltips *tooltips,
				    const char  *tip_text,
				    const char  *tip_private)
{
	GooVolumeToolButton *button;

	g_return_val_if_fail (GOO_IS_VOLUME_TOOL_BUTTON (tool_item), FALSE);

	button = GOO_VOLUME_TOOL_BUTTON (tool_item);
	/* gtk_tooltips_set_tip (tooltips, button->priv->button, tip_text, tip_private);*/
	
	return TRUE;
}


static void
goo_volume_tool_button_construct_contents (GooVolumeToolButton *button)
{
	GooVolumeToolButtonPrivate *priv;
	GtkWidget                  *box;
	GtkOrientation              orientation;

	priv = GOO_VOLUME_TOOL_BUTTON_GET_PRIVATE (button);

	orientation = gtk_tool_item_get_orientation (GTK_TOOL_ITEM (button));

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		box = gtk_hbox_new (FALSE, 0);
		gtk_arrow_set (GTK_ARROW (priv->arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	} else {
		box = gtk_vbox_new (FALSE, 0);
		gtk_arrow_set (GTK_ARROW (priv->arrow), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	}

	if (priv->button && priv->button->parent) {
		g_object_ref (priv->button);
		gtk_container_remove (GTK_CONTAINER (priv->button->parent),
				      priv->button);
		gtk_container_add (GTK_CONTAINER (box), priv->button);
		g_object_unref (priv->button);
	}

	if (priv->arrow_button && priv->arrow_button->parent) {
		g_object_ref (priv->arrow_button);
		gtk_container_remove (GTK_CONTAINER (priv->arrow_button->parent),
				      priv->arrow_button);
		gtk_box_pack_end (GTK_BOX (box), priv->arrow_button,
				  FALSE, FALSE, 0);
		g_object_unref (priv->arrow_button);
	}

	if (priv->box) {
		/* Note: we are not destroying the button and the arrow_button
		 * here because they were removed from their container above
		 */
		gtk_widget_destroy (priv->box);
	}

	priv->box = box;

	gtk_container_add (GTK_CONTAINER (button), priv->box);
	gtk_widget_show_all (priv->box);

	gtk_widget_queue_resize (GTK_WIDGET (button));
}


static void
goo_volume_tool_button_toolbar_reconfigured (GtkToolItem *toolitem)
{
	goo_volume_tool_button_construct_contents (GOO_VOLUME_TOOL_BUTTON (toolitem));

	/* chain up */
	GTK_TOOL_ITEM_CLASS (parent_class)->toolbar_reconfigured (toolitem);
}


static void
goo_volume_tool_button_class_init (GooVolumeToolButtonClass *klass)
{
	GObjectClass *object_class;
	GtkToolItemClass *toolitem_class;
	GtkToolButtonClass *toolbutton_class;
	
	parent_class = g_type_class_peek_parent (klass);
  
	object_class = (GObjectClass *)klass;
	toolitem_class = (GtkToolItemClass *)klass;
	toolbutton_class = (GtkToolButtonClass *)klass;

	goo_volume_tool_button_signals[CHANGED] =
                g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooVolumeToolButtonClass, changed),
			      NULL, NULL,
			      goo_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

	object_class->finalize = goo_volume_tool_button_finalize;
	toolitem_class->set_tooltip = goo_volume_tool_button_set_tooltip;
	toolitem_class->toolbar_reconfigured = goo_volume_tool_button_toolbar_reconfigured;

	g_type_class_add_private (object_class, sizeof (GooVolumeToolButtonPrivate));
}


static void
button_state_changed_cb (GtkWidget           *widget,
                         GtkStateType         previous_state,
                         GooVolumeToolButton *button)
{
	GooVolumeToolButtonPrivate *priv;
	GtkWidget *other;
	GtkStateType state = GTK_WIDGET_STATE (widget);
	
	priv = GOO_VOLUME_TOOL_BUTTON_GET_PRIVATE (button);

	other = (widget == priv->arrow_button) ? priv->button : priv->arrow_button;

	g_signal_handlers_block_by_func (other,
					 G_CALLBACK (button_state_changed_cb),
					 button);
	
	if (state == GTK_STATE_PRELIGHT) {
		gtk_widget_set_state (other, state);
	} else if (state == GTK_STATE_NORMAL) {
		gtk_widget_set_state (other, state);
	} else if (state == GTK_STATE_ACTIVE) {
		gtk_widget_set_state (other, GTK_STATE_NORMAL);
	}

	g_signal_handlers_unblock_by_func (other,
					   G_CALLBACK (button_state_changed_cb),
					   button);
}


static void
update_volume_label (GooVolumeToolButton *button)
{
	double     value = button->priv->value;
	char      *text;
	char      *stock_id;

	if ((value - 0.0) < 10e-3)
		stock_id = GOO_STOCK_VOLUME_ZERO;
	else if (value < 25.0)
		stock_id = GOO_STOCK_VOLUME_MIN;
	else if (value < 75.0)
		stock_id = GOO_STOCK_VOLUME_MED;
	else
		stock_id = GOO_STOCK_VOLUME_MAX;

	gtk_tool_button_set_stock_id  (GTK_TOOL_BUTTON (button), stock_id);

	text = g_strdup_printf ("%3.0f%%", value);
	gtk_label_set_text (GTK_LABEL (button->priv->volume_label), text);
	g_free (text);

	text = g_strdup_printf (_("Volume level: %3.0f%%"), 
				button->priv->value);
	gtk_tooltips_set_tip (button->priv->tips,
			      GTK_WIDGET (button->priv->button),
			      text,
			      NULL);
	g_free (text);
}


static gboolean
button_clicked_cb (GtkWidget           *widget,
		   GooVolumeToolButton *button)
{
	if (! button->priv->mute) {
		button->priv->mute_value = button->priv->value;
		goo_volume_tool_button_set_volume (button,
						   0.0,
						   TRUE);
	} else {
		if (FLOAT_EQUAL (button->priv->mute_value, 0.0))
			button->priv->mute_value = DEFAULT_VOLUME;
		goo_volume_tool_button_set_volume (button,
						   button->priv->mute_value,
						   TRUE);
	}

	return TRUE;
}


static void
arrow_button_toggled_cb (GtkToggleButton     *toggle_button,
			 GooVolumeToolButton *button)
{
	GooVolumeToolButtonPrivate *priv;

	priv = GOO_VOLUME_TOOL_BUTTON_GET_PRIVATE (button);

	if (gtk_toggle_button_get_active (toggle_button)) {
		GtkWidget     *widget = GTK_WIDGET (toggle_button);
		GtkAllocation  allocation = widget->allocation;
		int            root_x, root_y;

		gdk_window_get_origin (widget->window, &root_x, &root_y);
		gtk_window_move (GTK_WINDOW (priv->popup_win),
				 root_x + allocation.x,
				 root_y + allocation.y + allocation.height);
		update_volume_label (button);

		gtk_widget_show_all (priv->popup_win);
		gtk_window_resize (GTK_WINDOW (priv->popup_win),
				   SCALE_WIDTH,
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
ungrab (GooVolumeToolButton *button)
{
	gdk_pointer_ungrab (GDK_CURRENT_TIME);
	gdk_keyboard_ungrab (GDK_CURRENT_TIME);
	gtk_grab_remove (button->priv->popup_win);
	gtk_widget_hide (button->priv->popup_win);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button->priv->arrow_button), FALSE);
}


static void
volume_scale_value_changed_cb (GtkRange            *range,
			       GooVolumeToolButton *button)
{
	button->priv->value = gtk_range_get_value (range);
	button->priv->mute = FLOAT_EQUAL (button->priv->value, 0.0);
	update_volume_label (button);

	g_signal_emit (G_OBJECT (button), 
		       goo_volume_tool_button_signals[CHANGED],
		       0,
		       NULL);
}


static int
popup_win_event_cb (GtkWidget           *widget, 
		    GdkEvent            *event, 
		    GooVolumeToolButton *button)
{
	GooVolumeToolButtonPrivate *priv;
	GtkWidget                  *event_widget;

	priv = GOO_VOLUME_TOOL_BUTTON_GET_PRIVATE (button);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		event_widget = gtk_get_event_widget ((GdkEvent *)event);

		if ((event_widget == button->priv->button)
		    || (event_widget == button->priv->arrow_button)) {
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
button_scroll_event_cb (GtkWidget           *widget,
			GdkEventScroll      *event,
			GooVolumeToolButton *button)
{
	GooVolumeToolButtonPrivate *priv = GOO_VOLUME_TOOL_BUTTON_GET_PRIVATE (button);
	double direction = 1.0;

	if (event->direction == GDK_SCROLL_UP) 
		direction = 1.0;
	else if (event->direction == GDK_SCROLL_DOWN) 
		direction = -1.0;
	else
		return FALSE;

	goo_volume_tool_button_set_volume (button, 
					   priv->value + (direction * priv->step),
					   TRUE);

	return TRUE;

}


static void 
goo_volume_button_construct (GooVolumeToolButton *button,
			     double               from_value,
			     double               to_value,
			     double               step)
{
	struct _GooVolumeToolButtonPrivate *priv = button->priv;
	GtkWidget *box;
	GtkWidget *arrow;
	GtkWidget *arrow_button;
	GtkWidget *real_button;
	GtkWidget *out_frame;
	GtkWidget *volume_vbox;
	GtkWidget *label;

	priv->tips = gtk_tooltips_new ();
	gtk_object_ref (GTK_OBJECT (priv->tips));
	gtk_object_sink (GTK_OBJECT (priv->tips));

	/* Create the popup window. */

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

	/**/

	gtk_box_pack_start (GTK_BOX (volume_vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
	priv->volume_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (volume_vbox), priv->volume_label, FALSE, FALSE, 3);

	/* Construct the toolbar button.  */

	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (button), FALSE);
	gtk_tool_button_set_stock_id  (GTK_TOOL_BUTTON (button), GOO_STOCK_VOLUME_MAX);

	box = gtk_hbox_new (FALSE, 0);
	
	real_button = GTK_BIN (button)->child;
	g_object_ref (real_button);
	gtk_container_remove (GTK_CONTAINER (button), real_button);
	gtk_container_add (GTK_CONTAINER (box), real_button);
	g_object_unref (real_button);

	arrow_button = gtk_toggle_button_new ();
	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_button_set_relief (GTK_BUTTON (arrow_button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (arrow_button), arrow);
	gtk_box_pack_end (GTK_BOX (box), arrow_button,
			  FALSE, FALSE, 0);
	gtk_tooltips_set_tip (priv->tips, arrow_button, _("Change the volume level"), NULL);

	gtk_widget_show_all (box);

	gtk_container_add (GTK_CONTAINER (button), box);

	button->priv->button = real_button;
	button->priv->arrow = arrow;
	button->priv->arrow_button = arrow_button;
	button->priv->box = box;

	g_signal_connect (real_button, 
			  "state_changed",
			  G_CALLBACK (button_state_changed_cb), 
			  button);
	g_signal_connect (arrow_button, 
			  "state_changed",
			  G_CALLBACK (button_state_changed_cb), 
			  button);

	g_signal_connect (real_button, 
			  "clicked",
			  G_CALLBACK (button_clicked_cb), 
			  button);
	g_signal_connect (arrow_button, 
			  "toggled",
			  G_CALLBACK (arrow_button_toggled_cb), 
			  button);

	g_signal_connect (G_OBJECT (button->priv->button),
			  "scroll_event",
			  G_CALLBACK (button_scroll_event_cb), 
			  button);
}


static void
goo_volume_tool_button_init (GooVolumeToolButton *button)
{
	button->priv = GOO_VOLUME_TOOL_BUTTON_GET_PRIVATE (button);
	button->priv->mute = FALSE;
	button->priv->mute_value = 0.0;
}


static void
goo_volume_tool_button_finalize (GObject *object)
{
	GooVolumeToolButton *button;

	button = GOO_VOLUME_TOOL_BUTTON (object);

	gtk_object_unref (GTK_OBJECT (button->priv->tips));

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


GtkToolItem *
goo_volume_tool_button_new (double from_value,
			    double to_value,
			    double step)
{
	GooVolumeToolButton *button;

	button = g_object_new (GOO_TYPE_VOLUME_TOOL_BUTTON, NULL);

	gtk_tool_button_set_label (GTK_TOOL_BUTTON (button), _("Volume"));
	goo_volume_button_construct (GOO_VOLUME_TOOL_BUTTON (button), from_value, to_value, step);

	return GTK_TOOL_ITEM (button);
}


double
goo_volume_tool_button_get_volume (GooVolumeToolButton *button)
{
	return button->priv->value;
}


void
goo_volume_tool_button_set_volume (GooVolumeToolButton *button,
				   double               vol,
				   gboolean             notify)
{
	vol = CLAMP (vol, 0.0, 100.0);
	button->priv->value = vol;
	button->priv->mute = FLOAT_EQUAL (button->priv->value, 0.0);

	g_signal_handlers_block_by_data (button->priv->volume_scale, button);
	gtk_range_set_value (GTK_RANGE (button->priv->volume_scale), vol);
	g_signal_handlers_unblock_by_data (button->priv->volume_scale, button);

	update_volume_label (button);
	if (notify) 
		g_signal_emit (G_OBJECT (button), 
			       goo_volume_tool_button_signals[CHANGED],
			       0,
			       NULL);
}
