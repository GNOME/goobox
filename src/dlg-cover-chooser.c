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

#ifdef HAVE_NEON

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ne_auth.h>
#include <ne_request.h>
#include <ne_socket.h>
#include <ne_session.h>
#include <ne_uri.h>

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <glade/glade.h>
#include "typedefs.h"
#include "main.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "goo-window.h"
#include "gth-image-list.h"
#include "gnome-vfs-helpers.h"
#include "preferences.h"
#include "gconf-utils.h"

#define GLADE_CHOOSER_FILE "goo_cover_chooser.glade"
#define MAX_IMAGES 20
#define READ_TIMEOUT 20
#define BUFFER_SIZE 612
#define IMG_PERM 0600
#define DIR_PERM 0700
#define THUMB_SIZE 100
#define THUMB_BORDER 14
#define QUERY_RESULT "result.html"

typedef struct _DialogData DialogData;

typedef void (*FileSavedFunc) (DialogData *data, char *filename, int result);

struct _DialogData {
	GooWindow     *window;
	char          *artist, *album;
	GList         *url_list;
	GList         *current;
	guint          load_id, urls, url;
	char          *tmpdir;
	GList         *tmpfiles;
	int            fd;
	guint          read_id;

	ne_session    *session;
	ne_request    *request;

	FileSavedFunc  file_saved_func;
	char          *dest;

	GladeXML      *gui;

	GtkWidget     *dialog;
	GtkWidget     *image_list;
	GtkWidget     *progress_label;
	GtkWidget     *ok_button;
};


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	if (data->load_id != 0)
		g_source_remove (data->load_id);
	if (data->read_id != 0)
		g_source_remove (data->read_id);
	if (data->request != NULL) {
		ne_end_request (data->request);
		ne_request_destroy (data->request);
	}
	ne_session_destroy (data->session);

	if (data->tmpdir != NULL) {
		g_list_foreach (data->tmpfiles, (GFunc) gnome_vfs_unlink, NULL);
		path_list_free (data->tmpfiles);

		gnome_vfs_remove_directory (data->tmpdir);
		g_free (data->tmpdir);
	}
	g_free (data->dest);

	g_free (data->album);
	g_free (data->artist);
	path_list_free (data->url_list);
	g_object_unref (data->gui);
	g_free (data);
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

		goo_window_set_cover_image (data->window, src);
		g_list_free (selection);
	}
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("goobox", "choose_cover", &err);
	
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
append_image (DialogData *data,
	      const char *filename)
{
	GdkPixbuf *image;
	int        pos;

	image = gdk_pixbuf_new_from_file_at_size (filename, 
						  THUMB_SIZE,  THUMB_SIZE, 
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
image_saved_cb (DialogData *data,
		char       *filename,
		int         result)
{
	if (result == 0) {
		char *tmpfile = g_strdup (filename);

		debug (DEBUG_INFO, "LOAD IMAGE: %s\n", tmpfile);

		data->tmpfiles = g_list_prepend (data->tmpfiles, tmpfile);
		append_image (data, tmpfile);
	}

	data->load_id = g_idle_add (load_next_url, data);
}


static int
proxy_authentication (void       *userdata, 
		      const char *realm, 
		      int         attempt, 
		      char       *username, 
		      char       *password)
{
	char *user, *pwd;

	user = eel_gconf_get_string (HTTP_PROXY_USER, NULL);
	pwd = eel_gconf_get_string (HTTP_PROXY_PWD, NULL);

	if ((user == NULL) || (pwd == NULL))
		return 1;

	strncpy (username, user, NE_ABUFSIZ);
	strncpy (password, pwd, NE_ABUFSIZ);

	g_free (user);
	g_free (pwd);

	return attempt;
}


static gboolean
read_block_cb (gpointer callback_data)
{
	DialogData *data = callback_data;
	size_t      len;
	char        buf[BUFFER_SIZE];

	g_source_remove (data->read_id);
	data->read_id = 0;

	len = ne_read_response_block (data->request, buf, sizeof (buf));
	if (len > 0) {
		write (data->fd, buf, len);
		data->read_id = g_timeout_add (READ_TIMEOUT,
					       read_block_cb,
					       data);
	} else {
		if (len < 0)
			debug (DEBUG_INFO, "HTTP Request failed: %s\n", ne_get_error (data->session));
		ne_end_request (data->request);

		ne_request_destroy (data->request);
		data->request = NULL;

		close (data->fd);
		if (data->file_saved_func != NULL)
			(*data->file_saved_func) (data, data->dest, len);
	}

	return FALSE;
}


static void
copy_file_from_url (DialogData    *data,
		    const char    *src,
		    const char    *dest,
		    FileSavedFunc  file_saved_func)
{
	char *e_query;

	if (eel_gconf_get_boolean (HTTP_PROXY_USE_HTTP_PROXY, FALSE)) {
		char *host;
		int   port;

		host = eel_gconf_get_string (HTTP_PROXY_HOST, NULL);
		port = eel_gconf_get_integer (HTTP_PROXY_PORT, 80);

		if (host != NULL) {
			ne_session_proxy (data->session, host, port);
			g_free (host);
		}
	}

	if (eel_gconf_get_boolean (HTTP_PROXY_USE_AUTH, FALSE)) 
		ne_set_proxy_auth (data->session, proxy_authentication, data);

	e_query = ne_path_escape (src);
	debug (DEBUG_INFO, "QUERY: %s\n", e_query);
	data->request = ne_request_create (data->session, "GET", e_query);
	free (e_query);

 retry:
	if (ne_begin_request (data->request) != NE_OK) {
		debug (DEBUG_INFO, "HTTP Request failed: %s\n", ne_get_error (data->session));
		if (ne_end_request (data->request) == NE_RETRY)
			goto retry;
		ne_request_destroy (data->request);
		data->request = NULL;

		return;
	}

	data->file_saved_func = file_saved_func;
	g_free (data->dest);
	data->dest = g_strdup (dest);

	data->fd = open (data->dest, O_WRONLY | O_CREAT | O_TRUNC, IMG_PERM);

	data->read_id = g_timeout_add (READ_TIMEOUT,
				       read_block_cb,
				       data);
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
load_current_url (DialogData *data)
{
	char *url, *dest;
	char *filename;

	update_progress_label (data);

	if ((data->current == NULL) || (data->url >= MAX_IMAGES))
		return;

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
search_result_saved_cb (DialogData *data,
			char       *filename,
			int         result)
{
	int      fd, n;
	char     buf[BUFFER_SIZE];
	int      buf_offset = 0;
	GString *partial_url;

	if (result != 0) {
		/*FIXME*/
		return;
	}

	fd = open (filename, O_RDONLY);
	if (fd == 0) {
		/*FIXME*/
		return;
	}
	
	partial_url = NULL;
	while ((n = read (fd, buf+buf_offset, BUFFER_SIZE-buf_offset-1)) > 0) {
		char     *prefix = "/images?q=tbn:";
		int       prefix_len = strlen (prefix);
		char     *url_start;
		gboolean  copy_tail = TRUE;

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

			} else {
				char *url_tail = g_strndup (url_start, url_end - url_start);
				char *url;
				
				if (partial_url != NULL) {
					g_string_append (partial_url, url_tail);
					url = partial_url->str;
					g_string_free (partial_url, FALSE);
					partial_url = NULL;
				} else 
					url = url_tail;
					
				data->url_list = g_list_prepend (data->url_list, url);
				url_start = strstr (url_end + 1, prefix);
				
			}
		}

		if (copy_tail) {
			prefix_len = MIN (prefix_len, buf_offset + n);
			strncpy (buf, 
				 buf + buf_offset + n - prefix_len, 
				 prefix_len);
			buf_offset = prefix_len;
		} else 
			buf_offset = 0;
	}

	if (partial_url != NULL)
		g_string_free (partial_url, TRUE);

	close (fd);
	data->tmpfiles = g_list_prepend (data->tmpfiles, g_strdup (filename));

	/**/

	if (data->url_list == NULL) {
		gth_image_list_set_no_image_text (GTH_IMAGE_LIST (data->image_list), _("No image found"));

		return;
	}

	data->url_list = g_list_reverse (data->url_list);
	data->urls = MIN (g_list_length (data->url_list), MAX_IMAGES);
	data->url = 0;

	start_loading_images (data);
}


static gboolean
start_searching (gpointer callback_data)
{
	DialogData *data = callback_data;
	char       *query;
	char       *dest;

	g_source_remove (data->load_id);
	data->load_id = 0;

	data->session = ne_session_create ("http", "images.google.com", 80);
	ne_set_useragent (data->session, PACKAGE "/" VERSION);

	query = g_strconcat ("/images?q=",
			     data->album,
			     "+",
			     data->artist,
			     /* "&imgsz=medium", FIXME*/
			     NULL);
	dest = g_build_filename (data->tmpdir, QUERY_RESULT, NULL);

	copy_file_from_url (data, query, dest, search_result_saved_cb);

	g_free (dest);
	g_free (query);

	return FALSE;
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

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "cover_chooser_dialog");
	scrolled_window = glade_xml_get_widget (data->gui, "image_list_scrolledwindow");
	data->progress_label = glade_xml_get_widget (data->gui, "progress_label");
	data->ok_button = glade_xml_get_widget (data->gui, "cc_okbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "cc_cancelbutton");
	btn_help = glade_xml_get_widget (data->gui, "cc_helpbutton");

	data->image_list = gth_image_list_new (THUMB_SIZE + THUMB_BORDER);
	gth_image_list_set_view_mode (GTH_IMAGE_LIST (data->image_list),
				      GTH_VIEW_MODE_VOID);
	gth_image_list_set_selection_mode (GTH_IMAGE_LIST (data->image_list),
					   GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (scrolled_window), data->image_list);

	/* Set widgets data. */

	gtk_widget_set_sensitive (data->ok_button, FALSE);

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
	g_signal_connect (G_OBJECT (data->image_list), 
			  "selection_changed",
			  G_CALLBACK (image_list_selection_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->image_list), 
			  "item_activated",
			  G_CALLBACK (image_list_item_activated_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);

	/**/

	data->tmpdir = g_strdup (get_temp_work_dir ());
	ensure_dir_exists (data->tmpdir, DIR_PERM);

	gth_image_list_set_no_image_text (GTH_IMAGE_LIST (data->image_list),
					  _("Searching images..."));

	data->load_id = g_idle_add (start_searching, data);
}


#endif /* HAVE_NEON*/
