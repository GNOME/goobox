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
#define MAX_IMAGES 10

typedef struct {
	GooWindow   *window;
	GList       *urls;
	GList       *current;
	guint        load_id, url;
	char        *tmpdir;
	GList       *tmpfiles;
	int          fd;

	ne_session  *session;

	GladeXML    *gui;

	GtkWidget   *dialog;
	GtkWidget   *image_list;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	ne_session_destroy (data->session);

	if (data->tmpdir != NULL) {
		g_list_foreach (data->tmpfiles, (GFunc) gnome_vfs_unlink, NULL);
		path_list_free (data->tmpfiles);

		gnome_vfs_unlink (data->tmpdir);
		g_free (data->tmpdir);
	}

	path_list_free (data->urls);
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
		goo_window_set_cover_image (data->window, src);
		g_list_free (selection);
	}

	/**/

	gtk_widget_destroy (data->dialog);
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

	image = gdk_pixbuf_new_from_file (filename, NULL);
	if (image == NULL)
		return;

	pos = gth_image_list_append (GTH_IMAGE_LIST (data->image_list),
				     image,
				     filename,
				     NULL);
	gth_image_list_set_image_data (GTH_IMAGE_LIST (data->image_list),
				       pos,
				       (char*)filename);
	g_object_unref (image);
}


static void load_current_url (DialogData *data);


static gboolean
load_next_url (gpointer callback_data)
{
	DialogData *data = callback_data;

	g_source_remove (data->load_id);
	data->load_id = 0;

	if (data->url < MAX_IMAGES) { 
		data->current = data->current->next;
		load_current_url (data);
	}

	return FALSE;
}


static void
image_loaded_cb (gpointer   callback_data,
		 char      *filename,
		 int        result)
{
	DialogData *data = callback_data;

	if (result == 0) {
		data->tmpfiles = g_list_prepend (data->tmpfiles, filename);
		append_image (data, filename);
	}

	data->load_id = g_idle_add (load_next_url, data);
}


typedef void (*FileSavedFunc) (gpointer callback_data, char *filename, int result);


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


static int
response_accept_cb (void            *userdata, 
		    ne_request      *req, 
		    const ne_status *st)
{
	debug (DEBUG_INFO, "%d: %s\n", st->code, st->reason_phrase);
	return (st->code == 200) ? 1 : 0;
}


static void
response_block_reader_cb (void       *userdata, 
			  const char *buf, 
			  size_t      len)
{
	DialogData *data = userdata;
	write (data->fd, buf, len);
}


static void
copy_file_from_url (const char    *src,
		    char          *dest,
		    FileSavedFunc  file_saved_func,
		    gpointer       extra_data)
{
	DialogData *data = extra_data;
	ne_request *request;
	char       *e_query;

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
	request = ne_request_create (data->session, "GET", e_query);
	free (e_query);

	ne_add_response_body_reader (request,
				     response_accept_cb,
				     response_block_reader_cb,
				     data);

	if (ne_request_dispatch (request)) 
		debug (DEBUG_INFO, "HTTP Request failed: %s\n", ne_get_error (data->session));
	
	ne_request_destroy (request);

	close (data->fd);

	(*file_saved_func) (extra_data, dest, 0);
}


static void
load_current_url (DialogData *data)
{
	char *url, *dest;
	char *filename;

	if (data->current == NULL)
		return;

	url = data->current->data;

	debug (DEBUG_INFO, "LOAD %s\n", url);

	filename = g_strdup_printf ("%d.png", data->url++);
	dest = g_build_filename (data->tmpdir, filename, NULL);
	g_free (filename);

	data->fd = open (dest, O_WRONLY | O_CREAT | O_TRUNC, 0600); /*FIXME*/

	copy_file_from_url (url, dest, image_loaded_cb, data);
}


static void
start_loading (DialogData *data)
{
	data->session = ne_session_create ("http", "images.google.com", 80);
	ne_set_useragent (data->session, PACKAGE "/" VERSION);

	data->tmpdir = g_strdup (get_temp_work_dir ());
	ensure_dir_exists (data->tmpdir, 0755); /*FIXME*/

	data->current = data->urls;
	load_current_url (data);
}





void
dlg_cover_chooser (GooWindow *window,
		   GList     *url_list)
{
	DialogData *data;
	GtkWidget  *scrolled_window;
	GtkWidget  *btn_ok;
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

	data->urls = path_list_dup (url_list);
	data->url = 0;

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "cover_chooser_dialog");
	scrolled_window = glade_xml_get_widget (data->gui, "image_list_scrolledwindow");

	btn_ok = glade_xml_get_widget (data->gui, "cc_okbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "cc_cancelbutton");
	btn_help = glade_xml_get_widget (data->gui, "cc_helpbutton");

	data->image_list = gth_image_list_new (110); /*FIXME*/
	gth_image_list_set_view_mode (GTH_IMAGE_LIST (data->image_list),
				      GTH_VIEW_MODE_VOID);
	gth_image_list_set_selection_mode (GTH_IMAGE_LIST (data->image_list),
					   GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (scrolled_window), data->image_list);

	/* Set widgets data. */

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
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);

	start_loading (data);
}
