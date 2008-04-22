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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glade/glade.h>
#include "typedefs.h"
#include "main.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "goo-window.h"
#include "goo-stock.h"
#include "gth-image-list.h"
#include "preferences.h"
#include "gconf-utils.h"

#define GLADE_CHOOSER_FILE "cover_chooser.glade"
#define MAX_IMAGES 20
#define READ_TIMEOUT 20
#define BUFFER_SIZE 612
#define IMG_PERM 0600
#define DIR_PERM 0700
#define THUMB_SIZE 100
#define THUMB_BORDER 14
#define QUERY_RESULT "result.html"
#define ORIGINAL_COVER "original_cover.png"

typedef struct _DialogData DialogData;

typedef void (*FileSavedFunc) (DialogData *data, const char *filename, gboolean success);

struct _DialogData {
	GooWindow           *window;
	char                *artist;
	char                *album;
	GList               *url_list;
	GList               *current;
	guint                load_id, urls, url;
	char                *tmpdir;
	GList               *tmpfiles;
	char                *cover_backup;
	int                  max_images;
	gboolean             autofetching;

	FileSavedFunc        file_saved_func;
	char                *source;
	char                *dest;

	GnomeVFSAsyncHandle *vfs_handle;
	GnomeVFSResult       vfs_result;

	GladeXML            *gui;

	GtkWidget           *dialog;
	GtkWidget           *image_list;
	GtkWidget           *progress_label;
	GtkWidget           *ok_button;
	GtkWidget           *revert_button;
	GtkWidget           *cc_cancel_search_button;
};


static void
cancel_search (DialogData *data)
{
	if (data->vfs_handle != NULL) {
		gnome_vfs_async_cancel (data->vfs_handle);
		data->vfs_handle = NULL;
	}

	if (data->load_id != 0) {
		g_source_remove (data->load_id);
		data->load_id = 0;
	}
}


static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	cancel_search (data);
	
	if (data->tmpdir != NULL) {
		g_list_foreach (data->tmpfiles, (GFunc) gnome_vfs_unlink, NULL);
		path_list_free (data->tmpfiles);

		gnome_vfs_remove_directory (data->tmpdir);
		g_free (data->tmpdir);
	}

	g_free (data->dest);
	g_free (data->source);
	g_free (data->album);
	g_free (data->artist);
	path_list_free (data->url_list);
	if (data->gui != NULL)
		g_object_unref (data->gui);
	g_free (data);
}


/* -- copy_file_from_url() -- */


static int
copy_progress_update_cb (GnomeVFSAsyncHandle      *handle,
			 GnomeVFSXferProgressInfo *info,
			 gpointer                  callback_data)
{
	DialogData *data = callback_data;

	if (info->status != GNOME_VFS_XFER_PROGRESS_STATUS_OK) {
		data->vfs_result = info->status;
		return FALSE;
	} 
	if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) {
		debug (DEBUG_INFO, "COMPLETED");
	
		if (data->file_saved_func != NULL)
			(*data->file_saved_func) (data, data->dest, (data->vfs_result == GNOME_VFS_OK));
	}

	return TRUE;
}


static void
copy_file_from_url (DialogData    *data,
		    const char    *uri_source,
		    const char    *uri_dest,
		    FileSavedFunc  file_saved_func)
{
	GList                     *src_uri_list, *dest_uri_list;
	GnomeVFSXferOptions        xfer_options;
	GnomeVFSXferErrorMode      xfer_error_mode;
	GnomeVFSXferOverwriteMode  overwrite_mode;
	GnomeVFSResult             result;

	data->file_saved_func = file_saved_func;
	
	g_free (data->dest);
	data->dest = g_strdup (uri_dest);

	src_uri_list = g_list_prepend (NULL, gnome_vfs_uri_new (uri_source));
	dest_uri_list = g_list_prepend (NULL, gnome_vfs_uri_new (uri_dest));

	xfer_options    = GNOME_VFS_XFER_DEFAULT;
	xfer_error_mode = GNOME_VFS_XFER_ERROR_MODE_ABORT;
	overwrite_mode  = GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE;

	data->vfs_result = GNOME_VFS_OK;

	result = gnome_vfs_async_xfer (&data->vfs_handle,
				       src_uri_list,
				       dest_uri_list,
				       xfer_options,
				       xfer_error_mode,
				       overwrite_mode,
				       GNOME_VFS_PRIORITY_DEFAULT,
				       copy_progress_update_cb,
				       data,
				       NULL,
				       NULL);

	g_list_foreach (src_uri_list, (GFunc) gnome_vfs_uri_unref, NULL);
	g_list_free (src_uri_list);

	g_list_foreach (dest_uri_list, (GFunc) gnome_vfs_uri_unref, NULL);
	g_list_free (dest_uri_list);

	/**/

	if (result != GNOME_VFS_OK) {
		if (file_saved_func != NULL)
			(*file_saved_func) (data, uri_dest, FALSE);
	}
}


static void load_current_url (DialogData *data);


static gboolean
load_next_url (gpointer callback_data)
{
	DialogData *data = callback_data;

	g_source_remove (data->load_id);
	data->load_id = 0;

	data->current = data->current->next;
	load_current_url (data);

	return FALSE;
}


static void
append_image (DialogData *data,
	      const char *filename)
{
	GdkPixbuf *image;
	int        pos;

	image = gdk_pixbuf_new_from_file_at_size (filename, 
						  THUMB_SIZE, THUMB_SIZE, 
						  NULL);
	if (image == NULL)
		return;

	pos = gth_image_list_append (GTH_IMAGE_LIST (data->image_list),
				     image,
				     filename,
				     NULL);
	gth_image_list_set_image_data_full (GTH_IMAGE_LIST (data->image_list),
					    pos,
					    g_strdup (filename),
					    g_free);
	g_object_unref (image);
}


static void
image_saved_cb (DialogData *data,
		const char *filename,
		gboolean    success)
{
	if (success) {
		char *tmpfile = g_strdup (filename);

		debug (DEBUG_INFO, "LOAD IMAGE: %s\n", tmpfile);

		data->tmpfiles = g_list_prepend (data->tmpfiles, tmpfile);
		append_image (data, tmpfile);
	}

	data->load_id = g_idle_add (load_next_url, data);
}


static void
update_progress_label (DialogData *data)
{
	char *text;

	if (data->url < data->urls)
		text = g_strdup_printf (_("%u, loading image: %u"), 
					data->urls, 
					data->url + 1);
	else
		text = g_strdup_printf ("%u", data->urls);
	gtk_label_set_text (GTK_LABEL (data->progress_label), text);
	g_free (text);
}


static void
search_completed (DialogData *data)
{
	char *text;
	
	gtk_widget_set_sensitive (data->cc_cancel_search_button, FALSE);
	
	text = g_strdup_printf ("%u", data->urls);
	gtk_label_set_text (GTK_LABEL (data->progress_label), text);
	g_free (text);
}


static void
load_current_url (DialogData *data)
{
	char *url, *dest;
	char *filename;

	update_progress_label (data);

	if ((data->current == NULL) || (data->url >= data->max_images)) {
		search_completed (data);
		return;
	}

	url = data->current->data;

	debug (DEBUG_INFO, "LOAD %s\n", url);

	filename = g_strdup_printf ("%d.png", data->url);
	dest = g_build_filename (data->tmpdir, filename, NULL);
	g_free (filename);

	copy_file_from_url (data, url, dest, image_saved_cb);
	data->url++;
}


static void
start_loading_images (DialogData *data)
{
	gth_image_list_set_no_image_text (GTH_IMAGE_LIST (data->image_list),
					  _("Loading images"));

	data->current = data->url_list;
	load_current_url (data);
}


static gboolean
make_file_list_from_search_result (DialogData *data,
				   const char *filename)
{
	int       fd, n;
	char      buf[BUFFER_SIZE];
	int       buf_offset = 0;
	GString  *partial_url;
	gboolean  done = FALSE;
	int       urls = 0;

	fd = open (filename, O_RDONLY);
	if (fd == 0) 
		return FALSE;
	
	partial_url = NULL;
	while ((n = read (fd, buf + buf_offset, BUFFER_SIZE - buf_offset - 1)) > 0) {
		const char *prefix = "/images?q=tbn:";
		int         prefix_len = strlen (prefix);
		char       *url_start;
		gboolean    copy_tail = TRUE;

		buf[buf_offset+n] = 0;

		if (partial_url == NULL) 
			url_start = strstr (buf, prefix);
		else
			url_start = buf;

		while (url_start != NULL) {
			char *url_end;
			
			url_end = strstr (url_start, " ");
		
			if (url_end == NULL) {
				if (partial_url == NULL)
					partial_url = g_string_new (url_start);
				else 
					g_string_append (partial_url, url_start);
				url_start = NULL;
				copy_tail = FALSE;
			}
			else {
				char *url_tail = g_strndup (url_start, url_end - url_start);
				char *url;
				char *complete_url;
				
				if (partial_url != NULL) {
					g_string_append (partial_url, url_tail);
					g_free (url_tail);
					url = partial_url->str;
					g_string_free (partial_url, FALSE);
					partial_url = NULL;
				} 
				else 
					url = url_tail;
					
				complete_url = g_strconcat ("http://images.google.com", url, NULL);
				g_free (url);

				data->url_list = g_list_prepend (data->url_list, complete_url);
				urls++;
				
				if (urls >= data->max_images) {
					done = TRUE;
					break;
				}

				url_start = strstr (url_end + 1, prefix);
			}
		}

		if (done)
			break;

		if (copy_tail) {
			prefix_len = MIN (prefix_len, buf_offset + n);
			strncpy (buf, 
				 buf + buf_offset + n - prefix_len, 
				 prefix_len);
			buf_offset = prefix_len;
		} 
		else 
			buf_offset = 0;
	}

	if (partial_url != NULL)
		g_string_free (partial_url, TRUE);

	close (fd);
	data->tmpfiles = g_list_prepend (data->tmpfiles, g_strdup (filename));

	data->url_list = g_list_reverse (data->url_list);
	data->urls = urls;
	data->url = 0;

	return (data->url_list != NULL);
}


static void
search_result_saved_cb (DialogData *data,
			const char *filename,
			gboolean    success)
{
	if (! success) {
		_gtk_error_dialog_run (GTK_WINDOW (data->dialog),
				       _("Could not search for a cover on Internet"),
				       gnome_vfs_result_to_string (data->vfs_result));
		return;
	}

	if (! make_file_list_from_search_result (data, filename)) {
		gth_image_list_set_no_image_text (GTH_IMAGE_LIST (data->image_list),
						  _("No image found"));
		return;
	}

	start_loading_images (data);
}


static char*
get_query (DialogData *data)
{
	char *s, *e, *q;

	s = g_strdup_printf ("%s %s", data->album, data->artist);
	e = gnome_vfs_escape_string (s);
	q = g_strconcat ("http://images.google.com/images?q=", e, NULL);
	g_free (e);
	g_free (s);
	
	return q;
}


static gboolean
start_searching (gpointer callback_data)
{
	DialogData *data = callback_data;
	char       *query;
	char       *dest;

	g_source_remove (data->load_id);
	data->load_id = 0;

	query = get_query (data);
	dest = g_build_filename (data->tmpdir, QUERY_RESULT, NULL);
	copy_file_from_url (data, query, dest, search_result_saved_cb);

	g_free (dest);
	g_free (query);

	return FALSE;
}


/* callbacks */


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("goobox", "search_cover_on_internet", &err);
	
	if (err != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
						 0,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Could not display help: %s"),
						 err->message);
		
		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		
		gtk_widget_show (dialog);
		
		g_error_free (err);
	}
}


static void
revert_cb (GtkWidget  *widget, 
	   DialogData *data)
{
	char *original_cover;

	if (data->cover_backup == NULL) 
		return;

	original_cover = goo_window_get_cover_filename (data->window);
	file_copy (data->cover_backup, original_cover);
	goo_window_update_cover (data->window);
}


/* called when the "ok" button is clicked. */
static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	GthImageList *list = GTH_IMAGE_LIST (data->image_list);
	GList        *selection;

	selection =  gth_image_list_get_selection (list);
	if (selection != NULL) {
		char *src = selection->data;

		debug (DEBUG_INFO, "SET COVER: %s\n", src);

		if (! data->autofetching 
		    || goo_window_get_current_cd_autofetch (data->window))
			goo_window_set_cover_image (data->window, src);

		g_list_free (selection);
	}
}


static void
image_list_selection_changed_cb (GthImageList *list,
				 DialogData   *data)
{
	GList *selection;

	selection = gth_image_list_get_selection (list);
	gtk_widget_set_sensitive (data->ok_button, selection != NULL);
	g_list_free (selection);
}


static void
image_list_item_activated_cb (GthImageList *list,
			      int           pos,
			      DialogData   *data)
{
	ok_cb (NULL, data);
}


static void
backup_cover_image (DialogData   *data)
{
	GnomeVFSURI *uri;
	char        *original_cover;
	char        *cover_backup;

	original_cover = goo_window_get_cover_filename (data->window);
	if (original_cover == NULL) {	
		gtk_widget_set_sensitive (data->revert_button, FALSE);
		return;
	}

	uri = new_uri_from_path (original_cover);
	if (uri == NULL) {
		gtk_widget_set_sensitive (data->revert_button, FALSE);
		return;
	}
	gtk_widget_set_sensitive (data->revert_button, gnome_vfs_uri_exists (uri));
	gnome_vfs_uri_unref (uri);

	cover_backup = g_build_filename (data->tmpdir, ORIGINAL_COVER, NULL);
	file_copy (original_cover, cover_backup);
	g_free (original_cover);

	data->tmpfiles = g_list_prepend (data->tmpfiles, cover_backup);
	data->cover_backup = cover_backup;
}


static void
cancel_search_button_clicked_cb (GtkWidget  *widget, 
       				 DialogData *data)
{
	cancel_search (data);
	search_completed (data);	
}


void
dlg_cover_chooser (GooWindow  *window,
		   const char *album,
		   const char *artist)
{
	DialogData *data;
	GtkWidget  *scrolled_window;
	GtkWidget  *btn_cancel;
	GtkWidget  *btn_help;
	GtkWidget  *image;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_CHOOSER_FILE, NULL, NULL);
        if (!data->gui) {
		g_warning ("Could not find " GLADE_CHOOSER_FILE "\n");
		g_free (data);
                return;
        }

	data->album = g_strdup (album);
	data->artist = g_strdup (artist);
	data->max_images = MAX_IMAGES;

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "cover_chooser_dialog");
	scrolled_window = glade_xml_get_widget (data->gui, "image_list_scrolledwindow");
	data->progress_label = glade_xml_get_widget (data->gui, "progress_label");
	data->ok_button = glade_xml_get_widget (data->gui, "cc_okbutton");
	data->revert_button = glade_xml_get_widget (data->gui, "cc_revertbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "cc_cancelbutton");
	btn_help = glade_xml_get_widget (data->gui, "cc_helpbutton");

	data->cc_cancel_search_button = glade_xml_get_widget (data->gui, "cc_cancel_search_button");

	data->image_list = gth_image_list_new (THUMB_SIZE + THUMB_BORDER);
	gth_image_list_set_view_mode (GTH_IMAGE_LIST (data->image_list),
				      GTH_VIEW_MODE_VOID);
	gth_image_list_set_selection_mode (GTH_IMAGE_LIST (data->image_list),
					   GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (scrolled_window), data->image_list);

	/* Set widgets data. */

	gtk_widget_set_sensitive (data->ok_button, FALSE);

	image = gtk_image_new_from_stock (GOO_STOCK_RESET, GTK_ICON_SIZE_BUTTON);
	g_object_set (data->revert_button, 
		      "use_stock", TRUE,
		      "label", GOO_STOCK_RESET,
		      "image", image,
		      NULL);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (data->ok_button), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);
	g_signal_connect (G_OBJECT (data->revert_button), 
			  "clicked",
			  G_CALLBACK (revert_cb),
			  data);
	g_signal_connect (G_OBJECT (data->image_list), 
			  "selection_changed",
			  G_CALLBACK (image_list_selection_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->image_list), 
			  "item_activated",
			  G_CALLBACK (image_list_item_activated_cb),
			  data);
	g_signal_connect (G_OBJECT (data->cc_cancel_search_button), 
			  "clicked",
			  G_CALLBACK (cancel_search_button_clicked_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);

	/**/

	data->tmpdir = g_strdup (get_temp_work_dir ());
	ensure_dir_exists (data->tmpdir, DIR_PERM);
	backup_cover_image (data);

	gth_image_list_set_no_image_text (GTH_IMAGE_LIST (data->image_list),
					  _("Searching images..."));

	data->load_id = g_idle_add (start_searching, data);
}


static void
auto_fetch__image_saved_cb (DialogData *data,
			    const char *filename,
			    gboolean    success)
{
	if (success) {
		char *tmpfile = g_strdup (filename);
		
		debug (DEBUG_INFO, "LOAD IMAGE: %s\n", tmpfile);
		data->tmpfiles = g_list_prepend (data->tmpfiles, tmpfile);

		goo_window_set_cover_image (data->window, get_path_from_uri (filename));
	}

	destroy_cb (NULL, data);
}


static void
auto_fetch__search_result_saved_cb (DialogData *data,
				    const char *filename,
				    gboolean    success)
{
	if (! success || ! make_file_list_from_search_result (data, filename))
		return;

	if (data->url_list != NULL) {
		char *filename, *url, *dest;

		url = (char*) data->url_list->data;

		filename = g_strdup_printf ("%d.png", data->url);
		dest = g_build_filename (data->tmpdir, filename, NULL);
		g_free (filename);

		copy_file_from_url (data, url, dest, auto_fetch__image_saved_cb)
;
		g_free (dest);
	} 
	else
		destroy_cb (NULL, data);
}


static gboolean
auto_fetch_from_name__start_searching (gpointer callback_data)
{
	DialogData *data = callback_data;
	char       *query;
	char       *dest;

	g_source_remove (data->load_id);
	data->load_id = 0;

	query = get_query (data);
	dest = g_build_filename (data->tmpdir, QUERY_RESULT, NULL);
	copy_file_from_url (data, query, dest, auto_fetch__search_result_saved_cb);

	g_free (dest);
	g_free (query);

	return FALSE;
}


void
fetch_cover_image_from_name (GooWindow  *window,
		             const char *album,
		             const char *artist)
{
	DialogData *data;
	
	data = g_new0 (DialogData, 1);
	data->window = window;
	data->album = g_strdup (album);
	data->artist = g_strdup (artist);
	data->max_images = 1;
	data->autofetching = TRUE;

	data->tmpdir = get_temp_work_dir ();
	ensure_dir_exists (data->tmpdir, DIR_PERM);

	data->load_id = g_idle_add (auto_fetch_from_name__start_searching, data);
}


static gboolean
auto_fetch_from_asin__start_searching (gpointer callback_data)
{
	DialogData *data = callback_data;
	char       *dest;

	g_source_remove (data->load_id);
	data->load_id = 0;

	dest = g_strconcat ("file://", data->tmpdir, "/1.jpg", NULL);
	copy_file_from_url (data, data->source, dest, auto_fetch__image_saved_cb);
	g_free (dest);

	return FALSE;
}


void
fetch_cover_image_from_asin (GooWindow  *window,
		             const char *asin)
{
	DialogData *data;
	
	data = g_new0 (DialogData, 1);
	data->window = window;
	data->max_images = 1;
	data->autofetching = TRUE;

	data->tmpdir = g_strdup (get_temp_work_dir ());
	ensure_dir_exists (data->tmpdir, DIR_PERM);

	data->source = g_strdup_printf ("http://images.amazon.com/images/P/%s.01._SCLZZZZZZZ_.jpg", asin);
	data->load_id = g_idle_add (auto_fetch_from_asin__start_searching, data);
}
