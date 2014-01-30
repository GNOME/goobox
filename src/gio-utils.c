/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "glib-utils.h"
#include "gio-utils.h"


#define BUFFER_SIZE 4096


gboolean
g_load_file_in_buffer (GFile   *file,
		       void   **buffer,
		       gsize   *size,
		       GError **error)
{
	GFileInputStream *istream;
	gboolean          retval;
	guchar           *local_buffer;
	gsize             count;
	gssize            n;
	char              tmp_buffer[BUFFER_SIZE];

	istream = g_file_read (file, NULL, error);
	if (istream == NULL)
		return FALSE;

	retval = FALSE;
	local_buffer = NULL;
	count = 0;
	for (;;) {
		n = g_input_stream_read (G_INPUT_STREAM (istream), tmp_buffer, BUFFER_SIZE, NULL, error);
		if (n < 0) {
			g_free (local_buffer);
			retval = FALSE;
			break;
		}
		else if (n == 0) {
			*buffer = local_buffer;
			*size = count;
			retval = TRUE;
			break;
		}

		local_buffer = g_realloc (local_buffer, count + n + 1);
		memcpy (local_buffer + count, tmp_buffer, n);
		count += n;
	}

	g_object_unref (istream);

	return retval;
}


typedef struct {
	int                  io_priority;
	GCancellable        *cancellable;
	BufferReadyCallback  callback;
	gpointer             user_data;
	GInputStream        *stream;
	guchar               tmp_buffer[BUFFER_SIZE];
	guchar              *buffer;
	gsize                count;
} LoadData;


static void
load_data_free (LoadData *load_data)
{
	if (load_data->stream != NULL)
		g_object_unref (load_data->stream);
	g_free (load_data->buffer);
	g_free (load_data);
}


static void
load_file__stream_read_cb (GObject      *source_object,
			   GAsyncResult *result,
			   gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;
	gssize    count;

	count = g_input_stream_read_finish (load_data->stream, result, &error);
	if (count < 0) {
		load_data->callback (NULL, -1, error, load_data->user_data);
		load_data_free (load_data);
		return;
	}
	else if (count == 0) {
		if (load_data->buffer != NULL)
			((char *)load_data->buffer)[load_data->count] = 0;
		load_data->callback (load_data->buffer, load_data->count, NULL, load_data->user_data);
		load_data_free (load_data);
		return;
	}

	load_data->buffer = g_realloc (load_data->buffer, load_data->count + count + 1);
	memcpy (load_data->buffer + load_data->count, load_data->tmp_buffer, count);
	load_data->count += count;

	g_input_stream_read_async (load_data->stream,
				   load_data->tmp_buffer,
				   BUFFER_SIZE,
				   load_data->io_priority,
				   load_data->cancellable,
				   load_file__stream_read_cb,
				   load_data);
}


static void
load_file__file_read_cb (GObject      *source_object,
			 GAsyncResult *result,
			 gpointer      user_data)
{
	LoadData *load_data = user_data;
	GError   *error = NULL;

	load_data->stream = (GInputStream *) g_file_read_finish (G_FILE (source_object), result, &error);
	if (load_data->stream == NULL) {
		load_data->callback (NULL, -1, error, load_data->user_data);
		load_data_free (load_data);
		return;
	}

	load_data->count = 0;
	g_input_stream_read_async (load_data->stream,
				   load_data->tmp_buffer,
				   BUFFER_SIZE,
				   load_data->io_priority,
				   load_data->cancellable,
				   load_file__stream_read_cb,
				   load_data);
}


void
g_load_file_async (GFile               *file,
		   int                  io_priority,
		   GCancellable        *cancellable,
		   BufferReadyCallback  callback,
		   gpointer             user_data)
{
	LoadData *load_data;

	load_data = g_new0 (LoadData, 1);
	load_data->io_priority = io_priority;
	load_data->cancellable = cancellable;
	load_data->callback = callback;
	load_data->user_data = user_data;

	g_file_read_async (file, io_priority, cancellable, load_file__file_read_cb, load_data);
}


/* -- g_write_file_async -- */


typedef struct {
	int                  io_priority;
	GCancellable        *cancellable;
	BufferReadyCallback  callback;
	gpointer             user_data;
	guchar              *buffer;
	gsize                count;
	gsize                written;
	GError              *error;
} WriteData;


static void
write_data_free (WriteData *write_data)
{
	g_free (write_data);
}


static void
write_file__notify (gpointer user_data)
{
	WriteData *write_data = user_data;

	write_data->callback (write_data->buffer, write_data->count, write_data->error, write_data->user_data);
	write_data_free (write_data);
}


static void
write_file__stream_flush_cb (GObject      *source_object,
			     GAsyncResult *result,
			     gpointer      user_data)
{
	GOutputStream *stream = (GOutputStream *) source_object;
	WriteData     *write_data = user_data;
	GError        *error = NULL;

	g_output_stream_flush_finish (stream, result, &error);
	write_data->error = error;
	g_object_unref (stream);

	call_when_idle (write_file__notify, write_data);
}


static void
write_file__stream_write_ready_cb (GObject      *source_object,
				   GAsyncResult *result,
				   gpointer      user_data)
{
	GOutputStream *stream = (GOutputStream *) source_object;
	WriteData     *write_data = user_data;
	GError        *error = NULL;
	gssize         count;

	count = g_output_stream_write_finish (stream, result, &error);
	write_data->written += count;

	if ((count == 0) || (write_data->written == write_data->count)) {
		g_output_stream_flush_async (stream,
					     write_data->io_priority,
					     write_data->cancellable,
					     write_file__stream_flush_cb,
					     user_data);
		return;
	}

	g_output_stream_write_async (stream,
				     write_data->buffer + write_data->written,
				     write_data->count - write_data->written,
				     write_data->io_priority,
				     write_data->cancellable,
				     write_file__stream_write_ready_cb,
				     write_data);
}


static void
write_file__replace_ready_cb (GObject      *source_object,
			      GAsyncResult *result,
			      gpointer      user_data)
{
	WriteData     *write_data = user_data;
	GOutputStream *stream;
	GError        *error = NULL;

	stream = (GOutputStream*) g_file_replace_finish ((GFile*) source_object, result, &error);
	if (stream == NULL) {
		write_data->callback (write_data->buffer, write_data->count, error, write_data->user_data);
		write_data_free (write_data);
		return;
	}

	write_data->written = 0;
	g_output_stream_write_async (stream,
				     write_data->buffer,
				     write_data->count,
				     write_data->io_priority,
				     write_data->cancellable,
				     write_file__stream_write_ready_cb,
				     write_data);
}


void
g_write_file_async (GFile               *file,
		    void                *buffer,
		    gsize                count,
		    int                  io_priority,
		    GCancellable        *cancellable,
		    BufferReadyCallback  callback,
		    gpointer             user_data)
{
	WriteData *write_data;

	write_data = g_new0 (WriteData, 1);
	write_data->buffer = buffer;
	write_data->count = count;
	write_data->io_priority = io_priority;
	write_data->cancellable = cancellable;
	write_data->callback = callback;
	write_data->user_data = user_data;

	g_file_replace_async (file, NULL, FALSE, 0, io_priority, cancellable, write_file__replace_ready_cb, write_data);
}


GFile *
_g_file_create_unique (GFile       *parent,
		       const char  *display_name,
		       const char  *suffix,
		       GError     **error)
{
	GFile             *file = NULL;
	GError            *local_error = NULL;
	int                n;
	GFileOutputStream *stream;

	file = g_file_get_child_for_display_name (parent, display_name, &local_error);
	n = 0;
	do {
		char *new_display_name;

		if (file != NULL)
			g_object_unref (file);

		n++;
		if (n == 1)
			new_display_name = g_strdup_printf ("%s%s", display_name, suffix);
		else
			new_display_name = g_strdup_printf ("%s %d%s", display_name, n, suffix);

		file = g_file_get_child_for_display_name (parent, new_display_name, &local_error);
		if (local_error == NULL)
			stream = g_file_create (file, 0, NULL, &local_error);

		if ((stream == NULL) && g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			g_clear_error (&local_error);

		g_free (new_display_name);
	}
	while ((stream == NULL) && (local_error == NULL));

	if (stream == NULL) {
		g_object_unref (file);
		file = NULL;
	}
	else
		g_object_unref (stream);

	return file;
}


GFile *
_g_directory_create_unique (GFile       *parent,
			    const char  *display_name,
			    const char  *suffix,
			    GError     **error)
{
	GFile    *file = NULL;
	gboolean  created = FALSE;
	GError   *local_error = NULL;
	int       n;

	file = g_file_get_child_for_display_name (parent, display_name, &local_error);
	if (file == NULL) {
		g_propagate_error (error, local_error);
		return NULL;
	}

	n = 0;
	do {
		char *new_display_name;

		if (file != NULL)
			g_object_unref (file);

		n++;
		if (n == 1)
			new_display_name = g_strdup_printf ("%s%s", display_name, suffix);
		else
			new_display_name = g_strdup_printf ("%s %d%s", display_name, n, suffix);

		file = g_file_get_child_for_display_name (parent, new_display_name, &local_error);
		if (local_error == NULL)
			created = g_file_make_directory (file, NULL, &local_error);

		if (! created && g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
			g_clear_error (&local_error);

		g_free (new_display_name);
	}
	while (! created && (local_error == NULL));

	if (local_error != NULL) {
		g_object_unref (file);
		file = NULL;
	}

	if (local_error != NULL)
		g_propagate_error (error, local_error);

	return file;
}


/* -- g_write_file -- */


gboolean
g_write_file (GFile             *file,
	      gboolean           make_backup,
	      GFileCreateFlags   flags,
	      void              *buffer,
	      gsize              count,
	      GCancellable      *cancellable,
	      GError           **error)
{
	gboolean       success;
	GOutputStream *stream;

	stream = (GOutputStream *) g_file_replace (file, NULL, make_backup, flags, cancellable, error);
	if (stream != NULL)
		success = g_output_stream_write_all (stream, buffer, count, NULL, cancellable, error);
	else
		success = FALSE;

	_g_object_unref (stream);

	return success;
}
