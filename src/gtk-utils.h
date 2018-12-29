/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001-2008 Free Software Foundation, Inc.
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

#ifndef GTK_UTILS_H
#define GTK_UTILS_H

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define _GTK_ICON_NAME_DIALOG_ERROR "dialog-error-symbolic"
#define _GTK_ICON_NAME_DIALOG_INFO "dialog-information-symbolic"
#define _GTK_ICON_NAME_DIALOG_QUESTION "dialog-question-symbolic"
#define _GTK_ICON_NAME_DIALOG_WARNING "dialog-warning-symbolic"

#define _GTK_LABEL_CANCEL _("_Cancel")
#define _GTK_LABEL_CLOSE _("_Close")
#define _GTK_LABEL_OK _("_OK")
#define _GTK_LABEL_OPEN _("_Open")
#define _GTK_LABEL_RESET _("_Reset")

#define GOO_ICON_NAME_PLAY        "media-playback-start-symbolic"
#define GOO_ICON_NAME_PAUSE       "media-playback-pause-symbolic"
#define GOO_ICON_NAME_STOP        "media-playback-stop-symbolic"
#define GOO_ICON_NAME_NEXT        "media-skip-forward-symbolic"
#define GOO_ICON_NAME_PREV        "media-skip-backward-symbolic"
#define GOO_ICON_NAME_EJECT       "media-eject-symbolic"
#define GOO_ICON_NAME_EXTRACT     "document-save-symbolic"
#define GOO_ICON_NAME_RESET       "goo-reset"
#define GOO_ICON_NAME_NO_DISC     "media-optical-symbolic"
#define GOO_ICON_NAME_DATA_DISC   "drive-harddisk-symbolic"
#define GOO_ICON_NAME_AUDIO_CD    "media-optical-symbolic"
#define GOO_ICON_NAME_VOLUME_MAX  "audio-volume-high-symbolic"
#define GOO_ICON_NAME_VOLUME_MED  "audio-volume-medium-symbolic"
#define GOO_ICON_NAME_VOLUME_MIN  "audio-volume-low-symbolic"
#define GOO_ICON_NAME_VOLUME_ZERO "audio-volume-muted-symbolic"


typedef struct {
        const char *action_name;
        const char *accelerator;
} _GtkAccelerator;


GtkWidget *	_gtk_message_dialog_new			(GtkWindow        *parent,
							 GtkDialogFlags    flags,
							 const char       *message,
							 const char       *secondary_message,
							 const char       *first_button_text,
							 ...);
GtkWidget *	_gtk_error_dialog_new			(GtkWindow        *parent,
							 GtkDialogFlags    flags,
							 GList            *row_output,
							 const char       *primary_text,
							 const char       *secondary_text_format,
							 ...) G_GNUC_PRINTF (5, 6);
void		_gtk_error_dialog_from_gerror_run	(GtkWindow        *parent,
							 const char       *title,
							 GError          **gerror);
void		_gtk_error_dialog_from_gerror_show	(GtkWindow        *parent,
							 const char       *title,
							 GError          **gerror);
void		_gtk_dialog_add_to_window_group		(GtkDialog       *dialog);

GdkPixbuf *	_g_icon_get_pixbuf			(GIcon            *icon,
							 int               size,
							 GtkIconTheme     *icon_theme);
GdkPixbuf *	get_mime_type_pixbuf			(const char       *mime_type,
							 int               icon_size,
							 GtkIconTheme     *icon_theme);
int		_gtk_icon_get_pixel_size		(GtkWidget        *widget,
							 GtkIconSize       size);
void		show_help_dialog			(GtkWindow        *parent,
							 const char       *section);
void		_gtk_container_remove_children		(GtkContainer     *container,
							 gpointer          start_after_this,
							 gpointer          stop_before_this);
int		_gtk_container_get_pos			(GtkContainer     *container,
							 GtkWidget        *child);
guint		_gtk_container_get_n_children		(GtkContainer     *container);
GtkBuilder *	_gtk_builder_new_from_file		(const char       *filename);
GtkBuilder *	_gtk_builder_new_from_resource		(const char       *resource_path);
GtkWidget *	_gtk_builder_get_widget			(GtkBuilder       *builder,
							 const char       *name);
GtkWidget *	 _gtk_combo_box_new_with_texts		(const char       *first_text,
							 ...);
void		_gtk_combo_box_append_texts		(GtkComboBox      *combo_box,
							 const char       *first_text,
							 ...);
GtkWidget *	_gtk_image_new_from_xpm_data		(char             *xpm_data[]);
void		_gtk_tree_path_list_free		(GList            *list);
int		_gtk_paned_get_position2		(GtkPaned         *paned);
void		_gtk_paned_set_position2		(GtkPaned         *paned,
							 int               pos);
void		_g_launch_command			(GtkWidget		*parent,
							 const char		*command,
							 const char		*name,
							 GList			*files);
int		_gtk_count_selected			(GtkTreeSelection	*selection);
void		_gtk_window_add_accelerator_for_action	(GtkWindow		*window,
							 GtkAccelGroup		*accel_group,
							 const char		*action_name,
							 const char		*accel,
							 GVariant		*target);
void		_gtk_window_add_accelerators_from_menu	(GtkWindow		*window,
							 GMenuModel		*menu);
void		_g_action_map_enable_action		(GActionMap		*action_map,
							 const char		*action_name,
							 gboolean		 enabled);
void		_g_action_map_set_action_state		(GActionMap		*action_map,
							 const char		*action_name,
							 gboolean		 active);
void		_g_action_map_change_action_state	(GActionMap		*action_map,
							 const char		*action_name,
							 gboolean		 value);
GtkWidget *	_gtk_application_get_current_window	(GtkApplication		*application);
void		_gtk_application_add_accelerator_for_action
							(GtkApplication		*app,
							 const char		*action_name,
							 const char		*accel);
void		_gtk_application_add_accelerators	(GtkApplication		*app,
							 const _GtkAccelerator	*accelerators,
							 int			 n_accelerators);

gboolean	_gtk_window_get_monitor_info		(GtkWindow		*window,
							 GdkRectangle		*geometry,
							 int			*number,
							 char			**name);
gboolean	_gtk_widget_get_monitor_geometry	(GtkWidget		*widget,
							 GdkRectangle		*geometry);
gboolean	_gtk_settings_get_dialogs_use_header	(void);

G_END_DECLS

#endif
