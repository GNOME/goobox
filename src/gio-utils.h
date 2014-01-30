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

#ifndef _GIO_UTILS_H
#define _GIO_UTILS_H

#include <glib.h>
#include <gio/gio.h>
#include "typedefs.h"

G_BEGIN_DECLS

/* callback types */

typedef void (*BufferReadyCallback)  (void        *buffer,
				      gsize        count,
				      GError      *error,
				      gpointer     user_data);

/* -- load/write/create file  -- */

gboolean g_load_file_in_buffer       (GFile                 *file,
				      void                 **buffer,
				      gsize                 *size,
				      GError               **error);
void     g_load_file_async           (GFile                 *file,
				      int                    io_priority,
				      GCancellable          *cancellable,
				      BufferReadyCallback    callback,
				      gpointer               user_data);
gboolean g_write_file                (GFile                 *file,
				      gboolean               make_backup,
			              GFileCreateFlags       flags,
				      void                  *buffer,
				      gsize                  count,
				      GCancellable          *cancellable,
				      GError               **error);
void     g_write_file_async          (GFile                 *file,
				      void                  *buffer,
		    		      gsize                  count,
				      int                    io_priority,
				      GCancellable          *cancellable,
				      BufferReadyCallback    callback,
				      gpointer               user_data);
GFile * _g_file_create_unique        (GFile                 *parent,
				      const char            *display_name,
				      const char            *suffix,
				      GError               **error);
GFile * _g_directory_create_unique   (GFile                 *parent,
				      const char            *display_name,
				      const char            *suffix,
				      GError               **error);

G_END_DECLS

#endif /* _GIO_UTILS_H */
