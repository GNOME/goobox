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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 *  Author: Paolo Bacchilega
 */

#include <config.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-thumbnail.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include "gtk-file-chooser-preview.h"

#define _(String) gettext (String)

#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif

#define MIN_WIDTH 150
#define PREVIEW_SIZE 120


struct _GtkFileChooserPreviewPrivateData {
	char                  *uri;
	GnomeThumbnailFactory *thumb_factory;
	GtkWidget             *image;
	GtkWidget             *image_info;
	GdkWindow             *bin_window;
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
gtk_file_chooser_preview_size_request (GtkWidget      *widget,
				       GtkRequisition *requisition)
{	
	if (GTK_WIDGET_CLASS (parent_class)->size_request)
		(* GTK_WIDGET_CLASS (parent_class)->size_request) (widget, requisition);
	requisition->width = MAX (requisition->width, MIN_WIDTH);
}


static void
gtk_file_chooser_preview_style_set (GtkWidget *widget,
				    GtkStyle  *prev_style)
{
	GtkWidget *event_box;

	GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

	event_box = gtk_bin_get_child (GTK_BIN (widget));
	
	gtk_widget_modify_bg (event_box, GTK_STATE_NORMAL,
			      &widget->style->base[GTK_STATE_NORMAL]);
	gtk_widget_modify_bg (event_box, GTK_STATE_INSENSITIVE,
			      &widget->style->base[GTK_STATE_NORMAL]);
}


static void 
gtk_file_chooser_preview_class_init (GtkFileChooserPreviewClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class;

        parent_class = g_type_class_peek_parent (class);

        gobject_class->finalize = gtk_file_chooser_preview_finalize;

	widget_class = (GtkWidgetClass*) class;
	widget_class->size_request = gtk_file_chooser_preview_size_request;
	widget_class->style_set = gtk_file_chooser_preview_style_set;
}


static void 
gtk_file_chooser_preview_init (GtkFileChooserPreview *preview)
{
	GtkFileChooserPreviewPrivateData *priv;

	preview->priv = g_new0 (GtkFileChooserPreviewPrivateData, 1);
	priv = preview->priv;

	priv->thumb_factory = gnome_thumbnail_factory_new (GNOME_THUMBNAIL_SIZE_NORMAL);
	priv->uri = NULL;
}


static void 
gtk_file_chooser_preview_finalize (GObject *object)
{
        GtkFileChooserPreview *preview;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GTK_IS_FILE_CHOOSER_PREVIEW (object));

	preview = GTK_FILE_CHOOSER_PREVIEW (object);
	if (preview->priv != NULL) {
		GtkFileChooserPreviewPrivateData *priv = preview->priv;

		if (priv->thumb_factory != NULL)
			g_object_unref (priv->thumb_factory);

		g_free (priv->uri);
		priv->uri = NULL;

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
	gtk_widget_set_sensitive (GTK_BIN (preview)->child, FALSE);
}


static void
gtk_file_chooser_preview_construct (GtkFileChooserPreview  *preview)
{
	GtkFileChooserPreviewPrivateData *priv = preview->priv;
	GtkWidget *event_box;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *vbox2;

	event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (preview), event_box);
	gtk_widget_show (event_box);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (event_box), vbox);
	gtk_widget_show (vbox);

	button = gtk_button_new ();
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	label = gtk_label_new_with_mnemonic (_("Preview"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
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

	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, FALSE, 0);

	priv->image = gtk_image_new ();
	gtk_widget_show (priv->image);
	gtk_box_pack_start (GTK_BOX (vbox2), priv->image, FALSE, FALSE, 0);

	priv->image_info = gtk_label_new (NULL);
	gtk_widget_hide (priv->image_info);
	gtk_box_pack_start (GTK_BOX (vbox2), priv->image_info, FALSE, FALSE, 0);

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


static GnomeVFSFileInfo*
vfs_get_file_info (const gchar *path)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult    result;
	char             *escaped;

	if (! path || ! *path) return 0; 

	info = gnome_vfs_file_info_new ();
	escaped = gnome_vfs_escape_path_string (path);
	result = gnome_vfs_get_file_info (escaped, 
					  info, 
					  (GNOME_VFS_FILE_INFO_DEFAULT | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));
	g_free (escaped);

	if (result != GNOME_VFS_OK) {
		gnome_vfs_file_info_unref (info);
		info = NULL;
	}

	return info;
}


void
gtk_file_chooser_preview_set_uri (GtkFileChooserPreview *preview,
				  const char            *uri)
{
	GtkFileChooserPreviewPrivateData *priv = preview->priv;
	GnomeVFSFileInfo  *info;
	GdkPixbuf         *pixbuf;
	char              *thumb_uri;

	if (uri == NULL)

	g_free (priv->uri);
	priv->uri = NULL;

	if (uri == NULL) {
		set_void_preview (preview);
		return;
	}

	priv->uri = g_strdup (uri);
	info = vfs_get_file_info (uri);

	if (info == NULL) {
		set_void_preview (preview);
		return;
	}

	thumb_uri = gnome_thumbnail_factory_lookup (priv->thumb_factory,
						    uri,
						    info->mtime);
	if (thumb_uri != NULL) 
		pixbuf = gdk_pixbuf_new_from_file (thumb_uri, NULL);
	else {
		char *mime_type = gnome_vfs_get_mime_type (uri);

		pixbuf = gnome_thumbnail_factory_generate_thumbnail (priv->thumb_factory, uri, mime_type);
		if (pixbuf != NULL)
			gnome_thumbnail_factory_save_thumbnail (priv->thumb_factory, pixbuf, uri, info->mtime);
		else
			gnome_thumbnail_factory_create_failed_thumbnail (priv->thumb_factory, uri, info->mtime);

		g_free (mime_type);
	}

	if (pixbuf != NULL) {
		char *text;

		text = gnome_vfs_format_file_size_for_display (info->size); 
		gtk_label_set_text (GTK_LABEL (priv->image_info), text);
		g_free (text);

		gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), pixbuf); 
		
		g_object_unref (pixbuf);
		gtk_widget_show (priv->image);
		gtk_widget_show (priv->image_info);

		gtk_widget_set_sensitive (GTK_BIN (preview)->child, TRUE);

	} else 
		set_void_preview (preview);

	gnome_vfs_file_info_unref (info);
}


const char *
gtk_file_chooser_preview_get_uri (GtkFileChooserPreview *preview)
{
	return preview->priv->uri;
}
