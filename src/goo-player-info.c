/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004, 2007 Free Software Foundation, Inc.
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
#include "goo-player-info.h"
#include "goo-marshal.h"
#include "goo-stock.h"
#include "goo-window.h"
#include "glib-utils.h"

#define SPACING 0
#define TITLE1_FORMAT "<span size='large' weight='bold'>%s</span>"
#define TITLE2_FORMAT "%s"
#define TITLE3_FORMAT "<i>%s</i>"
#define TIME_FORMAT "%s"
#define PLAYING_FORMAT "[ <small>%s</small> ]"
#define SCALE_WIDTH 150
#define COVER_SIZE 80
#define TRAY_COVER_SIZE 80
#define MIN_WIDTH 420
#define MIN_TOOLTIP_WIDTH 300
#define MAX_TOOLTIP_WIDTH 400
#define MIN_CHARS 45
#define UPDATE_TIMEOUT 50

struct _GooPlayerInfoPrivateData {
	GooWindow   *window;
	gboolean     interactive;
	GtkWidget   *cover_frame;
	GtkWidget   *title1_label;
	GtkWidget   *title2_label;
	GtkWidget   *title3_label;
	GtkWidget   *time_label;
	GtkWidget   *playing_label;
	GtkWidget   *time_scale;
	GtkWidget   *cover_image;
	GtkWidget   *cover_button;
	GtkWidget   *status_image;
	GtkWidget   *notebook;
	char        *current_time;
	char        *total_time;
	char         time[64];
	gint64       track_length;
	gboolean     dragging;
	guint        update_id;

	double       fraction;
	guint        update_progress_timeout;
};

enum {
	COVER_CLICKED,
	SKIP_TO,
        LAST_SIGNAL
};

static guint goo_player_info_signals[LAST_SIGNAL] = { 0 };

enum { TARGET_URL };
static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, TARGET_URL }
};
static guint n_targets = sizeof (target_table) / sizeof (target_table[0]);

static void goo_player_info_finalize    (GObject *object);

#define GOO_PLAYER_INFO_GET_PRIVATE_DATA(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), GOO_TYPE_PLAYER_INFO, GooPlayerInfoPrivateData))

G_DEFINE_TYPE (GooPlayerInfo, goo_player_info, GTK_TYPE_HBOX)

static void
goo_player_info_get_preferred_width (GtkWidget *widget,
				     int       *minimum_width,
				     int       *natural_width)
{
	GooPlayerInfo *info = GOO_PLAYER_INFO (widget);

	if (info->priv->interactive)
		*minimum_width = *natural_width = MIN_WIDTH;
	else
		*minimum_width = *natural_width = MIN_TOOLTIP_WIDTH;
}


static void
goo_player_info_class_init (GooPlayerInfoClass *class)
{
        GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	goo_player_info_signals[COVER_CLICKED] =
                g_signal_new ("cover_clicked",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerInfoClass, cover_clicked),
			      NULL, NULL,
			      goo_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	goo_player_info_signals[SKIP_TO] =
                g_signal_new ("skip_to",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerInfoClass, skip_to),
			      NULL, NULL,
			      goo_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);

	gobject_class = G_OBJECT_CLASS (class);
        gobject_class->finalize = goo_player_info_finalize;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->get_preferred_width = goo_player_info_get_preferred_width;

	g_type_class_add_private (class, sizeof (GooPlayerInfoPrivateData));
}


static void
set_label (GtkWidget     *label,
	   const char    *format,
	   const char    *text)
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
set_title1 (GooPlayerInfo *info,
	    const char    *text)
{
	set_label (info->priv->title1_label, TITLE1_FORMAT, text);
}


static void
set_title2 (GooPlayerInfo *info,
	    const char    *text)
{
	set_label (info->priv->title2_label, TITLE2_FORMAT, text);
}


static void
set_title3 (GooPlayerInfo *info,
	    const char    *text)
{
	set_label (info->priv->title3_label, TITLE3_FORMAT, text);
}


static void
set_playing (GooPlayerInfo *info,
	     const char    *text)
{
	set_label (info->priv->playing_label, PLAYING_FORMAT, text);
}


static void
set_time (GooPlayerInfo *info,
	  const char    *text)
{
	set_label (info->priv->time_label, TIME_FORMAT, text);
}


static void
time_scale_value_changed_cb (GtkRange      *range,
			     GooPlayerInfo *info)
{
	double new_value;
	gint64 current_time;

	new_value = gtk_range_get_value (range);
	current_time = info->priv->track_length * (new_value / 100.0);
	g_free (info->priv->current_time);
	info->priv->current_time = _g_format_duration_for_display (current_time * 1000);
	/* translators: this is the current_time / total_time label */
	sprintf (info->priv->time, _("%s / %s"), info->priv->total_time, info->priv->current_time);
	set_time (info, info->priv->time);

	if (! info->priv->dragging) {
		int seconds;

		seconds = (int) (new_value * info->priv->track_length);
		g_signal_emit (info, goo_player_info_signals[SKIP_TO], 0, seconds);
	}
}


static gboolean
update_time_label_cb (gpointer data)
{
	GooPlayerInfo *info = data;
	GooPlayerInfoPrivateData *priv = info->priv;
	double new_value = gtk_range_get_value (GTK_RANGE (priv->time_scale));
	gint64 current_time;

	if (priv->update_id != 0) {
		g_source_remove (priv->update_id);
		priv->update_id = 0;
	}

	current_time = priv->track_length * new_value;
	g_free (info->priv->current_time);
	info->priv->current_time = _g_format_duration_for_display (current_time * 1000);
	sprintf (priv->time, _("%s / %s"), priv->current_time, priv->total_time);
	set_time (info, priv->time);

	priv->update_id = g_timeout_add (UPDATE_TIMEOUT,
					 update_time_label_cb,
					 data);

	return FALSE;
}


static gboolean
time_scale_button_press_cb (GtkRange         *range,
			    GdkEventButton   *event,
			    GooPlayerInfo    *info)
{
	info->priv->dragging = TRUE;
	info->priv->update_id = g_timeout_add (UPDATE_TIMEOUT,
					       update_time_label_cb,
					       info);
	return FALSE;
}


static gboolean
time_scale_button_release_cb (GtkRange         *range,
			      GdkEventButton   *event,
			      GooPlayerInfo    *info)
{
	if (info->priv->update_id != 0) {
		g_source_remove (info->priv->update_id);
		info->priv->update_id = 0;
	}

	info->priv->dragging = FALSE;
	return FALSE;
}


static void
cover_button_clicked_cb (GtkWidget     *button,
			 GooPlayerInfo *info)
{
	g_signal_emit (info, goo_player_info_signals[COVER_CLICKED], 0);
}


/* -- drag and drop -- */


void
cover_button_drag_data_received  (GtkWidget          *widget,
				  GdkDragContext     *context,
				  gint                x,
				  gint                y,
				  GtkSelectionData   *data,
				  guint               dnd_info,
				  guint               time,
				  gpointer            extra_data)
{
	GooPlayerInfo  *info = extra_data;
	char          **uris;

	gtk_drag_finish (context, TRUE, FALSE, time);

	uris = gtk_selection_data_get_uris (data);
	if (uris[0] != NULL) {
		GFile *file;
		char  *cover_filename;

		file = g_file_new_for_uri (uris[0]);
		cover_filename = g_file_get_path (file);
		goo_window_set_cover_image (info->priv->window, cover_filename);

		g_free (cover_filename);
		g_object_unref (file);
	}

	g_strfreev (uris);
}


static void
goo_player_info_init (GooPlayerInfo *info)
{
	info->priv = GOO_PLAYER_INFO_GET_PRIVATE_DATA (info);
}


static void goo_player_info_update_state (GooPlayerInfo *info);


static void
goo_player_info_construct (GooPlayerInfo *info)
{
	GooPlayerInfoPrivateData *priv;
	GtkWidget *vbox, *time_box;

	priv = info->priv;

	priv->dragging = FALSE;
	priv->current_time = NULL;
	priv->total_time = NULL;
	priv->update_id = 0;

	gtk_widget_set_can_focus (GTK_WIDGET (info), FALSE);
	gtk_box_set_spacing (GTK_BOX (info), SPACING);
	gtk_box_set_homogeneous (GTK_BOX (info), FALSE);

	/* Title and Artist */

	priv->title1_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->title1_label), 0.0, 0.5);

	priv->title2_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->title2_label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->title2_label), priv->interactive);

	priv->title3_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->title3_label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->title3_label), priv->interactive);

	gtk_label_set_ellipsize (GTK_LABEL (priv->title1_label),
				 PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars (GTK_LABEL (priv->title2_label), MIN_CHARS);

	gtk_label_set_ellipsize (GTK_LABEL (priv->title2_label),
				 PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars (GTK_LABEL (priv->title2_label), MIN_CHARS);

	gtk_label_set_ellipsize (GTK_LABEL (priv->title3_label),
				 PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars (GTK_LABEL (priv->title3_label), MIN_CHARS);

	/* Time */

	time_box = gtk_hbox_new (FALSE, 6);

	priv->time_scale = gtk_hscale_new_with_range (0.0, 1.0, 0.01);
	gtk_range_set_increments (GTK_RANGE (priv->time_scale), 0.01, 0.1);
	gtk_scale_set_draw_value (GTK_SCALE (priv->time_scale), FALSE);
	gtk_widget_set_size_request (priv->time_scale, SCALE_WIDTH, -1);
	/* gtk_range_set_update_policy (GTK_RANGE (priv->time_scale), GTK_UPDATE_DISCONTINUOUS); FIXME */
	gtk_widget_set_no_show_all (priv->time_scale, TRUE);

	priv->time_label = gtk_label_new (NULL);
	gtk_widget_set_no_show_all (priv->time_label, TRUE);

	priv->playing_label = gtk_label_new (NULL);
	gtk_widget_set_no_show_all (priv->playing_label, TRUE);

	gtk_box_pack_start (GTK_BOX (time_box), priv->time_scale, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (time_box), priv->time_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (time_box), priv->playing_label, FALSE, FALSE, 0);

	/* Image */


	if (priv->interactive) {
		priv->cover_button = gtk_button_new ();
		gtk_button_set_relief (GTK_BUTTON (priv->cover_button),
				       GTK_RELIEF_NONE);

		gtk_widget_set_tooltip_text (GTK_WIDGET (priv->cover_button),
					     _("Click here to choose a cover for this CD"));

		g_signal_connect (G_OBJECT (priv->cover_button),
				  "clicked",
				  G_CALLBACK (cover_button_clicked_cb),
				  info);
		gtk_drag_dest_set (priv->cover_button,
				   GTK_DEST_DEFAULT_ALL,
				   target_table, n_targets,
				   GDK_ACTION_COPY);
		g_signal_connect (G_OBJECT (priv->cover_button),
				  "drag_data_received",
				  G_CALLBACK (cover_button_drag_data_received),
				  info);
	}

	priv->cover_image = gtk_image_new_from_stock (GOO_STOCK_NO_DISC, GTK_ICON_SIZE_DIALOG);
	if (priv->interactive)
		gtk_widget_set_size_request (priv->cover_image, COVER_SIZE, COVER_SIZE);
	else
		gtk_widget_set_size_request (priv->cover_image, TRAY_COVER_SIZE, TRAY_COVER_SIZE);
	gtk_widget_show (priv->cover_image);

	if (priv->interactive)
		gtk_container_add (GTK_CONTAINER (priv->cover_button), priv->cover_image);
	else
		priv->cover_button = priv->cover_image;

	/* Status image */

	priv->status_image = gtk_image_new_from_stock (GOO_STOCK_NO_DISC, GTK_ICON_SIZE_DIALOG);
	if (priv->interactive)
		gtk_widget_set_size_request (priv->status_image, COVER_SIZE, COVER_SIZE);
	else
		gtk_widget_set_size_request (priv->status_image, TRAY_COVER_SIZE, TRAY_COVER_SIZE);
	gtk_widget_show (priv->cover_image);
	/*gtk_container_set_border_width (GTK_CONTAINER (priv->status_image), 6);*/

	/* Frame */

	priv->notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (priv->notebook), 0);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_widget_show (priv->notebook);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), priv->status_image, NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), priv->cover_button, NULL);

	priv->cover_frame = gtk_frame_new (NULL);
	gtk_container_set_border_width (GTK_CONTAINER (priv->cover_frame), 6);
	if (! priv->interactive)
		gtk_frame_set_shadow_type (GTK_FRAME (priv->cover_frame), GTK_SHADOW_ETCHED_IN);
	gtk_widget_show (priv->cover_frame);
	gtk_container_add (GTK_CONTAINER (priv->cover_frame), priv->notebook);

	gtk_box_pack_start (GTK_BOX (info), priv->cover_frame, FALSE, FALSE, 0);

	/**/

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title1_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title2_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title3_label, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), time_box, FALSE, FALSE, 0);

	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (info), vbox, TRUE, TRUE, 0);

	/**/

	g_signal_connect (priv->time_scale,
			  "value_changed",
			  G_CALLBACK (time_scale_value_changed_cb),
			  info);
	g_signal_connect (priv->time_scale,
			  "button_press_event",
			  G_CALLBACK (time_scale_button_press_cb),
			  info);
	g_signal_connect (priv->time_scale,
			  "button_release_event",
			  G_CALLBACK (time_scale_button_release_cb),
			  info);
}


static void
goo_player_info_finalize (GObject *object)
{
        GooPlayerInfo *info;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_PLAYER_INFO (object));

	info = GOO_PLAYER_INFO (object);
	if (info->priv != NULL) {
		g_free (info->priv->current_time);
		g_free (info->priv->total_time);
		if (info->priv->update_id != 0) {
			g_source_remove (info->priv->update_id);
			info->priv->update_id = 0;
		}
		info->priv = NULL;
	}

	G_OBJECT_CLASS (goo_player_info_parent_class)->finalize (object);
}


static void
goo_player_info_set_time (GooPlayerInfo  *info,
			  gint64          current_time)
{
	if (info->priv->dragging)
		return;

	g_free (info->priv->current_time);
	info->priv->current_time = _g_format_duration_for_display (current_time * 1000);
	sprintf (info->priv->time, _("%s / %s"), info->priv->current_time, info->priv->total_time);
	set_time (info, info->priv->time);

	g_signal_handlers_block_by_data (info->priv->time_scale, info);
	gtk_range_set_value (GTK_RANGE (info->priv->time_scale), (double) current_time / info->priv->track_length);
	g_signal_handlers_unblock_by_data (info->priv->time_scale, info);

	/* FIXME: this doesn't update the tooltip
	gtk_tooltip_trigger_tooltip_query (gdk_screen_get_display (gtk_status_icon_get_screen (goo_window_get_status_icon (info->priv->window))));
	*/
}


static gboolean
update_progress_cb (gpointer data)
{
	GooPlayerInfo *info = data;

	if ((info->priv->fraction < 0.0) || (info->priv->fraction > 1.0))
		/* nothing */;
	else
		goo_player_info_set_time (info, info->priv->fraction * info->priv->track_length);

	return FALSE;
}


static void
player_progress_cb (GooPlayer     *player,
		    double         fraction,
		    GooPlayerInfo *info)
{
	info->priv->fraction = fraction;
	info->priv->update_progress_timeout = g_idle_add (update_progress_cb, info);
}


double
goo_player_info_get_progress (GooPlayerInfo *info)
{
	return info->priv->fraction;
}


static void
update_subtitle (GooPlayerInfo *info,
		 TrackInfo     *track)
{
 	AlbumInfo *album;

	album = goo_window_get_album (info->priv->window);

	if ((album->title == NULL) || (album->artist == NULL)) {
		g_free (info->priv->total_time);
		info->priv->total_time = _g_format_duration_for_display (track->length * 1000);
		set_title2 (info, info->priv->total_time);
	}
	else {
		set_title2 (info, album->artist);
		set_title3 (info, album->title);
		gtk_label_set_selectable (GTK_LABEL (info->priv->title2_label), info->priv->interactive);
		gtk_label_set_selectable (GTK_LABEL (info->priv->title3_label), info->priv->interactive);
	}
}


static void
show_simple_text (GooPlayerInfo *info,
		  const char     *text)
{
	set_title1 (info, text);
	set_title2 (info, "");
	set_title3 (info, "");

	/* center vertically the only displayed label */

	gtk_box_set_child_packing (GTK_BOX (gtk_widget_get_parent (info->priv->title1_label)),
				   info->priv->title1_label,
				   TRUE,
				   TRUE,
				   0,
				   GTK_PACK_START);
}


static void
show_all_labels (GooPlayerInfo *info)
{
	gtk_widget_show (info->priv->title1_label);
	gtk_widget_show (info->priv->title2_label);
	gtk_widget_show (info->priv->title3_label);

	gtk_box_set_child_packing (GTK_BOX (gtk_widget_get_parent (info->priv->title1_label)),
				   info->priv->title1_label,
				   FALSE,
				   FALSE,
				   0,
				   GTK_PACK_START);
}


static void
goo_player_info_update_state (GooPlayerInfo *info)
{
	GooPlayerInfoPrivateData *priv = info->priv;
	GooPlayerState  state;
	AlbumInfo      *album;
	GooPlayer      *player;

	if (info->priv->window == NULL)
		return;
	player = goo_window_get_player (info->priv->window);
	if (player == NULL)
		return;

	state = goo_player_get_state (player);
	album = goo_window_get_album (info->priv->window);

	if ((state == GOO_PLAYER_STATE_PLAYING)
	    || (state == GOO_PLAYER_STATE_PAUSED))
	{
	    	if (info->priv->interactive)
			gtk_widget_show (priv->time_scale);
		gtk_widget_show (priv->time_label);
		gtk_widget_show (priv->playing_label);
	}
	else {
		gtk_widget_hide (priv->time_scale);
		gtk_widget_hide (priv->time_label);
		gtk_widget_hide (priv->playing_label);
	}

	gtk_label_set_selectable (GTK_LABEL (priv->title1_label), FALSE);
	gtk_label_set_selectable (GTK_LABEL (priv->title2_label), FALSE);
	gtk_label_set_selectable (GTK_LABEL (priv->title3_label), FALSE);

	if ((state == GOO_PLAYER_STATE_ERROR) || (state == GOO_PLAYER_STATE_NO_DISC))
	{
		show_simple_text (info, _("No disc"));
	}
	else if (state == GOO_PLAYER_STATE_DATA_DISC) {
		show_simple_text (info, _("Data disc"));
	}
	else {
		TrackInfo *track;

		track = album_info_get_track (album, goo_player_get_current_track (player));

		if (track != NULL) {
			char *state_s = "";

			show_all_labels (info);

			priv->track_length = track->length;

			if (state == GOO_PLAYER_STATE_PAUSED)
				state_s = _("Paused");
			set_playing (info, state_s);

			set_title1 (info, track->title);
			gtk_label_set_selectable (GTK_LABEL (priv->title1_label), info->priv->interactive);

			update_subtitle (info, track);
		}
		else if (state == GOO_PLAYER_STATE_EJECTING) {
			show_simple_text (info, _("Ejecting CD"));
		}
		else if (state == GOO_PLAYER_STATE_UPDATING) {
			show_simple_text (info, _("Checking CD drive"));
		}
		else if (state == GOO_PLAYER_STATE_SEEKING) {
			show_simple_text (info, _("Reading CD"));
		}
		else if (state == GOO_PLAYER_STATE_LISTING) {
			show_simple_text (info, _("Reading CD"));
		}
		else if (album->title == NULL) {
			show_simple_text (info, _("Audio CD"));
		}
		else {
			char year[128];

			show_all_labels (info);

			if (g_date_valid (album->release_date) != 0)
				sprintf (year, "%u", g_date_get_year (album->release_date));
			else
				year[0] = '\0';

			set_title1 (info, album->title);
			gtk_label_set_selectable (GTK_LABEL (priv->title1_label), info->priv->interactive);

			if (album->artist != NULL) {
				set_title2 (info, album->artist);
				set_title3 (info, year);
				gtk_label_set_selectable (GTK_LABEL (priv->title2_label), info->priv->interactive);
			}
			else {
				set_title2 (info, year);
				set_title3 (info, "");
			}
		}

		track_info_unref (track);
	}
}


static void
goo_player_info_set_sensitive (GooPlayerInfo  *info,
			       gboolean        value)
{
	gtk_widget_set_sensitive (info->priv->cover_button, value);
}


static void
player_state_changed_cb (GooPlayer     *player,
			 GooPlayerInfo *info)
{
	goo_player_info_update_state (info);
	goo_player_info_set_sensitive (info, (goo_player_get_state (player) != GOO_PLAYER_STATE_ERROR) && (goo_player_get_discid (player) != NULL));
}


static void
player_start_cb (GooPlayer       *player,
		 GooPlayerAction  action,
		 GooPlayerInfo   *info)
{
	goo_player_info_update_state (info);
}


static void
goo_player_info_set_total_time (GooPlayerInfo  *info,
				gint64          total_time)
{
	GooPlayerInfoPrivateData *priv = info->priv;

	g_free (priv->total_time);
	priv->total_time = _g_format_duration_for_display (total_time * 1000);
	goo_player_info_update_state (info);
}


static void
player_done_cb (GooPlayer       *player,
		GooPlayerAction  action,
		GError          *error,
		GooPlayerInfo   *info)
{
	AlbumInfo *album;

	switch (action) {
	case GOO_PLAYER_ACTION_LIST:
		goo_player_info_update_state (info);
		album = goo_player_get_album (player);
		goo_player_info_set_total_time (info, album->total_length);
		break;
	case GOO_PLAYER_ACTION_METADATA:
		goo_player_info_update_state (info);
		break;
	case GOO_PLAYER_ACTION_SEEK_SONG:
		goo_player_info_update_state (info);
		album = goo_player_get_album (player);
		goo_player_info_set_total_time (info, album_info_get_track (album, goo_player_get_current_track (player))->length);
		break;
	case GOO_PLAYER_ACTION_PLAY:
	case GOO_PLAYER_ACTION_STOP:
	case GOO_PLAYER_ACTION_MEDIUM_REMOVED:
		goo_player_info_set_time (info, 0);
		break;
	default:
		break;
	}
}


static void
goo_player_info_set_cover (GooPlayerInfo *info,
			   const char    *cover)
{
	gboolean cover_set = FALSE;

	if (cover == NULL)
		return;

	if (strcmp (cover, "no-disc") == 0) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (info->priv->notebook), 0);
		gtk_image_set_from_stock (GTK_IMAGE (info->priv->status_image),
					  GOO_STOCK_NO_DISC,
					  GTK_ICON_SIZE_DIALOG);
	}
	else if (strcmp (cover, "data-disc") == 0) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (info->priv->notebook), 0);
		gtk_image_set_from_stock (GTK_IMAGE (info->priv->status_image),
					  GOO_STOCK_DATA_DISC,
					  GTK_ICON_SIZE_DIALOG);
	}
	else if (strcmp (cover, "audio-cd") == 0) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (info->priv->notebook), 1);
		gtk_image_set_from_stock (GTK_IMAGE (info->priv->cover_image),
					  GOO_STOCK_AUDIO_CD,
					  GTK_ICON_SIZE_DIALOG);
	}
	else {
		GdkPixbuf *image;

		image = gdk_pixbuf_new_from_file_at_size (cover, 80, 80, NULL);
		if (image != NULL) {
			gtk_notebook_set_current_page (GTK_NOTEBOOK (info->priv->notebook), 1);
			gtk_image_set_from_pixbuf (GTK_IMAGE (info->priv->cover_image), image);
			cover_set = TRUE;
			g_object_unref (image);
		}
		else
			goo_player_info_set_cover (info, "audio-cd");
	}

	if (! info->priv->interactive)
		gtk_frame_set_shadow_type (GTK_FRAME (info->priv->cover_frame), cover_set ? GTK_SHADOW_NONE : GTK_SHADOW_ETCHED_IN);
}


static void
window_update_cover_cb (GooWindow     *window,
			GooPlayerInfo *info)
{
	GooPlayerState  state;
	char           *filename;

	state = goo_player_get_state (goo_window_get_player (window));

	if ((state == GOO_PLAYER_STATE_ERROR) || (state == GOO_PLAYER_STATE_NO_DISC)) {
	    	goo_player_info_set_cover (info, "no-disc");
	    	return;
	}

	if (state == GOO_PLAYER_STATE_DATA_DISC) {
	    	goo_player_info_set_cover (info, "data-disc");
	    	return;
	}

	filename = goo_window_get_cover_filename (window);
	if (filename == NULL) {
		goo_player_info_set_cover (info, "audio-cd");
		return;
	}

	goo_player_info_set_cover (info, filename);
	g_free (filename);
}


GtkWidget *
goo_player_info_new (GooWindow *window,
		     gboolean   interactive)
{
	GooPlayerInfo *info;
	GooPlayer     *player;

	g_return_val_if_fail (window != NULL, NULL);

	player = goo_window_get_player (window);
	g_return_val_if_fail (player != NULL, NULL);

	info = GOO_PLAYER_INFO (g_object_new (GOO_TYPE_PLAYER_INFO, NULL));

	info->priv->window = window;
	info->priv->interactive = interactive;
	goo_player_info_construct (info);

	g_signal_connect (window,
			  "update_cover",
			  G_CALLBACK (window_update_cover_cb),
			  info);

	g_signal_connect (player,
			  "start",
			  G_CALLBACK (player_start_cb),
			  info);
	g_signal_connect (player,
			  "done",
			  G_CALLBACK (player_done_cb),
			  info);
	g_signal_connect (player,
			  "progress",
			  G_CALLBACK (player_progress_cb),
			  info);
	g_signal_connect (player,
			  "state_changed",
			  G_CALLBACK (player_state_changed_cb),
			  info);

	return GTK_WIDGET (info);
}


GdkPixbuf *
goo_player_info_get_cover (GooPlayerInfo *info)
{
	if (gtk_image_get_storage_type (GTK_IMAGE (info->priv->cover_image)) == GTK_IMAGE_PIXBUF)
		return gtk_image_get_pixbuf (GTK_IMAGE (info->priv->cover_image));
	else
		return NULL;
}
