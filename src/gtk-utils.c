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
_gtk_message_dialog_new (GtkWindow        *parent,
			 GtkDialogFlags    flags,
			 const char       *icon_name,
			 const char       *message,
			 const char       *secondary_message,
			 const char       *first_button_text,
			 ...)
{
	GtkBuilder  *builder;
	GtkWidget   *dialog;
	GtkWidget   *label;
	va_list      args;
	const gchar *text;
	int          response_id;
	char        *markup_text;

	builder = _gtk_builder_new_from_resource ("message-dialog.ui");
	dialog = _gtk_builder_get_widget (builder, "message_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	gtk_window_set_modal (GTK_WINDOW (dialog), (flags & GTK_DIALOG_MODAL));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), (flags & GTK_DIALOG_DESTROY_WITH_PARENT));
	g_object_set_data_full (G_OBJECT (dialog), "builder", builder, g_object_unref);

	if (flags & GTK_DIALOG_MODAL)
		_gtk_dialog_add_to_window_group (GTK_DIALOG (dialog));

	/* set the icon */

	gtk_image_set_from_icon_name (GTK_IMAGE (_gtk_builder_get_widget (builder, "icon_image")),
				      icon_name,
				      GTK_ICON_SIZE_DIALOG);

	/* set the message */

	label = _gtk_builder_get_widget (builder, "message_label");

	if (message != NULL) {
		char *escaped_message;

		escaped_message = g_markup_escape_text (message, -1);
		if (secondary_message != NULL) {
			char *escaped_secondary_message;

			escaped_secondary_message = g_markup_escape_text (secondary_message, -1);
			markup_text = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
						       escaped_message,
						       escaped_secondary_message);

			g_free (escaped_secondary_message);
		}
		else
			markup_text = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>", escaped_message);

		g_free (escaped_message);
	}
	else
		markup_text = g_markup_escape_text (secondary_message, -1);

	gtk_label_set_markup (GTK_LABEL (label), markup_text);
	g_free (markup_text);

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


/* -- _gtk_ok_dialog_with_checkbutton_new -- */


typedef struct {
	GSettings *settings;
	char      *key;
} DialogWithButtonData;


static void
dialog_with_button_data_free (DialogWithButtonData *data)
{
	_g_object_unref (data->settings);
	g_free (data->key);
	g_free (data);
}


static void
ok__check_button_toggled_cb (GtkToggleButton      *button,
			     DialogWithButtonData *data)
{
	g_settings_set_boolean (data->settings, data->key, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}


GtkWidget*
_gtk_ok_dialog_with_checkbutton_new (GtkWindow        *parent,
				     GtkDialogFlags    flags,
				     const char       *message,
				     const char       *ok_button_text,
				     const char       *check_button_label,
				     GSettings        *settings,
				     const char       *key)
{
	GtkWidget		*d;
	GtkBuilder              *builder;
	GtkWidget		*check_button;
	DialogWithButtonData	*data;

	d = _gtk_message_dialog_new (parent,
				     flags,
				     _GTK_ICON_NAME_DIALOG_INFO,
				     message,
				     NULL,
				     ok_button_text, GTK_RESPONSE_OK,
				     NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_OK);

	/* setup the checkbutton */

	builder = g_object_get_data (G_OBJECT (d), "builder");
	check_button = _gtk_builder_get_widget(builder, "message_checkbutton");
	gtk_container_add (GTK_CONTAINER (check_button), gtk_label_new_with_mnemonic (check_button_label));
	gtk_widget_show_all (check_button);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), g_settings_get_boolean (settings, key));

	data = g_new0 (DialogWithButtonData, 1);
	data->settings = g_object_ref (settings);
	data->key = g_strdup (key);
	g_object_set_data_full (G_OBJECT (d), "settings-data", data, (GDestroyNotify) dialog_with_button_data_free);

	g_signal_connect (G_OBJECT (check_button),
			  "toggled",
			  G_CALLBACK (ok__check_button_toggled_cb),
			  data);

	return d;
}


void
_gtk_error_dialog_from_gerror_run (GtkWindow   *parent,
				   const char  *title,
				   GError     **gerror)
{
	GtkWidget *d;

	g_return_if_fail (*gerror != NULL);

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_DESTROY_WITH_PARENT,
				     _GTK_ICON_NAME_DIALOG_ERROR,
				     title,
				     (*gerror)->message,
				     _GTK_LABEL_OK, GTK_RESPONSE_OK,
				     NULL);
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

	d = _gtk_message_dialog_new (parent,
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     _GTK_ICON_NAME_DIALOG_ERROR,
				     title,
				     (gerror != NULL) ? (*gerror)->message : NULL,
				     _GTK_LABEL_OK, GTK_RESPONSE_OK,
				     NULL);
	g_signal_connect (d, "response", G_CALLBACK (error_dialog_response_cb), NULL);

	gtk_window_present (GTK_WINDOW (d));

	if (gerror != NULL)
		g_clear_error (gerror);
}



void
_gtk_error_dialog_run (GtkWindow        *parent,
		       const gchar      *format,
		       ...)
{
	GtkWidget *d;
	char      *message;
	va_list    args;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d =  _gtk_message_dialog_new (parent,
				      GTK_DIALOG_MODAL,
				      _GTK_ICON_NAME_DIALOG_ERROR,
				      message,
				      NULL,
				      _GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
}


void
_gtk_info_dialog_run (GtkWindow        *parent,
		      const gchar      *format,
		      ...)
{
	GtkWidget *d;
	char      *message;
	va_list    args;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d =  _gtk_message_dialog_new (parent,
				      GTK_DIALOG_MODAL,
				      _GTK_ICON_NAME_DIALOG_INFO,
				      message,
				      NULL,
				      _GTK_LABEL_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
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

		dialog = _gtk_message_dialog_new (parent,
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  _GTK_ICON_NAME_DIALOG_ERROR,
						  _("Could not display help"),
						  error->message,
						  _GTK_LABEL_OK, GTK_RESPONSE_OK,
						  NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

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
