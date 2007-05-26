/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <glade/glade.h>
#include <musicbrainz/queries.h>
#include <musicbrainz/mb_c.h>
#include "dlg-properties.h"
#include "metadata.h"

#define GLADE_PREF_FILE "properties.glade"

enum { NUMBER_COLUMN, TITLE_COLUMN, ARTIST_COLUMN, DATA_COLUMN, N_COLUMNS };

typedef struct {
	GooWindow         *window;
	GladeXML          *gui;
	GtkWidget         *dialog;
	GtkListStore      *list_store; 
	GtkTreeViewColumn *author_column;
	GtkWidget         *p_title_entry;
	GtkWidget         *p_artist_entry;
	GtkWidget         *p_artist_combobox;
	GtkWidget         *p_year_spinbutton;
	GtkWidget         *p_info_label;
	GtkWidget         *p_info_box;
	GtkWidget         *p_navigation_box;
	GtkWidget         *p_album_label;
	
	GList             *albums;
	int                n_albums, current_album;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	data->window->properties_dialog = NULL;
	g_object_unref (G_OBJECT (data->gui));
	g_free (data);
}

static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}


static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("goo", "properties", &err);
	
	if (err != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
						 GTK_DIALOG_MODAL,
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
set_data_from_album (DialogData *data,
		     AlbumInfo  *album)
{
	GList *scan;
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->p_artist_combobox), album->various_artist ? 1 : 0);
	
	if (album->title != NULL)
		gtk_entry_set_text (GTK_ENTRY (data->p_title_entry), album->title);
	if (album->artist != NULL)
		gtk_entry_set_text (GTK_ENTRY (data->p_artist_entry), album->artist);
	if (g_date_valid (album->release_date))
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->p_year_spinbutton), g_date_get_year (album->release_date));	

	gtk_list_store_clear (data->list_store);
	for (scan = album->tracks; scan; scan = scan->next) {
		TrackInfo   *track = scan->data;
		GtkTreeIter  iter;

		gtk_list_store_append (data->list_store, &iter);
		gtk_list_store_set (data->list_store, &iter,
				    DATA_COLUMN, track,
				    NUMBER_COLUMN, track->number + 1,
				    TITLE_COLUMN, track->title,
				    ARTIST_COLUMN, track->artist,
				    -1);
	}
}


static void
show_album (DialogData *data,
	    int         n)
{
	char *s;
	
	if ((n < 0) || (n >= data->n_albums))
		return;
	data->current_album = n;
		
	gtk_widget_hide (data->p_info_box);
	gtk_widget_show (data->p_navigation_box);
		
	s = g_strdup_printf (_("Album %d of %d"), data->current_album + 1, data->n_albums); 
	gtk_label_set_text (GTK_LABEL (data->p_album_label), s);
	g_free (s);
	
	set_data_from_album (data, g_list_nth (data->albums, n)->data);
}


static void
search_cb (GtkWidget  *widget, 
	   DialogData *data)
{
	musicbrainz_t  mb;
	char          *mb_args[2];
	
	mb = mb_New ();
	mb_UseUTF8 (mb, TRUE);
	mb_SetDepth (mb, 4);
	/*mb_SetMaxItems(mb, 10);*/
	
	mb_args[0] = (char*) gtk_entry_get_text (GTK_ENTRY (data->p_title_entry));
	mb_args[1] = NULL;
	if (! mb_QueryWithArgs (mb, MBQ_FindAlbumByName, mb_args)) {
		char mb_error[1024];
		char *s;
		
	        mb_GetQueryError (mb, mb_error, sizeof (mb_error));
	        s = g_strdup_printf (_("Search failed: %s\n"), mb_error);
        	gtk_label_set_text (GTK_LABEL (data->p_info_label), s);
        	g_free (s);
	}
	else {	
		data->albums = get_album_list (mb);
		data->n_albums = g_list_length (data->albums);
	
		if (data->n_albums == 0) { 
			gtk_label_set_text (GTK_LABEL (data->p_info_label), _("No album found"));
			gtk_widget_show (data->p_info_box);
			gtk_widget_hide (data->p_navigation_box);
		} 
		else 
			show_album (data, 0);
	}

	mb_Delete (mb);
}


static void
add_columns (DialogData  *data,
 	     GtkTreeView *treeview)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GValue             value = { 0, };

	/* The Number column. */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("#"));

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column,
					 renderer,
					 TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", NUMBER_COLUMN,
                                             NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);

	gtk_tree_view_column_set_sort_column_id (column, NUMBER_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* Title */
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Title"));
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, TITLE_COLUMN);
	
	renderer = gtk_cell_renderer_text_new ();
	
	g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
	g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
	g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
	g_value_unset (&value);
	
	g_value_init (&value, G_TYPE_BOOLEAN);
	g_value_set_boolean (&value, TRUE);
	g_object_set_property (G_OBJECT (renderer), "editable", &value);
	g_value_unset (&value);
	
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", TITLE_COLUMN,
                                             NULL);
		
	gtk_tree_view_append_column (treeview, column);

	/* Author */
	
	data->author_column = column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Artist"));
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, ARTIST_COLUMN);
	
	renderer = gtk_cell_renderer_text_new ();
	
	g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
	g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
	g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
	g_value_unset (&value);
	
	g_value_init (&value, G_TYPE_BOOLEAN);
	g_value_set_boolean (&value, TRUE);
	g_object_set_property (G_OBJECT (renderer), "editable", &value);
	g_value_unset (&value);
	
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "text", ARTIST_COLUMN,
                                             NULL);
		
	gtk_tree_view_append_column (treeview, column);   	
}


static void
artist_combobox_changed_cb (GtkComboBox *widget,
                            DialogData  *data)
{
	gboolean single_artist = gtk_combo_box_get_active (widget) == 0;
	gtk_tree_view_column_set_visible (data->author_column, ! single_artist);
	if (single_artist)
		gtk_widget_show (data->p_artist_entry);
	else
		gtk_widget_hide (data->p_artist_entry);
}


static void
prev_album_button_clicked_cb (GtkButton  *button,
                              DialogData *data)
{
	show_album (data, data->current_album - 1);
}


static void
next_album_button_clicked_cb (GtkButton  *button,
                              DialogData *data)
{
	show_album (data, data->current_album + 1);
}


static void
reset_album_button_clicked_cb (GtkButton  *button,
                               DialogData *data)
{
	gtk_widget_hide (data->p_info_box);
	gtk_widget_hide (data->p_navigation_box);
	set_data_from_album (data, goo_window_get_album (data->window));
}


void
dlg_properties (GooWindow *window)
{
	DialogData *data;
	GtkWidget  *btn_cancel;
	GtkWidget  *btn_ok;
	GtkWidget  *btn_help;
	GtkWidget  *btn_search;
	GtkWidget  *tree_view;
        GtkWidget  *p_prev_album_button;
        GtkWidget  *p_next_album_button;
        GtkWidget  *p_reset_album_button;
        
        if (window->properties_dialog != NULL) {
        	gtk_window_present (GTK_WINDOW (window->properties_dialog));
        	return;
        }
        
	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_PREF_FILE, NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_PREF_FILE "\n");
		g_free (data);
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "properties_dialog");
	window->properties_dialog = data->dialog;

	data->p_artist_entry = glade_xml_get_widget (data->gui, "p_artist_entry");
	data->p_artist_combobox = glade_xml_get_widget (data->gui, "p_artist_combobox");
	data->p_title_entry = glade_xml_get_widget (data->gui, "p_title_entry");
	data->p_info_box = glade_xml_get_widget (data->gui, "p_info_box");
	data->p_info_label = glade_xml_get_widget (data->gui, "p_info_label");
	data->p_navigation_box = glade_xml_get_widget (data->gui, "p_navigation_box");
	data->p_album_label = glade_xml_get_widget (data->gui, "p_album_label");
	data->p_year_spinbutton = glade_xml_get_widget (data->gui, "p_year_spinbutton");
	
	btn_ok = glade_xml_get_widget (data->gui, "p_okbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "p_cancelbutton");
	btn_help = glade_xml_get_widget (data->gui, "p_helpbutton");
	btn_search = glade_xml_get_widget (data->gui, "p_search_button");
	tree_view = glade_xml_get_widget (data->gui, "p_track_treeview");
	
	p_prev_album_button = glade_xml_get_widget (data->gui, "p_prev_album_button");
	p_next_album_button = glade_xml_get_widget (data->gui, "p_next_album_button");
	p_reset_album_button = glade_xml_get_widget (data->gui, "p_undobutton");
	
	/**/
	
	data->list_store = gtk_list_store_new (N_COLUMNS,
					       G_TYPE_INT,
					       G_TYPE_STRING,
					       G_TYPE_STRING,
					       G_TYPE_POINTER);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (data->list_store));
	add_columns (data, GTK_TREE_VIEW (tree_view));
	
	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
			  	  "clicked",
			  	  G_CALLBACK (gtk_widget_destroy),
			  	  data->dialog);
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_search), 
			  "clicked",
			  G_CALLBACK (search_cb),
			  data);
	g_signal_connect (G_OBJECT (data->p_artist_combobox), 
			  "changed",
			  G_CALLBACK (artist_combobox_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (p_prev_album_button), 
			  "clicked",
			  G_CALLBACK (prev_album_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (p_next_album_button), 
			  "clicked",
			  G_CALLBACK (next_album_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (p_reset_album_button), 
			  "clicked",
			  G_CALLBACK (reset_album_button_clicked_cb),
			  data);
			  		  
	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
	
	set_data_from_album (data, goo_window_get_album (data->window));
}
