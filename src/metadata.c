/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2007-2011 Free Software Foundation, Inc.
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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <discid/discid.h>
#include <musicbrainz5/mb5_c.h>
#include "album-info.h"
#include "glib-utils.h"
#include "goo-error.h"
#include "metadata.h"


#define QUERY_AGENT (PACKAGE_NAME "-" PACKAGE_VERSION)

#ifndef DISCID_HAVE_SPARSE_READ
#define discid_read_sparse(disc, dev, i) discid_read(disc, dev)
#endif


static TrackInfo *
get_track_info (Mb5Track mb_track,
		int      n_track)
{
	TrackInfo         *track;
	Mb5Recording       recording;
	int                required_size = 0;
	char              *title = NULL;
	Mb5ArtistCredit    artist_credit;
	Mb5NameCreditList  name_credit_list;
	int                i;

	track = track_info_new (n_track, 0, 0);

	/* title */

	recording = mb5_track_get_recording (mb_track);
	if (recording != NULL) {
		required_size = mb5_recording_get_title (recording, title, 0);
		title = g_new (char, required_size + 1);
		mb5_recording_get_title (recording, title, required_size + 1);
	}
	else {
		required_size = mb5_track_get_title (mb_track, title, 0);
		title = g_new (char, required_size + 1);
		mb5_track_get_title (mb_track, title, required_size + 1);
	}
	track_info_set_title (track, title);
	debug (DEBUG_INFO, "==> [MB] TRACK %d: %s\n", n_track, title);

	g_free (title);

	/* artist */

	artist_credit = mb5_track_get_artistcredit (mb_track);
	name_credit_list = mb5_artistcredit_get_namecreditlist (artist_credit);
	for (i = 0; i < mb5_namecredit_list_size (name_credit_list); i++) {
		Mb5NameCredit  name_credit = mb5_namecredit_list_item (name_credit_list, i);
		Mb5Artist      artist;
		char          *artist_name = NULL;
		char          *artist_id = NULL;

		artist = mb5_namecredit_get_artist (name_credit);

		required_size = mb5_artist_get_name (artist, artist_name, 0);
		artist_name = g_new (char, required_size + 1);
		mb5_artist_get_name (artist, artist_name, required_size + 1);
		debug (DEBUG_INFO, "==> [MB] ARTIST NAME: %s\n", artist_name);

		required_size = mb5_artist_get_id (artist, artist_id, 0);
		artist_id = g_new (char, required_size + 1);
		mb5_artist_get_id (artist, artist_id, required_size + 1);
		debug (DEBUG_INFO, "==> [MB] ARTIST ID: %s\n", artist_id);

		track_info_set_artist (track, artist_name, artist_id);

		g_free (artist_name);
		g_free (artist_id);
	}

	return track;
}


static AlbumInfo *
get_album_info (Mb5Release release,
		Mb5Medium  medium)
{
	AlbumInfo         *album;
	char              *value;
	int                required_size;
	Mb5ReleaseGroup    release_group;
	Mb5ArtistCredit    artist_credit;
	Mb5NameCreditList  name_credit_list;
	int                i;
	GList             *tracks;
	Mb5TrackList       track_list;
	int                n_track;

	album = album_info_new ();

	/* id */

	value = NULL;
	required_size = mb5_release_get_id (release, value, 0);
	value = g_new (char, required_size + 1);
	mb5_release_get_id (release, value, required_size + 1);
	debug (DEBUG_INFO, "==> [MB] ALBUM_ID: %s\n", value);
	album_info_set_id (album, value);
	g_free (value);

	/* title */

	value = NULL;
	required_size = mb5_medium_get_title  (medium, value, 0);
	value = g_new (char, required_size + 1);
	mb5_medium_get_title  (medium, value, required_size + 1);
	debug (DEBUG_INFO, "==> [MB] MEDIUM NAME: %s\n", value);
	album_info_set_title (album, value);
	g_free (value);

	if ((album->title == NULL) || (album->title[0] == 0)) {
		value = NULL;
		required_size = mb5_release_get_title  (release, value, 0);
		value = g_new (char, required_size + 1);
		mb5_release_get_title  (release, value, required_size + 1);
		debug (DEBUG_INFO, "==> [MB] RELEASE NAME: %s\n", value);
		album_info_set_title (album, value);
		g_free (value);
	}

	/* asin */

	value = NULL;
	required_size = mb5_release_get_asin (release, value, 0);
	value = g_new (char, required_size + 1);
	mb5_release_get_asin (release, value, required_size + 1);
	debug (DEBUG_INFO, "==> [MB] ASIN: %s\n", value);
	album_info_set_asin (album, value);
	g_free (value);

	/* release date */

	release_group = mb5_release_get_releasegroup (release);
	value = NULL;
	required_size = mb5_releasegroup_get_firstreleasedate (release_group, value, 0);
	value = g_new (char, required_size + 1);
	mb5_releasegroup_get_firstreleasedate (release_group, value, required_size + 1);
	debug (DEBUG_INFO, "==> [MB] RELEASE DATE: %s\n", value);
	if (value != NULL) {
		int y = 0, m = 0, d = 0;

		if (sscanf (value, "%d-%d-%d", &y, &m, &d) > 0) {
			GDate *date;

			date = g_date_new_dmy ((d > 0) ? d : 1, (m > 0) ? m : 1, (y > 0) ? y : 1);
			album_info_set_release_date (album, date);
			g_date_free (date);
		}
	}
	g_free (value);

	/* artist */

	artist_credit = mb5_release_get_artistcredit (release);
	name_credit_list = mb5_artistcredit_get_namecreditlist (artist_credit);
	for (i = 0; i < mb5_namecredit_list_size (name_credit_list); i++) {
		Mb5NameCredit  name_credit = mb5_namecredit_list_item (name_credit_list, i);
		Mb5Artist      artist;
		char          *artist_name = NULL;
		char          *artist_id = NULL;

		artist = mb5_namecredit_get_artist (name_credit);

		required_size = mb5_artist_get_name (artist, artist_name, 0);
		artist_name = g_new (char, required_size + 1);
		mb5_artist_get_name (artist, artist_name, required_size + 1);
		debug (DEBUG_INFO, "==> [MB] ARTIST NAME: %s\n", artist_name);

		required_size = mb5_artist_get_id (artist, artist_id, 0);
		artist_id = g_new (char, required_size + 1);
		mb5_artist_get_id (artist, artist_id, required_size + 1);
		debug (DEBUG_INFO, "==> [MB] ARTIST ID: %s\n", artist_id);

		album_info_set_artist (album, artist_name, artist_id);

		g_free (artist_name);
		g_free (artist_id);
	}

	/* tracks */

	track_list = mb5_medium_get_tracklist (medium);

	debug (DEBUG_INFO, "==> [MB] N TRACKS: %d\n", mb5_track_list_size (track_list));

	tracks = NULL;
	for (n_track = 0; n_track < mb5_track_list_size (track_list); n_track++) {
		TrackInfo *track;

		track = get_track_info (mb5_track_list_item (track_list, n_track), n_track);
		if (album->artist == NULL)
			album_info_set_artist (album, track->artist, KEEP_PREVIOUS_VALUE);
		tracks = g_list_prepend (tracks, track);
	}
	tracks = g_list_reverse (tracks);
	album_info_set_tracks (album, tracks);

	return album;
}


static GList *
get_album_list (Mb5ReleaseList   release_list,
		const char      *disc_id,
		Mb5Query         query,
		GError         **error)
{
	GList *albums = NULL;
	int    i;

	debug (DEBUG_INFO, "[MB] Num Albums: %d (Disc ID: %s)\n", mb5_release_list_size (release_list), disc_id);

	for (i = 0; i < mb5_release_list_size (release_list); i++) {
		Mb5Release    release;
		char        **param_names;
		char        **param_values;
		char          release_id[256];
		Mb5Metadata   metadata;

		release = mb5_release_list_item (release_list, i);

		/* query the full release info */

		param_names = g_new (char *, 2);
		param_values = g_new (char *, 2);
		param_names[0] = g_strdup ("inc");
		param_values[0] = g_strdup ("artists labels recordings release-groups url-rels discids artist-credits");
		param_names[1] = NULL;
		param_values[1] = NULL;
		mb5_release_get_id (release, release_id, sizeof (release_id));

		metadata = mb5_query_query (query, "release", release_id, "", 1, param_names, param_values);
		if (metadata != NULL) {
			Mb5Release    release_info;
			Mb5MediumList medium_list;
			int           n_medium;

			release_info = mb5_metadata_get_release (metadata);
			if (disc_id != NULL)
				medium_list = mb5_release_media_matching_discid (release_info, disc_id);
			else
				medium_list = mb5_release_get_mediumlist (release_info);
			for (n_medium = 0; n_medium <= mb5_medium_list_size (medium_list); n_medium++) {
				Mb5Medium medium = mb5_medium_list_item (medium_list, n_medium);
				albums = g_list_prepend (albums, get_album_info (release_info, medium));
			}

			if (disc_id != NULL)
				mb5_medium_list_delete (medium_list);
			mb5_metadata_delete (metadata);
		}
		else if (error != NULL) {
			int   requested_size;
			char *error_message;

			requested_size = mb5_query_get_lasterrormessage (query, error_message, 0);
			error_message = g_new (char, requested_size + 1);
			mb5_query_get_lasterrormessage (query, error_message, requested_size + 1);
			*error = g_error_new (GOO_ERROR, GOO_ERROR_METADATA, "%s", error_message);

			g_free (error_message);

		}

		g_strfreev (param_names);
		g_strfreev (param_values);
	}

	return g_list_reverse (albums);
}


/* -- metadata_get_cd_info_from_device -- */


typedef struct {
	char      *device;
	char      *disc_id;
	AlbumInfo *album_info;
} GetCDInfoData;


static void
get_cd_info_data_free (GetCDInfoData *data)
{
	g_free (data->device);
	g_free (data->disc_id);
	album_info_unref (data->album_info);
	g_free (data);
}


static void
get_cd_info_from_device_thread (GTask        *task,
				gpointer      source_object,
				gpointer      task_data,
				GCancellable *cancellable)
{
	GetCDInfoData *data = task_data;
	GList         *tracks;
	DiscId        *disc;

	data->album_info = album_info_new ();
	tracks = NULL;
	disc = discid_new ();
	if (discid_read_sparse (disc, data->device, 0)) {
		int first_track;
		int last_track;
		int i;

		data->disc_id = g_strdup (discid_get_id (disc));
		debug (DEBUG_INFO, "==> [MB] DISC ID: %s\n", data->disc_id);

		first_track = discid_get_first_track_num (disc);
		debug (DEBUG_INFO, "==> [MB] FIRST TRACK: %d\n", first_track);

		last_track = discid_get_last_track_num (disc);
		debug (DEBUG_INFO, "==> [MB] LAST TRACK: %d\n", last_track);

		for (i = first_track; i <= last_track; i++) {
			gint64 from_sector;
			gint64 n_sectors;

			from_sector = discid_get_track_offset (disc, i);
			n_sectors = discid_get_track_length (disc, i);

			debug (DEBUG_INFO, "==> [MB] Track %d: [%"G_GINT64_FORMAT", %"G_GINT64_FORMAT"]\n", i, from_sector, from_sector + n_sectors);

			tracks = g_list_prepend (tracks, track_info_new (i - first_track, from_sector, from_sector + n_sectors));
		}
	}
	tracks = g_list_reverse (tracks);
	album_info_set_tracks (data->album_info, tracks);

	track_list_free (tracks);
	discid_free (disc);
}


void
metadata_get_cd_info_from_device (const char          *device,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
	GTask          *task;
	GetCDInfoData  *data;

	task = g_task_new (NULL, cancellable, callback, user_data);

	data = g_new0 (GetCDInfoData, 1);
	data->device = g_strdup (device);
	g_task_set_task_data (task, data, (GDestroyNotify) get_cd_info_data_free);

	g_task_run_in_thread (task, get_cd_info_from_device_thread);

	g_object_unref (task);
}


gboolean
metadata_get_cd_info_from_device_finish (GAsyncResult  *result,
					 char         **disc_id,
					 AlbumInfo    **album_info,
					 GError       **error)
{
	GetCDInfoData *data;

	g_return_val_if_fail (g_task_is_valid (result, NULL), FALSE);

        data = g_task_get_task_data (G_TASK (result));
        if (disc_id != NULL)
        	*disc_id = g_strdup (data->disc_id);
        if (album_info != NULL)
        	*album_info = album_info_ref (data->album_info);

        return TRUE;
}


/* -- metadata_get_album_info_from_disc_id -- */


typedef struct {
	char  *disc_id;
} AlbumFromIDData;


static void
album_from_id_data_free (AlbumFromIDData *data)
{
	g_free (data->disc_id);
	g_free (data);
}


static void
get_album_info_from_disc_id_thread (GTask        *task,
				    gpointer      source_object,
				    gpointer      task_data,
				    GCancellable *cancellable)
{
	AlbumFromIDData *data = task_data;
	Mb5Query         query;
	Mb5Metadata      metadata;

	query = mb5_query_new (QUERY_AGENT, NULL, 0);
	metadata = mb5_query_query (query, "discid", data->disc_id, "", 0, NULL, NULL);
	if (metadata != NULL) {
		Mb5Disc         disc;
		Mb5ReleaseList  release_list;
		GList          *albums;
		GError         *error = NULL;

		disc = mb5_metadata_get_disc (metadata);
		release_list = mb5_disc_get_releaselist (disc);
		albums = get_album_list (release_list, data->disc_id, query, &error);
		if (error != NULL)
			g_task_return_error (task, error);
		else
			g_task_return_pointer (task, albums, (GDestroyNotify) album_list_free);

		mb5_metadata_delete (metadata);
	}
	else {
		int   requested_size;
		char *error_message = NULL;

		requested_size = mb5_query_get_lasterrormessage (query, error_message, 0);
		error_message = g_new (char, requested_size + 1);
		mb5_query_get_lasterrormessage (query, error_message, requested_size + 1);
		g_task_return_new_error (task, GOO_ERROR, GOO_ERROR_METADATA, "%s", error_message);

		g_free (error_message);
	}

	mb5_query_delete (query);
}


void
metadata_get_album_info_from_disc_id (const char          *disc_id,
				      GCancellable        *cancellable,
				      GAsyncReadyCallback  callback,
				      gpointer             user_data)
{
	GTask           *task;
	AlbumFromIDData *data;


	task = g_task_new (NULL, cancellable, callback, user_data);

	data = g_new0 (AlbumFromIDData, 1);
	data->disc_id = g_strdup (disc_id);
	g_task_set_task_data (task, data, (GDestroyNotify) album_from_id_data_free);

	g_task_run_in_thread (task, get_album_info_from_disc_id_thread);

	g_object_unref (task);
}


GList *
metadata_get_album_info_from_disc_id_finish (GAsyncResult  *result,
					     GError       **error)
{
	g_return_val_if_fail (g_task_is_valid (result, NULL), NULL);
	return g_task_propagate_pointer (G_TASK (result), error);
}


/* -- metadata_search_album_by_title -- */


typedef struct {
	char  *title;
} SearchByTitleData;


static void
search_by_tile_data_free (SearchByTitleData *data)
{
	g_free (data->title);
	g_free (data);
}


static void
search_album_by_title_thread (GTask        *task,
			      gpointer      source_object,
			      gpointer      task_data,
			      GCancellable *cancellable)
{
	SearchByTitleData  *data = task_data;
	Mb5Query            query;
	char              **param_names;
	char              **param_values;
	Mb5Metadata         metadata;
	Mb5ReleaseList 	    release_list;

	query = mb5_query_new (PACKAGE_NAME, NULL, 0);

	param_names = g_new (char *, 3);
	param_values = g_new (char *, 3);

	param_names[0] = g_strdup ("query");
	param_values[0] = g_strdup_printf ("title:%s", data->title);
	param_names[1] = g_strdup ("limit");
	param_values[1] = g_strdup ("5");
	param_names[2] = NULL;
	param_values[2] = NULL;

	metadata = mb5_query_query (query, "release", "", "", 2, param_names, param_values);
	if (metadata != NULL) {
		GList  *albums;
		GError *error = NULL;

		release_list = mb5_metadata_get_releaselist (metadata);
		albums = get_album_list (release_list, NULL, query, &error);
		if (error != NULL)
			g_task_return_error (task, error);
		else
			g_task_return_pointer (task, albums, (GDestroyNotify) album_list_free);

		mb5_metadata_delete (metadata);
	}
	else {
		int   requested_size;
		char *error_message = NULL;

		requested_size = mb5_query_get_lasterrormessage (query, error_message, 0);
		error_message = g_new (char, requested_size + 1);
		mb5_query_get_lasterrormessage (query, error_message, requested_size + 1);
		g_task_return_new_error (task, GOO_ERROR, GOO_ERROR_METADATA, "%s", error_message);

		g_free (error_message);
	}

	g_strfreev (param_names);
	g_strfreev (param_values);
	mb5_query_delete (query);
}


void
metadata_search_album_by_title (const char          *title,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
	GTask              *task;
	SearchByTitleData  *data;

	task = g_task_new (NULL, cancellable, callback, user_data);

	data = g_new0 (SearchByTitleData, 1);
	data->title = g_strdup (title);
	g_task_set_task_data (task, data, (GDestroyNotify) search_by_tile_data_free);

	g_task_run_in_thread (task, search_album_by_title_thread);

	g_object_unref (task);
}


GList *
metadata_search_album_by_title_finish (GAsyncResult  *result,
				       GError       **error)
{
	g_return_val_if_fail (g_task_is_valid (result, NULL), NULL);
	return g_task_propagate_pointer (G_TASK (result), error);
}
