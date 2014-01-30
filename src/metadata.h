/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2007 The Free Software Foundation, Inc.
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

#ifndef METADATA_H
#define METADATA_H

#include <glib.h>
#include "album-info.h"

void      metadata_get_cd_info_from_device            (const char           *device,
					               GCancellable         *cancellable,
					               GAsyncReadyCallback   callback,
					               gpointer              user_data);
gboolean  metadata_get_cd_info_from_device_finish     (GAsyncResult         *result,
						       char                **disc_id,
						       AlbumInfo           **album_info,
                				       GError              **error);
void      metadata_get_album_info_from_disc_id        (const char           *disc_id,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       gpointer              user_data);
GList *   metadata_get_album_info_from_disc_id_finish (GAsyncResult         *result,
						       GError              **error);
void      metadata_search_album_by_title              (const char           *title,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       gpointer              user_data);
GList *   metadata_search_album_by_title_finish       (GAsyncResult         *result,
						       GError              **error);

#endif /* METADATA_H */
