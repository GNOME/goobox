/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include <math.h>
#include <string.h>
#include "gtk-utils.h"
#include "glib-utils.h"


#define REQUEST_ENTRY_WIDTH 220
#define RESOURCE_UI_PATH "/org/gnome/Goobox/ui/"


GtkWidget *
_gtk_message_dialog_new (GtkWindow      *parent,
			 GtkDialogFlags  flags,
			 const char     *message,
			 const char     *secondary_message,
			 const gchar    *first_button_text,
			 ...)
{
	GtkWidget   *dialog;
	va_list      args;
	const gchar *text;
	int          response_id;

	dialog = gtk_message_dialog_new (parent,
					 flags,
					 GTK_MESSAGE_OTHER,
					 GTK_BUTTONS_NONE,
					 "%s", message);

	if (secondary_message != NULL)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", secondary_message);

	if (flags & GTK_DIALOG_MODAL)
		_gtk_dialog_add_to_window_group (GTK_DIALOG (dialog));

	/* add the buttons */

	if (first_button_text == NULL)
		return dialog;

	va_start (args, first_button_text);

	text = first_button_text;
	response_id = va_arg (args, gint);

	while (text != NULL) {
		gtk_dialog_add_button (GTK_DIALOG (dialog), text, response_id);

		text = va_arg (args, char*);
		if (text == NULL)
			break;
		response_id = va_arg (args, int);
	}

	va_end (args);

	return dialog;
}


GtkWidget *
_gtk_error_dialog_new (GtkWindow      *parent,
		       GtkDialogFlags  flags,
		       GList          *row_output,
		       const char     *primary_text,
		       const char     *secondary_text_format,
		       ...)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent,
					 flags,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 "%s", primary_text);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

	if (flags & GTK_DIALOG_MODAL)
		_gtk_dialog_add_to_window_group (GTK_DIALOG (dialog));

	/* label */

	if (secondary_text_format != NULL) {
		va_list  args;
		char    *secondary_message;

		va_start (args, secondary_text_format);
		secondary_message = g_strdup_vprintf (secondary_text_format, args);
		va_end (args);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", secondary_message);

		g_free (secondary_message);
	}

	/* output */

	if ((row_output != NULL) && (secondary_text_format == NULL)) {
		GtkWidget     *output_box;
		GtkWidget     *label;
		GtkWidget     *scrolled_window;
		GtkWidget     *text_view;
		GtkTextBuffer *text_buffer;
		GtkTextIter    iter;
		GList         *scan;

		output_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
		gtk_box_pack_end (GTK_BOX (gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (dialog))),
				  output_box, TRUE, TRUE, 0);

		label = gtk_label_new_with_mnemonic (_("C_ommand Line Output:"));
		gtk_box_pack_start (GTK_BOX (output_box), label, FALSE, FALSE, 0);

		scrolled_window = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
						"shadow-type", GTK_SHADOW_IN,
						"width-request", 450,
						"height-request", 200,
						NULL);
		gtk_box_pack_start (GTK_BOX (output_box), scrolled_window, TRUE, TRUE, 0);

		text_view = gtk_text_view_new ();
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), text_view);
		gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);

		text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
		gtk_text_buffer_create_tag (text_buffer, "monospace",
					    "family", "monospace",
					    NULL);
		gtk_text_buffer_get_iter_at_offset (text_buffer, &iter, 0);
		for (scan = row_output; scan; scan = scan->next) {
			char  *line = scan->data;
			char  *utf8_line;
			gsize  bytes_written;

			utf8_line = g_locale_to_utf8 (line, -1, NULL, &bytes_written, NULL);
			gtk_text_buffer_insert_with_tags_by_name (text_buffer,
								  &iter,
								  utf8_line,
								  bytes_written,
								  "monospace", NULL);
			g_free (utf8_line);

			gtk_text_buffer_insert (text_buffer, &iter, "\n", 1);
		}

		gtk_widget_show_all (output_box);
	}

	return dialog;
}


void
_gtk_error_dialog_from_gerror_run (GtkWindow   *parent,
				   const char  *title,
				   GError     **gerror)
{
	GtkWidget *d;

	g_return_if_fail (*gerror != NULL);

	d = _gtk_error_dialog_new (parent,
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   NULL,
				   title,
				   "%s",
				   (*gerror)->message);
	gtk_dialog_run (GTK_DIALOG (d));

	gtk_widget_destroy (d);
	g_clear_error (gerror);
}


static void
error_dialog_response_cb (GtkDialog *dialog,
			  int        response,
			  gpointer   user_data)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}


void
_gtk_error_dialog_from_gerror_show (GtkWindow   *parent,
				    const char  *title,
				    GError     **gerror)
{
	GtkWidget *d;

	d = _gtk_error_dialog_new (parent,
				   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				   NULL,
				   title,
				   (gerror != NULL) ? "%s" : NULL,
				   (*gerror)->message);
	g_signal_connect (d, "response", G_CALLBACK (error_dialog_response_cb), NULL);

	gtk_window_present (GTK_WINDOW (d));

	if (gerror != NULL)
		g_clear_error (gerror);
}


void
_gtk_dialog_add_to_window_group (GtkDialog *dialog)
{
	GtkWidget *toplevel;

	g_return_if_fail (dialog != NULL);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (dialog));
	if (gtk_widget_is_toplevel (toplevel) && gtk_window_has_group (GTK_WINDOW (toplevel)))
		gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)), GTK_WINDOW (dialog));
}


static GdkPixbuf *
get_themed_icon_pixbuf (GThemedIcon  *icon,
			int           size,
			GtkIconTheme *icon_theme)
{
	char        **icon_names;
	GtkIconInfo  *icon_info;
	GdkPixbuf    *pixbuf;
	GError       *error = NULL;

	g_object_get (icon, "names", &icon_names, NULL);

	icon_info = gtk_icon_theme_choose_icon (icon_theme, (const char **)icon_names, size, 0);
	if (icon_info == NULL)
		icon_info = gtk_icon_theme_lookup_icon (icon_theme, "text-x-generic", size, GTK_ICON_LOOKUP_USE_BUILTIN);

	pixbuf = gtk_icon_info_load_icon (icon_info, &error);
	if (pixbuf == NULL) {
		g_warning ("could not load icon pixbuf: %s\n", error->message);
		g_clear_error (&error);
	}

	g_object_unref (icon_info);
	g_strfreev (icon_names);

	return pixbuf;
}


static GdkPixbuf *
get_file_icon_pixbuf (GFileIcon *icon,
		      int        size)
{
	GFile     *file;
	char      *filename;
	GdkPixbuf *pixbuf;

	file = g_file_icon_get_file (icon);
	filename = g_file_get_path (file);
	pixbuf = gdk_pixbuf_new_from_file_at_size (filename, size, -1, NULL);
	g_free (filename);
	g_object_unref (file);

	return pixbuf;
}


static gboolean
scale_keeping_ratio_min (int      *width,
			 int      *height,
			 int       min_width,
			 int       min_height,
			 int       max_width,
			 int       max_height,
			 gboolean  allow_upscaling)
{
	double   w = *width;
	double   h = *height;
	double   min_w = min_width;
	double   min_h = min_height;
	double   max_w = max_width;
	double   max_h = max_height;
	double   factor;
	int      new_width, new_height;
	gboolean modified;

	if ((*width < max_width) && (*height < max_height) && ! allow_upscaling)
		return FALSE;

	if (((*width < min_width) || (*height < min_height)) && ! allow_upscaling)
		return FALSE;

	factor = MAX (MIN (max_w / w, max_h / h), MAX (min_w / w, min_h / h));
	new_width  = MAX ((int) floor (w * factor + 0.50), 1);
	new_height = MAX ((int) floor (h * factor + 0.50), 1);

	modified = (new_width != *width) || (new_height != *height);

	*width = new_width;
	*height = new_height;

	return modified;
}


static gboolean
scale_keeping_ratio (int      *width,
		     int      *height,
		     int       max_width,
		     int       max_height,
		     gboolean  allow_upscaling)
{
	return scale_keeping_ratio_min (width,
					height,
					0,
					0,
					max_width,
					max_height,
					allow_upscaling);
}


GdkPixbuf *
_g_icon_get_pixbuf (GIcon        *icon,
		    int           size,
		    GtkIconTheme *theme)
{
	GdkPixbuf *pixbuf;
	int        w, h;

	if (icon == NULL)
		return NULL;

	if (G_IS_THEMED_ICON (icon))
		pixbuf = get_themed_icon_pixbuf (G_THEMED_ICON (icon), size, theme);
	if (G_IS_FILE_ICON (icon))
		pixbuf = get_file_icon_pixbuf (G_FILE_ICON (icon), size);

	if (pixbuf == NULL)
		return NULL;

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	if (scale_keeping_ratio (&w, &h, size, size, FALSE)) {
		GdkPixbuf *scaled;

		scaled = gdk_pixbuf_scale_simple (pixbuf, w, h, GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled;
	}

	return pixbuf;
}


GdkPixbuf *
get_mime_type_pixbuf (const char   *mime_type,
		      int           icon_size,
		      GtkIconTheme *icon_theme)
{
	GdkPixbuf *pixbuf = NULL;
	GIcon     *icon;

	if (icon_theme == NULL)
		icon_theme = gtk_icon_theme_get_default ();

	icon = g_content_type_get_icon (mime_type);
	pixbuf = _g_icon_get_pixbuf (icon, icon_size, icon_theme);
	g_object_unref (icon);

	return pixbuf;
}


int
_gtk_icon_get_pixel_size (GtkWidget   *widget,
			  GtkIconSize  size)
{
	int icon_width, icon_height;

	gtk_icon_size_lookup (size, &icon_width, &icon_height);

	return MAX (icon_width, icon_height);
}


void
show_help_dialog (GtkWindow  *parent,
		  const char *section)
{
	char   *uri;
	GError *error = NULL;

	uri = g_strconcat ("help:goobox", section ? "?" : NULL, section, NULL);
	if (! gtk_show_uri_on_window (parent, uri, GDK_CURRENT_TIME, &error)) {
  		GtkWidget *dialog;

		dialog = _gtk_error_dialog_new (parent,
						GTK_DIALOG_DESTROY_WITH_PARENT,
						NULL,
						_("Could not display help"),
						"%s",
						error->message);

		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_widget_show (dialog);

		g_clear_error (&error);
	}
	g_free (uri);
}


void
_gtk_container_remove_children (GtkContainer *container,
				gpointer      start_after_this,
				gpointer      stop_before_this)
{
	GList *children;
	GList *scan;

	children = gtk_container_get_children (container);

	if (start_after_this != NULL) {
		for (scan = children; scan; scan = scan->next)
			if (scan->data == start_after_this) {
				scan = scan->next;
				break;
			}
	}
	else
		scan = children;

	for (/* void */; scan && (scan->data != stop_before_this); scan = scan->next)
		gtk_container_remove (container, scan->data);
	g_list_free (children);
}


int
_gtk_container_get_pos (GtkContainer *container,
			GtkWidget    *child)
{
	GList    *children;
	gboolean  found = FALSE;
	int       idx;
	GList    *scan;

	children = gtk_container_get_children (container);
	for (idx = 0, scan = children; ! found && scan; idx++, scan = scan->next) {
		if (child == scan->data) {
			found = TRUE;
			break;
		}
	}
	g_list_free (children);

	return ! found ? -1 : idx;
}


guint
_gtk_container_get_n_children (GtkContainer *container)
{
	GList *children;
	guint  len;

	children = gtk_container_get_children (container);
	len = g_list_length (children);
	g_list_free (children);

	return len;
}


GtkBuilder *
_gtk_builder_new_from_file (const char *ui_file)
{
	char       *filename;
	GtkBuilder *builder;
	GError     *error = NULL;

	filename = g_build_filename (GOO_UIDIR, ui_file, NULL);
	builder = gtk_builder_new ();
	if (! gtk_builder_add_from_file (builder, filename, &error)) {
		g_warning ("%s\n", error->message);
		g_clear_error (&error);
	}
	g_free (filename);

	return builder;
}


GtkBuilder *
_gtk_builder_new_from_resource (const char *resource_path)
{
	GtkBuilder *builder;
	char       *full_path;
	GError     *error = NULL;

	builder = gtk_builder_new ();
	full_path = g_strconcat (RESOURCE_UI_PATH, resource_path, NULL);
	if (! gtk_builder_add_from_resource (builder, full_path, &error)) {
		g_warning ("%s\n", error->message);
		g_clear_error (&error);
	}
	g_free (full_path);

	return builder;
}


GtkWidget *
_gtk_builder_get_widget (GtkBuilder *builder,
			 const char *name)
{
	return (GtkWidget *) gtk_builder_get_object (builder, name);
}


GtkWidget *
_gtk_combo_box_new_with_texts (const char *first_text,
			       ...)
{
	GtkWidget  *combo_box;
	va_list     args;
	const char *text;

	combo_box = gtk_combo_box_text_new ();

	va_start (args, first_text);

	text = first_text;
	while (text != NULL) {
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo_box), NULL, text);
		text = va_arg (args, const char *);
	}

	va_end (args);

	return combo_box;
}


void
_gtk_combo_box_append_texts (GtkComboBox *combo_box,
			     const char  *first_text,
			     ...)
{
	va_list     args;
	const char *text;

	va_start (args, first_text);

	text = first_text;
	while (text != NULL) {
		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combo_box), NULL, text);
		text = va_arg (args, const char *);
	}

	va_end (args);
}


GtkWidget *
_gtk_image_new_from_xpm_data (char * xpm_data[])
{
	GdkPixbuf *pixbuf;
	GtkWidget *image;

	pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) xpm_data);
	image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_widget_show (image);

	g_object_unref (G_OBJECT (pixbuf));

	return image;
}


void
_gtk_tree_path_list_free (GList *list)
{
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}


int
_gtk_paned_get_position2 (GtkPaned *paned)
{
	GtkRequisition requisition;
	int            pos;
	int            size;

	if (! gtk_widget_get_visible (GTK_WIDGET (paned)))
		return 0;

	pos = gtk_paned_get_position (paned);

	gtk_window_get_size (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (paned))), &(requisition.width), &(requisition.height));
	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (paned)) == GTK_ORIENTATION_HORIZONTAL)
		size = requisition.width;
	else
		size = requisition.height;

	return size - pos;
}


void
_gtk_paned_set_position2 (GtkPaned *paned,
			  int       pos)
{
	GtkWidget     *top_level;
	GtkAllocation  allocation;
	int            size;

	top_level = gtk_widget_get_toplevel (GTK_WIDGET (paned));
	if (! gtk_widget_is_toplevel (top_level))
		return;
	gtk_widget_get_allocation (top_level, &allocation);
	if (gtk_orientable_get_orientation (GTK_ORIENTABLE (paned)) == GTK_ORIENTATION_HORIZONTAL)
		size = allocation.width;
	else
		size = allocation.height;

	if (pos > 0)
		gtk_paned_set_position (paned, size - pos);
}


void
_g_launch_command (GtkWidget  *parent,
		   const char *command,
		   const char *name,
		   GList      *files)
{
	GError              *error = NULL;
	GAppInfo            *app_info;
	GdkAppLaunchContext *launch_context;

	app_info = g_app_info_create_from_commandline (command, name, G_APP_INFO_CREATE_SUPPORTS_URIS, &error);
	if (app_info == NULL) {
		_gtk_error_dialog_from_gerror_show(GTK_WINDOW (parent), _("Could not launch the application"), &error);
		return;
	}

	launch_context = gdk_display_get_app_launch_context (gtk_widget_get_display (parent));
	if (! g_app_info_launch (app_info, files, G_APP_LAUNCH_CONTEXT (launch_context), &error)) {
		_gtk_error_dialog_from_gerror_show(GTK_WINDOW (parent), _("Could not launch the application"), &error);
		return;
	}

	g_object_unref (launch_context);
	g_object_unref (app_info);
}


static void
count_selected (GtkTreeModel *model,
		GtkTreePath  *path,
		GtkTreeIter  *iter,
		gpointer      data)
{
	int *n = data;
	*n = *n + 1;
}


int
_gtk_count_selected (GtkTreeSelection *selection)
{
	int n;

	if (selection == NULL)
		return 0;

	n = 0;
	gtk_tree_selection_selected_foreach (selection, count_selected, &n);

	return n;
}


/* -- _gtk_window_add_accelerator_for_action -- */


typedef struct {
	GtkWindow *window;
	char      *action_name;
	GVariant  *target;
} AccelData;


static void
accel_data_free (gpointer  user_data,
                 GClosure *closure)
{
	AccelData *accel_data = user_data;

	g_return_if_fail (accel_data != NULL);

	if (accel_data->target != NULL)
		g_variant_unref (accel_data->target);
	g_free (accel_data->action_name);
	g_free (accel_data);
}


static void
window_accelerator_activated_cb (GtkAccelGroup  *accel_group,
				 GObject                *object,
				 guint           key,
				 GdkModifierType         mod,
				 gpointer                user_data)
{
	AccelData *accel_data = user_data;
	GAction   *action;

	action = g_action_map_lookup_action (G_ACTION_MAP (accel_data->window), accel_data->action_name);
	if (action != NULL)
		g_action_activate (action, accel_data->target);
}


void
_gtk_window_add_accelerator_for_action (GtkWindow       *window,
					GtkAccelGroup   *accel_group,
					const char      *action_name,
					const char      *accel,
					GVariant        *target)
{
	AccelData       *accel_data;
	guint            key;
	GdkModifierType  mods;
	GClosure        *closure;

	if ((action_name == NULL) || (accel == NULL))
		return;

	if (g_str_has_prefix (action_name, "app."))
		return;

	accel_data = g_new0 (AccelData, 1);
	accel_data->window = window;
	/* remove the win. prefix from the action name */
	if (g_str_has_prefix (action_name, "win."))
		accel_data->action_name = g_strdup (action_name + strlen ("win."));
	else
		accel_data->action_name = g_strdup (action_name);
	if (target != NULL)
		accel_data->target = g_variant_ref (target);

	gtk_accelerator_parse (accel, &key, &mods);
	closure = g_cclosure_new (G_CALLBACK (window_accelerator_activated_cb),
				  accel_data,
				  accel_data_free);
	gtk_accel_group_connect (accel_group,
				 key,
				 mods,
				 0,
				 closure);
}


/* -- _gtk_window_add_accelerators_from_menu --  */


static void
add_accelerators_from_menu_item (GtkWindow      *window,
				 GtkAccelGroup  *accel_group,
				 GMenuModel     *model,
				 int             item)
{
	GMenuAttributeIter      *iter;
	const char              *key;
	GVariant                *value;
	const char              *accel = NULL;
	const char              *action = NULL;
	GVariant                *target = NULL;

	iter = g_menu_model_iterate_item_attributes (model, item);
	while (g_menu_attribute_iter_get_next (iter, &key, &value)) {
               	if (g_str_equal (key, "action") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
               		action = g_variant_get_string (value, NULL);
               	else if (g_str_equal (key, "accel") && g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
               		accel = g_variant_get_string (value, NULL);
               	else if (g_str_equal (key, "target"))
               		target = g_variant_ref (value);
               	g_variant_unref (value);
	}
	g_object_unref (iter);

	_gtk_window_add_accelerator_for_action (window,
						accel_group,
						action,
						accel,
						target);

       	if (target != NULL)
       		g_variant_unref (target);
}

static void
add_accelerators_from_menu (GtkWindow      *window,
			    GtkAccelGroup  *accel_group,
			    GMenuModel     *model)
{
	int              i;
	GMenuLinkIter   *iter;
	const char      *key;
	GMenuModel      *m;

	for (i = 0; i < g_menu_model_get_n_items (model); i++) {
		add_accelerators_from_menu_item (window, accel_group, model, i);

		iter = g_menu_model_iterate_item_links (model, i);
		while (g_menu_link_iter_get_next (iter, &key, &m)) {
			add_accelerators_from_menu (window, accel_group, m);
			g_object_unref (m);
		}
		g_object_unref (iter);
	}
}


void
_gtk_window_add_accelerators_from_menu (GtkWindow  *window,
					GMenuModel *menu)
{
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new ();
	add_accelerators_from_menu (window, accel_group, menu);
	gtk_window_add_accel_group (window, accel_group);
}


void
_g_action_map_enable_action (GActionMap *action_map,
			     const char *action_name,
			     gboolean    enabled)
{
	GAction *action;

	action = g_action_map_lookup_action (action_map, action_name);
	g_return_if_fail (action != NULL);

	g_object_set (action, "enabled", enabled, NULL);
}


void
_g_action_map_set_action_state (GActionMap *action_map,
				const char *action_name,
				gboolean    active)
{
        GAction *action;

        action = g_action_map_lookup_action (action_map, action_name);
        g_return_if_fail (action != NULL);

        g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (active));
}


void
_g_action_map_change_action_state (GActionMap *action_map,
				   const char *action_name,
				   gboolean    value)
{
        GAction  *action;
        GVariant *old_state;
        GVariant *new_state;

        action = g_action_map_lookup_action (action_map, action_name);
        g_return_if_fail (action != NULL);

        old_state = g_action_get_state (action);
        new_state = g_variant_new_boolean (value);
        if ((old_state == NULL) || ! g_variant_equal (old_state, new_state))
                g_action_change_state (action, new_state);

        if (old_state != NULL)
                g_variant_unref (old_state);
}


GtkWidget *
_gtk_application_get_current_window (GtkApplication *application)
{
	GList *windows;

	windows = gtk_application_get_windows (application);
	if (windows == NULL)
		return NULL;

	return GTK_WIDGET (windows->data);
}


void
_gtk_application_add_accelerator_for_action (GtkApplication   *app,
					     const char       *action_name,
					     const char       *accel)
{
	const char *accels[2];

	accels[0] = accel;
	accels[1] = NULL;
	gtk_application_set_accels_for_action (app, action_name, accels);
}


void
_gtk_application_add_accelerators (GtkApplication        *app,
				   const _GtkAccelerator *accelerators,
				   int                    n_accelerators)
{
	int i;

	for (i = 0; i < n_accelerators; i++) {
		const _GtkAccelerator *acc = accelerators + i;
		_gtk_application_add_accelerator_for_action (GTK_APPLICATION (app),
							     acc->action_name,
							     acc->accelerator);
	}
}


gboolean
_gtk_window_get_monitor_info (GtkWindow	    *window,
			      GdkRectangle  *geometry,
			      int           *number,
			      char         **name)
{
#if GTK_CHECK_VERSION(3, 22, 0)

	GdkWindow  *win;
	GdkMonitor *monitor;

	win = gtk_widget_get_window (GTK_WIDGET (window));
	if (win == NULL)
		return FALSE;

	monitor = gdk_display_get_monitor_at_window (gdk_window_get_display (win), win);
	if (monitor == NULL)
		return FALSE;

	if (geometry != NULL)
		gdk_monitor_get_geometry (monitor, geometry);

	if ((number != NULL) || (name != NULL)) {
		GdkDisplay *display;
		int         monitor_num;
		const char *monitor_name;
		int         i;

		display = gdk_monitor_get_display (monitor);
		monitor_num = 0;
		for (i = 0; /* void */; i++) {
			GdkMonitor *m = gdk_display_get_monitor (display, i);
			if (m == monitor) {
				monitor_num = i;
				monitor_name = gdk_monitor_get_model (monitor);
				break;
			}
			if (m == NULL)
				break;
		}

		if (number != NULL) *number = monitor_num;
		if (name != NULL) *name = g_strdup (monitor_name);
	}

#else

	GdkWindow *win;
	GdkScreen *screen;
	int        monitor_num;

	win = gtk_widget_get_window (GTK_WIDGET (window));
	if (win == NULL)
		return FALSE;

	screen = gdk_window_get_screen (win);
	if (screen == NULL)
		return FALSE;

	monitor_num = gdk_screen_get_monitor_at_window (screen, win);
	if (number != NULL)
		*number = monitor_num;
	if (geometry != NULL)
		gdk_screen_get_monitor_geometry (screen, monitor_num, geometry);
	if (name != NULL)
		*name = gdk_screen_get_monitor_plug_name (screen, monitor_num);

#endif

	return TRUE;
}


gboolean
_gtk_widget_get_monitor_geometry (GtkWidget    *widget,
				  GdkRectangle *geometry)
{
	gboolean   result = FALSE;
	GtkWidget *window;

	window = gtk_widget_get_toplevel (widget);
	if (GTK_IS_WINDOW (window)) {
		if (_gtk_window_get_monitor_info (GTK_WINDOW (window), geometry, NULL, NULL)) {
			result = TRUE;
		}
	}

	return result;
}


gboolean
_gtk_settings_get_dialogs_use_header (void)
{
	gboolean use_header;

	g_object_get (gtk_settings_get_default (),
		      "gtk-dialogs-use-header", &use_header,
		      NULL);

	return use_header;
}
