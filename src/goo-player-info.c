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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include "goo-player-info.h"
#include "goo-marshal.h"
#include "goo-window.h"
#include "glib-utils.h"
#include "gtk-utils.h"

#define TITLE1_FORMAT "<span size='large'>%s</span>"
#define TITLE2_FORMAT "%s"
#define TITLE3_FORMAT "%s"
#define TIME_FORMAT "%s"
#define SCALE_WIDTH 150
#define COVER_SIZE 100
#define TRAY_COVER_SIZE 80
#define MIN_WIDTH 500
#define MIN_TOOLTIP_WIDTH 300
#define MAX_TOOLTIP_WIDTH 400
#define MIN_CHARS 45
#define UPDATE_TIMEOUT 50


struct _GooPlayerInfoPrivate {
	GooWindow   *window;
	GtkWidget   *cover_frame;
	GtkWidget   *title1_label;
	GtkWidget   *title2_label;
	GtkWidget   *title3_label;
	GtkWidget   *time_label;
	GtkWidget   *cover_image;
	GtkWidget   *cover_button;
	GtkWidget   *status_button;
	GtkWidget   *status_image;
	GtkWidget   *notebook;
	char        *total_time;
	char         time[64];
	gint64       track_length;
	gboolean     dragging;
	guint        update_id;
	double       fraction;
	guint        update_progress_timeout;
	GdkPixbuf   *original_cover;
	char        *cover_file;
};


G_DEFINE_TYPE_WITH_CODE (GooPlayerInfo, goo_player_info, GTK_TYPE_BOX,
			 G_ADD_PRIVATE (GooPlayerInfo))


enum {
	COVER_CLICKED,
	UPDATE_STATUS,
	LAST_SIGNAL
};

enum {
	TARGET_URL
};


static guint goo_player_info_signals[LAST_SIGNAL] = { 0 };
static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, TARGET_URL }
};
static guint n_targets = sizeof (target_table) / sizeof (target_table[0]);


static void
goo_player_info_get_preferred_width (GtkWidget *widget,
				     int       *minimum_width,
				     int       *natural_width)
{
	*minimum_width = *natural_width = MIN_WIDTH;
}


static void
set_label (GtkWidget  *label,
	  const char  *format,
	   const char *text)
{
	char *esc;
	char *markup;

	if ((text == NULL) || (*text == '\0')) {
		gtk_label_set_text (GTK_LABEL (label), "");
		gtk_widget_hide (label);
		return;
	}

	esc = g_markup_escape_text (text, -1);
	markup = g_strdup_printf (format, esc);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	gtk_widget_show (label);

	g_free (markup);
	g_free (esc);
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
set_total_time (GooPlayerInfo *info,
		const char    *text)
{
	set_label (info->priv->time_label, TIME_FORMAT, text);
}


static void
cover_button_clicked_cb (GtkWidget     *button,
			 GooPlayerInfo *info)
{
	g_signal_emit (info, goo_player_info_signals[COVER_CLICKED], 0);
}


static void
status_button_clicked_cb (GtkWidget     *button,
			  GooPlayerInfo *info)
{
	g_signal_emit (info, goo_player_info_signals[UPDATE_STATUS], 0);
}


/* -- drag and drop -- */


static void
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
goo_player_info_construct (GooPlayerInfo *info)
{
	GooPlayerInfoPrivate *priv;
	GtkWidget            *vbox;

	priv = info->priv;

	priv->dragging = FALSE;
	priv->total_time = NULL;
	priv->update_id = 0;

	gtk_widget_set_margin_top (GTK_WIDGET (info), 10);
	gtk_widget_set_margin_end (GTK_WIDGET (info), 10);
	gtk_widget_set_margin_bottom (GTK_WIDGET (info), 10);
	gtk_widget_set_margin_start (GTK_WIDGET (info), 10);

	gtk_widget_set_can_focus (GTK_WIDGET (info), FALSE);
	gtk_box_set_spacing (GTK_BOX (info), 12);
	gtk_box_set_homogeneous (GTK_BOX (info), FALSE);

	/* Title and Artist */

	priv->title1_label = gtk_label_new (NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (priv->title1_label), "goobox-info-album");
	gtk_label_set_xalign (GTK_LABEL (priv->title1_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (priv->title1_label), 0.5);

	priv->title2_label = gtk_label_new (NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (priv->title2_label), "goobox-info-artist");
	gtk_label_set_xalign (GTK_LABEL (priv->title2_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (priv->title2_label), 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->title2_label), TRUE);

	priv->title3_label = gtk_label_new (NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (priv->title3_label), "goobox-info-track");
	gtk_label_set_xalign (GTK_LABEL (priv->title3_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (priv->title3_label), 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->title3_label), TRUE);

	priv->time_label = gtk_label_new (NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (priv->time_label), "goobox-info-time");
	gtk_label_set_xalign (GTK_LABEL (priv->time_label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (priv->time_label), 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->time_label), TRUE);

	gtk_label_set_ellipsize (GTK_LABEL (priv->title1_label), PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars (GTK_LABEL (priv->title2_label), MIN_CHARS);

	gtk_label_set_ellipsize (GTK_LABEL (priv->title2_label), PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars (GTK_LABEL (priv->title2_label), MIN_CHARS);

	gtk_label_set_ellipsize (GTK_LABEL (priv->title3_label), PANGO_ELLIPSIZE_END);
	gtk_label_set_width_chars (GTK_LABEL (priv->title3_label), MIN_CHARS);

	/* Image */

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

	priv->cover_image = gtk_image_new_from_icon_name (GOO_ICON_NAME_NO_DISC, GTK_ICON_SIZE_DIALOG);
	gtk_widget_set_size_request (priv->cover_image, COVER_SIZE, COVER_SIZE);
	gtk_widget_show (priv->cover_image);

	gtk_container_add (GTK_CONTAINER (priv->cover_button), priv->cover_image);

	/* Status image */

	priv->status_button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->status_button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (priv->status_button),
			  "clicked",
			  G_CALLBACK (status_button_clicked_cb),
			  info);

	priv->status_image = gtk_image_new_from_icon_name (GOO_ICON_NAME_NO_DISC, GTK_ICON_SIZE_DIALOG);
	gtk_widget_set_size_request (priv->status_image, COVER_SIZE, COVER_SIZE);
	gtk_widget_show (priv->cover_image);
	gtk_container_add (GTK_CONTAINER (priv->status_button), priv->status_image);
	/*gtk_container_set_border_width (GTK_CONTAINER (priv->status_image), 6);*/

	/* Frame */

	priv->notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (priv->notebook), 0);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_widget_show (priv->notebook);

	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), priv->status_button, NULL);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), priv->cover_button, NULL);

	priv->cover_frame = gtk_frame_new (NULL);
	gtk_style_context_add_class (gtk_widget_get_style_context (priv->cover_frame), "goobox-cover-frame");
	gtk_container_set_border_width (GTK_CONTAINER (priv->cover_frame), 0);
	gtk_widget_show (priv->cover_frame);
	gtk_container_add (GTK_CONTAINER (priv->cover_frame), priv->notebook);

	gtk_box_pack_start (GTK_BOX (info), priv->cover_frame, FALSE, FALSE, 0);

	/**/

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title1_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title2_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title3_label, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), priv->time_label, FALSE, FALSE, 0);

	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (info), vbox, TRUE, TRUE, 0);
}


static void
goo_player_info_finalize (GObject *object)
{
        GooPlayerInfo *info;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GOO_IS_PLAYER_INFO (object));

	info = GOO_PLAYER_INFO (object);
	g_free (info->priv->total_time);
	if (info->priv->update_id != 0) {
		g_free (info->priv->cover_file);
		g_object_unref (info->priv->original_cover);
		g_source_remove (info->priv->update_id);
		info->priv->update_id = 0;
	}

	G_OBJECT_CLASS (goo_player_info_parent_class)->finalize (object);
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
	GooPlayerInfoPrivate *priv = info->priv;
	GooPlayerState        state;
	AlbumInfo            *album;
	GooPlayer            *player;

	if (info->priv->window == NULL)
		return;

	player = goo_window_get_player (info->priv->window);
	if (player == NULL)
		return;

	state = goo_player_get_state (player);
	album = goo_window_get_album (info->priv->window);

	gtk_label_set_selectable (GTK_LABEL (priv->title1_label), FALSE);
	gtk_label_set_selectable (GTK_LABEL (priv->title2_label), FALSE);
	gtk_label_set_selectable (GTK_LABEL (priv->title3_label), FALSE);

	set_total_time (info, priv->total_time);

	if ((state == GOO_PLAYER_STATE_ERROR) || (state == GOO_PLAYER_STATE_NO_DISC))
	{
		show_simple_text (info, _("No disc"));
	}
	else if (state == GOO_PLAYER_STATE_DATA_DISC) {
		show_simple_text (info, _("Data disc"));
	}
	else {
		if (state == GOO_PLAYER_STATE_EJECTING) {
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
			gtk_label_set_selectable (GTK_LABEL (priv->title1_label), TRUE);

			if (album->artist != NULL) {
				set_title2 (info, album->artist);
				set_title3 (info, year);
				gtk_label_set_selectable (GTK_LABEL (priv->title2_label), TRUE);
			}
			else {
				set_title2 (info, year);
				set_title3 (info, "");
			}
		}
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
	g_free (info->priv->total_time);
	info->priv->total_time = (total_time > 0) ? _g_format_duration_for_display (total_time * 1000) : NULL;
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
	case GOO_PLAYER_ACTION_SEEK_SONG:
		goo_player_info_update_state (info);
		break;
	default:
		break;
	}
}


static void
goo_player_info_set_cover (GooPlayerInfo *info,
			   const char    *cover)
{
	if (cover == NULL)
		return;

	g_clear_object (&info->priv->original_cover);
	if (info->priv->cover_file != NULL) {
		g_free (info->priv->cover_file);
		info->priv->cover_file = NULL;
	}

	if (strcmp (cover, "no-disc") == 0) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (info->priv->notebook), 0);
		gtk_image_set_from_icon_name (GTK_IMAGE (info->priv->status_image),
					      GOO_ICON_NAME_NO_DISC,
					      GTK_ICON_SIZE_DIALOG);
	}
	else if (strcmp (cover, "data-disc") == 0) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (info->priv->notebook), 0);
		gtk_image_set_from_icon_name (GTK_IMAGE (info->priv->status_image),
					      GOO_ICON_NAME_DATA_DISC,
					      GTK_ICON_SIZE_DIALOG);
	}
	else if (strcmp (cover, "audio-cd") == 0) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK (info->priv->notebook), 1);
		gtk_image_set_from_icon_name (GTK_IMAGE (info->priv->cover_image),
					      GOO_ICON_NAME_NO_DISC,
					      GTK_ICON_SIZE_DIALOG);
	}
	else {
		info->priv->cover_file = g_strdup (cover);
		info->priv->original_cover = gdk_pixbuf_new_from_file (cover, NULL);
		if (info->priv->original_cover != NULL) {
			GdkPixbuf *image;

			image = gdk_pixbuf_scale_simple (info->priv->original_cover, COVER_SIZE, COVER_SIZE, GDK_INTERP_BILINEAR);
			gtk_notebook_set_current_page (GTK_NOTEBOOK (info->priv->notebook), 1);
			gtk_image_set_from_pixbuf (GTK_IMAGE (info->priv->cover_image), image);

			g_object_unref (image);
		}
		else
			goo_player_info_set_cover (info, "audio-cd");
	}
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


static void
goo_player_info_class_init (GooPlayerInfoClass *class)
{
        GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = G_OBJECT_CLASS (class);
        gobject_class->finalize = goo_player_info_finalize;

	widget_class = GTK_WIDGET_CLASS (class);
	widget_class->get_preferred_width = goo_player_info_get_preferred_width;

	goo_player_info_signals[COVER_CLICKED] =
		g_signal_new ("cover_clicked",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerInfoClass, cover_clicked),
			      NULL, NULL,
			      goo_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	goo_player_info_signals[UPDATE_STATUS] =
		g_signal_new ("update-status",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GooPlayerInfoClass, update_status),
			      NULL, NULL,
			      goo_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}


static void
goo_player_info_init (GooPlayerInfo *info)
{
	gtk_orientable_set_orientation (GTK_ORIENTABLE (info), GTK_ORIENTATION_HORIZONTAL);
	info->priv = goo_player_info_get_instance_private (info);
}


GtkWidget *
goo_player_info_new (GooWindow *window)
{
	GooPlayerInfo *info;
	GooPlayer     *player;

	g_return_val_if_fail (window != NULL, NULL);

	player = goo_window_get_player (window);
	g_return_val_if_fail (player != NULL, NULL);

	info = GOO_PLAYER_INFO (g_object_new (GOO_TYPE_PLAYER_INFO, NULL));

	info->priv->window = window;
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
			  "state_changed",
			  G_CALLBACK (player_state_changed_cb),
			  info);

	return GTK_WIDGET (info);
}


GdkPixbuf *
goo_player_info_get_cover (GooPlayerInfo *info)
{
	return info->priv->original_cover;
}


const char *
goo_player_info_get_cover_file (GooPlayerInfo *info)
{
	return info->priv->cover_file;
}
