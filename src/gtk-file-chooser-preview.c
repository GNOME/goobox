/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  gtk-file-chooser-preview: image preview widget for the GtkFileChooser.
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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *  Author: Paolo Bacchilega
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "gnome-desktop-thumbnail.h"
#include "gtk-file-chooser-preview.h"


#define MIN_WIDTH 150
#define PREVIEW_SIZE 120


struct _GtkFileChooserPreviewPrivateData {
	char       *uri;
	GnomeDesktopThumbnailFactory
	           *thumb_factory;
	GtkWidget  *image;
	GtkWidget  *image_info;
	GdkWindow  *bin_window;
};


static GtkFrameClass *parent_class = NULL;

static void gtk_file_chooser_preview_class_init  (GtkFileChooserPreviewClass *class);
static void gtk_file_chooser_preview_init        (GtkFileChooserPreview *preview);
static void gtk_file_chooser_preview_finalize    (GObject *object);


GType
gtk_file_chooser_preview_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GtkFileChooserPreviewClass),
			NULL,
			NULL,
			(GClassInitFunc) gtk_file_chooser_preview_class_init,
			NULL,
			NULL,
			sizeof (GtkFileChooserPreview),
			0,
			(GInstanceInitFunc) gtk_file_chooser_preview_init
		};

		type = g_type_register_static (GTK_TYPE_FRAME,
					       "GtkFileChooserPreview",
					       &type_info,
					       0);
	}

        return type;
}


static void
gtk_file_chooser_preview_get_preferred_width (GtkWidget *widget,
					      int       *minimal_width,
					      int       *natural_width)
{
	*minimal_width = *natural_width = MIN_WIDTH;
}


static void
gtk_file_chooser_preview_class_init (GtkFileChooserPreviewClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class;

        parent_class = g_type_class_peek_parent (class);

        gobject_class->finalize = gtk_file_chooser_preview_finalize;

	widget_class = (GtkWidgetClass*) class;
	widget_class->get_preferred_width = gtk_file_chooser_preview_get_preferred_width;
}


static void
gtk_file_chooser_preview_init (GtkFileChooserPreview *preview)
{
	preview->priv = g_new0 (GtkFileChooserPreviewPrivateData, 1);
	preview->priv->thumb_factory = gnome_desktop_thumbnail_factory_new (GNOME_DESKTOP_THUMBNAIL_SIZE_NORMAL);
	preview->priv->uri = NULL;
}


static void
gtk_file_chooser_preview_finalize (GObject *object)
{
        GtkFileChooserPreview *preview;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GTK_IS_FILE_CHOOSER_PREVIEW (object));

	preview = GTK_FILE_CHOOSER_PREVIEW (object);
	if (preview->priv != NULL) {
		_g_object_unref (preview->priv->thumb_factory);
		g_free (preview->priv->uri);

		g_free (preview->priv);
		preview->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
set_void_preview (GtkFileChooserPreview *preview)
{
	gtk_widget_hide (preview->priv->image);
	gtk_widget_hide (preview->priv->image_info);
	gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (preview)), FALSE);
}


static void
gtk_file_chooser_preview_construct (GtkFileChooserPreview  *preview)
{
	GtkWidget *event_box;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *vbox2;

	event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (preview), event_box);
	gtk_widget_show (event_box);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (event_box), vbox);
	gtk_widget_show (vbox);

	button = gtk_button_new ();
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	label = gtk_label_new_with_mnemonic (_("Preview"));
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_label_set_yalign (GTK_LABEL (label), 0.5);
	gtk_container_add (GTK_CONTAINER (button), label);
	gtk_widget_show (label);

	g_signal_connect (button, "button_press_event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (button, "button_release_event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (button, "enter_notify_event",
			  G_CALLBACK (gtk_true),
			  NULL);
	g_signal_connect (button, "leave_notify_event",
			  G_CALLBACK (gtk_true),
			  NULL);

	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, FALSE, 0);

	preview->priv->image = gtk_image_new ();
	gtk_widget_show (preview->priv->image);
	gtk_box_pack_start (GTK_BOX (vbox2), preview->priv->image, FALSE, FALSE, 0);

	preview->priv->image_info = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (preview->priv->image_info), GTK_JUSTIFY_CENTER);
	gtk_label_set_xalign (GTK_LABEL (preview->priv->image_info), 0.5);
	gtk_label_set_yalign (GTK_LABEL (preview->priv->image_info), 0.5);
	gtk_widget_hide (preview->priv->image_info);
	gtk_box_pack_start (GTK_BOX (vbox2), preview->priv->image_info, FALSE, FALSE, 0);

	set_void_preview (preview);
}


GtkWidget *
gtk_file_chooser_preview_new (void)
{
	GtkFileChooserPreview  *preview;

	preview = GTK_FILE_CHOOSER_PREVIEW (g_object_new (GTK_TYPE_FILE_CHOOSER_PREVIEW,
							  "shadow", GTK_SHADOW_IN,
							  NULL));
	gtk_file_chooser_preview_construct (preview);
	return (GtkWidget*) preview;
}


void
gtk_file_chooser_preview_set_uri (GtkFileChooserPreview *preview,
				  const char            *uri)
{
	GFile     *file;
	GFileInfo *info;
	GTimeVal   timeval;
	GdkPixbuf *pixbuf;
	char      *thumb_uri;

	g_free (preview->priv->uri);
	preview->priv->uri = NULL;
	if (uri == NULL) {
		set_void_preview (preview);
		return;
	}

	preview->priv->uri = g_strdup (uri);
	file = g_file_new_for_uri (uri);
	info = g_file_query_info (file, "time::*,standard::content-type,standard::size", G_FILE_QUERY_INFO_NONE, NULL, NULL);
	if (info == NULL) {
		set_void_preview (preview);
		g_object_unref (file);
		return;
	}

	g_file_info_get_modification_time (info, &timeval);

	thumb_uri = gnome_desktop_thumbnail_factory_lookup (preview->priv->thumb_factory,
							    uri,
							    timeval.tv_sec);
	if (thumb_uri != NULL) {
		pixbuf = gdk_pixbuf_new_from_file (thumb_uri, NULL);
	}
	else {
		pixbuf = gnome_desktop_thumbnail_factory_generate_thumbnail (preview->priv->thumb_factory, uri, g_file_info_get_content_type (info));
		if (pixbuf != NULL)
			gnome_desktop_thumbnail_factory_save_thumbnail (preview->priv->thumb_factory, pixbuf, uri, timeval.tv_sec);
		else
			gnome_desktop_thumbnail_factory_create_failed_thumbnail (preview->priv->thumb_factory, uri, timeval.tv_sec);
	}

	if (pixbuf != NULL) {
		const char *w, *h;
		char       *size_text, *text;

		w = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Width");
		h = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Height");
		size_text = g_format_size (g_file_info_get_size (info));
		text = g_strconcat (size_text,
				    ((w == NULL)?NULL:"\n"),
				    w, " x ", h, " ", _("pixels"),
				    NULL);
		g_free (size_text);

		gtk_label_set_markup (GTK_LABEL (preview->priv->image_info), text);
		g_free (text);

		gtk_image_set_from_pixbuf (GTK_IMAGE (preview->priv->image), pixbuf);

		g_object_unref (pixbuf);
		gtk_widget_show (preview->priv->image);
		gtk_widget_show (preview->priv->image_info);

		gtk_widget_set_sensitive (gtk_bin_get_child (GTK_BIN (preview)), TRUE);
	}
	else
		set_void_preview (preview);

	g_object_unref (info);
}


const char *
gtk_file_chooser_preview_get_uri (GtkFileChooserPreview *preview)
{
	return preview->priv->uri;
}
