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
#include <libgnomeui/gnome-dialog.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-propertybox.h>
#include <libgnomeui/gnome-pixmap.h>
#include <glade/glade.h>
#include "main.h"
#include "gconf-utils.h"
#include "typedefs.h"
#include "goo-window.h"
#include "preferences.h"
#include "bacon-cd-selection.h"
#include "goo-stock.h"

#define GLADE_PREF_FILE "goobox.glade"

static int ogg_rate[] = { 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
static int mp3_rate[] = { 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
static int flac_compression[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

#define N_VALUES 10
#define OGG_DEFAULT_VALUE 4
#define MP3_DEFAULT_VALUE 4
#define FLAC_DEFAULT_VALUE 5
#define DEFAULT_VALUE 4
#define DEFAULT_OGG_QUALITY 0.3
#define DEFAULT_FLAC_COMPRESSION 5
#define DEFAULT_MP3_BITRATE 128

typedef struct {
	GooWindow *window;
	int        ogg_value;
	int        flac_value;
	int        mp3_value;

	GladeXML  *gui;

	GtkWidget *dialog;
	GtkWidget *drive_selector;

	GtkWidget *p_ogg_quality_label;
	GtkWidget *p_ogg_scale;
	GtkWidget *p_ogg_smaller_label;
	GtkWidget *p_ogg_higher_label;

	GtkWidget *p_flac_quality_label;
	GtkWidget *p_flac_scale;
	GtkWidget *p_flac_smaller_label;
	GtkWidget *p_flac_higher_label;

	GtkWidget *p_mp3_quality_label;
	GtkWidget *p_mp3_scale;
	GtkWidget *p_mp3_smaller_label;
	GtkWidget *p_mp3_higher_label;
} DialogData;


/* called when the "apply" button is clicked. */
static void
apply_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	const char *device;

	eel_gconf_set_float (PREF_ENCODER_OGG_QUALITY, (float) data->ogg_value / 10.0);
	eel_gconf_set_integer (PREF_ENCODER_FLAC_COMPRESSION, flac_compression[data->flac_value]);
	eel_gconf_set_integer (PREF_ENCODER_MP3_BITRATE, mp3_rate[data->mp3_value]);

	/**/

	device = bacon_cd_selection_get_device (BACON_CD_SELECTION (data->drive_selector));
	if (device != NULL) 
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


static void
update_info (DialogData *data)
{
	char *text = NULL;

	text = g_strdup_printf (_("Nominal bitrate: %d Kbps"), ogg_rate[data->ogg_value]);
	gtk_label_set_text (GTK_LABEL (data->p_ogg_quality_label), text);
	g_free (text);

	text = g_strdup_printf (_("Compression level: %d"), flac_compression[data->flac_value]);
	gtk_label_set_text (GTK_LABEL (data->p_flac_quality_label), text);
	g_free (text);

	text = g_strdup_printf (_("Bitrate: %d Kbps"), mp3_rate[data->mp3_value]);
	gtk_label_set_text (GTK_LABEL (data->p_mp3_quality_label), text);
	g_free (text);
}


static void
reset_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	data->ogg_value = OGG_DEFAULT_VALUE;
	gtk_range_set_value (GTK_RANGE (data->p_ogg_scale), data->ogg_value * 10.0);

	data->flac_value = FLAC_DEFAULT_VALUE;
	gtk_range_set_value (GTK_RANGE (data->p_flac_scale), data->flac_value * 10.0);

	data->mp3_value = MP3_DEFAULT_VALUE;
	gtk_range_set_value (GTK_RANGE (data->p_mp3_scale), data->mp3_value * 10.0);

	update_info (data);
}


static void
drive_selector_device_changed_cb (GtkOptionMenu *option_menu,
				  const char    *device_path,
				  DialogData    *data)
{
	apply_cb (NULL, data);
}


static void
scale_value_changed_cb (GtkRange   *range,
			DialogData *data)
{
	double value = gtk_range_get_value (range);
	int    i_value;

	i_value = (int) ((value / 10.0) + 0.5);

	if (range == (GtkRange*) data->p_ogg_scale) {
		if (data->ogg_value == i_value)
			return;
		data->ogg_value = i_value;

	} else if (range == (GtkRange*) data->p_flac_scale) {
		if (data->flac_value == i_value)
			return;
		data->flac_value = i_value;

	} else if (range == (GtkRange*) data->p_mp3_scale) {
		if (data->mp3_value == i_value)
			return;
		data->mp3_value = i_value;
	} 

	update_info (data);
}





static int 
find_index (int a[], int v)
{
	int i;
	for (i = 0; i < N_VALUES; i++)
		if (a[i] == v)
			return i;
	return DEFAULT_VALUE;
}


static int
get_current_value (DialogData    *data,
		   GooFileFormat  format)
{
	int index = DEFAULT_VALUE;
	int value;

	switch (format) {
	case GOO_FILE_FORMAT_OGG:
		index = (int) (eel_gconf_get_float (PREF_ENCODER_OGG_QUALITY, DEFAULT_OGG_QUALITY) * 10.0 + 0.05);
		break;

	case GOO_FILE_FORMAT_FLAC:
		value = eel_gconf_get_integer (PREF_ENCODER_FLAC_COMPRESSION, DEFAULT_FLAC_COMPRESSION);
		index = find_index (flac_compression, value);
		break;

	case GOO_FILE_FORMAT_MP3:
		value = eel_gconf_get_integer (PREF_ENCODER_MP3_BITRATE, DEFAULT_MP3_BITRATE);
		index = find_index (mp3_rate, value);
		break;

	default:
		break;
	}

	return index;
}


/* create the main dialog. */
void
dlg_preferences (GooWindow *window)
{
	DialogData       *data;
	GtkWidget        *btn_close;
	GtkWidget        *btn_help;
	GtkWidget        *btn_reset;
	GtkWidget        *box;
	char             *device = NULL;
	char             *text;

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_PREF_FILE, NULL, NULL);
        if (!data->gui) {
                g_warning ("Could not find " GLADE_PREF_FILE "\n");
		g_free (data);
                return;
        }

	eel_gconf_preload_cache ("/apps/goobox/general", GCONF_CLIENT_PRELOAD_ONELEVEL);

	goo_window_set_location (data->window, NULL);

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "preferences_dialog");

	data->p_ogg_quality_label = glade_xml_get_widget (data->gui, "p_ogg_quality_label");
	data->p_ogg_scale = glade_xml_get_widget (data->gui, "p_ogg_scale");
	data->p_ogg_smaller_label = glade_xml_get_widget (data->gui, "p_ogg_smaller_label");
	data->p_ogg_higher_label = glade_xml_get_widget (data->gui, "p_ogg_higher_label");

	data->p_flac_quality_label = glade_xml_get_widget (data->gui, "p_flac_quality_label");
	data->p_flac_scale = glade_xml_get_widget (data->gui, "p_flac_scale");
	data->p_flac_smaller_label = glade_xml_get_widget (data->gui, "p_flac_smaller_label");
	data->p_flac_higher_label = glade_xml_get_widget (data->gui, "p_flac_higher_label");

	data->p_mp3_quality_label = glade_xml_get_widget (data->gui, "p_mp3_quality_label");
	data->p_mp3_scale = glade_xml_get_widget (data->gui, "p_mp3_scale");
	data->p_mp3_smaller_label = glade_xml_get_widget (data->gui, "p_mp3_smaller_label");
	data->p_mp3_higher_label = glade_xml_get_widget (data->gui, "p_mp3_higher_label");

	box = glade_xml_get_widget (data->gui, "p_drive_selector_box");
	btn_close = glade_xml_get_widget (data->gui, "p_closebutton");
	btn_help = glade_xml_get_widget (data->gui, "p_helpbutton");

	btn_reset = glade_xml_get_widget (data->gui, "p_resetbutton");
	gtk_button_set_use_stock (GTK_BUTTON (btn_reset), TRUE);
	gtk_button_set_label (GTK_BUTTON (btn_reset), GOO_STOCK_RESET);

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

	text = g_strdup_printf ("<small><i>%s</i></small>", _("Smaller size"));
	gtk_label_set_markup (GTK_LABEL (data->p_ogg_smaller_label), text);
	g_free (text);

	text = g_strdup_printf ("<small><i>%s</i></small>", _("Higher quality"));
	gtk_label_set_markup (GTK_LABEL (data->p_ogg_higher_label), text);
	g_free (text);

	text = g_strdup_printf ("<small><i>%s</i></small>", _("Smaller size"));
	gtk_label_set_markup (GTK_LABEL (data->p_mp3_smaller_label), text);
	g_free (text);

	text = g_strdup_printf ("<small><i>%s</i></small>", _("Higher quality"));
	gtk_label_set_markup (GTK_LABEL (data->p_mp3_higher_label), text);
	g_free (text);

	text = g_strdup_printf ("<small><i>%s</i></small>", _("Faster compression"));
	gtk_label_set_markup (GTK_LABEL (data->p_flac_smaller_label), text);
	g_free (text);

	text = g_strdup_printf ("<small><i>%s</i></small>", _("Higher compression"));
	gtk_label_set_markup (GTK_LABEL (data->p_flac_higher_label), text);
	g_free (text);

	/**/

	data->ogg_value = get_current_value (data, GOO_FILE_FORMAT_OGG);
	gtk_range_set_value (GTK_RANGE (data->p_ogg_scale), data->ogg_value * 10.0);

	data->flac_value = get_current_value (data, GOO_FILE_FORMAT_FLAC);
	gtk_range_set_value (GTK_RANGE (data->p_flac_scale), data->flac_value * 10.0);

	data->mp3_value = get_current_value (data, GOO_FILE_FORMAT_MP3);
	gtk_range_set_value (GTK_RANGE (data->p_mp3_scale), data->mp3_value * 10.0);

	update_info (data);

	/**/

	data->drive_selector = bacon_cd_selection_new ();
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
	g_signal_connect (G_OBJECT (btn_reset), 
			  "clicked",
			  G_CALLBACK (reset_cb),
			  data);

	g_signal_connect (G_OBJECT (data->drive_selector), 
			  "device_changed",
			  G_CALLBACK (drive_selector_device_changed_cb),
			  data);

	g_signal_connect (G_OBJECT (data->p_ogg_scale), 
			  "value_changed",
			  G_CALLBACK (scale_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->p_flac_scale), 
			  "value_changed",
			  G_CALLBACK (scale_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->p_mp3_scale), 
			  "value_changed",
			  G_CALLBACK (scale_value_changed_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
