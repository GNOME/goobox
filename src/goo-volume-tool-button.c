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

/* This file contains code taken from:
 * 
 * GTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
 * 
 * and:
 * 
 * Volume Button / popup widget
 * (c) copyright 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * 
 */

#include <config.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "goo-volume-tool-button.h"
#include "goo-marshal.h"
#include "goo-stock.h"

#define SCALE_WIDTH 25
#define SCALE_HEIGHT 100
#define DEFAULT_VOLUME 1.0
#define CLICK_TIMEOUT 250
#define SCROLL_TIMEOUT 100

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
	
	guint        timeout : 1;
	guint32      pop_time;
	
	guint        down_button_scroll_id;
	gboolean     down_button_pressed;
	guint        up_button_scroll_id;
	gboolean     up_button_pressed;
};

enum {
	CHANGED,
        LAST_SIGNAL
};

static guint goo_volume_tool_button_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GooVolumeToolButton, goo_volume_tool_button, GTK_TYPE_TOOL_BUTTON)


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
	GtkWidget      *box;
	GtkOrientation  orientation;

	orientation = gtk_tool_item_get_orientation (GTK_TOOL_ITEM (button));

	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		box = gtk_hbox_new (FALSE, 0);
		gtk_arrow_set (GTK_ARROW (button->priv->arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	} 
	else {
		box = gtk_vbox_new (FALSE, 0);
		gtk_arrow_set (GTK_ARROW (button->priv->arrow), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	}

	if (button->priv->button && button->priv->button->parent) {
		g_object_ref (button->priv->button);
		gtk_container_remove (GTK_CONTAINER (button->priv->button->parent),
				      button->priv->button);
		gtk_container_add (GTK_CONTAINER (box), button->priv->button);
		g_object_unref (button->priv->button);
	}

	if (button->priv->arrow_button && button->priv->arrow_button->parent) {
		g_object_ref (button->priv->arrow_button);
		gtk_container_remove (GTK_CONTAINER (button->priv->arrow_button->parent),
				      button->priv->arrow_button);
		gtk_box_pack_end (GTK_BOX (box), button->priv->arrow_button,
				  FALSE, FALSE, 0);
		g_object_unref (button->priv->arrow_button);
	}

	if (button->priv->box) {
		/* Note: we are not destroying the button and the arrow_button
		 * here because they were removed from their container above
		 */
		gtk_widget_destroy (button->priv->box);
	}

	button->priv->box = box;

	gtk_container_add (GTK_CONTAINER (button), button->priv->box);
	gtk_widget_show_all (button->priv->box);

	gtk_widget_queue_resize (GTK_WIDGET (button));
}


static void
update_volume_label (GooVolumeToolButton *button)
{
	double  value = button->priv->value;
	char   *text;
	char   *icon;

	if ((value - 0.0) < 10e-3)
		icon = GOO_STOCK_VOLUME_ZERO;
	else if (value < 0.25)
		icon = GOO_STOCK_VOLUME_MIN;
	else if (value < 0.75)
		icon = GOO_STOCK_VOLUME_MED;
	else
		icon = GOO_STOCK_VOLUME_MAX;

	gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (button), icon);
	
	text = g_strdup_printf ("<small>%3.0f%%</small>", value * 100.0);
	gtk_label_set_markup (GTK_LABEL (button->priv->volume_label), text);
	g_free (text);

	text = g_strdup_printf (_("Volume level: %3.0f%%"), value * 100.0);
	gtk_tooltips_set_tip (button->priv->tips,
			      button->priv->button,
			      text,
			      NULL);
	g_free (text);
}


static void
goo_volume_tool_button_toolbar_reconfigured (GtkToolItem *toolitem)
{
	goo_volume_tool_button_construct_contents (GOO_VOLUME_TOOL_BUTTON (toolitem));
	
	/* chain up */
	GTK_TOOL_ITEM_CLASS (goo_volume_tool_button_parent_class)->toolbar_reconfigured (toolitem);
}


static void
goo_volume_tool_button_init (GooVolumeToolButton *button)
{
	button->priv = G_TYPE_INSTANCE_GET_PRIVATE ((button), GOO_TYPE_VOLUME_TOOL_BUTTON, GooVolumeToolButtonPrivate);
	button->priv->mute = FALSE;
	button->priv->mute_value = 0.0;
	button->priv->value = 0.0;
	button->priv->step = 0.05;
}


static void
goo_volume_tool_button_finalize (GObject *object)
{
	GooVolumeToolButton *button;

	button = GOO_VOLUME_TOOL_BUTTON (object);
	gtk_object_unref (GTK_OBJECT (button->priv->tips));
	G_OBJECT_CLASS (goo_volume_tool_button_parent_class)->finalize (object);
}


static void
goo_volume_tool_button_class_init (GooVolumeToolButtonClass *klass)
{
	GObjectClass       *object_class;
	GtkToolItemClass   *toolitem_class;
	GtkToolButtonClass *toolbutton_class;
	GtkWidgetClass     *gtkwidget_class;
	
	object_class = (GObjectClass *) klass;
	toolitem_class = (GtkToolItemClass *) klass;
	toolbutton_class = (GtkToolButtonClass *) klass;
	gtkwidget_class = (GtkWidgetClass *) klass;

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


static gboolean
button_clicked_cb (GtkWidget           *widget,
		   GooVolumeToolButton *button)
{
	if (! button->priv->mute) {
		button->priv->mute_value = button->priv->value;
		goo_volume_tool_button_set_volume (button,
						   0.0,
						   TRUE);
	} 
	else {
		if (FLOAT_EQUAL (button->priv->mute_value, 0.0))
			button->priv->mute_value = DEFAULT_VOLUME;
		goo_volume_tool_button_set_volume (button,
						   button->priv->mute_value,
						   TRUE);
	}

	return TRUE;
}


static gboolean
arrow_button_press_cb (GtkToggleButton     *toggle_button,
		       GdkEventButton      *event,
		       GooVolumeToolButton *button)
{
	GtkWidget      *widget;
	GtkAdjustment  *adj;
  	int             x, y, m, dx, dy, sx, sy, ystartoff, mouse_y;
  	float           v;
  	GdkEventButton *e;

	gtk_toggle_button_set_active (toggle_button, TRUE);
	
	widget = GTK_WIDGET (toggle_button);
	adj = gtk_range_get_adjustment (GTK_RANGE (button->priv->volume_scale));
	
	button->priv->timeout = TRUE;

    	gtk_window_set_screen (GTK_WINDOW (button->priv->popup_win), gtk_widget_get_screen (widget));
  
  	gdk_window_get_origin (widget->window, &x, &y);
  	x += widget->allocation.x;
  	y += widget->allocation.y;
  
	gtk_window_move (GTK_WINDOW (button->priv->popup_win),
			 x, y - (SCALE_HEIGHT / 2));
	update_volume_label (button);
	gtk_widget_show_all (button->priv->popup_win);

	gdk_window_get_origin (button->priv->popup_win->window, &dx, &dy);
  	dy += button->priv->popup_win->allocation.y;
  	gdk_window_get_origin (button->priv->volume_scale->window, &sx, &sy);
  	sy += button->priv->volume_scale->allocation.y;
  	ystartoff = sy - dy;
  	mouse_y = event->y;

	v = gtk_range_get_value (GTK_RANGE (button->priv->volume_scale)) / (adj->upper - adj->lower);
  	x += (widget->allocation.width - button->priv->popup_win->allocation.width) / 2;
  	y -= ystartoff;
  	y -= GTK_RANGE (button->priv->volume_scale)->min_slider_size / 2;
  	m = button->priv->volume_scale->allocation.height - GTK_RANGE (button->priv->volume_scale)->min_slider_size;
  	y -= m * (1.0 - v);
  	y += mouse_y;
  	gtk_window_move (GTK_WINDOW (button->priv->popup_win), x, y);
  	gdk_window_get_origin (button->priv->volume_scale->window, &sx, &sy);
	
	gdk_pointer_grab (button->priv->popup_win->window,
			  TRUE,
			  (GDK_POINTER_MOTION_MASK 
			   | GDK_BUTTON_PRESS_MASK 
			   | GDK_BUTTON_RELEASE_MASK),
			  NULL, 
			  NULL, 
			  GDK_CURRENT_TIME);
	gdk_keyboard_grab (button->priv->popup_win->window, TRUE, GDK_CURRENT_TIME);
	gtk_widget_grab_focus (button->priv->volume_scale);
	gtk_grab_add (button->priv->popup_win);

	/* forward event to the slider */
	e = (GdkEventButton *) gdk_event_copy ((GdkEvent *) event);
	e->window = button->priv->volume_scale->window;
	e->x = button->priv->volume_scale->allocation.width / 2;
	m = button->priv->volume_scale->allocation.height - GTK_RANGE (button->priv->volume_scale)->min_slider_size;
	e->y = ((1.0 - v) * m) + GTK_RANGE (button->priv->volume_scale)->min_slider_size / 2 + 1;
	gtk_widget_event (button->priv->volume_scale, (GdkEvent *) e);
	e->window = event->window;
	gdk_event_free ((GdkEvent *) e);		

	button->priv->pop_time = event->time;

	return TRUE;
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


static gboolean
scale_button_release_cb (GtkToggleButton     *toggle_button,
		         GdkEventButton      *event,
		         GooVolumeToolButton *button)
{
	if (button->priv->timeout) {
		/* if we did a quick click, leave the window open; else, hide it */
		if (event->time > button->priv->pop_time + CLICK_TIMEOUT) {
			ungrab (button);
			return FALSE;
		}
		button->priv->timeout = FALSE;
	}
	
	return FALSE;
}


static int
popup_win_event_cb (GtkWidget           *widget, 
		    GdkEvent            *event, 
		    GooVolumeToolButton *button)
{
	GtkWidget *event_widget;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		event_widget = gtk_get_event_widget ((GdkEvent *)event);

		if ((event_widget == button->priv->button)
		    || (event_widget == button->priv->arrow_button)) {
			ungrab (button);
			return TRUE;
		} 
		else {
			int x, y, w, h;
			
			gdk_window_get_geometry (button->priv->popup_win->window,
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
	double direction = 1.0;

	if (event->direction == GDK_SCROLL_UP) 
		direction = 1.0;
	else if (event->direction == GDK_SCROLL_DOWN) 
		direction = -1.0;
	else
		return FALSE;

	goo_volume_tool_button_set_volume (button, 
					   button->priv->value + (direction * button->priv->step),
					   TRUE);

	return TRUE;
}


/* down button */


static gboolean
down_button_scroll (gpointer data)
{
	GooVolumeToolButton *button = data;

	goo_volume_tool_button_set_volume (button, 
					   button->priv->value - button->priv->step,
					   TRUE);
	return TRUE;
}


static void
down_button_pressed_cb (GtkButton *gtk_button,
                        gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	button->priv->down_button_pressed = TRUE;
	if (button->priv->down_button_scroll_id != 0)
		return;
	button->priv->down_button_scroll_id = g_timeout_add (SCROLL_TIMEOUT, down_button_scroll, button); 
}


static void
down_button_released_cb (GtkButton *gtk_button,
                         gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	button->priv->down_button_pressed = FALSE;
	if (button->priv->down_button_scroll_id == 0)
		return;
	g_source_remove (button->priv->down_button_scroll_id);
	button->priv->down_button_scroll_id = 0;
}


static void
down_button_enter_cb (GtkButton *gtk_button,
                      gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	if (! button->priv->down_button_pressed)
		return;
	if (button->priv->down_button_scroll_id != 0)
		return;
	button->priv->down_button_scroll_id = g_timeout_add (SCROLL_TIMEOUT, down_button_scroll, button); 
}


static void
down_button_leave_cb (GtkButton *gtk_button,
                      gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	if (button->priv->down_button_scroll_id == 0) 
		return;
	g_source_remove (button->priv->down_button_scroll_id);
	button->priv->down_button_scroll_id = 0;
}


static void
down_button_clicked_cb (GtkButton *gtk_button,
                        gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	button->priv->down_button_pressed = FALSE;
	down_button_scroll (button);
	if (button->priv->down_button_scroll_id != 0) { 
		g_source_remove (button->priv->down_button_scroll_id);
		button->priv->down_button_scroll_id = 0;
	}
}


/* up button */


static gboolean
up_button_scroll (gpointer data)
{
	GooVolumeToolButton *button = data;
	
	goo_volume_tool_button_set_volume (button, 
					   button->priv->value + button->priv->step,
					   TRUE);
	return TRUE;
}


static void
up_button_pressed_cb (GtkButton *gtk_button,
                      gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	button->priv->up_button_pressed = TRUE;
	if (button->priv->up_button_scroll_id != 0)
		return;
	button->priv->up_button_scroll_id = g_timeout_add (SCROLL_TIMEOUT, up_button_scroll, button); 
}


static void
up_button_released_cb (GtkButton *gtk_button,
                       gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	button->priv->up_button_pressed = FALSE;
	if (button->priv->up_button_scroll_id == 0)
		return;
	g_source_remove (button->priv->up_button_scroll_id);
	button->priv->up_button_scroll_id = 0;
}


static void
up_button_enter_cb (GtkButton *gtk_button,
                    gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	if (! button->priv->up_button_pressed)
		return;
	if (button->priv->up_button_scroll_id != 0)
		return;
	button->priv->up_button_scroll_id = g_timeout_add (SCROLL_TIMEOUT, up_button_scroll, button); 
}


static void
up_button_leave_cb (GtkButton *gtk_button,
                    gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	if (button->priv->up_button_scroll_id == 0) 
		return;
	g_source_remove (button->priv->up_button_scroll_id);
	button->priv->up_button_scroll_id = 0;
}


static void
up_button_clicked_cb (GtkButton *gtk_button,
                      gpointer   user_data)
{
	GooVolumeToolButton *button = user_data;
	
	button->priv->up_button_pressed = FALSE;
	up_button_scroll (button);
	if (button->priv->up_button_scroll_id != 0) { 
		g_source_remove (button->priv->up_button_scroll_id);
		button->priv->up_button_scroll_id = 0;
	}
}


static void 
goo_volume_button_construct (GooVolumeToolButton *button)
{
	GtkWidget *box;
	GtkWidget *arrow;
	GtkWidget *arrow_button;
	GtkWidget *real_button;
	GtkWidget *out_frame;
	GtkWidget *volume_vbox;
	GtkWidget *up_button;
	GtkWidget *down_button;
	
	button->priv->tips = gtk_tooltips_new ();
	gtk_object_ref (GTK_OBJECT (button->priv->tips));
	gtk_object_sink (GTK_OBJECT (button->priv->tips));

	/* Create the popup window. */

	button->priv->popup_win = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_wmclass (GTK_WINDOW (button->priv->popup_win), "", "goo_volume_button");
	
	g_signal_connect (G_OBJECT (button->priv->popup_win),
			  "event",
			  G_CALLBACK (popup_win_event_cb), 
			  button);

	out_frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (out_frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (button->priv->popup_win), out_frame);

	/**/

	volume_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (volume_vbox), 0);
	gtk_container_add (GTK_CONTAINER (out_frame), volume_vbox);

	up_button = gtk_button_new_with_label (_("+"));
	gtk_button_set_relief (GTK_BUTTON (up_button), GTK_RELIEF_NONE);
	g_signal_connect (up_button, 
			  "pressed",
			  G_CALLBACK (up_button_pressed_cb), 
			  button);
	g_signal_connect (up_button, 
			  "released",
			  G_CALLBACK (up_button_released_cb), 
			  button);
	g_signal_connect (up_button, 
			  "enter",
			  G_CALLBACK (up_button_enter_cb), 
			  button);
	g_signal_connect (up_button, 
			  "leave",
			  G_CALLBACK (up_button_leave_cb), 
			  button);
	g_signal_connect (up_button, 
			  "clicked",
			  G_CALLBACK (up_button_clicked_cb), 
			  button);
	gtk_box_pack_start (GTK_BOX (volume_vbox), up_button, FALSE, FALSE, 0);

	button->priv->volume_scale = gtk_vscale_new_with_range (0.0, 1.0, 0.1);
	gtk_range_set_inverted (GTK_RANGE (button->priv->volume_scale), TRUE);
	gtk_scale_set_draw_value (GTK_SCALE (button->priv->volume_scale), FALSE);
	/*gtk_range_set_update_policy (GTK_RANGE (button->priv->volume_scale), GTK_UPDATE_DELAYED);*/
	gtk_range_set_increments (GTK_RANGE (button->priv->volume_scale), 0.1, 0.1);
	gtk_widget_set_size_request (button->priv->volume_scale, -1, SCALE_HEIGHT);
	 
	gtk_box_pack_start (GTK_BOX (volume_vbox), button->priv->volume_scale, TRUE, TRUE, 0);

	g_signal_connect (button->priv->volume_scale,
			  "value_changed",
			  G_CALLBACK (volume_scale_value_changed_cb), 
			  button);
	g_signal_connect (button->priv->volume_scale,
			  "button_release_event",
			  G_CALLBACK (scale_button_release_cb), 
			  button);
			  
	down_button = gtk_button_new_with_label (_("-"));
	gtk_button_set_relief (GTK_BUTTON (down_button), GTK_RELIEF_NONE);
	g_signal_connect (down_button, 
			  "pressed",
			  G_CALLBACK (down_button_pressed_cb), 
			  button);
	g_signal_connect (down_button, 
			  "released",
			  G_CALLBACK (down_button_released_cb), 
			  button);
	g_signal_connect (down_button, 
			  "enter",
			  G_CALLBACK (down_button_enter_cb), 
			  button);
	g_signal_connect (down_button, 
			  "leave",
			  G_CALLBACK (down_button_leave_cb), 
			  button);
	g_signal_connect (down_button, 
			  "clicked",
			  G_CALLBACK (down_button_clicked_cb), 
			  button);
	gtk_box_pack_start (GTK_BOX (volume_vbox), down_button, FALSE, FALSE, 0);

	/**/

	gtk_box_pack_start (GTK_BOX (volume_vbox), gtk_hseparator_new (), FALSE, FALSE, 0);
	button->priv->volume_label = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (volume_vbox), button->priv->volume_label, FALSE, FALSE, 3);

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
	gtk_tooltips_set_tip (button->priv->tips, arrow_button, _("Change the volume level"), NULL);

	gtk_widget_show_all (box);
	gtk_widget_hide (button->priv->volume_label);
	
	gtk_container_add (GTK_CONTAINER (button), box);

	button->priv->button = real_button;
	button->priv->arrow = arrow;
	button->priv->arrow_button = arrow_button;
	button->priv->box = box;

	g_signal_connect (real_button, 
			  "clicked",
			  G_CALLBACK (button_clicked_cb), 
			  button);
	g_signal_connect (arrow_button, 
			  "button_press_event",
			  G_CALLBACK (arrow_button_press_cb), 
			  button);

	g_signal_connect (G_OBJECT (button->priv->button),
			  "scroll_event",
			  G_CALLBACK (button_scroll_event_cb), 
			  button);
}


GtkToolItem *
goo_volume_tool_button_new (void)
{
	GooVolumeToolButton *button;

	button = g_object_new (GOO_TYPE_VOLUME_TOOL_BUTTON, NULL);

	gtk_tool_button_set_label (GTK_TOOL_BUTTON (button), _("Volume"));
	goo_volume_button_construct (GOO_VOLUME_TOOL_BUTTON (button));

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
	button->priv->value = CLAMP (vol, 0.0, 1.0);
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
