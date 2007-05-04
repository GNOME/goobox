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

#include <string.h>
#include <libgnome/libgnome.h>
#include <gconf/gconf-client.h>
#include "typedefs.h"
#include "preferences.h"
#include "main.h"
#include "file-utils.h"
#include "gconf-utils.h"
#include "goo-window.h"


#define DIALOG_PREFIX "/apps/goobox/dialogs/"
#define SOUND_JUICER_EXE "sound-juicer"


typedef struct {
	int   i_value;
	char *s_value;
} EnumStringTable;


static int
get_enum_from_string (EnumStringTable *table,
		      const char      *s_value)
{
	int i;

	/* return the first value if s_value is invalid */
	
	if (s_value == NULL)
		return table[0].i_value; 

	for (i = 0; table[i].s_value != NULL; i++)
		if (strcmp (s_value, table[i].s_value) == 0)
			return table[i].i_value;
	
	return table[0].i_value;
}


static char *
get_string_from_enum (EnumStringTable *table,
		      int              i_value)
{
	int i;

	for (i = 0; table[i].s_value != NULL; i++)
		if (i_value == table[i].i_value)
			return table[i].s_value;
	
	/* return the first value if i_value is invalid */

	return table[0].s_value;
}


/* --------------- */


static void
set_dialog_property_int (const char *dialog, 
			 const char *property, 
			 int         value)
{
	char *key;

	key = g_strconcat (DIALOG_PREFIX, dialog, "/", property, NULL);
	eel_gconf_set_integer (key, value);
	g_free (key);
}


void
pref_util_save_window_geometry (GtkWindow  *window,
				const char *dialog)
{
	int x, y, width, height;

	gtk_window_get_position (window, &x, &y);
	set_dialog_property_int (dialog, "x", x); 
	set_dialog_property_int (dialog, "y", y); 

	gtk_window_get_size (window, &width, &height);
	set_dialog_property_int (dialog, "width", width); 
	set_dialog_property_int (dialog, "height", height); 
}


static int
get_dialog_property_int (const char *dialog, 
			 const char *property)
{
	char *key;
	int   value;

	key = g_strconcat (DIALOG_PREFIX, dialog, "/", property, NULL);
	value = eel_gconf_get_integer (key, -1);
	g_free (key);

	return value;
}


void
pref_util_restore_window_geometry (GtkWindow  *window,
				   const char *dialog)
{
	int x, y, width, height;

	x = get_dialog_property_int (dialog, "x");
	y = get_dialog_property_int (dialog, "y");
	width = get_dialog_property_int (dialog, "width");
	height = get_dialog_property_int (dialog, "height");

	if (width != -1 && height != 1) 
		gtk_window_set_default_size (window, width, height);

	gtk_window_present (window);
}




/* --------------- */

static EnumStringTable file_format_table [] = {
	{ GOO_FILE_FORMAT_OGG,  "ogg" },
	{ GOO_FILE_FORMAT_FLAC, "flac" },
	{ GOO_FILE_FORMAT_WAVE, "wave" },
	{ 0, NULL }
};


static EnumStringTable sort_method_table [] = {
	{ WINDOW_SORT_BY_NUMBER, "number" },
	{ WINDOW_SORT_BY_TIME, "time" },
        { WINDOW_SORT_BY_TITLE, "title" },
	{ 0, NULL }
};


static EnumStringTable sort_type_table [] = {
	{ GTK_SORT_ASCENDING, "ascending" },
	{ GTK_SORT_DESCENDING, "descending" },
	{ 0, NULL }
};


#define GET_SET_FUNC(func_name, pref_name, type)			\
type									\
pref_get_##func_name (void)						\
{									\
	char *s_value;							\
	char  i_value;							\
									\
	s_value = eel_gconf_get_string (pref_name, 			\
                                        func_name##_table[0].s_value);	\
	i_value = get_enum_from_string (func_name##_table, s_value);	\
	g_free (s_value);						\
									\
	return (type) i_value;						\
}									\
									\
									\
void									\
pref_set_##func_name (type i_value)					\
{									\
	char *s_value;							\
									\
	s_value = get_string_from_enum (func_name##_table, i_value);	\
	eel_gconf_set_string (pref_name, s_value);			\
}


GET_SET_FUNC(file_format, PREF_EXTRACT_FILETYPE, GooFileFormat)


/* -------------- */


WindowSortMethod
preferences_get_sort_method (void)
{
	char *s_value;
	int   i_value;

	s_value = eel_gconf_get_string (PREF_PLAYLIST_SORT_METHOD, "name");
	i_value = get_enum_from_string (sort_method_table, s_value);
	g_free (s_value);

	return (WindowSortMethod) i_value;
}


void
preferences_set_sort_method (WindowSortMethod i_value)
{
	char *s_value;

	s_value = get_string_from_enum (sort_method_table, i_value);
	eel_gconf_set_string (PREF_PLAYLIST_SORT_METHOD, s_value);
}


GtkSortType
preferences_get_sort_type (void)
{
	char *s_value;
	int   i_value;

	s_value = eel_gconf_get_string (PREF_PLAYLIST_SORT_TYPE, "ascending");
	i_value = get_enum_from_string (sort_type_table, s_value);
	g_free (s_value);

	return (GtkSortType) i_value;
}


void
preferences_set_sort_type (GtkSortType i_value)
{
	char *s_value;

	s_value = get_string_from_enum (sort_type_table, i_value);
	eel_gconf_set_string (PREF_PLAYLIST_SORT_TYPE, s_value);
}


gboolean
preferences_get_use_sound_juicer (void)
{
	return eel_gconf_get_boolean (PREF_GENERAL_USE_SJ, FALSE) && is_program_in_path (SOUND_JUICER_EXE);
}
