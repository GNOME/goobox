/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2012 Free Software Foundation, Inc.
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
#include <glib/gi18n.h>
#include "goo-player-bar.h"
#include "goo-marshal.h"
#include "goo-stock.h"
#include "glib-utils.h"
#include "gth-toggle-menu-action.h"


#define SCALE_WIDTH 150
#define TIME_LABEL_WIDTH_IN_CHARS 8
#define PLAY_BUTTON_SIZE GTK_ICON_SIZE_SMALL_TOOLBAR
#define MIN_WIDTH 500
#define UPDATE_TIMEOUT 50


G_DEFINE_TYPE (GooPlayerBar, goo_player_bar, GTK_TYPE_BOX)


struct _GooPlayerBarPrivateData {
	GooPlayer *player;
	GtkWidget *current_time_label;
	GtkWidget *remaining_time_label;
	GtkWidget *time_scale;
	GtkWidget *time_box;
	gint64     track_length;
	gint64     current_time;
	gboolean   dragging;
	guint      update_id;
	double     fraction;
	guint      update_progress_timeout;
};


enum {
	SKIP_TO,
        LAST_SIGNAL
};
static guint goo_player_bar_signals[LAST_SIGNAL] = { 0 };


static void
goo_player_bar_get_preferred_width (GtkWidget *widget,
				    int       *minimum_width,
				    int       *natural_width)
{
	*minimum_width = *natural_width = MIN_WIDTH;
}


static void
set_label (GtkWidget  *label,
	   const char *format,
	   const char *text)
{
	char *e_text;
	char *markup;

	if ((text == NULL) || (*text == '\0')) {
		gtk_label_set_text (GTK_LABEL (label), "");
		gtk_widget_hide (label);
		return;
	}

	e_text = g_markup_escape_text (text, -1);
	markup = g_strdup_printf (format, e_text);
	g_free (e_text);

	gtk_label_set_markup (GTK_LABEL (label), markup);
	gtk_widget_show (label);

	g_free (markup);
}


static void
_goo_player_bar_update_current_time (GooPlayerBar *self)
{
	char *s;

	s = _g_format_duration_for_display (self->priv->current_time * 1000);
	set_label (self->priv->current_time_label, "%s", s);
	g_free (s);

	s = _g_format_duration_for_display ((self->priv->track_length - self->priv->current_time) * 1000);
	if (self->priv->track_length - self->priv->current_time > 0)
		set_label (self->priv->remaining_time_label, "-%s", s);
	else
		set_label (self->priv->remaining_time_label, "%s", s);
	g_free (s);
}


static void
time_scale_value_changed_cb (GtkRange     *range,
			     GooPlayerBar *self)
{
	self->priv->current_time = self->priv->track_length * gtk_range_get_value (range);
	_goo_player_bar_update_current_time (self);

	if (! self->priv->dragging) {
		int seconds;

		seconds = (int) (gtk_range_get_value (range) * self->priv->track_length);
		g_signal_emit (self, goo_player_bar_signals[SKIP_TO], 0, seconds);
	}
}


static gboolean
update_time_label_cb (gpointer data)
{
	GooPlayerBar *self = data;

	if (self->priv->update_id != 0) {
		g_source_remove (self->priv->update_id);
		self->priv->update_id = 0;
	}

	self->priv->current_time = self->priv->track_length * gtk_range_get_value (GTK_RANGE (self->priv->time_scale));
	_goo_player_bar_update_current_time (self);

	self->priv->update_id = g_timeout_add (UPDATE_TIMEOUT,
					       update_time_label_cb,
					       data);

	return FALSE;
}


static gboolean
time_scale_button_press_cb (GtkRange         *range,
			    GdkEventButton   *event,
			    GooPlayerBar    *self)
{
	self->priv->dragging = TRUE;
	if (self->priv->update_id == 0)
		self->priv->update_id = g_timeout_add (UPDATE_TIMEOUT,
						       update_time_label_cb,
						       self);
	return FALSE;
}


static gboolean
time_scale_button_release_cb (GtkRange         *range,
			      GdkEventButton   *event,
			      GooPlayerBar    *self)
{
	if (self->priv->update_id != 0) {
		g_source_remove (self->priv->update_id);
		self->priv->update_id = 0;
	}

	self->priv->dragging = FALSE;
	g_signal_emit_by_name (range, "value-changed");

	return FALSE;
}


static void
goo_player_bar_init (GooPlayerBar *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GOO_TYPE_PLAYER_BAR, GooPlayerBarPrivateData);
	self->priv->dragging = FALSE;
	self->priv->track_length = 0;
	self->priv->current_time = 0;
	self->priv->update_id = 0;
	self->priv->update_progress_timeout = 0;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_can_focus (GTK_WIDGET (self), FALSE);
}


static GtkWidget *
_gtk_button_new_from_icon_name (const char *icon_name,
				GtkIconSize size)
{
	GtkWidget *button;
	GtkWidget *image;

	button = gtk_button_new ();
	image = gtk_image_new_from_icon_name (icon_name, size);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (button), image);

	return button;
}


static GtkWidget *
_gtk_menu_button_new_from_icon_name (const char *icon_name)
{
	GtkWidget *button;
	GtkWidget *image;

	button = gtk_menu_button_new ();
	image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (button), image);

	return button;
}


static void
toggle_play_notify_icon_name_cb (GObject    *gobject,
				 GParamSpec *pspec,
				 gpointer    user_data)
{
	GtkWidget *button = user_data;
	GtkWidget *image;

	image = gtk_bin_get_child (GTK_BIN (button));
	if ((image != NULL) && GTK_IS_IMAGE (image))
		gtk_image_set_from_icon_name (GTK_IMAGE (image), gtk_action_get_icon_name (GTK_ACTION (gobject)), PLAY_BUTTON_SIZE);
}


static void
_gtk_button_sync_with_action (GtkWidget *button,
			      GtkAction *action)
{
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (button), action);
	gtk_widget_set_tooltip_text (button, gtk_action_get_tooltip (action));
	if (GTK_IS_MENU_BUTTON (button))
		g_object_set (button,
			      "menu", gth_toggle_menu_action_get_menu (GTH_TOGGLE_MENU_ACTION (action)),
			      NULL);
}


static void
goo_player_bar_construct (GooPlayerBar   *self,
			  GtkActionGroup *actions)
{
	GtkWidget *frame;
	GtkWidget *main_box;
	GtkWidget *button_box;
	GtkWidget *button;
    gboolean rtl;

    rtl = gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL;

	frame = gtk_event_box_new ();
	gtk_style_context_add_class (gtk_widget_get_style_context (frame), "goobox-player-bar");
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (self), frame, TRUE, TRUE, 0);

	main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (main_box), 10);
	gtk_box_set_spacing (GTK_BOX (main_box), 6);
	gtk_box_set_homogeneous (GTK_BOX (main_box), FALSE);
	gtk_widget_show (main_box);
	gtk_container_add (GTK_CONTAINER (frame), main_box);

	/* Play buttons */

	button = _gtk_button_new_from_icon_name (GOO_STOCK_PLAY, PLAY_BUTTON_SIZE);
	_gtk_button_sync_with_action (button, gtk_action_group_get_action (actions, "TogglePlay"));
	g_signal_connect (gtk_action_group_get_action (actions, "TogglePlay"),
			  "notify::icon-name",
			  G_CALLBACK (toggle_play_notify_icon_name_cb),
			  button);
	gtk_box_pack_start (GTK_BOX (main_box), button, FALSE, FALSE, 0);

	button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (button_box), GTK_STYLE_CLASS_LINKED);
	gtk_box_pack_start (GTK_BOX (main_box), button_box, FALSE, FALSE, 0);

	button = _gtk_button_new_from_icon_name (rtl ? GOO_STOCK_NEXT : GOO_STOCK_PREV, GTK_ICON_SIZE_SMALL_TOOLBAR);
	_gtk_button_sync_with_action (button, gtk_action_group_get_action (actions, "Prev"));
	gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);

	button = _gtk_button_new_from_icon_name (rtl ? GOO_STOCK_PREV : GOO_STOCK_NEXT, GTK_ICON_SIZE_SMALL_TOOLBAR);
	_gtk_button_sync_with_action (button, gtk_action_group_get_action (actions, "Next"));
	gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);

	/* Time */

	self->priv->time_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_no_show_all (self->priv->time_box, TRUE);
	gtk_box_pack_start (GTK_BOX (main_box), self->priv->time_box, TRUE, FALSE, 0);

	self->priv->current_time_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (self->priv->current_time_label), 1.0, 0.5);
	gtk_label_set_width_chars (GTK_LABEL (self->priv->current_time_label), TIME_LABEL_WIDTH_IN_CHARS);

	self->priv->time_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.01);
	gtk_range_set_increments (GTK_RANGE (self->priv->time_scale), 0.01, 0.1);
	gtk_scale_set_draw_value (GTK_SCALE (self->priv->time_scale), FALSE);
	gtk_widget_set_size_request (self->priv->time_scale, SCALE_WIDTH, -1);
	gtk_widget_show (self->priv->time_scale);

	self->priv->remaining_time_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (self->priv->remaining_time_label), 0.0, 0.5);
	gtk_label_set_width_chars (GTK_LABEL (self->priv->remaining_time_label), TIME_LABEL_WIDTH_IN_CHARS);

	gtk_box_pack_start (GTK_BOX (self->priv->time_box), self->priv->current_time_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self->priv->time_box), self->priv->time_scale, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self->priv->time_box), self->priv->remaining_time_label, FALSE, FALSE, 0);

	/* Other actions */

	button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_spacing (GTK_BOX (button_box), 6);
	gtk_box_pack_end (GTK_BOX (main_box), button_box, FALSE, FALSE, 0);

	button = _gtk_button_new_from_icon_name (GOO_STOCK_EXTRACT, GTK_ICON_SIZE_SMALL_TOOLBAR);
	_gtk_button_sync_with_action (button, gtk_action_group_get_action (actions, "Extract"));
	gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);

	button = _gtk_menu_button_new_from_icon_name ("emblem-system-symbolic");
	_gtk_button_sync_with_action (button, gtk_action_group_get_action (actions, "OtherActions"));
	gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);

	/* signals */

	g_signal_connect (self->priv->time_scale,
			  "value_changed",
			  G_CALLBACK (time_scale_value_changed_cb),
			  self);
	g_signal_connect (self->priv->time_scale,
			  "button_press_event",
			  G_CALLBACK (time_scale_button_press_cb),
			  self);
	g_signal_connect (self->priv->time_scale,
			  "button_release_event",
			  G_CALLBACK (time_scale_button_release_cb),
			  self);
}


static void
goo_player_bar_finalize (GObject *object)
{
        GooPlayerBar *self;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_PLAYER_BAR (object));

	self = GOO_PLAYER_BAR (object);

	if (self->priv->update_progress_timeout != 0) {
		g_source_remove (self->priv->update_progress_timeout);
		self->priv->update_progress_timeout = 0;
	}

	if (self->priv->update_id != 0) {
		g_source_remove (self->priv->update_id);
		self->priv->update_id = 0;
	}

	G_OBJECT_CLASS (goo_player_bar_parent_class)->finalize (object);
}


static void
goo_player_bar_class_init (GooPlayerBarClass *class)
{
        GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (class, sizeof (GooPlayerBarPrivateData));

	gobject_class = G_OBJECT_CLASS (class);
        gobject_class->finalize = goo_player_bar_finalize;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->get_preferred_width = goo_player_bar_get_preferred_width;

	goo_player_bar_signals[SKIP_TO] =
                g_signal_new ("skip-to",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerBarClass, skip_to),
			      NULL, NULL,
			      goo_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);
}


static void
_goo_player_bar_set_time (GooPlayerBar *self,
			  gint64        current_time)
{
	if (self->priv->dragging)
		return;

	self->priv->current_time = current_time;
	_goo_player_bar_update_current_time (self);

	g_signal_handlers_block_by_data (self->priv->time_scale, self);
	gtk_range_set_value (GTK_RANGE (self->priv->time_scale), (double) current_time / self->priv->track_length);
	g_signal_handlers_unblock_by_data (self->priv->time_scale, self);
}


static gboolean
update_progress_cb (gpointer data)
{
	GooPlayerBar *self = data;

	self->priv->update_progress_timeout = 0;

	if ((self->priv->fraction >= 0.0) && (self->priv->fraction <= 1.0))
		_goo_player_bar_set_time (self, self->priv->fraction * self->priv->track_length);

	return FALSE;
}


static void
player_progress_cb (GooPlayer     *player,
		    double         fraction,
		    GooPlayerBar *self)
{
	self->priv->fraction = fraction;
	if (self->priv->update_progress_timeout == 0)
		self->priv->update_progress_timeout = g_idle_add (update_progress_cb, self);
}


static void
goo_player_bar_set_sensitive (GooPlayerBar *self,
			      gboolean      value)
{
	/* FIXME */
}


static void
goo_player_bar_update_state (GooPlayerBar *self)
{
	GooPlayerState state;

	if (self->priv->player == NULL)
		return;

	state = goo_player_get_state (self->priv->player);

	if ((state == GOO_PLAYER_STATE_PLAYING)
	    || (state == GOO_PLAYER_STATE_PAUSED))
	{
		gtk_widget_show (self->priv->time_box);
	}
	else {
		gtk_widget_hide (self->priv->time_box);
	}
}


static void
player_state_changed_cb (GooPlayer     *player,
			 GooPlayerBar *self)
{
	goo_player_bar_update_state (self);
	goo_player_bar_set_sensitive (self, (goo_player_get_state (player) != GOO_PLAYER_STATE_ERROR) && (goo_player_get_discid (player) != NULL));
}


static void
player_start_cb (GooPlayer       *player,
		 GooPlayerAction  action,
		 GooPlayerBar    *self)
{
	goo_player_bar_update_state (self);
}


static void
player_done_cb (GooPlayer       *player,
		GooPlayerAction  action,
		GError          *error,
		GooPlayerBar    *self)
{
	AlbumInfo *album;

	switch (action) {
	case GOO_PLAYER_ACTION_LIST:
		goo_player_bar_update_state (self);
		_goo_player_bar_set_time (self, 0);
		break;
	case GOO_PLAYER_ACTION_METADATA:
		goo_player_bar_update_state (self);
		break;
	case GOO_PLAYER_ACTION_SEEK_SONG:
		album = goo_player_get_album (player);
		self->priv->track_length = album_info_get_track (album, goo_player_get_current_track (player))->length;
		goo_player_bar_update_state (self);
		_goo_player_bar_set_time (self, 0);
		break;
	case GOO_PLAYER_ACTION_PLAY:
	case GOO_PLAYER_ACTION_STOP:
	case GOO_PLAYER_ACTION_MEDIUM_REMOVED:
		_goo_player_bar_set_time (self, 0);
		break;
	default:
		break;
	}
}


GtkWidget *
goo_player_bar_new (GooPlayer      *player,
		    GtkActionGroup *actions)
{
	GooPlayerBar *self;

	g_return_val_if_fail (player != NULL, NULL);

	self = GOO_PLAYER_BAR (g_object_new (GOO_TYPE_PLAYER_BAR, NULL));
	self->priv->player = g_object_ref (player);
	goo_player_bar_construct (self, actions);

	g_signal_connect (player,
			  "start",
			  G_CALLBACK (player_start_cb),
			  self);
	g_signal_connect (player,
			  "done",
			  G_CALLBACK (player_done_cb),
			  self);
	g_signal_connect (player,
			  "progress",
			  G_CALLBACK (player_progress_cb),
			  self);
	g_signal_connect (player,
			  "state_changed",
			  G_CALLBACK (player_state_changed_cb),
			  self);

	return GTK_WIDGET (self);
}


double
goo_player_bar_get_progress (GooPlayerBar *self)
{
	return self->priv->fraction;
}
