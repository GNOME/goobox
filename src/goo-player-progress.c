/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2018 Free Software Foundation, Inc.
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

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include "goo-player-progress.h"
#include "goo-marshal.h"
#include "glib-utils.h"
#include "gtk-utils.h"


#define SCALE_WIDTH 150
#define TIME_LABEL_WIDTH_IN_CHARS 8
#define PLAY_BUTTON_SIZE GTK_ICON_SIZE_SMALL_TOOLPROGRESS
#define MIN_WIDTH 500
#define UPDATE_TIMEOUT 50
#define CHILD_NAME_TIME_SCALE "child-name"
#define CHILD_NAME_TITLE "title"


struct _GooPlayerProgressPrivate {
	GooPlayer *player;
	GtkWidget *current_time_label;
	GtkWidget *remaining_time_label;
	GtkWidget *time_scale;
	GtkWidget *time_box;
	GtkWidget *title;
	GtkWidget *stack;
	gint64     track_length;
	gint64     current_time;
	gboolean   dragging;
	guint      update_id;
	double     fraction;
	guint      update_progress_timeout;
};


G_DEFINE_TYPE_WITH_CODE (GooPlayerProgress, goo_player_progress, GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GooPlayerProgress))


enum {
	SKIP_TO,
	LAST_SIGNAL
};
static guint goo_player_progress_signals[LAST_SIGNAL] = { 0 };


static void
goo_player_progress_get_preferred_width (GtkWidget *widget,
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
_goo_player_progress_update_current_time (GooPlayerProgress *self)
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
			     GooPlayerProgress *self)
{
	self->priv->current_time = self->priv->track_length * gtk_range_get_value (range);
	_goo_player_progress_update_current_time (self);

	if (! self->priv->dragging) {
		int seconds;

		seconds = (int) (gtk_range_get_value (range) * self->priv->track_length);
		g_signal_emit (self, goo_player_progress_signals[SKIP_TO], 0, seconds);
	}
}


static gboolean
update_time_label_cb (gpointer data)
{
	GooPlayerProgress *self = data;

	if (self->priv->update_id != 0) {
		g_source_remove (self->priv->update_id);
		self->priv->update_id = 0;
	}

	self->priv->current_time = self->priv->track_length * gtk_range_get_value (GTK_RANGE (self->priv->time_scale));
	_goo_player_progress_update_current_time (self);

	self->priv->update_id = g_timeout_add (UPDATE_TIMEOUT,
					       update_time_label_cb,
					       data);

	return FALSE;
}


static gboolean
time_scale_button_press_cb (GtkRange          *range,
			    GdkEventButton    *event,
			    GooPlayerProgress *self)
{
	self->priv->dragging = TRUE;
	if (self->priv->update_id == 0)
		self->priv->update_id = g_timeout_add (UPDATE_TIMEOUT,
						       update_time_label_cb,
						       self);
	return FALSE;
}


static gboolean
time_scale_button_release_cb (GtkRange          *range,
			      GdkEventButton    *event,
			      GooPlayerProgress *self)
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
goo_player_progress_init (GooPlayerProgress *self)
{
	self->priv = goo_player_progress_get_instance_private (self);
	self->priv->dragging = FALSE;
	self->priv->track_length = 0;
	self->priv->current_time = 0;
	self->priv->update_id = 0;
	self->priv->update_progress_timeout = 0;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_can_focus (GTK_WIDGET (self), FALSE);
}


static void
goo_player_progress_update_state (GooPlayerProgress *self)
{
	GooPlayerState state;

	if (self->priv->player == NULL)
		return;

	state = goo_player_get_state (self->priv->player);

	if ((state == GOO_PLAYER_STATE_PLAYING)
	    || (state == GOO_PLAYER_STATE_PAUSED))
	{
		gtk_stack_set_visible_child_name (GTK_STACK (self->priv->stack), CHILD_NAME_TIME_SCALE);
	}
	else {
		gtk_stack_set_visible_child_name (GTK_STACK (self->priv->stack), CHILD_NAME_TITLE);
	}
}


static void
goo_player_progress_construct (GooPlayerProgress *self)
{
	GtkWidget *main_box;

	self->priv->stack = main_box = gtk_stack_new ();
	gtk_stack_set_vhomogeneous (GTK_STACK (main_box), TRUE);
	gtk_stack_set_transition_type (GTK_STACK (main_box), GTK_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
	gtk_container_set_border_width (GTK_CONTAINER (main_box), 10);
	gtk_widget_show (main_box);
	gtk_box_pack_start (GTK_BOX (self), main_box, TRUE, TRUE, 0);

	self->priv->time_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_halign (self->priv->time_box, GTK_ALIGN_CENTER);
	gtk_widget_show (self->priv->time_box);
	gtk_stack_add_named (GTK_STACK (main_box), self->priv->time_box, CHILD_NAME_TIME_SCALE);

	self->priv->current_time_label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (self->priv->current_time_label), 1.0);
	gtk_label_set_yalign (GTK_LABEL (self->priv->current_time_label), 0.5);
	gtk_label_set_width_chars (GTK_LABEL (self->priv->current_time_label), TIME_LABEL_WIDTH_IN_CHARS);

	self->priv->time_scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.01);
	gtk_range_set_increments (GTK_RANGE (self->priv->time_scale), 0.01, 0.1);
	gtk_scale_set_draw_value (GTK_SCALE (self->priv->time_scale), FALSE);
	gtk_widget_set_size_request (self->priv->time_scale, SCALE_WIDTH, -1);
	gtk_widget_show (self->priv->time_scale);

	self->priv->remaining_time_label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (self->priv->remaining_time_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (self->priv->remaining_time_label), 0.5);
	gtk_label_set_width_chars (GTK_LABEL (self->priv->remaining_time_label), TIME_LABEL_WIDTH_IN_CHARS);

	gtk_box_pack_start (GTK_BOX (self->priv->time_box), self->priv->current_time_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self->priv->time_box), self->priv->time_scale, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (self->priv->time_box), self->priv->remaining_time_label, FALSE, FALSE, 0);

	self->priv->title = gtk_label_new ("");
	gtk_label_set_line_wrap (GTK_LABEL (self->priv->title), FALSE);
	gtk_label_set_single_line_mode (GTK_LABEL (self->priv->title), TRUE);
	gtk_label_set_ellipsize (GTK_LABEL (self->priv->title), PANGO_ELLIPSIZE_END);
	gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->title), "title");
	gtk_widget_show (self->priv->title);
	gtk_stack_add_named (GTK_STACK (main_box), self->priv->title, CHILD_NAME_TITLE);

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

	goo_player_progress_update_state (self);
}


static void
goo_player_progress_finalize (GObject *object)
{
	GooPlayerProgress *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GOO_IS_PLAYER_PROGRESS (object));

	self = GOO_PLAYER_PROGRESS (object);

	if (self->priv->update_progress_timeout != 0) {
		g_source_remove (self->priv->update_progress_timeout);
		self->priv->update_progress_timeout = 0;
	}

	if (self->priv->update_id != 0) {
		g_source_remove (self->priv->update_id);
		self->priv->update_id = 0;
	}

	G_OBJECT_CLASS (goo_player_progress_parent_class)->finalize (object);
}


static void
goo_player_progress_class_init (GooPlayerProgressClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = G_OBJECT_CLASS (class);
	gobject_class->finalize = goo_player_progress_finalize;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->get_preferred_width = goo_player_progress_get_preferred_width;

	goo_player_progress_signals[SKIP_TO] =
		g_signal_new ("skip-to",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerProgressClass, skip_to),
			      NULL, NULL,
			      goo_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);
}


static void
_goo_player_progress_set_time (GooPlayerProgress *self,
			       gint64             current_time)
{
	if (self->priv->dragging)
		return;

	self->priv->current_time = current_time;
	_goo_player_progress_update_current_time (self);

	g_signal_handlers_block_by_data (self->priv->time_scale, self);
	gtk_range_set_value (GTK_RANGE (self->priv->time_scale), (double) current_time / self->priv->track_length);
	g_signal_handlers_unblock_by_data (self->priv->time_scale, self);
}


static gboolean
update_progress_cb (gpointer data)
{
	GooPlayerProgress *self = data;

	self->priv->update_progress_timeout = 0;

	if ((self->priv->fraction >= 0.0) && (self->priv->fraction <= 1.0))
		_goo_player_progress_set_time (self, self->priv->fraction * self->priv->track_length);

	return FALSE;
}


static void
player_progress_cb (GooPlayer         *player,
		    double             fraction,
		    GooPlayerProgress *self)
{
	self->priv->fraction = fraction;
	if (self->priv->update_progress_timeout == 0)
		self->priv->update_progress_timeout = g_idle_add (update_progress_cb, self);
}


static void
goo_player_progress_set_sensitive (GooPlayerProgress *self,
				   gboolean           value)
{
	/* FIXME */
}


static void
player_state_changed_cb (GooPlayer     *player,
			 GooPlayerProgress *self)
{
	goo_player_progress_update_state (self);
	goo_player_progress_set_sensitive (self, (goo_player_get_state (player) != GOO_PLAYER_STATE_ERROR) && (goo_player_get_discid (player) != NULL));
}


static void
player_start_cb (GooPlayer       *player,
		 GooPlayerAction  action,
		 GooPlayerProgress    *self)
{
	goo_player_progress_update_state (self);
}


static void
player_done_cb (GooPlayer       *player,
		GooPlayerAction  action,
		GError          *error,
		GooPlayerProgress    *self)
{
	AlbumInfo *album;

	switch (action) {
	case GOO_PLAYER_ACTION_LIST:
		goo_player_progress_update_state (self);
		_goo_player_progress_set_time (self, 0);
		break;
	case GOO_PLAYER_ACTION_METADATA:
		goo_player_progress_update_state (self);
		break;
	case GOO_PLAYER_ACTION_SEEK_SONG:
		album = goo_player_get_album (player);
		self->priv->track_length = album_info_get_track (album, goo_player_get_current_track (player))->length;
		goo_player_progress_update_state (self);
		_goo_player_progress_set_time (self, 0);
		break;
	case GOO_PLAYER_ACTION_PLAY:
	case GOO_PLAYER_ACTION_STOP:
	case GOO_PLAYER_ACTION_MEDIUM_REMOVED:
		_goo_player_progress_set_time (self, 0);
		break;
	case GOO_PLAYER_ACTION_PAUSE:
		break;
	default:
		break;
	}
}


GtkWidget *
goo_player_progress_new (GooPlayer	*player)
{
	GooPlayerProgress *self;

	g_return_val_if_fail (player != NULL, NULL);

	self = GOO_PLAYER_PROGRESS (g_object_new (GOO_TYPE_PLAYER_PROGRESS, NULL));
	self->priv->player = g_object_ref (player);
	goo_player_progress_construct (self);

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


void
goo_player_progress_set_title (GooPlayerProgress	*progress,
			       const char 		*title)
{
	gtk_label_set_text (GTK_LABEL (progress->priv->title), title);
}


double
goo_player_progress_get_progress (GooPlayerProgress *self)
{
	return self->priv->fraction;
}
