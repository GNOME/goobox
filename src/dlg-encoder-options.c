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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gnome.h>
#include <glade/glade.h>
#include "typedefs.h"
#include "preferences.h"
#include "main.h"
#include "gconf-utils.h"
#include "goo-stock.h"
#include "dlg-extract.h"

#define GLADE_RIPPER_FILE "goobox.glade"

typedef struct {
	GtkWidget     *extract_dialog;
	GooFileFormat  format;
	int            value;
	gboolean       rip;

	GladeXML      *gui;

	GtkWidget     *dialog;
	GtkWidget     *eo_quality_label;
	GtkWidget     *eo_scale;
	GtkWidget     *eo_smaller_label;
	GtkWidget     *eo_higher_label;
	GtkWidget     *eo_info_label;
} DialogData;


static int ogg_rate[] = { 64, 80, 96, 112, 128, 160, 196, 224, 256, 320 };
static int mp3_rate[] = { 64, 80, 96, 112, 128, 160, 196, 224, 256, 320 };
static int flac_compression[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

#define N_VALUES 10
#define OGG_DEFAULT_VALUE 4
#define MP3_DEFAULT_VALUE 4
#define FLAC_DEFAULT_VALUE 5
#define DEFAULT_VALUE 4
#define DEFAULT_OGG_QUALITY 0.3
#define DEFAULT_FLAC_COMPRESSION 5
#define DEFAULT_MP3_BITRATE 128


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget, 
	    DialogData *data)
{
	g_object_unref (data->gui);
	if (data->rip)
		dlg_extract__start_ripping (data->extract_dialog);
	else
		dlg_extract__close (data->extract_dialog);
	g_free (data);
}


static void
ok_cb (GtkWidget  *widget, 
       DialogData *data)
{
	data->rip = TRUE;

	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:
		eel_gconf_set_float (PREF_ENCODER_OGG_QUALITY, (float) data->value / 10.0);
		break;
	case GOO_FILE_FORMAT_FLAC:
		eel_gconf_set_integer (PREF_ENCODER_FLAC_COMPRESSION, flac_compression[data->value]);
		break;
	case GOO_FILE_FORMAT_MP3:
		eel_gconf_set_integer (PREF_ENCODER_MP3_BITRATE, mp3_rate[data->value]);
		break;
	default:
		break;
	}

	gtk_widget_destroy (data->dialog);
}


static void
update_info (DialogData *data)
{
	char *text = NULL;

	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:
		text = g_strdup_printf (_("Nominal bitrate: %d Kbps"), ogg_rate[data->value]);
		break;
	case GOO_FILE_FORMAT_FLAC:
		text = g_strdup_printf (_("Compression level: %d"), flac_compression[data->value]);
		break;
	case GOO_FILE_FORMAT_MP3:
		text = g_strdup_printf (_("Bitrate: %d Kbps"), mp3_rate[data->value]);
		break;
	default:
		break;
	}

	if (text != NULL) {
		gtk_label_set_text (GTK_LABEL (data->eo_info_label), text);
		g_free (text);
	}
}


static void
reset_cb (GtkWidget  *widget, 
	  DialogData *data)
{
	switch (data->format) {
	case GOO_FILE_FORMAT_OGG:
		data->value = OGG_DEFAULT_VALUE;
		break;
	case GOO_FILE_FORMAT_FLAC:
		data->value = FLAC_DEFAULT_VALUE;
		break;
	case GOO_FILE_FORMAT_MP3:
		data->value = MP3_DEFAULT_VALUE;
		break;
	default:
		data->value = DEFAULT_VALUE;
		break;
	}
	
	gtk_range_set_value (GTK_RANGE (data->eo_scale), data->value * 10.0);
	update_info (data);
}


static void
eo_scale_value_changed_cb (GtkRange   *range,
			   DialogData *data)
{
	double value = gtk_range_get_value (range);
	int    i_value;

	i_value = (int) ((value / 10.0) + 0.5);
	if (data->value == i_value)
		return;
	data->value = i_value;
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
get_current_value (DialogData *data)
{
	int index = DEFAULT_VALUE;
	int value;

	switch (data->format) {
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
dlg_encoder_options (GtkWidget     *dialog,
		     GtkWindow     *parent,
		     GooFileFormat  format)
{
	DialogData  *data;
	GtkWidget   *btn_ok;
	GtkWidget   *btn_cancel;
	GtkWidget   *btn_reset;

	data = g_new0 (DialogData, 1);
	data->extract_dialog = dialog;
	data->format = format;
	data->value = -1;
	data->rip = FALSE;
	data->gui = glade_xml_new (GOO_GLADEDIR "/" GLADE_RIPPER_FILE, NULL, NULL);
        if (!data->gui) {
		g_warning ("Could not find " GLADE_RIPPER_FILE "\n");
		g_free (data);
                return;
        }

	/* Get the widgets. */

	data->dialog = glade_xml_get_widget (data->gui, "encoder_options_dialog");
	data->eo_quality_label = glade_xml_get_widget (data->gui, "eo_quality_label");
	data->eo_scale = glade_xml_get_widget (data->gui, "eo_scale");
	data->eo_smaller_label = glade_xml_get_widget (data->gui, "eo_smaller_label");
	data->eo_higher_label = glade_xml_get_widget (data->gui, "eo_higher_label");
	data->eo_info_label = glade_xml_get_widget (data->gui, "eo_info_label");

	btn_ok = glade_xml_get_widget (data->gui, "eo_okbutton");
	btn_cancel = glade_xml_get_widget (data->gui, "eo_cancelbutton");
	btn_reset = glade_xml_get_widget (data->gui, "eo_resetbutton");

	gtk_button_set_use_stock (GTK_BUTTON (btn_reset), TRUE);
	gtk_button_set_label (GTK_BUTTON (btn_reset), GOO_STOCK_RESET);

	/* Set widgets data. */

	if (data->format == GOO_FILE_FORMAT_FLAC) {
		char *text;
		text = g_strdup_printf ("<small><i>%s</i></small>", _("Faster compression"));
		gtk_label_set_markup (GTK_LABEL (data->eo_smaller_label), text);
		g_free (text);

		text = g_strdup_printf ("<small><i>%s</i></small>", _("Higher compression"));
		gtk_label_set_markup (GTK_LABEL (data->eo_higher_label), text);
		g_free (text);
	} 

	/**/

	data->value = get_current_value (data);
	gtk_range_set_value (GTK_RANGE (data->eo_scale), 
			     data->value * 10.0);
	update_info (data);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog), 
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect (G_OBJECT (data->eo_scale), 
			  "value_changed",
			  G_CALLBACK (eo_scale_value_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_reset), 
			  "clicked",
			  G_CALLBACK (reset_cb),
			  data);
	g_signal_connect (G_OBJECT (btn_ok), 
			  "clicked",
			  G_CALLBACK (ok_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (btn_cancel), 
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  data->dialog);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), parent);
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}
