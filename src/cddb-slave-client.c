/*
 * cddb-slave-client.c: Client side wrapper for accessing CDDBSlave really
 *                      easily.
 *
 * Copyright (C) 2001-2002 Iain Holmes
 *
 *
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bonobo-activation/bonobo-activation.h>

#include "GNOME_Media_CDDBSlave2.h"
#include "cddb-slave-client.h"

#include <bonobo/bonobo-listener.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-exception.h>

#define CDDB_SLAVE_IID "OAFIID:GNOME_Media_CDDBSlave2"

#define PARENT_TYPE G_TYPE_OBJECT
static GObjectClass *parent_class = NULL;

struct _CDDBSlaveClientPrivate {
	GNOME_Media_CDDBSlave2 objref;
};

static void
finalize (GObject *object)
{
	CDDBSlaveClient *client;

	client = CDDB_SLAVE_CLIENT (object);
	if (client->priv == NULL)
		return;

	if (client->priv->objref != CORBA_OBJECT_NIL) {
		bonobo_object_release_unref (client->priv->objref, NULL);
		client->priv->objref = CORBA_OBJECT_NIL;
	}

	g_free (client->priv);
	client->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (CDDBSlaveClientClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
init (CDDBSlaveClient *client)
{
	client->priv = g_new (CDDBSlaveClientPrivate, 1);
}

/* Standard functions */
GType
cddb_slave_client_get_type (void)
{
	static GType client_type = 0;

	if (!client_type) {
		GTypeInfo client_info = {
			sizeof (CDDBSlaveClientClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (CDDBSlaveClient), 0,
			(GInstanceInitFunc) init,
		};

		client_type = g_type_register_static (PARENT_TYPE, "CDDBSlaveClient", &client_info, 0);
	}

	return client_type;
}

/*** Public API ***/
/**
 * cddb_slave_client_construct:
 * @client: The CDDBSlaveClient to construct.
 * @corba_object: The CORBA_Object to construct it from.
 *
 * Constructs @client from @corba_object.
 */
void
cddb_slave_client_construct (CDDBSlaveClient *client,
			     CORBA_Object corba_object)
{
	g_return_if_fail (client != NULL);
	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));
	g_return_if_fail (corba_object != CORBA_OBJECT_NIL);

	client->priv->objref = corba_object;
}

/**
 * cddb_slave_client_new_from_id:
 * @id: The oafiid of the component to make a client for.
 *
 * Makes a client for the object returned by activating @id.
 *
 * Returns: A newly created CDDBSlaveClient or NULL on error.
 */
CDDBSlaveClient *
cddb_slave_client_new_from_id (const char *id)
{
	CDDBSlaveClient *client;
	CORBA_Environment ev;
	CORBA_Object objref;

	g_return_val_if_fail (id != NULL, NULL);

	CORBA_exception_init (&ev);
	objref = bonobo_activation_activate_from_id ((char *) id, 0, NULL, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Could no activate %s.\n%s", id,
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);
	if (objref == CORBA_OBJECT_NIL) {
		g_warning ("Could not start component %s.", id);
		return NULL;
	}

	client = g_object_new (cddb_slave_client_get_type (), NULL);
	cddb_slave_client_construct (client, objref);

	return client;
}

/**
 * cddb_slave_client_new:
 *
 * Creates a new CDDBSlaveClient, using the default CDDBSlave component.
 *
 * Returns: A newly created CDDBSlaveClient or NULL on error.
 */
CDDBSlaveClient *
cddb_slave_client_new (void)
{
	return cddb_slave_client_new_from_id (CDDB_SLAVE_IID);
}

/**
 * cddb_slave_client_query:
 * @client: The CDDBSlaveClient to perform the query.
 * @discid: The ID of the CD to be searched for.
 * @ntrks: The number of tracks on the CD.
 * @offsets: A string of all the frame offsets to the starting location of
 *           each track, seperated by spaces.
 * @nsecs: Total playing length of the CD in seconds.
 * @name: The name of the the program performing the query. eg. GTCD.
 * @version: The version of the program performing the query.
 *
 * Asks the CDDBSlave that @client is a client for to perform a query on the
 * CDDB server that it is set up to connect to.
 * The @name string will be sent as "@name (CDDBSlave 2)",
 * eg. "GTCD (CDDBSlave 2)"
 *
 * Returns: A boolean indicating if there was an error in sending the query to
 *          the CDDBSlave
 */
gboolean
cddb_slave_client_query (CDDBSlaveClient *client,
			 const char *discid,
			 int ntrks,
			 const char *offsets,
			 int nsecs,
			 const char *name,
			 const char *version)
{
	GNOME_Media_CDDBSlave2 cddb;
	CORBA_Environment ev;
	gboolean result;

	g_return_val_if_fail (client != NULL, FALSE);
	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), FALSE);
	g_return_val_if_fail (discid != NULL, FALSE);
	g_return_val_if_fail (ntrks > 0, FALSE);
	g_return_val_if_fail (offsets != NULL, FALSE);
	g_return_val_if_fail (nsecs > 0, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);
	g_return_val_if_fail (version != NULL, FALSE);

	CORBA_exception_init (&ev);
	cddb = client->priv->objref;
	GNOME_Media_CDDBSlave2_query (cddb, discid, ntrks, offsets, nsecs,
				      name, version, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("Error sending request: %s", CORBA_exception_id (&ev));
		result = FALSE;
	} else {
		result = TRUE;
	}

	CORBA_exception_free (&ev);
	return result;
}

void
cddb_slave_client_save (CDDBSlaveClient *client,
			const char *discid)
{
	GNOME_Media_CDDBSlave2 cddb;
	CORBA_Environment ev;

	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));
	g_return_if_fail (discid != NULL);

	CORBA_exception_init (&ev);
	cddb = client->priv->objref;
	GNOME_Media_CDDBSlave2_save (cddb, discid, &ev);

	if (BONOBO_EX (&ev)) {
		g_warning ("Could not save %s\n%s", discid, CORBA_exception_id (&ev));
	}

	CORBA_exception_free (&ev);
}

/**
 * cddb_slave_client_add_listener:
 * @client: Client of the CDDBSlave to add a listener to.
 * @listener: BonoboListener to add.
 *
 * Adds a listener to the CDDBSlave that belongs to @client.
 */
void
cddb_slave_client_add_listener (CDDBSlaveClient *client,
				BonoboListener *listener)
{
	CORBA_Object client_objref, listener_objref;
	CORBA_Object event_source;
	CORBA_Environment ev;

	g_return_if_fail (client != NULL);
	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));
	g_return_if_fail (listener != NULL);
	g_return_if_fail (BONOBO_IS_LISTENER (listener));

	client_objref = client->priv->objref;
	listener_objref = bonobo_object_corba_objref (BONOBO_OBJECT (listener));

	CORBA_exception_init (&ev);
	event_source = Bonobo_Unknown_queryInterface (client_objref,
						      "IDL:Bonobo/EventSource:1.0", &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error doing QI for event source\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return;
	}

	/* Add the listener */
	Bonobo_EventSource_addListener (event_source, listener_objref, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error adding listener\n%s", CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return;
	}

	bonobo_object_release_unref (event_source, NULL);
	CORBA_exception_free (&ev);
	return;
}

/**
 * cddb_slave_client_remove_listener:
 * @client: Client of the CDDBSlave to remove a listener from.
 * @listener: The listener to remove.
 *
 * Removes a listener from the CDDBSlave that belongs to @client.
 */
void
cddb_slave_client_remove_listener (CDDBSlaveClient *client,
				   BonoboListener *listener)
{
	CORBA_Object client_objref;
	CORBA_Object event_source;
	CORBA_Object listener_objref;
	CORBA_Environment ev;

	g_return_if_fail (client != NULL);
	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));
	g_return_if_fail (BONOBO_IS_LISTENER (listener));

	client_objref = client->priv->objref;
	listener_objref = bonobo_object_corba_objref (BONOBO_OBJECT (listener));

	CORBA_exception_init (&ev);
	event_source = Bonobo_Unknown_queryInterface (client_objref,
						      "IDL:Bonobo/EventSource:1.0", &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error doing QI for event source\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return;
	}

	/* Remove the listener */
	Bonobo_EventSource_removeListener (event_source, listener_objref, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error removing listener\n%s", CORBA_exception_id (&ev));
	}

	bonobo_object_release_unref (event_source, NULL);
	CORBA_exception_free (&ev);

	return;
}

/**
 * cddb_slave_client_is_valid:
 *
 * Checks if an entry is marked as valid in the CDDB slave cache.
 *
 * Returns: %TRUE if the given discid is a valid entry in the cache.
 */
gboolean
cddb_slave_client_is_valid (CDDBSlaveClient *client,
                            const char *discid)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	CORBA_boolean ret;

	g_return_val_if_fail (client != NULL, FALSE);
	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), FALSE);
	g_return_val_if_fail (discid != NULL, FALSE);

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	ret = GNOME_Media_CDDBSlave2_isValid (objref, discid, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error checking if the discid is a valid entry\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return FALSE;
	}

	CORBA_exception_free (&ev);
	return ret;
}

/**
 * cddb_slave_client_get_disc_title:
 *
 */
char *
cddb_slave_client_get_disc_title (CDDBSlaveClient *client,
				  const char *discid)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	CORBA_char *ret;

	g_return_val_if_fail (client != NULL, NULL);
	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), NULL);
	g_return_val_if_fail (discid != NULL, NULL);

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	ret = GNOME_Media_CDDBSlave2_getDiscTitle (objref, discid, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error getting disc title\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);
	return g_strdup (ret);
}

/**
 * cddb_slave_client_set_disc_title:
 *
 */
void
cddb_slave_client_set_disc_title (CDDBSlaveClient *client,
				  const char *discid,
				  const char *title)
{
	CORBA_Object objref;
	CORBA_Environment ev;

	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	GNOME_Media_CDDBSlave2_setDiscTitle (objref, discid,
					     title ? title : "", &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error setting disc title\n%s",
			   CORBA_exception_id (&ev));
	}

	CORBA_exception_free (&ev);
}

char *
cddb_slave_client_get_artist (CDDBSlaveClient *client,
			      const char *discid)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	CORBA_char *ret;

	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), NULL);
	g_return_val_if_fail (discid != NULL, NULL);

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	ret = GNOME_Media_CDDBSlave2_getArtist (objref, discid, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error getting artist\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);
	return g_strdup (ret);
}

void
cddb_slave_client_set_artist (CDDBSlaveClient *client,
			      const char *discid,
			      const char *artist)
{
	CORBA_Object objref;
	CORBA_Environment ev;

	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	GNOME_Media_CDDBSlave2_setArtist (objref, discid,
					  artist ? artist : "", &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error setting artist\n%s",
			   CORBA_exception_id (&ev));
	}

	CORBA_exception_free (&ev);
}

int
cddb_slave_client_get_ntrks (CDDBSlaveClient *client,
			     const char *discid)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	CORBA_short ret;

	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), -1);
	g_return_val_if_fail (discid != NULL, -1);

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	ret = GNOME_Media_CDDBSlave2_getNTrks (objref, discid, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error getting ntrks\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return -1;
	}

	CORBA_exception_free (&ev);
	return ret;
}

CDDBSlaveClientTrackInfo **
cddb_slave_client_get_tracks (CDDBSlaveClient *client,
			      const char *discid)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	GNOME_Media_CDDBSlave2_TrackList *list;
	CDDBSlaveClientTrackInfo **ret;
	int i;

	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), NULL);
	g_return_val_if_fail (discid != NULL, NULL);

	objref = client->priv->objref;

	CORBA_exception_init (&ev);

	GNOME_Media_CDDBSlave2_getAllTracks (objref, discid, &list, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error getting tracks\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return NULL;
	}
	CORBA_exception_free (&ev);

	ret = g_new (CDDBSlaveClientTrackInfo *, list->_length + 1);
	for (i = 0; i < list->_length; i++) {
		ret[i] = g_new (CDDBSlaveClientTrackInfo, 1);
		ret[i]->name = g_strdup (list->_buffer[i].name);
		ret[i]->length = list->_buffer[i].length;
		ret[i]->comment = g_strdup (list->_buffer[i].comment);
	}

	/* NULL terminator */
	ret[i] = NULL;

	CORBA_free (list);
	return ret;
}

void
cddb_slave_client_free_track_info (CDDBSlaveClientTrackInfo **track_info)
{
	int i;
	for (i = 0; track_info[i] != NULL; i++) {
		g_free (track_info[i]->name);
		g_free (track_info[i]->comment);
		g_free (track_info[i]);
	}

	g_free (track_info);
}

void
cddb_slave_client_set_tracks (CDDBSlaveClient *client,
			      const char *discid,
			      CDDBSlaveClientTrackInfo **track_info)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	GNOME_Media_CDDBSlave2_TrackList *list;
	int i;

	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));

	for (i = 0; track_info[i] != NULL; i++) {
		; /* Count the number of tracks */
	}

	list = GNOME_Media_CDDBSlave2_TrackList__alloc ();
	list->_length = i;
	list->_maximum = i;
	list->_buffer = CORBA_sequence_GNOME_Media_CDDBSlave2_TrackInfo_allocbuf (i);

	for (i = 0; track_info[i] != NULL; i++) {
		list->_buffer[i].name = CORBA_string_dup (track_info[i]->name ? track_info[i]->name : "");
		list->_buffer[i].length = 0; /* We can't change the length of a track :) */
		list->_buffer[i].comment = CORBA_string_dup (track_info[i]->comment ?
							     track_info[i]->comment : "");
	}

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	GNOME_Media_CDDBSlave2_setAllTracks (objref, discid, list, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error setting all tracks\n%s", CORBA_exception_id (&ev));
	}

	CORBA_exception_free (&ev);
	CORBA_free (list);
}

char *
cddb_slave_client_get_comment (CDDBSlaveClient *client,
			       const char *discid)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	CORBA_char *ret;

	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), NULL);
	g_return_val_if_fail (discid != NULL, NULL);

	objref = client->priv->objref;

	CORBA_exception_init (&ev);

	ret = GNOME_Media_CDDBSlave2_getComment (objref, discid, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error getting comment\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);
	return g_strdup (ret);
}

void
cddb_slave_client_set_comment (CDDBSlaveClient *client,
			       const char *discid,
			       const char *comment)
{
	CORBA_Object objref;
	CORBA_Environment ev;

	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));

	objref = client->priv->objref;

	CORBA_exception_init (&ev);

	GNOME_Media_CDDBSlave2_setComment (objref, discid,
					   comment ? comment : "", &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error setting comment\n%s", CORBA_exception_id (&ev));
	}
	CORBA_exception_free (&ev);
}

int
cddb_slave_client_get_year (CDDBSlaveClient *client,
			    const char *discid)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	CORBA_short ret;

	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), -1);
	g_return_val_if_fail (discid != NULL, -1);

	objref = client->priv->objref;

	CORBA_exception_init (&ev);

	ret = GNOME_Media_CDDBSlave2_getYear (objref, discid, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error getting year\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return -1;
	}

	CORBA_exception_free (&ev);
	return ret;
}

void
cddb_slave_client_set_year (CDDBSlaveClient *client,
			    const char *discid,
			    int year)
{
	CORBA_Object objref;
	CORBA_Environment ev;

	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	GNOME_Media_CDDBSlave2_setYear (objref, discid, year, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error setting year\n%s", CORBA_exception_id (&ev));
	}
	CORBA_exception_free (&ev);
}

char *
cddb_slave_client_get_genre (CDDBSlaveClient *client,
			     const char *discid)
{
	CORBA_Object objref;
	CORBA_Environment ev;
	CORBA_char *ret;

	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), NULL);
	g_return_val_if_fail (discid != NULL, NULL);

	objref = client->priv->objref;

	CORBA_exception_init (&ev);

	ret = GNOME_Media_CDDBSlave2_getGenre (objref, discid, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error getting genre\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return NULL;
	}

	CORBA_exception_free (&ev);
	return g_strdup (ret);
}

void
cddb_slave_client_set_genre (CDDBSlaveClient *client,
			     const char *discid,
			     const char *genre)
{
	CORBA_Object objref;
	CORBA_Environment ev;

	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));

	objref = client->priv->objref;

	CORBA_exception_init (&ev);
	GNOME_Media_CDDBSlave2_setGenre (objref, discid,
					 genre ? genre : "", &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error setting genre\n%s",
			   CORBA_exception_id (&ev));
	}
	CORBA_exception_free (&ev);
}
