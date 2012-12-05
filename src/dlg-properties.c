/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2001-2009 Free Software Foundation, Inc.
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
#include "dlg-properties.h"
#include "goo-stock.h"
#include "gtk-utils.h"
#include "metadata.h"


#define GET_WIDGET(x) _gtk_builder_get_widget (data->builder, (x))


enum {
	NUMBER_COLUMN,
	TITLE_COLUMN,
	ARTIST_COLUMN,
	DATA_COLUMN,
	N_COLUMNS
};


typedef struct {
	GooWindow         *window;
	GtkWidget         *dialog;
	GtkBuilder        *builder;
	GtkListStore      *list_store;
	GtkTreeViewColumn *author_column;
	GList             *albums;
	int                n_albums, current_album;
	GCancellable      *cancellable;
	gboolean           searching;
	gboolean           closing;
} DialogData;


static void
dialog_destroy_cb (GtkWidget  *widget,
		   DialogData *data)
{
	data->window->properties_dialog = NULL;
	g_object_unref (data->cancellable);
	g_object_unref (data->builder);
	g_free (data);
}


static void
close_dialog (DialogData *data)
{
	if (data->searching) {
		data->closing = TRUE;
		g_cancellable_cancel (data->cancellable);
		return;
	}

	gtk_widget_destroy (data->dialog);
}


static void
close_button_clicked_cb (GtkButton *button,
			 gpointer   user_data)
{
	close_dialog ((DialogData *) user_data);
}


static gboolean
dialog_delete_event_cb (GtkWidget *widget,
			GdkEvent  *event,
			gpointer   user_data)
{
	close_dialog ((DialogData *) user_data);
	return TRUE;
}


static void
set_album_from_data (DialogData *data)
{
	AlbumInfo   *album;
	const char  *album_artist;
	GtkTreeIter  iter;

	album = album_info_copy (goo_window_get_album (data->window));
	album_info_set_title (album, gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("title_entry"))));
	album_artist = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("artist_entry")));
	album_info_set_artist (album, album_artist, "");
	album->various_artist = gtk_combo_box_get_active (GTK_COMBO_BOX (GET_WIDGET ("artist_combobox"))) == 1;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("year_checkbutton")))) {
		GDate *date;

		date = g_date_new_dmy (1, 1, gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (GET_WIDGET ("year_spinbutton"))));
		album_info_set_release_date (album, date);
		g_date_free (date);
	}
	else {
		GDate *date;

		date = g_date_new ();
		album_info_set_release_date (album, date);
		g_date_free (date);
	}

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (data->list_store), &iter)) {
		GList     *scan = album->tracks;
		TrackInfo *track;
		char      *title;
		char      *artist;

		do {
			track = scan->data;
			gtk_tree_model_get (GTK_TREE_MODEL (data->list_store),
					    &iter,
					    TITLE_COLUMN, &title,
					    ARTIST_COLUMN, &artist,
					    -1);

			track_info_set_title (track, title);
			if (album->various_artist)
				track_info_set_artist (track, artist, "");
			else
				track_info_set_artist (track, album_artist, "");

			g_free (title);
			g_free (artist);

			scan = scan->next;
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (data->list_store), &iter));
	}

	goo_player_set_album (goo_window_get_player (data->window), album);

	album_info_unref (album);
}


static void
ok_button_clicked_cb (GtkWidget  *widget,
		      DialogData *data)
{
	set_album_from_data (data);
	gtk_widget_destroy (data->dialog);
}


static void
set_data_from_album (DialogData *data,
		     AlbumInfo  *album)
{
	GList *scan;

	gtk_combo_box_set_active (GTK_COMBO_BOX (GET_WIDGET ("artist_combobox")), album->various_artist ? 1 : 0);

	if (album->title != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("title_entry")), album->title);
	if (album->artist != NULL)
		gtk_entry_set_text (GTK_ENTRY (GET_WIDGET ("artist_entry")), album->artist);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("year_checkbutton")), g_date_valid (album->release_date));
	if (g_date_valid (album->release_date))
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("year_spinbutton")), g_date_get_year (album->release_date));
	else
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("year_spinbutton")), 0);

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

	gtk_widget_hide (GET_WIDGET ("info_box"));
	gtk_widget_show (GET_WIDGET ("navigation_box"));

	s = g_strdup_printf (_("Album %d of %d"), data->current_album + 1, data->n_albums);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("album_label")), s);
	g_free (s);

	set_data_from_album (data, g_list_nth (data->albums, n)->data);
}


static GList *
remove_incompatible_albums (GList     *albums,
			    AlbumInfo *album)
{
	GList *scan;

	for (scan = albums; scan; /* nothing */) {
		AlbumInfo *album2 = scan->data;
		gboolean   incompatible = FALSE;

		if (album2->n_tracks != album->n_tracks)
			incompatible = TRUE;

		if (incompatible) {
			GList *tmp = scan;

			scan = scan->next;
			albums = g_list_remove_link (albums, tmp);
			album_info_unref (tmp->data);
			g_list_free (tmp);
		}
		else
			scan = scan->next;
	}

	return albums;
}


static void
search_album_by_title_ready_cb (GObject      *source_object,
			        GAsyncResult *result,
				gpointer      user_data)
{
	DialogData *data = user_data;
	GError     *error = NULL;

	data->searching = FALSE;

	if (data->closing) {
		close_dialog (data);
		return;
	}

	data->albums = metadata_search_album_by_title_finish (result, &error);
	data->albums = remove_incompatible_albums (data->albums, goo_window_get_album (data->window));
	data->n_albums = g_list_length (data->albums);

	if (data->n_albums == 0) {
		gtk_image_set_from_stock (GTK_IMAGE (GET_WIDGET ("info_icon")), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
		gtk_label_set_text (GTK_LABEL (GET_WIDGET ("info_label")), _("No album found"));
		gtk_widget_show (GET_WIDGET ("info_box"));
		gtk_widget_hide (GET_WIDGET ("navigation_box"));
	}
	else
		show_album (data, 0);
}


static void
search_cb (GtkWidget  *widget,
	   DialogData *data)
{
	if (data->searching)
		return;

	data->searching = TRUE;

	gtk_image_set_from_stock (GTK_IMAGE (GET_WIDGET ("info_icon")), GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON);
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("info_label")), _("Searching disc info..."));
	gtk_widget_show (GET_WIDGET ("info_box"));
	gtk_widget_hide (GET_WIDGET ("navigation_box"));

	metadata_search_album_by_title (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("title_entry"))),
					data->cancellable,
					search_album_by_title_ready_cb,
					data);
}


static void
title_cell_edited_cb (GtkCellRendererText *renderer,
                      char                *path,
                      char                *new_text,
                      gpointer             user_data)
{
	DialogData  *data = user_data;
	GtkTreePath *t_path;
	GtkTreeIter  iter;

	t_path = gtk_tree_path_new_from_string (path);
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (data->list_store), &iter, t_path))
		gtk_list_store_set (data->list_store, &iter,
				    TITLE_COLUMN, new_text,
				    -1);
	gtk_tree_path_free (t_path);
}


static void
artist_cell_edited_cb (GtkCellRendererText *renderer,
                       char                *path,
                       char                *new_text,
                       gpointer             user_data)
{
	DialogData  *data = user_data;
	GtkTreePath *t_path;
	GtkTreeIter  iter;

	t_path = gtk_tree_path_new_from_string (path);
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (data->list_store), &iter, t_path))
		gtk_list_store_set (data->list_store, &iter,
				    ARTIST_COLUMN, new_text,
				    -1);
	gtk_tree_path_free (t_path);
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

	g_signal_connect (G_OBJECT (renderer),
			  "edited",
			  G_CALLBACK (title_cell_edited_cb),
			  data);

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

	g_signal_connect (G_OBJECT (renderer),
			  "edited",
			  G_CALLBACK (artist_cell_edited_cb),
			  data);

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
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (GET_WIDGET ("track_treeview")), ! single_artist);
	if (single_artist)
		gtk_widget_show (GET_WIDGET ("artist_entry"));
	else
		gtk_widget_hide (GET_WIDGET ("artist_entry"));
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
undo_button_clicked_cb (GtkButton  *button,
                        DialogData *data)
{
	gtk_widget_hide (GET_WIDGET ("info_box"));
	gtk_widget_hide (GET_WIDGET ("navigation_box"));
	set_data_from_album (data, goo_window_get_album (data->window));
}


static void
year_checkbutton_toggled_cb (GtkToggleButton *button,
                             DialogData      *data)
{
	gtk_widget_set_sensitive (GET_WIDGET ("year_spinbutton"), gtk_toggle_button_get_active (button));
}


void
dlg_properties (GooWindow *window)
{
	DialogData *data;
        GtkWidget  *image;

        if (window->properties_dialog != NULL) {
        	gtk_window_present (GTK_WINDOW (window->properties_dialog));
        	return;
        }

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->builder = _gtk_builder_new_from_resource ("properties.ui");
	data->cancellable = g_cancellable_new ();
	data->searching = FALSE;
	data->closing = FALSE;

	/* Get the widgets. */

	data->dialog = GET_WIDGET ("properties_dialog");
	window->properties_dialog = data->dialog;

	/* Set widgets data. */

	image = gtk_image_new_from_stock (GOO_STOCK_RESET, GTK_ICON_SIZE_BUTTON);
	g_object_set (GET_WIDGET ("undo_button"),
		      "use_stock", TRUE,
		      "label", GOO_STOCK_RESET,
		      "image", image,
		      NULL);

	data->list_store = gtk_list_store_new (N_COLUMNS,
					       G_TYPE_INT,
					       G_TYPE_STRING,
					       G_TYPE_STRING,
					       G_TYPE_POINTER);
	gtk_tree_view_set_model (GTK_TREE_VIEW (GET_WIDGET ("track_treeview")), GTK_TREE_MODEL (data->list_store));
	add_columns (data, GTK_TREE_VIEW (GET_WIDGET ("track_treeview")));

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "delete-event",
			  G_CALLBACK (dialog_delete_event_cb),
			  data);
	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (dialog_destroy_cb),
			  data);
	g_signal_connect (GET_WIDGET ("cancel_button"),
			  "clicked",
			  G_CALLBACK (close_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("search_button"),
			  "clicked",
			  G_CALLBACK (search_cb),
			  data);
	g_signal_connect (GET_WIDGET ("artist_combobox"),
			  "changed",
			  G_CALLBACK (artist_combobox_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("prev_album_button"),
			  "clicked",
			  G_CALLBACK (prev_album_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("next_album_button"),
			  "clicked",
			  G_CALLBACK (next_album_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("undo_button"),
			  "clicked",
			  G_CALLBACK (undo_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("year_checkbutton"),
			  "toggled",
                          G_CALLBACK (year_checkbutton_toggled_cb),
                          data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);

	set_data_from_album (data, goo_window_get_album (data->window));
}
