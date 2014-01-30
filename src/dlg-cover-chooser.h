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
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DLG_COVER_CHOOSER_H
#define DLG_COVER_CHOOSER_H

#include "goo-window.h"
#include "album-info.h"

typedef enum {
	FETCH_COVER_STAGE_0,
	FETCH_COVER_STAGE_AFTER_LIBCOVERART,
	FETCH_COVER_STAGE_AFTER_ASIN,
	FETCH_COVER_STAGE_AFTER_WEB_SEARCH
} FetchCoverStage;

void   dlg_cover_chooser                 (GooWindow       *window,
					  const char      *album,
				       	  const char      *artist);
void   fetch_cover_image_from_name       (GooWindow       *window,
				      	  const char      *album,
				          const char      *artist,
					  GCancellable    *cancellable);
void   fetch_cover_image_from_asin       (GooWindow       *window,
					  const char      *asin,
					  GCancellable    *cancellable);
void   fetch_cover_image_from_album_info (GooWindow       *window,
					  AlbumInfo       *album,
					  FetchCoverStage  stage,
					  GCancellable    *cancellable);

#endif /* DLG_COVER_CHOOSER_H */
