/*
 * cddb-slave-client.c: Client side wrapper for accessing CDDBSlave really
 *                      easily.
 *
 * Copyright (C) 2001-2002 Iain Holmes
 *
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifndef __CDDB_SLAVE_CLIENT_H__
#define __CDDB_SLAVE_CLIENT_H__

#include <glib-object.h>
#include <bonobo/bonobo-listener.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif

#define CDDB_SLAVE_CLIENT_TYPE (cddb_slave_client_get_type ())
#define CDDB_SLAVE_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDDB_SLAVE_CLIENT_TYPE, CDDBSlaveClient))
#define CDDB_SLAVE_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CDDB_SLAVE_CLIENT_TYPE, CDDBSlaveClientClass))
#define IS_CDDB_SLAVE_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDDB_SLAVE_CLIENT_TYPE))
#define IS_CDDB_SLAVE_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDDB_SLAVE_CLIENT_TYPE))

#define CDDB_SLAVE_CLIENT_CDDB_FINISHED "GNOME_Media_CDDBSlave2:CDDB-Finished"

typedef struct _CDDBSlaveClient CDDBSlaveClient;
typedef struct _CDDBSlaveClientPrivate CDDBSlaveClientPrivate;
typedef struct _CDDBSlaveClientClass CDDBSlaveClientClass;

typedef struct _CDDBSlaveClientTrackInfo {
	char *name;
	int length;
	char *comment;
} CDDBSlaveClientTrackInfo;

struct _CDDBSlaveClient {
	GObject parent;

	CDDBSlaveClientPrivate *priv;
};

struct _CDDBSlaveClientClass {
	GObjectClass parent_class;
};

GType cddb_slave_client_get_type (void);
void cddb_slave_client_construct (CDDBSlaveClient *client,
				  CORBA_Object corba_object);
CDDBSlaveClient *cddb_slave_client_new_from_id (const char *id);
CDDBSlaveClient *cddb_slave_client_new (void);
gboolean cddb_slave_client_query (CDDBSlaveClient *client,
				  const char *discid,
				  int ntrks,
				  const char *offsets,
				  int nsecs,
				  const char *name,
				  const char *version);
void cddb_slave_client_save (CDDBSlaveClient *client,
			     const char *discid);
void cddb_slave_client_add_listener (CDDBSlaveClient *client,
				     BonoboListener *listener);
void cddb_slave_client_remove_listener (CDDBSlaveClient *client,
					BonoboListener *listener);

gboolean cddb_slave_client_is_valid (CDDBSlaveClient *client,
                                     const char *discid);

char *cddb_slave_client_get_disc_title (CDDBSlaveClient *client,
					const char *discid);
char *cddb_slave_client_get_artist (CDDBSlaveClient *client,
				    const char *discid);
int cddb_slave_client_get_ntrks (CDDBSlaveClient *client,
				 const char *discid);
CDDBSlaveClientTrackInfo **cddb_slave_client_get_tracks (CDDBSlaveClient *client,
							 const char *discid);
char *cddb_slave_client_get_comment (CDDBSlaveClient *client,
				     const char *discid);
int cddb_slave_client_get_year (CDDBSlaveClient *client,
				const char *discid);
char *cddb_slave_client_get_genre (CDDBSlaveClient *client,
				   const char *discid);


void cddb_slave_client_set_disc_title (CDDBSlaveClient *client,
                                       const char *discid,
                                       const char *title);
void cddb_slave_client_set_artist (CDDBSlaveClient *client,
                                   const char *discid,
                                   const char *artist);
void cddb_slave_client_set_tracks (CDDBSlaveClient *client,
                                   const char *discid,
                                   CDDBSlaveClientTrackInfo **track_info);
void cddb_slave_client_set_comment (CDDBSlaveClient *client,
                                    const char *discid,
                                    const char *comment);
void cddb_slave_client_set_year (CDDBSlaveClient *client,
                                 const char *discid,
                                 int year);
void cddb_slave_client_set_genre (CDDBSlaveClient *client,
                                  const char *discid,
                                  const char *genre);

void cddb_slave_client_free_track_info (CDDBSlaveClientTrackInfo **track_info);

#ifdef __cplusplus
}
#endif

#endif
