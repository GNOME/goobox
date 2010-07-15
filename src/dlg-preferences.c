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
#include <brasero/brasero-drive-selection.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "gconf-utils.h"
#include "goo-window.h"
#include "gtk-utils.h"
#include "preferences.h"
#include "typedefs.h"

#define N_VALUES 10
#define DEFAULT_OGG_QUALITY 0.5
#define DEFAULT_FLAC_COMPRESSION 5
#define GET_WIDGET(x) _gtk_builder_get_widget (data->builder, (x))


enum {
	TEXT_COLUMN,
	DATA_COLUMN,
	PRESENT_COLUMN,
	N_COLUMNS
};


static int flac_compression[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };


typedef struct {
	GooWindow    *window;
	GtkBuilder   *builder;
	GtkWidget    *dialog;
	GtkWidget    *drive_selector;
	GtkWidget    *filetype_combobox;
	GtkTreeModel *filetype_model;
	int           ogg_value;
	int           flac_value;
} DialogData;


static void
apply_button_clicked_cb (GtkWidget  *widget,
			 DialogData *data)
{
	const char   *destination;
	BraseroDrive *drive;

	destination = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")));
	eel_gconf_set_uri (PREF_EXTRACT_DESTINATION, destination);

	pref_set_file_format (gtk_combo_box_get_active (GTK_COMBO_BOX (data->filetype_combobox)));
	eel_gconf_set_boolean (PREF_EXTRACT_SAVE_PLAYLIST, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("save_playlist_checkbutton"))));
	eel_gconf_set_boolean (PREF_GENERAL_AUTOPLAY, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("autoplay_checkbutton"))));
	
	/**/

	drive = brasero_drive_selection_get_active (BRASERO_DRIVE_SELECTION (data->drive_selector));
	if (drive == NULL)
		return;

	eel_gconf_set_string (PREF_GENERAL_DEVICE, brasero_drive_get_device (drive));
	goo_window_set_drive (data->window, drive);

	g_object_unref (drive);
}


static void
dialog_destroy_cb (GtkWidget  *widget,
		   DialogData *data)
{
	apply_button_clicked_cb (widget, data);
	data->window->preferences_dialog = NULL;

	g_object_unref (data->builder);
	g_free (data);
}


static void
close_button_clicked_cb (GtkWidget  *widget,
			 DialogData *data)
{
	apply_button_clicked_cb (widget, data);
	gtk_widget_destroy (data->dialog);
}


static void
help_button_clicked_cb (GtkWidget  *widget,
			DialogData *data)
{
	show_help_dialog (GTK_WINDOW (data->window), "preferences");
}


void dlg_format (DialogData *dialog_data, GooFileFormat format);


static void
filetype_properties_clicked_cb (GtkWidget  *widget,
			        DialogData *data)
{
	dlg_format (data, gtk_combo_box_get_active (GTK_COMBO_BOX (data->filetype_combobox)));
}


static void
drive_selector_device_changed_cb (GtkWidget   *drive_selector,
				  const char  *device_path,
				  DialogData  *data)
{
	apply_button_clicked_cb (NULL, data);
}


static void
filetype_combobox_changed_cb (GtkComboBox *widget,
                              DialogData  *data)
{
	int format;
	
	format = gtk_combo_box_get_active (GTK_COMBO_BOX (data->filetype_combobox));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (GET_WIDGET ("encoding_notebook")), format);
	gtk_widget_set_sensitive (GET_WIDGET ("filetype_properties_button"), format != GOO_FILE_FORMAT_WAVE);
}


static void
set_description_label (DialogData *data, 
		       const char *widget_name, 
		       const char *label_text)
{
	char *text;
	
	text = g_markup_printf_escaped ("<small><i>%s</i></small>", label_text);
	gtk_label_set_markup (GTK_LABEL (GET_WIDGET (widget_name)), text);

	g_free (text);
}


void
dlg_preferences (GooWindow *window)
{
	DialogData      *data;
	char            *destination = NULL;
	GooFileFormat    file_format;
	GstElement      *encoder;
	gboolean         ogg_encoder, flac_encoder, wave_encoder;
	gboolean         find_first_available;
	GtkTreeIter      iter;
        GtkCellRenderer *renderer;
        
        if (window->preferences_dialog != NULL) {
        	gtk_window_present (GTK_WINDOW (window->preferences_dialog));
        	return;
        }
        
	data = g_new0 (DialogData, 1);
	data->window = window;
	data->builder = _gtk_builder_new_from_file ("preferences.ui", "");

	eel_gconf_preload_cache ("/apps/goobox/general", GCONF_CLIENT_PRELOAD_ONELEVEL);

	/* Get the widgets. */

	data->dialog = GET_WIDGET ("preferences_dialog");
	window->preferences_dialog = data->dialog;

	/* Set widgets data. */

	if (preferences_get_use_sound_juicer ()) {
		GtkWidget *notebook;
		GtkWidget *encoder_page;

		notebook = GET_WIDGET ("notebook");
		gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);

		encoder_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1);
		gtk_widget_hide (encoder_page);

		gtk_container_set_border_width (GTK_CONTAINER (GET_WIDGET ("general_vbox")), 0);
	}

	/* Extraction */
	
	data->filetype_model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS,
                                                                   G_TYPE_STRING,
                                                                   G_TYPE_INT,
                                                                   G_TYPE_BOOLEAN));
	data->filetype_combobox = gtk_combo_box_new_with_model (data->filetype_model);
	
	renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->filetype_combobox),
                                    renderer,
                                    FALSE);
        gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (data->filetype_combobox),
                                         renderer,
                                         "text", TEXT_COLUMN,
                                         "sensitive", PRESENT_COLUMN,
                                         NULL);
	gtk_widget_show (data->filetype_combobox);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("filetype_combobox_box")), data->filetype_combobox, TRUE, TRUE, 0);

	/**/
	
	destination = eel_gconf_get_uri (PREF_EXTRACT_DESTINATION, "");
	if ((destination == NULL) || (strcmp (destination, "") == 0))
		destination = g_filename_to_uri (g_get_user_special_dir (G_USER_DIRECTORY_MUSIC), NULL, NULL);
	if (destination == NULL)
		destination = g_filename_to_uri (g_get_home_dir (), NULL, NULL);
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (GET_WIDGET ("destination_filechooserbutton")), destination);
	g_free (destination);

	encoder = gst_element_factory_make (OGG_ENCODER, "encoder");
	ogg_encoder = encoder != NULL;
        gtk_list_store_append (GTK_LIST_STORE (data->filetype_model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (data->filetype_model),
                            &iter,
                            TEXT_COLUMN, _("Ogg Vorbis"),
                            DATA_COLUMN, GOO_FILE_FORMAT_OGG,
                            PRESENT_COLUMN, ogg_encoder,
                           -1);
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (FLAC_ENCODER, "encoder");
	flac_encoder = encoder != NULL;
        gtk_list_store_append (GTK_LIST_STORE (data->filetype_model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (data->filetype_model),
                            &iter,
                            TEXT_COLUMN, _("FLAC"),
                            DATA_COLUMN, GOO_FILE_FORMAT_FLAC,
                            PRESENT_COLUMN, flac_encoder,
                           -1);
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (WAVE_ENCODER, "encoder");
	wave_encoder = encoder != NULL;
        gtk_list_store_append (GTK_LIST_STORE (data->filetype_model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (data->filetype_model),
                            &iter,
                            TEXT_COLUMN, _("Waveform PCM"),
                            DATA_COLUMN, GOO_FILE_FORMAT_WAVE,
                            PRESENT_COLUMN, wave_encoder,
                           -1);
	if (encoder != NULL) 
		gst_object_unref (GST_OBJECT (encoder));

	file_format = pref_get_file_format ();

	find_first_available = (((file_format == GOO_FILE_FORMAT_OGG) && !ogg_encoder)
				|| ((file_format == GOO_FILE_FORMAT_FLAC) && !flac_encoder)
				|| ((file_format == GOO_FILE_FORMAT_WAVE) && !wave_encoder));

	if (find_first_available) {
		if (ogg_encoder)
			file_format = GOO_FILE_FORMAT_OGG;
		else if (flac_encoder)
			file_format = GOO_FILE_FORMAT_FLAC;
		else if (wave_encoder)
			file_format = GOO_FILE_FORMAT_WAVE;
	}
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->filetype_combobox), file_format);
	filetype_combobox_changed_cb (NULL, data);

	/**/
	
	set_description_label (data, "ogg_description_label", _("Vorbis is an open source, lossy audio codec with high quality output at a lower file size than MP3."));
	set_description_label (data, "flac_description_label", _("Free Lossless Audio Codec (FLAC) is an open source codec that compresses but does not degrade audio quality."));
	set_description_label (data, "wave_description_label", _("WAV+PCM is a lossless format that holds uncompressed, raw pulse-code modulated (PCM) audio."));
		
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("save_playlist_checkbutton")), eel_gconf_get_boolean (PREF_EXTRACT_SAVE_PLAYLIST, TRUE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("autoplay_checkbutton")), eel_gconf_get_boolean (PREF_GENERAL_AUTOPLAY, TRUE));
	
	/**/

	data->drive_selector = brasero_drive_selection_new ();
	gtk_widget_show (data->drive_selector);
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("drive_selector_box")), data->drive_selector, TRUE, TRUE, 0);

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "destroy",
			  G_CALLBACK (dialog_destroy_cb),
			  data);
	g_signal_connect (GET_WIDGET ("close_button"),
			  "clicked",
			  G_CALLBACK (close_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("help_button"),
			  "clicked",
			  G_CALLBACK (help_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("filetype_properties_button"),
			  "clicked",
			  G_CALLBACK (filetype_properties_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->drive_selector), 
			  "changed",
			  G_CALLBACK (drive_selector_device_changed_cb),
			  data);
	g_signal_connect (data->filetype_combobox,
			  "changed",
			  G_CALLBACK (filetype_combobox_changed_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_show (data->dialog);
}


/* -- format dialog -- */


typedef struct {
	GooFileFormat  format;
	int            value;
	
	GtkBuilder    *builder;
	GtkWidget     *dialog;
	GtkWidget     *f_quality_label;
	GtkWidget     *f_quality_scale;
} FormatDialogData;


static void
format_dialog_destroy_cb (GtkWidget        *widget, 
	    		  FormatDialogData *data)
{
	g_object_unref (data->builder);
	g_free (data);
}


static void
format_dialog_ok_button_clicked_cb (GtkWidget        *widget, 
	 			    FormatDialogData *data)
{
	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:
		eel_gconf_set_float (PREF_ENCODER_OGG_QUALITY, (float) data->value / 10.0);
		break;
	case GOO_FILE_FORMAT_FLAC:
		eel_gconf_set_integer (PREF_ENCODER_FLAC_COMPRESSION, flac_compression[data->value]);
		break;
	default:
		break;
	}
	
	gtk_widget_destroy (data->dialog);
}


static void
format_dialog_scale_value_changed_cb (GtkRange         *range,
				      FormatDialogData *data)
{
	data->value = (int) gtk_range_get_value (range);
}


static int 
find_index (int a[], int v)
{
	int i;
	for (i = 0; i < N_VALUES; i++)
		if (a[i] == v)
			return i;
	return 5;
}


static int
get_current_value (GooFileFormat format)
{
	int   index = 0;
	int   value;

	switch (format) {
	case GOO_FILE_FORMAT_OGG:
		index = (int) (eel_gconf_get_float (PREF_ENCODER_OGG_QUALITY, DEFAULT_OGG_QUALITY) * 10.0 + 0.05);
		break;
	case GOO_FILE_FORMAT_FLAC:
		value = eel_gconf_get_integer (PREF_ENCODER_FLAC_COMPRESSION, DEFAULT_FLAC_COMPRESSION);
		index = find_index (flac_compression, value);
		break;
	default:
		break;
	}

	return index;
}


static double
scale_value (double v)
{
	return v * 1.0 + 0.0;
}


void
dlg_format (DialogData    *preferences_data,
	    GooFileFormat  format)
{
	FormatDialogData *data;
        char             *text;
        
	data = g_new0 (FormatDialogData, 1);
	data->format = format;
	data->builder = _gtk_builder_new_from_file ("format-options.ui", "");
	data->dialog = GET_WIDGET ("format_dialog");
	
	/* Set widgets data. */		

	if (format == GOO_FILE_FORMAT_FLAC) {
		gtk_adjustment_set_upper (GTK_ADJUSTMENT (GET_WIDGET ("quality_adjustment")), 9.0);

		text = g_strdup_printf ("<small><i>%s</i></small>", _("Faster compression"));
		gtk_label_set_markup (GTK_LABEL (GET_WIDGET ("smaller_value_label")), text);
		g_free (text);

		text = g_strdup_printf ("<small><i>%s</i></small>", _("Higher compression"));
		gtk_label_set_markup (GTK_LABEL (GET_WIDGET ("bigger_value_label")), text);
		g_free (text);
	}
	else if (format == GOO_FILE_FORMAT_OGG) {
		gtk_adjustment_set_upper (GTK_ADJUSTMENT (GET_WIDGET ("quality_adjustment")), 10.0);

		text = g_strdup_printf ("<small><i>%s</i></small>", _("Smaller size"));
		gtk_label_set_markup (GTK_LABEL (GET_WIDGET ("smaller_value_label")), text);
		g_free (text);

		text = g_strdup_printf ("<small><i>%s</i></small>", _("Higher quality"));
		gtk_label_set_markup (GTK_LABEL (GET_WIDGET ("bigger_value_label")), text);
		g_free (text);
	}

	data->value = get_current_value (format);
	gtk_range_set_value (GTK_RANGE (GET_WIDGET ("quality_scale")), scale_value (data->value));

	switch (format) {
	case GOO_FILE_FORMAT_OGG:
		text = g_strdup_printf ("<big><b>%s</b></big>", _("Ogg Vorbis"));
		break;
	case GOO_FILE_FORMAT_FLAC:
		text = g_strdup_printf ("<big><b>%s</b></big>", _("FLAC"));
		break;
	default:
		text = g_strdup ("");
		break;
	}
	gtk_label_set_markup (GTK_LABEL (GET_WIDGET ("title_label")), text);
	g_free (text);

	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:
		text = _("Quality:");
		break;
	case GOO_FILE_FORMAT_FLAC:
		text = _("Compression level:");
		break;
	default:
		text = "";
		break;
	}
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("quality_label")), text);

	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:
		text = _("Vorbis is an open source, lossy audio codec with high quality output at a lower file size than MP3.");
		break;
	case GOO_FILE_FORMAT_FLAC:
		text = _("Free Lossless Audio Codec (FLAC) is an open source codec that compresses but does not degrade audio quality.");
		break;
	case GOO_FILE_FORMAT_WAVE:
		text = _("WAV+PCM is a lossless format that holds uncompressed, raw pulse-code modulated (PCM) audio.");
		break;
	default:
		text = "";
		break;
	}
	gtk_label_set_text (GTK_LABEL (GET_WIDGET ("description_label")), text);

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "destroy",
			  G_CALLBACK (format_dialog_destroy_cb),
			  data);

	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (format_dialog_ok_button_clicked_cb),
			  data);
	g_signal_connect (GET_WIDGET ("quality_scale"),
			  "value_changed",
			  G_CALLBACK (format_dialog_scale_value_changed_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (preferences_data->dialog));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
