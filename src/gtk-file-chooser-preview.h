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

#ifndef GTK_FILE_CHOOSER_PREVIEW_H
#define GTK_FILE_CHOOSER_PREVIEW_H

#include <gtk/gtk.h>

#define GTK_TYPE_FILE_CHOOSER_PREVIEW              (gtk_file_chooser_preview_get_type ())
#define GTK_FILE_CHOOSER_PREVIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_CHOOSER_PREVIEW, GtkFileChooserPreview))
#define GTK_FILE_CHOOSER_PREVIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_CHOOSER_PREVIEW, GtkFileChooserPreviewClass))
#define GTK_IS_FILE_CHOOSER_PREVIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_CHOOSER_PREVIEW))
#define GTK_IS_FILE_CHOOSER_PREVIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_CHOOSER_PREVIEW))
#define GTK_FILE_CHOOSER_PREVIEW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_FILE_CHOOSER_PREVIEW, GtkFileChooserPreviewClass))

typedef struct _GtkFileChooserPreview            GtkFileChooserPreview;
typedef struct _GtkFileChooserPreviewClass       GtkFileChooserPreviewClass;
typedef struct _GtkFileChooserPreviewPrivateData GtkFileChooserPreviewPrivateData;

struct _GtkFileChooserPreview
{
	GtkFrame __parent;
	GtkFileChooserPreviewPrivateData *priv;
};

struct _GtkFileChooserPreviewClass
{
	GtkFrameClass __parent_class;
};

GType         gtk_file_chooser_preview_get_type (void);
GtkWidget *   gtk_file_chooser_preview_new      (void);
void          gtk_file_chooser_preview_set_uri  (GtkFileChooserPreview *player,
						 const char            *uri);
const char *  gtk_file_chooser_preview_get_uri  (GtkFileChooserPreview *player);

#endif /* GTK_FILE_CHOOSER_PREVIEW_H */
