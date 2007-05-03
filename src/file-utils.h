/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <sys/types.h>
#include <time.h>
#include <libgnomevfs/gnome-vfs-result.h>
#include <libgnomevfs/gnome-vfs-file-size.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>


#define FILENAME_MAX_LENGTH 30 /* FIXME: find out the best value */

#define get_home_relative_dir(x)        \
        g_strconcat (g_get_home_dir (), \
                     "/",               \
                     (x),               \
                     NULL)

gboolean            path_exists                  (const char *path);
gboolean            uri_exists                   (const char *uri);
gboolean            uri_is_file                  (const char *uri);
gboolean            uri_is_dir                   (const char *uri);
gboolean            dir_is_empty                 (const char *s);
gboolean            path_in_path                 (const char  *path_src,
						  const char  *path_dest);
GnomeVFSFileSize    get_file_size                (const char *s);
time_t              get_file_mtime               (const char *s);
time_t              get_file_ctime               (const char *s);
gboolean            file_copy                    (const char *from, 
						  const char *to);
gboolean            file_move                    (const char *from, 
						  const char *to);
gint                file_in_path                 (const char *name);
GnomeVFSResult      ensure_dir_exists            (const char *a_path,
						  mode_t       mode);
gboolean            file_is_hidden               (const char *name);
G_CONST_RETURN char *file_name_from_path         (const char *path);
gchar *             remove_level_from_path       (const char *path);
gchar *             remove_extension_from_path   (const char *path);
gchar *             remove_ending_separator      (const char *path);
gboolean            file_extension_is            (const char *filename, 
						  const char *ext);
void                path_list_free               (GList *path_list);
GList *             path_list_dup                (GList *path_list);
gboolean            is_mime_type                 (const char* type, 
						  const char* pattern);
gboolean            strchrs                      (const char *str,
						  const char *chars);
char*               escape_str_common            (const char *str, 
						  const char *meta_chars,
						  const char  prefix,
						  const char  postfix);
char*               escape_str                   (const char  *str, 
						  const char  *meta_chars);
gchar *             shell_escape                 (const char *filename);
char *              escape_uri                   (const char *uri);
gchar *             application_get_command      (const GnomeVFSMimeApplication *app);
gboolean            match_patterns               (char       **patterns, 
						  const char  *string,
						  int          flags);
char **             search_util_get_patterns     (const char  *pattern_string);
GnomeVFSFileSize    get_dest_free_space          (const char  *path);
gboolean            rmdir_recursive              (const char *directory);
const char *        eat_spaces                   (const char *line);
char **             split_line                   (const char *line, 
						  int   n_fields);
const char *        get_last_field               (const char *line,
						  int         last_field);
char *              get_temp_work_dir            (void);
char *              get_temp_work_dir_name       (void);

char *              escape_uri                   (const char *uri);
GnomeVFSURI *       new_uri_from_path            (const char *path);
char *              get_uri_from_local_path      (const char *local_path);
const char *        get_path_from_uri            (const char *uri);
char *              tracktitle_to_filename       (const char *trackname);
gboolean            is_program_in_path           (const char *program_name);

#endif /* FILE_UTILS_H */
