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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <gst/gst.h>
#include "main.h"
#include "gconf-utils.h"
#include "typedefs.h"
#include "goo-window.h"
#include "preferences.h"
#include "bacon-cd-selection.h"
#include "goo-stock.h"
#include "file-utils.h"

#define GLADE_PREF_FILE "goobox.glade"

enum { TEXT_COLUMN, DATA_COLUMN, PRESENT_COLUMN, N_COLUMNS };

/*static int ogg_rate[] = { 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };*/
static int flac_compression[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

#define N_VALUES 10
#define DEFAULT_OGG_QUALITY 0.5
#define DEFAULT_FLAC_COMPRESSION 5

typedef struct {
	GooWindow    *window;
	int           ogg_value;
	int           flac_value;

	GladeXML     *gui;

	GtkWidget    *dialog;
	GtkWidget    *drive_selector;

	GtkWidget    *p_destination_filechooserbutton;

	GtkTreeModel *filetype_model;
	GtkWidget    *p_filetype_combobox;
	GtkWidget    *p_encoding_notebook;
	GtkWidget    *p_filetype_properties_button;
	GtkWidget    *p_save_playlist_checkbutton;
} DialogData;


/* called when the "apply" button is clicked. */
static void
apply_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	const char    *destination;
	const char    *device;
	const char    *current_device;

	destination = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->p_destination_filechooserbutton));
	eel_gconf_set_path (PREF_EXTRACT_DESTINATION, destination);

	pref_set_file_format (gtk_combo_box_get_active (GTK_COMBO_BOX (data->p_filetype_combobox)));
	eel_gconf_set_boolean (PREF_EXTRACT_SAVE_PLAYLIST, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->p_save_playlist_checkbutton)));
	
	/**/

	device = bacon_cd_selection_get_device (BACON_CD_SELECTION (data->drive_selector));
	if (device == NULL) 
		return;
		
	current_device = goo_player_get_location (goo_window_get_player (data->window));
	
	if ((current_device != NULL) && (strcmp (current_device, device) == 0)) 
		return;
		
	eel_gconf_set_string (PREF_GENERAL_DEVICE, device);
	goo_window_set_location (data->window, device);
	goo_window_update (data->window);
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	apply_cb (widget, data);
	data->window->preferences_dialog = NULL;
	g_object_unref (G_OBJECT (data->gui));
	g_free (data);
}


/* called when the "close" button is clicked. */
static void
close_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	apply_cb (widget, data);
	gtk_widget_destroy (data->dialog);
}


/* called when the "help" button is clicked. */
static void
help_cb (GtkWidget  *widget, 
	 DialogData *data)
{
	GError *err;

	err = NULL;  
	gnome_help_display ("goo", "preferences", &err);
	
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


void dlg_format (DialogData *dialog_data, GooFileFormat format);


static void
filetype_properties_clicked_cb	(GtkWidget  *widget, 
				 DialogData *data)
{
	int format;
	
	format = gtk_combo_box_get_active (GTK_COMBO_BOX (data->p_filetype_combobox));
	dlg_format (data, format);	
}


static void
drive_selector_device_changed_cb (GtkOptionMenu *option_menu,
				  const char    *device_path,
				  DialogData    *data)
{
	apply_cb (NULL, data);
}


static void
filetype_combobox_changed_cb (GtkComboBox *widget,
                              DialogData  *data)
{
	int format;
	
	format = gtk_combo_box_get_active (GTK_COMBO_BOX (data->p_filetype_combobox));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (data->p_encoding_notebook), format);
	gtk_widget_set_sensitive (data->p_filetype_properties_button, format != GOO_FILE_FORMAT_WAVE);
}


static void
set_description_label (DialogData *data, 
		       const char *widget_name, 
		       const char *label_text)
{
	GtkWidget *label;
	char      *text;
	
	label = glade_xml_get_widget (data->gui, widget_name);
	text = g_markup_printf_escaped ("<small><i>%s</i></small>", label_text);
	gtk_label_set_markup (GTK_LABEL (label), text);
	g_free (text);
}


void
dlg_preferences (GooWindow *window)
{
	DialogData      *data;
	GtkWidget       *btn_close;
	GtkWidget       *btn_help;
	GtkWidget       *box;
	GtkWidget       *filetype_combobox_box;
	GtkWidget       *p_filetype_properties_button;
	char            *device = NULL;
	char            *destination = NULL;
	GooFileFormat    file_format;
	GstElement      *encoder;
	gboolean         ogg_encoder, flac_encoder, wave_encoder;
	gboolean         find_first_available;
	char            *esc_uri = NULL;
	GtkTreeIter      iter;
        GtkCellRenderer *renderer;
        
        if (window->preferences_dialog != NULL) {
        	gtk_window_present (GTK_WINDOW (window->preferences_dialog));
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

	eel_gconf_preload_cache ("/apps/goobox/general", GCONF_CLIENT_PRELOAD_ONELEVEL);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "preferences_dialog");
	window->preferences_dialog = data->dialog;

	data->p_destination_filechooserbutton = glade_xml_get_widget (data->gui, "p_destination_filechooserbutton");

	filetype_combobox_box = glade_xml_get_widget (data->gui, "filetype_combobox_box");
	data->p_encoding_notebook = glade_xml_get_widget (data->gui, "p_encoding_notebook");
	data->p_filetype_properties_button = glade_xml_get_widget (data->gui, "p_filetype_properties_button");
	data->p_save_playlist_checkbutton = glade_xml_get_widget (data->gui, "p_save_playlist_checkbutton");
	p_filetype_properties_button = glade_xml_get_widget (data->gui, "p_filetype_properties_button");
	
	box = glade_xml_get_widget (data->gui, "p_drive_selector_box");
	btn_close = glade_xml_get_widget (data->gui, "p_closebutton");
	btn_help = glade_xml_get_widget (data->gui, "p_helpbutton");

	/* Set widgets data. */

	if (preferences_get_use_sound_juicer ()) {
		GtkWidget *notebook;
		GtkWidget *encoder_page;
		GtkWidget *vbox;

		notebook = glade_xml_get_widget (data->gui, "p_notebook");
		gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);

		encoder_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1);
		gtk_widget_hide (encoder_page);

		vbox = glade_xml_get_widget (data->gui, "general_vbox");
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	}

	/* Extraction */
	
	data->filetype_model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS,
                                                                   G_TYPE_STRING,
                                                                   G_TYPE_INT,
                                                                   G_TYPE_BOOLEAN));
	data->p_filetype_combobox = gtk_combo_box_new_with_model (data->filetype_model);
	
	renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (data->p_filetype_combobox),
                                    renderer,
                                    FALSE);
        gtk_cell_layout_set_attributes  (GTK_CELL_LAYOUT (data->p_filetype_combobox),
                                         renderer,
                                         "text", TEXT_COLUMN,
                                         "sensitive", PRESENT_COLUMN,
                                         NULL);
	gtk_widget_show (data->p_filetype_combobox);
	gtk_box_pack_start (GTK_BOX (filetype_combobox_box), data->p_filetype_combobox, TRUE, TRUE, 0);

	/**/
	
	
	destination = eel_gconf_get_path (PREF_EXTRACT_DESTINATION, "");
	if ((destination == NULL) || (strcmp (destination, "") == 0)) { 
		char *tmp;
		
		tmp = xdg_user_dir_lookup ("MUSIC");
		destination = get_uri_from_local_path (tmp);
		g_free (tmp);
	}	
	gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (data->p_destination_filechooserbutton), destination);
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
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (data->p_filetype_combobox), file_format);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (data->p_encoding_notebook), file_format);

	/**/
	
	set_description_label (data, "ogg_description_label", _("Vorbis is an open source, lossy audio codec with high quality output at a lower file size than MP3."));
	set_description_label (data, "flac_description_label", _("Free Lossless Audio Codec (FLAC) is an open source codec that compresses but does not degrade audio quality."));
	set_description_label (data, "wave_description_label", _("WAV+PCM is a lossless format that holds uncompressed, raw pulse-code modulated (PCM) audio."));
		
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->p_save_playlist_checkbutton), eel_gconf_get_boolean (PREF_EXTRACT_SAVE_PLAYLIST, TRUE));

	/**/

	data->drive_selector = bacon_cd_selection_new (Drives, goo_player_get_drive (goo_window_get_player (window)));
	gtk_widget_show (data->drive_selector);
	gtk_box_pack_start (GTK_BOX (box), data->drive_selector, TRUE, TRUE, 0);
	
	device = eel_gconf_get_string (PREF_GENERAL_DEVICE, bacon_cd_selection_get_default_device (BACON_CD_SELECTION (data->drive_selector)));
	bacon_cd_selection_set_device (BACON_CD_SELECTION (data->drive_selector), device);
	g_free (device);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (btn_close), 
			  "clicked",
			  G_CALLBACK (close_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_help), 
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);

	g_signal_connect (G_OBJECT (p_filetype_properties_button), 
			  "clicked",
			  G_CALLBACK (filetype_properties_clicked_cb),
			  data);

	g_signal_connect (G_OBJECT (data->drive_selector), 
			  "device_changed",
			  G_CALLBACK (drive_selector_device_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->p_filetype_combobox), 
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
	
	GladeXML      *gui;
	GtkWidget     *dialog;
	GtkWidget     *f_quality_label;
	GtkWidget     *f_quality_scale;
} FormatDialogData;


static void
format_dialog_destroy_cb (GtkWidget        *widget, 
	    		  FormatDialogData *data)
{
	g_object_unref (G_OBJECT (data->gui));
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
	GtkWidget  *btn_ok;
        GtkWidget  *f_smaller_value_label;
        GtkWidget  *f_bigger_value_label;
        GtkWidget  *f_title_label;
        GtkWidget  *f_description_label;
        char       *text;
        
	data = g_new0 (FormatDialogData, 1);
	data->format = format;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_PREF_FILE, NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_PREF_FILE "\n");
		g_free (data);
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "format_dialog");

	data->f_quality_label = glade_xml_get_widget (data->gui, "f_quality_label");
	data->f_quality_scale = glade_xml_get_widget (data->gui, "f_quality_scale");
	f_smaller_value_label = glade_xml_get_widget (data->gui, "f_smaller_value_label");
	f_bigger_value_label = glade_xml_get_widget (data->gui, "f_bigger_value_label");
	f_title_label = glade_xml_get_widget (data->gui, "f_title_label");
	f_description_label = glade_xml_get_widget (data->gui, "f_description_label");
	
	btn_ok = glade_xml_get_widget (data->gui, "f_okbutton");
	
	/* Set widgets data. */		

	data->value = get_current_value (format);
	gtk_range_set_value (GTK_RANGE (data->f_quality_scale), scale_value (data->value));

	if (format == GOO_FILE_FORMAT_FLAC) {	
		text = g_strdup_printf ("<small><i>%s</i></small>", _("Faster compression"));
		gtk_label_set_markup (GTK_LABEL (f_smaller_value_label), text);
		g_free (text);

		text = g_strdup_printf ("<small><i>%s</i></small>", _("Higher compression"));
		gtk_label_set_markup (GTK_LABEL (f_bigger_value_label), text);
		g_free (text);
	}
	else {
		text = g_strdup_printf ("<small><i>%s</i></small>", _("Smaller size"));
		gtk_label_set_markup (GTK_LABEL (f_smaller_value_label), text);
		g_free (text);

		text = g_strdup_printf ("<small><i>%s</i></small>", _("Higher quality"));
		gtk_label_set_markup (GTK_LABEL (f_bigger_value_label), text);
		g_free (text);
	}

	switch (format) {
	case GOO_FILE_FORMAT_OGG:
		text = g_strdup_printf ("<big><b>%s</b></big>", _("Ogg Vorbis"));
		break;
	case GOO_FILE_FORMAT_FLAC:
		text = g_strdup_printf ("<big><b>%s</b></big>", _("FLAC"));
		break;
	default:
		text = "";
		break;
	}
	gtk_label_set_markup (GTK_LABEL (f_title_label), text);
	g_free (text);

	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:
		text = _("Quality:");
		break;
	case GOO_FILE_FORMAT_FLAC:
		text = _("Compression level:");
		break;
	default:
		break;
	}
	gtk_label_set_text (GTK_LABEL (data->f_quality_label), text);

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
		break;
	}
	gtk_label_set_text (GTK_LABEL (f_description_label), text);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (format_dialog_destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (format_dialog_ok_button_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->f_quality_scale), 
			  "value_changed",
			  G_CALLBACK (format_dialog_scale_value_changed_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (preferences_data->dialog));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
