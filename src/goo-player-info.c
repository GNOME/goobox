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
#include "goo-player-info.h"
#include "goo-marshal.h"
#include "glib-utils.h"
#include "goo-stock.h"

#define SPACING 0
#define TITLE1_FORMAT "<span size='large' weight='bold'>%s</span>"
#define TITLE2_FORMAT "%s"
#define TITLE3_FORMAT "<i>%s</i>"
#define TIME_FORMAT "%s"
#define PLAYING_FORMAT "%s"
#define SCALE_WIDTH 150
#define COVER_SIZE 80
#define MIN_WIDTH 400

struct _GooPlayerInfoPrivateData {
	GooPlayer   *player;
	GtkWidget   *title1_label;
	GtkWidget   *title2_label;
	GtkWidget   *title3_label;
	GtkWidget   *time_label;
	GtkWidget   *playing_label;
	GtkWidget   *time_scale;
	GtkWidget   *cover_image;
	GtkWidget   *cover_button;
	GtkTooltips *tips;
	char         current_time[64];
	char         total_time[64];
	char         time[64];
	gint64       song_length;
	gboolean     update_slider;
};

enum {
	COVER_CLICKED,
	SKIP_TO,
        LAST_SIGNAL
};

static GtkHBoxClass *parent_class = NULL;
static guint goo_player_info_signals[LAST_SIGNAL] = { 0 };

static void goo_player_info_class_init  (GooPlayerInfoClass *class);
static void goo_player_info_init        (GooPlayerInfo *player);
static void goo_player_info_finalize    (GObject *object);


GType
goo_player_info_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GooPlayerInfoClass),
			NULL,
			NULL,
			(GClassInitFunc) goo_player_info_class_init,
			NULL,
			NULL,
			sizeof (GooPlayerInfo),
			0,
			(GInstanceInitFunc) goo_player_info_init
		};

		type = g_type_register_static (GTK_TYPE_HBOX,
					       "GooPlayerInfo",
					       &type_info,
					       0);
	}

        return type;
}


static void
goo_player_info_size_request (GtkWidget      *widget,
			      GtkRequisition *requisition)
{	
	if (GTK_WIDGET_CLASS (parent_class)->size_request)
		(* GTK_WIDGET_CLASS (parent_class)->size_request) (widget, requisition);
	requisition->width = MAX (requisition->width, MIN_WIDTH);
}


static void 
goo_player_info_class_init (GooPlayerInfoClass *class)
{
        GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

        parent_class = g_type_class_peek_parent (class);

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

	widget_class = (GtkWidgetClass*) class;
	widget_class->size_request = goo_player_info_size_request;
}


static void
set_label (GtkWidget     *label,
	   const char    *format,
	   const char    *text)
{
	char *markup;
	markup = g_strdup_printf (format, text);
	gtk_label_set_markup (GTK_LABEL (label), markup);
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
	double new_value = gtk_range_get_value (range);
	int    seconds = (int) (new_value * info->priv->song_length);
	g_signal_emit (info, goo_player_info_signals[SKIP_TO], 0, seconds);
}


static void
time_scale_enter_notify_cb (GtkRange         *range,
			    GdkEventCrossing *event,
			    GooPlayerInfo    *info)
{
	info->priv->update_slider = FALSE;
}


static void
time_scale_leave_notify_cb (GtkRange         *range,
			    GdkEventCrossing *event,
			    GooPlayerInfo    *info)
{
	info->priv->update_slider = TRUE;
}


static void
cover_button_clicked_cb (GtkWidget     *button,
			 GooPlayerInfo *info)
{
	g_signal_emit (info, goo_player_info_signals[COVER_CLICKED], 0);
}


static void 
goo_player_info_init (GooPlayerInfo *info)
{
	GooPlayerInfoPrivateData *priv;
	GtkWidget *vbox, *time_box;

	info->priv = g_new0 (GooPlayerInfoPrivateData, 1);
	priv = info->priv;

	priv->update_slider = TRUE;
	priv->current_time[0] = '\0';
	priv->total_time[0] = '\0';

	GTK_BOX (info)->spacing = SPACING;
	GTK_BOX (info)->homogeneous = FALSE;

	priv->tips = gtk_tooltips_new ();
	gtk_object_ref (GTK_OBJECT (priv->tips));
	gtk_object_sink (GTK_OBJECT (priv->tips));

	/* Title and Artist */

	priv->title1_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->title1_label), 0.0, 0.5);

	priv->title2_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->title2_label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->title2_label), TRUE);

	priv->title3_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->title3_label), 0.0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (priv->title3_label), TRUE);

	/* time */

	time_box = gtk_hbox_new (FALSE, 6);

	priv->time_label = gtk_label_new (NULL);
	gtk_widget_set_no_show_all (priv->time_label, TRUE);

	priv->time_scale = gtk_hscale_new_with_range (0.0, 1.0, 0.01);
	gtk_range_set_increments (GTK_RANGE (priv->time_scale), 0.01, 0.1);
	gtk_scale_set_draw_value (GTK_SCALE (priv->time_scale), FALSE);
	gtk_widget_set_size_request (priv->time_scale, SCALE_WIDTH, -1);
	gtk_range_set_update_policy (GTK_RANGE (priv->time_scale), GTK_UPDATE_DISCONTINUOUS);
	gtk_widget_set_no_show_all (priv->time_scale, TRUE);

	priv->playing_label = gtk_label_new (NULL);
	gtk_widget_set_no_show_all (priv->playing_label, TRUE);

	gtk_box_pack_start (GTK_BOX (time_box), priv->time_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (time_box), priv->time_scale, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (time_box), priv->playing_label, FALSE, FALSE, 0);

	/* Image */

	priv->cover_button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->cover_button),
			       GTK_RELIEF_NONE);
	gtk_tooltips_set_tip (priv->tips,
			      GTK_WIDGET (priv->cover_button),
			      _("Click here to choose a cover for this CD"),
			      NULL);
	g_signal_connect (G_OBJECT (priv->cover_button),
			  "clicked",
			  G_CALLBACK (cover_button_clicked_cb),
			  info);

	priv->cover_image = gtk_image_new_from_stock (GOO_STOCK_NO_COVER, GTK_ICON_SIZE_DIALOG);
	gtk_widget_set_size_request (priv->cover_image, COVER_SIZE, COVER_SIZE);
	gtk_widget_show (priv->cover_image);
	gtk_container_add (GTK_CONTAINER (priv->cover_button), priv->cover_image);

	gtk_box_pack_start (GTK_BOX (info), priv->cover_button, FALSE, FALSE, 0);

	/*FIXME
	gtk_box_pack_start (GTK_BOX (info), 
			    gtk_vseparator_new (),
			    FALSE, FALSE, 0);
	*/

	/**/

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title1_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title2_label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->title3_label, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), time_box, FALSE, FALSE, 0);

	gtk_widget_show_all (vbox);
	gtk_box_pack_start (GTK_BOX (info), vbox, TRUE, TRUE, 0);

	/**/

	set_title1 (info, "");
	set_title2 (info, "");
	set_title3 (info, "");

	g_signal_connect (priv->time_scale, 
			  "value_changed",
			  G_CALLBACK (time_scale_value_changed_cb), 
			  info);
	g_signal_connect (priv->time_scale, 
			  "enter-notify-event",
			  G_CALLBACK (time_scale_enter_notify_cb), 
			  info);
	g_signal_connect (priv->time_scale, 
			  "leave-notify-event",
			  G_CALLBACK (time_scale_leave_notify_cb), 
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
		if (info->priv->player != NULL)
			g_object_unref (info->priv->player);
		gtk_object_unref (GTK_OBJECT (info->priv->tips));
		g_free (info->priv);
		info->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


GtkWidget *
goo_player_info_new (GooPlayer *player)
{
	GooPlayerInfo *info;
	
	info = GOO_PLAYER_INFO (g_object_new (GOO_TYPE_PLAYER_INFO, NULL));
	if (player != NULL) {
		g_object_ref (player);
		info->priv->player = player;
        }

	return GTK_WIDGET (info);
}


void
goo_player_info_set_player (GooPlayerInfo  *info,
			    GooPlayer      *player)
{
	if (info->priv->player != NULL) {
		g_object_unref (info->priv->player);
		info->priv->player = NULL;
	}

	if (player != NULL) {
		g_object_ref (player);
		info->priv->player = player;
	}
}


static void
update_subtitle (GooPlayerInfo  *info,
		 SongInfo       *song)
{
	GooPlayerInfoPrivateData *priv = info->priv;
	const char *title, *subtitle;

	title = goo_player_get_title (priv->player);
	subtitle = goo_player_get_subtitle (priv->player);

	if ((title == NULL) || (subtitle == NULL)) {
		set_time_string (priv->total_time, song->time);
		set_title2 (info, priv->total_time);
	} else {
		set_title2 (info, subtitle);
		set_title3 (info, title);
		gtk_label_set_selectable (GTK_LABEL (priv->title2_label), TRUE);
		gtk_label_set_selectable (GTK_LABEL (priv->title3_label), TRUE);
	}
}


void
goo_player_info_update_state (GooPlayerInfo  *info)
{
	GooPlayerInfoPrivateData *priv = info->priv;
	GooPlayerState state;

	if (priv->player == NULL)
		return;

	state = goo_player_get_state (priv->player);

	if ((state == GOO_PLAYER_STATE_PLAYING) 
	    || (state == GOO_PLAYER_STATE_PAUSED)) {
		gtk_widget_show (priv->time_scale);
		gtk_widget_show (priv->time_label);
		gtk_widget_show (priv->playing_label);
	} else {
		gtk_widget_hide (priv->time_scale);
		gtk_widget_hide (priv->time_label);
		gtk_widget_hide (priv->playing_label);
	}

	gtk_label_set_selectable (GTK_LABEL (priv->title1_label), FALSE);
	gtk_label_set_selectable (GTK_LABEL (priv->title2_label), FALSE);
	gtk_label_set_selectable (GTK_LABEL (priv->title3_label), FALSE);

	if (state == GOO_PLAYER_STATE_ERROR) {
		GError *error = goo_player_get_error (priv->player);
		set_title1 (info, error->message);
		set_title2 (info, "");
		set_title3 (info, "");
		g_error_free (error);

	} else {
		SongInfo *song = goo_player_get_song (priv->player, goo_player_get_current_song (priv->player));

		if (song != NULL) {
			char *state_s = "";

			priv->song_length = song->time;

			if (state == GOO_PLAYER_STATE_PAUSED)
				state_s = _("Paused");
			set_playing (info, state_s);

			set_title1 (info, song->title);
			gtk_label_set_selectable (GTK_LABEL (priv->title1_label), TRUE);

			update_subtitle (info, song);

			song_info_unref (song);

		} else if (state == GOO_PLAYER_STATE_EJECTING) {
			set_title1 (info, _("Ejecting CD"));
			set_title2 (info, "");
			
		} else if (state == GOO_PLAYER_STATE_UPDATING) {
			set_title1 (info, _("Checking CD drive"));
			set_title2 (info, "");
			
		} else if (state == GOO_PLAYER_STATE_SEEKING) {
			set_title1 (info, _("Reading CD"));
			set_title2 (info, "");

		} else if (state == GOO_PLAYER_STATE_LISTING) {
			set_title1 (info, _("Reading CD"));
			set_title2 (info, "");

		}  else {
			const char *title, *subtitle;

			title = goo_player_get_title (priv->player);
			subtitle = goo_player_get_subtitle (priv->player);

			if (title != NULL) {
				set_title1 (info, title);
				gtk_label_set_selectable (GTK_LABEL (priv->title1_label), TRUE);
			} else
				set_title1 (info, _("Audio CD"));

			if (subtitle != NULL) {
				set_title2 (info, subtitle);
				set_title3 (info, priv->total_time);
				gtk_label_set_selectable (GTK_LABEL (priv->title2_label), TRUE);
			} else {
				set_title2 (info, priv->total_time);
				set_title3 (info, "");
			}
		}
	}
}


void
goo_player_info_set_total_time (GooPlayerInfo  *info,
				gint64          total_time)
{
	GooPlayerInfoPrivateData *priv = info->priv;

	set_time_string (priv->total_time, total_time);
	goo_player_info_update_state (info);
}


void
goo_player_info_set_time (GooPlayerInfo  *info,
			  gint64          current_time)
{
	GooPlayerInfoPrivateData *priv = info->priv;

	set_time_string (priv->current_time, current_time);
	sprintf (priv->time, "%s", priv->current_time);
	set_time (info, priv->time);

	if (priv->update_slider) {
		g_signal_handlers_block_by_data (priv->time_scale, info);
		gtk_range_set_value (GTK_RANGE (priv->time_scale), (double) current_time / priv->song_length);
		g_signal_handlers_unblock_by_data (priv->time_scale, info);
	}
}


void
goo_player_info_set_sensitive (GooPlayerInfo  *info,
			       gboolean        value)
{
	gtk_widget_set_sensitive (info->priv->cover_button, value);
}


void
goo_player_info_set_cover (GooPlayerInfo  *info,
			   GdkPixbuf      *cover)
{
	if (cover != NULL)
		gtk_image_set_from_pixbuf (GTK_IMAGE (info->priv->cover_image), cover);
	else
		gtk_image_set_from_stock (GTK_IMAGE (info->priv->cover_image), GOO_STOCK_NO_COVER, GTK_ICON_SIZE_DIALOG);
}
