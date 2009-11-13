/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  gThumb
 *
 *  Copyright (C) 2001, 2002 The Free Software Foundation, Inc.
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

/* eel-gconf-extensions.c - Stuff to make GConf easier to use.

   Copyright (C) 2000, 2001 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

/* Modified by Paolo Bacchilega <paolo.bacch@tin.it> for gThumb. */

#include <config.h>
#include <string.h>
#include <errno.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf.h>
#include "gconf-utils.h"
#include "gtk-utils.h"
#include "goo-error.h"
#include "glib-utils.h"

#define HOME_DIR "~"


static GConfClient *global_gconf_client = NULL;


void
eel_global_client_free (void)
{
	if (global_gconf_client == NULL) {
		return;
	}
	
	g_object_unref (global_gconf_client);
	global_gconf_client = NULL;
}


GConfClient *
eel_gconf_client_get_global (void)
{
	/* Initialize gconf if needed */
	if (!gconf_is_initialized ()) {
		char   *argv[] = { "eel-preferences", NULL };
		GError *error = NULL;
		
		if (!gconf_init (1, argv, &error)) {
			if (eel_gconf_handle_error (&error)) {
				return NULL;
			}
		}
	}
	
	if (global_gconf_client == NULL) 
		global_gconf_client = gconf_client_get_default ();
	
	return global_gconf_client;
}


gboolean
eel_gconf_handle_error (GError **error)
{
	static gboolean shown_dialog = FALSE;
	
	g_return_val_if_fail (error != NULL, FALSE);

	if (*error != NULL) {
		g_warning ("GConf error:\n  %s", (*error)->message);
		if (! shown_dialog) {
			shown_dialog = TRUE;
			_gtk_error_dialog_run (NULL, 
					       "GConf error:\n  %s\n"
					       "All further errors "
					       "shown only on terminal",
					       (*error)->message);
		}
		g_error_free (*error);
		*error = NULL;

		return TRUE;
	}

	return FALSE;
}


static gboolean
check_type (const char      *key, 
	    GConfValue      *val, 
	    GConfValueType   t, 
	    GError         **err)
{
	if (val->type != t) {
		g_set_error (err,
			     GOO_ERROR,
			     errno,
			     "Type mismatch for key %s",
			     key);
		return FALSE;
	} else
		return TRUE;
}


void
eel_gconf_set_boolean (const char *key,
		       gboolean    boolean_value)
{
	GConfClient *client;
	GError      *error = NULL;
	
	g_return_if_fail (key != NULL);

	client = eel_gconf_client_get_global ();
	g_return_if_fail (client != NULL);
	
	gconf_client_set_bool (client, key, boolean_value, &error);
	eel_gconf_handle_error (&error);
}


gboolean
eel_gconf_get_boolean (const char *key,
		       gboolean    def)
{
	GError      *error = NULL;
	gboolean     result = def;
	GConfClient *client;
	GConfValue  *val;

	g_return_val_if_fail (key != NULL, def);
	
	client = eel_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, def);
	
	val = gconf_client_get (client, key, &error);

	if (val != NULL) {
		if (check_type (key, val, GCONF_VALUE_BOOL, &error))
			result = gconf_value_get_bool (val);
		else
			eel_gconf_handle_error (&error);
		gconf_value_free (val);

	} else if (error != NULL)
		eel_gconf_handle_error (&error);
	
	return result;
}


void
eel_gconf_set_integer (const char *key,
		       int         int_value)
{
	GConfClient *client;
	GError      *error = NULL;

	g_return_if_fail (key != NULL);

	client = eel_gconf_client_get_global ();
	g_return_if_fail (client != NULL);

	gconf_client_set_int (client, key, int_value, &error);
	eel_gconf_handle_error (&error);
}


int
eel_gconf_get_integer (const char *key,
		       int         def)
{
	GError      *error = NULL;
	int          result = def;
	GConfClient *client;
	GConfValue  *val;

	g_return_val_if_fail (key != NULL, def);
	
	client = eel_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, def);
	
	val = gconf_client_get (client, key, &error);

	if (val != NULL) {
		if (check_type (key, val, GCONF_VALUE_INT, &error))
			result = gconf_value_get_int (val);
		else
			eel_gconf_handle_error (&error);
		gconf_value_free (val);

	} else if (error != NULL)
		eel_gconf_handle_error (&error);
	
	return result;
}


int
eel_gconf_get_enum (const char *key,
		    GType       enum_type,
		    int         def_val)
{
	GEnumValue *def_enum_val;
	char       *value_nick;
	GEnumValue *value;
	
	def_enum_val = _g_enum_type_get_value (enum_type, def_val);
	value_nick = eel_gconf_get_string (key, def_enum_val->value_nick);
	value = _g_enum_type_get_value_by_nick (enum_type, value_nick);	
	g_free (value_nick);
	
	return (value != NULL) ? value->value : 0;	
}


void
eel_gconf_set_enum (const char *key,
		    GType       enum_type,
		    int         value)
{
	GEnumValue *enum_value;
	
	enum_value = _g_enum_type_get_value (enum_type, value);
	if (enum_value != NULL)
		eel_gconf_set_string (key, enum_value->value_nick);
}


void
eel_gconf_set_float (const char *key,
		     float       float_value)
{
	GConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = eel_gconf_client_get_global ();
	g_return_if_fail (client != NULL);

	gconf_client_set_float (client, key, float_value, &error);
	eel_gconf_handle_error (&error);
}


float
eel_gconf_get_float (const char *key,
		     float       def)
{
	GError      *error = NULL;
	float        result = def;
	GConfClient *client;
	GConfValue  *val;

	g_return_val_if_fail (key != NULL, def);
	
	client = eel_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, def);
	
	val = gconf_client_get (client, key, &error);

	if (val != NULL) {
		if (check_type (key, val, GCONF_VALUE_FLOAT, &error))
			result = gconf_value_get_float (val);
		else
			eel_gconf_handle_error (&error);
		gconf_value_free (val);

	} else if (error != NULL)
		eel_gconf_handle_error (&error);
	
	return result;
}


void
eel_gconf_set_string (const char *key,
		      const char *string_value)
{
	GConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = eel_gconf_client_get_global ();
	g_return_if_fail (client != NULL);
	
	gconf_client_set_string (client, key, string_value, &error);
	eel_gconf_handle_error (&error);
}


char *
eel_gconf_get_string (const char *key,
		      const char *def)
{
	GError      *error = NULL;
	char        *result;
	GConfClient *client;
	char        *val;

	if (def != NULL)
		result = g_strdup (def);
	else
		result = NULL;

	g_return_val_if_fail (key != NULL, result);	

	client = eel_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, result);
	
	val = gconf_client_get_string (client, key, &error);

	/* Return the default value if the key does not exist,
	   or if it is empty. */
	if (val != NULL && strcmp (val, "")) {
		g_return_val_if_fail (error == NULL, result);
		g_free (result);
		result = g_strdup (val);

	} else if (error != NULL)
		eel_gconf_handle_error (&error);

	return result;
}


void
eel_gconf_set_locale_string (const char *key,
			     const char *string_value)
{
	char *utf8;

	utf8 = g_locale_to_utf8 (string_value, -1, 0, 0, 0);

	if (utf8 != NULL) {
		eel_gconf_set_string (key, utf8);
		g_free (utf8);
	}
}


char *
eel_gconf_get_locale_string (const char *key,
			     const char *def)
{
	char *utf8;
	char *result;

	utf8 = eel_gconf_get_string (key, def);

	if (utf8 == NULL)
		return NULL;

	result = g_locale_from_utf8 (utf8, -1, 0, 0, 0);
	g_free (utf8);

	return result;
}


void
eel_gconf_set_string_list (const char *key,
			   const GSList *slist)
{
	GConfClient *client;
	GError *error;

	g_return_if_fail (key != NULL);

	client = eel_gconf_client_get_global ();
	g_return_if_fail (client != NULL);

	error = NULL;
	gconf_client_set_list (client, key, GCONF_VALUE_STRING,
			       /* Need cast cause of GConf api bug */
			       (GSList *) slist,
			       &error);
	eel_gconf_handle_error (&error);
}


GSList *
eel_gconf_get_string_list (const char *key)
{
	GSList *slist;
	GConfClient *client;
	GError *error;
	
	g_return_val_if_fail (key != NULL, NULL);
	
	client = eel_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);
	
	error = NULL;
	slist = gconf_client_get_list (client, key, GCONF_VALUE_STRING, &error);
	if (eel_gconf_handle_error (&error)) {
		slist = NULL;
	}

	return slist;
}


GSList *
eel_gconf_get_path_list (const char *key)
{
	GSList *str_slist, *slist, *scan;

	str_slist = eel_gconf_get_string_list (key);

	slist = NULL;
	for (scan = str_slist; scan; scan = scan->next) {
		char *str = scan->data;
		char *path = _g_replace (str, HOME_DIR, g_get_home_dir ());
		slist = g_slist_prepend (slist, path);
	}

	g_slist_foreach (str_slist, (GFunc) g_free, NULL);
	g_slist_free (str_slist);

	return g_slist_reverse (slist);
}


void
eel_gconf_set_path_list (const char    *key,
			 const GSList  *string_list_value)
{
	GSList       *path_slist;
	const GSList *scan;

	path_slist = NULL;
	for (scan = string_list_value; scan; scan = scan->next) {
		char *value = scan->data;
		char *path = _g_replace (value, g_get_home_dir (), HOME_DIR);
		path_slist = g_slist_prepend (path_slist, path);
	}
	path_slist = g_slist_reverse (path_slist);

	eel_gconf_set_string_list (key, path_slist);

	g_slist_foreach (path_slist, (GFunc) g_free, NULL);
	g_slist_free (path_slist);
}


GSList *
eel_gconf_get_locale_string_list (const char *key)
{
	GSList *utf8_slist, *slist, *scan;

	utf8_slist = eel_gconf_get_string_list (key);

	slist = NULL;
	for (scan = utf8_slist; scan; scan = scan->next) {
		char *utf8 = scan->data;
		char *locale = g_locale_from_utf8 (utf8, -1, 0, 0, 0);
		slist = g_slist_prepend (slist, locale);
	}

	g_slist_foreach (utf8_slist, (GFunc) g_free, NULL);
	g_slist_free (utf8_slist);

	return g_slist_reverse (slist);
}


void
eel_gconf_set_locale_string_list (const char   *key,
				  const GSList *string_list_value)
{
	GSList       *utf8_slist;
	const GSList *scan;

	utf8_slist = NULL;
	for (scan = string_list_value; scan; scan = scan->next) {
		char *locale = scan->data;
		char *utf8 = g_locale_to_utf8 (locale, -1, 0, 0, 0);
		utf8_slist = g_slist_prepend (utf8_slist, utf8);
	}

	utf8_slist = g_slist_reverse (utf8_slist);

	eel_gconf_set_string_list (key, utf8_slist);

	g_slist_foreach (utf8_slist, (GFunc) g_free, NULL);
	g_slist_free (utf8_slist);
}


char *
eel_gconf_get_path (const char *key,
		    const char *def_val)
{
	char *value;
	char *path;
	
	value = eel_gconf_get_string (key, def_val);
	path = _g_replace (value, HOME_DIR, g_get_home_dir ());
	g_free (value);

	return path;
}


void
eel_gconf_set_path (const char *key,
		    const char *path)
{
	char *value;

	value = _g_replace (path, g_get_home_dir (), HOME_DIR);
	eel_gconf_set_string (key, value);
	g_free (value);
}


char *
eel_gconf_get_uri (const char *key,
	           const char *def_val)
{
	GRegex *regex;
	char   *s;
	char   *home;
	char   *uri;

	regex = g_regex_new ("~", 0, 0, NULL);
	s = eel_gconf_get_string (key, def_val);
	home = g_uri_escape_string (g_get_home_dir (), G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);
	uri = g_regex_replace_literal (regex, s, -1, 0, home, 0, NULL);

	g_free (home);
	g_free (s);
	g_regex_unref (regex);

	return uri;
}


void
eel_gconf_set_uri (const char *key,
		   const char *value)
{
	char   *home;
	GRegex *regex;
	char   *s;

	home = g_uri_escape_string (g_get_home_dir (), G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);
	regex = g_regex_new (home, 0, 0, NULL);
	s = g_regex_replace_literal (regex, value, -1, 0, "~", 0, NULL);
	eel_gconf_set_string (key, s);

	g_free (s);
	g_regex_unref (regex);
	g_free (home);
}


gboolean
eel_gconf_is_default (const char *key)
{
	gboolean result;
	GConfValue *value;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, FALSE);
	
	value = gconf_client_get_without_default  (eel_gconf_client_get_global (), key, &error);

	if (eel_gconf_handle_error (&error)) {
		if (value != NULL) {
			gconf_value_free (value);
		}
		return FALSE;
	}

	result = (value == NULL);
	eel_gconf_value_free (value);
	return result;
}


gboolean
eel_gconf_monitor_add (const char *directory)
{
	GError *error = NULL;
	GConfClient *client;

	g_return_val_if_fail (directory != NULL, FALSE);

	client = gconf_client_get_default ();
	g_return_val_if_fail (client != NULL, FALSE);

	gconf_client_add_dir (client,
			      directory,
			      GCONF_CLIENT_PRELOAD_NONE,
			      &error);
	
	if (eel_gconf_handle_error (&error)) {
		return FALSE;
	}

	return TRUE;
}


gboolean
eel_gconf_monitor_remove (const char *directory)
{
	GError *error = NULL;
	GConfClient *client;

	if (directory == NULL) {
		return FALSE;
	}

	client = gconf_client_get_default ();
	g_return_val_if_fail (client != NULL, FALSE);
	
	gconf_client_remove_dir (client,
				 directory,
				 &error);
	
	if (eel_gconf_handle_error (&error)) {
		return FALSE;
	}
	
	return TRUE;
}


void
eel_gconf_preload_cache (const char             *directory,
			 GConfClientPreloadType  preload_type)
{
	GError *error = NULL;
	GConfClient *client;

	if (directory == NULL) {
		return;
	}

	client = gconf_client_get_default ();
	g_return_if_fail (client != NULL);
	
	gconf_client_preload (client,
			      directory,
			      preload_type,
			      &error);
	
	eel_gconf_handle_error (&error);
}


void
eel_gconf_suggest_sync (void)
{
	GConfClient *client;
	GError *error = NULL;

	client = eel_gconf_client_get_global ();
	g_return_if_fail (client != NULL);
	
	gconf_client_suggest_sync (client, &error);
	eel_gconf_handle_error (&error);
}


GConfValue*
eel_gconf_get_value (const char *key)
{
	GConfValue *value = NULL;
	GConfClient *client;
	GError *error = NULL;

	g_return_val_if_fail (key != NULL, NULL);

	client = eel_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);

	value = gconf_client_get (client, key, &error);
	
	if (eel_gconf_handle_error (&error)) {
		if (value != NULL) {
			gconf_value_free (value);
			value = NULL;
		}
	}

	return value;
}


GConfValue*
eel_gconf_get_default_value (const char *key)
{
	GConfValue *value = NULL;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, NULL);

	client = eel_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);

	value = gconf_client_get_default_from_schema (client, key, &error);
	
	if (eel_gconf_handle_error (&error)) {
		if (value != NULL) {
			gconf_value_free (value);
			value = NULL;
		}
	}

	return value;
}


static int
eel_strcmp (const char *string_a, const char *string_b)
{
        /* FIXME bugzilla.eazel.com 5450: Maybe we need to make this
         * treat 'NULL < ""', or have a flavor that does that. If we
         * didn't have code that already relies on 'NULL == ""', I
         * would change it right now.
         */
        return strcmp (string_a == NULL ? "" : string_a,
                       string_b == NULL ? "" : string_b);
}


static gboolean
eel_str_is_equal (const char *string_a, const char *string_b)
{
        /* FIXME bugzilla.eazel.com 5450: Maybe we need to make this
         * treat 'NULL != ""', or have a flavor that does that. If we
         * didn't have code that already relies on 'NULL == ""', I
         * would change it right now.
         */
        return eel_strcmp (string_a, string_b) == 0;
}
 

static gboolean
simple_value_is_equal (const GConfValue *a,
		       const GConfValue *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	switch (a->type) {
	case GCONF_VALUE_STRING:
		return eel_str_is_equal (gconf_value_get_string (a),
					 gconf_value_get_string (b));
		break;

	case GCONF_VALUE_INT:
		return gconf_value_get_int (a) ==
			gconf_value_get_int (b);
		break;

	case GCONF_VALUE_FLOAT:
		return gconf_value_get_float (a) ==
			gconf_value_get_float (b);
		break;

	case GCONF_VALUE_BOOL:
		return gconf_value_get_bool (a) ==
			gconf_value_get_bool (b);
		break;
	default:
		g_assert_not_reached ();
		break;
	}
	
	return FALSE;
}


gboolean
eel_gconf_value_is_equal (const GConfValue *a,
			  const GConfValue *b)
{
	GSList *node_a;
	GSList *node_b;

	if (a == NULL && b == NULL) {
		return TRUE;
	}

	if (a == NULL || b == NULL) {
		return FALSE;
	}

	if (a->type != b->type) {
		return FALSE;
	}

	switch (a->type) {
	case GCONF_VALUE_STRING:
	case GCONF_VALUE_INT:
	case GCONF_VALUE_FLOAT:
	case GCONF_VALUE_BOOL:
		return simple_value_is_equal (a, b);
		break;
		
	case GCONF_VALUE_LIST:
		if (gconf_value_get_list_type (a) !=
		    gconf_value_get_list_type (b)) {
			return FALSE;
		}

		node_a = gconf_value_get_list (a);
		node_b = gconf_value_get_list (b);
		
		if (node_a == NULL && node_b == NULL) {
			return TRUE;
		}

		if (g_slist_length (node_a) !=
		    g_slist_length (node_b)) {
			return FALSE;
		}
		
		for (;
		     node_a != NULL && node_b != NULL;
		     node_a = node_a->next, node_b = node_b->next) {
			g_assert (node_a->data != NULL);
			g_assert (node_b->data != NULL);
			if (!simple_value_is_equal (node_a->data, node_b->data)) {
				return FALSE;
			}
		}
		
		return TRUE;
	default:
		/* FIXME: pair ? */
		g_assert (0);
		break;
	}
	
	g_assert_not_reached ();
	return FALSE;
}


void
eel_gconf_value_free (GConfValue *value)
{
	if (value == NULL) {
		return;
	}
	
	gconf_value_free (value);
}


guint
eel_gconf_notification_add (const char *key,
			    GConfClientNotifyFunc notification_callback,
			    gpointer callback_data)
{
	guint notification_id;
	GConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, EEL_GCONF_UNDEFINED_CONNECTION);
	g_return_val_if_fail (notification_callback != NULL, EEL_GCONF_UNDEFINED_CONNECTION);

	client = eel_gconf_client_get_global ();
	g_return_val_if_fail (client != NULL, EEL_GCONF_UNDEFINED_CONNECTION);
	
	notification_id = gconf_client_notify_add (client,
						   key,
						   notification_callback,
						   callback_data,
						   NULL,
						   &error);
	
	if (eel_gconf_handle_error (&error)) {
		if (notification_id != EEL_GCONF_UNDEFINED_CONNECTION) {
			gconf_client_notify_remove (client, notification_id);
			notification_id = EEL_GCONF_UNDEFINED_CONNECTION;
		}
	}
	
	return notification_id;
}


void
eel_gconf_notification_remove (guint notification_id)
{
	GConfClient *client;

	if (notification_id == EEL_GCONF_UNDEFINED_CONNECTION) {
		return;
	}
	
	client = eel_gconf_client_get_global ();
	g_return_if_fail (client != NULL);

	gconf_client_notify_remove (client, notification_id);
}


GSList *
eel_gconf_value_get_string_list (const GConfValue *value)
{
 	GSList *result;
 	const GSList *slist;
 	const GSList *node;
	const char *string;
	const GConfValue *next_value;

	if (value == NULL) {
		return NULL;
	}

	g_return_val_if_fail (value->type == GCONF_VALUE_LIST, NULL);
	g_return_val_if_fail (gconf_value_get_list_type (value) == GCONF_VALUE_STRING, NULL);

	slist = gconf_value_get_list (value);
	result = NULL;
	for (node = slist; node != NULL; node = node->next) {
		next_value = node->data;
		g_return_val_if_fail (next_value != NULL, NULL);
		g_return_val_if_fail (next_value->type == GCONF_VALUE_STRING, NULL);
		string = gconf_value_get_string (next_value);
		result = g_slist_append (result, g_strdup (string));
	}
	return result;
}


void
eel_gconf_value_set_string_list (GConfValue *value,
				 const GSList *string_list)
{
 	const GSList *node;
	GConfValue *next_value;
 	GSList *value_list;

	g_return_if_fail (value->type == GCONF_VALUE_LIST);
	g_return_if_fail (gconf_value_get_list_type (value) == GCONF_VALUE_STRING);

	value_list = NULL;
	for (node = string_list; node != NULL; node = node->next) {
		next_value = gconf_value_new (GCONF_VALUE_STRING);
		gconf_value_set_string (next_value, node->data);
		value_list = g_slist_append (value_list, next_value);
	}

	gconf_value_set_list (value, value_list);

	for (node = value_list; node != NULL; node = node->next) {
		gconf_value_free (node->data);
	}
	g_slist_free (value_list);
}
