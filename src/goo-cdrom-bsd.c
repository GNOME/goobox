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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#ifdef HAVE_BSD

#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/cdio.h>
#include <errno.h>

#include <gnome.h>
#include "goo-cdrom.h"
#include "goo-cdrom-bsd.h"
#include "glib-utils.h"

static GooCdromClass *parent_class = NULL;

static void goo_cdrom_bsd_class_init  (GooCdromBsdClass *class);


GType
goo_cdrom_bsd_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (GooCdromBsdClass),
			NULL,
			NULL,
			(GClassInitFunc) goo_cdrom_bsd_class_init,
			NULL,
			NULL,
			sizeof (GooCdromBsd),
			0,
			(GInstanceInitFunc) NULL
		};

		type = g_type_register_static (GOO_TYPE_CDROM,
					       "GooCdromBsd",
					       &type_info,
					       0);
	}

        return type;
}


static int
open_device (GooCdrom *cdrom)
{
	const char *device;
	int         fd = -1;

	device = goo_cdrom_get_device (cdrom);
	if ((fd = open (device, O_RDONLY | O_NONBLOCK)) < 0) {
		goo_cdrom_set_error_from_errno (cdrom);
		goo_cdrom_set_state (cdrom, GOO_CDROM_STATE_ERROR);
	}

	return fd;
}


static gboolean
goo_cdrom_bsd_eject (GooCdrom *cdrom)
{
	int      fd;
	gboolean result = FALSE;

	debug (DEBUG_INFO, "EJECT\n");

	fd = open_device (cdrom);
	if (fd >= 0) {
		ioctl (fd, CDIOCALLOW, 0);
		if (ioctl (fd, CDIOCEJECT, 0) >= 0) 
			result = TRUE;
		else 
			goo_cdrom_set_error_from_errno (cdrom);
		close (fd);
	}

	if (result)
		goo_cdrom_set_state (cdrom, GOO_CDROM_STATE_TRAY_OPEN);

	return result;
}


static int msf = 1;
static struct cd_toc_entry	toc_buffer[100];

static int
read_toc_entrys (int fd,
		 int len)
{
	struct ioc_read_toc_entry t;

	t.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	t.starting_track = 0;
	t.data_len = len;
	t.data = toc_buffer;

	return (ioctl (fd, CDIOREADTOCENTRYS, (char *) &t));
}


static int
update_state_from_fd (GooCdrom *cdrom,
		      int       fd)
{
	int		      rc, i, n;
	struct ioc_toc_header h;

	rc = ioctl (fd, CDIOREADTOCHEADER, &h);
	if (rc < 0) {
		goo_cdrom_set_error_from_string (cdrom, _("Error reading CD"));
		return -1;
	}

	n = h.ending_track - h.starting_track + 1;
	rc = read_toc_entrys (fd, (n + 1) * sizeof (struct cd_toc_entry));
	if (rc < 0) {
		goo_cdrom_set_error_from_errno (cdrom);
		return -1;
	}
	
	for (i = 0; i < n; i++) {
		if (!(toc_buffer[i].control & 4))
			return GOO_CDROM_STATE_OK;
	}

	return GOO_CDROM_STATE_DATA_CD;
}


static gboolean
goo_cdrom_bsd_close_tray (GooCdrom *cdrom)
{
	int      fd;
	gboolean result = FALSE;

	debug (DEBUG_INFO, "CLOSE TRAY\n");

	fd = open_device (cdrom);
	if (fd >= 0) {
		GooCdromState new_state = -1;

		ioctl (fd, CDIOCALLOW, 0);
		if (ioctl (fd, CDIOCCLOSE, 0) >= 0) {
			new_state = update_state_from_fd (cdrom, fd);
			result = (new_state != -1);
		} else 
			goo_cdrom_set_error_from_errno (cdrom);
		close (fd);

		if (result)
			goo_cdrom_set_state (cdrom, new_state);
	}

	return result;
}


static gboolean
lock_tray (GooCdrom *cdrom,
	   gboolean  lock)
{
	int      fd;
	gboolean result = FALSE;

	if (lock)
		debug (DEBUG_INFO, "LOCK TRAY\n");
	else
		debug (DEBUG_INFO, "UNLOCK TRAY\n");

	fd = open_device (cdrom);
	if (fd >= 0) {
		unsigned long cmd = lock ? CDIOCPREVENT : CDIOCALLOW;
		GooCdromState new_state = -1;

		if (ioctl (fd, cmd) != -1) {
			new_state = update_state_from_fd (cdrom, fd);
			result = (new_state != -1);
		} else 
			goo_cdrom_set_error_from_errno (cdrom);
		close (fd);

		if (result)
			goo_cdrom_set_state (cdrom, new_state);
	}

	return result;
}


static gboolean
goo_cdrom_bsd_lock_tray (GooCdrom *cdrom)
{
	return lock_tray (cdrom, TRUE);
}


static gboolean
goo_cdrom_bsd_unlock_tray (GooCdrom *cdrom)
{
	return lock_tray (cdrom, FALSE);
}


static gboolean
goo_cdrom_bsd_is_cdrom_device (GooCdrom   *cdrom,
			       const char *device)
{
	return TRUE;
}


static gboolean
goo_cdrom_bsd_update_state (GooCdrom *cdrom)
{
	int      fd;
	gboolean result = FALSE;

	fd = open_device (cdrom);
	if (fd >= 0) {
		GooCdromState new_state = update_state_from_fd (cdrom, fd);
		result = (new_state != -1);
		close (fd);
		if (result)
			goo_cdrom_set_state (cdrom, new_state);
	} 

	return result;
}


static void 
goo_cdrom_bsd_class_init (GooCdromBsdClass *class)
{
        GooCdromClass *cdrom_class = GOO_CDROM_CLASS (class);

        parent_class = g_type_class_peek_parent (class);

	cdrom_class->eject            = goo_cdrom_bsd_eject;
	cdrom_class->close_tray       = goo_cdrom_bsd_close_tray;
	cdrom_class->lock_tray        = goo_cdrom_bsd_lock_tray;
	cdrom_class->unlock_tray      = goo_cdrom_bsd_unlock_tray;
	cdrom_class->update_state     = goo_cdrom_bsd_update_state;
	cdrom_class->is_cdrom_device  = goo_cdrom_bsd_is_cdrom_device;
}


GooCdrom *
goo_cdrom_bsd_new (const char *device)
{
	GooCdrom *cdrom;

	cdrom = GOO_CDROM (g_object_new (GOO_TYPE_CDROM_BSD, 
					 "default_device", DEFAULT_DEVICE,
					 NULL));
	goo_cdrom_construct (cdrom, device);

	return cdrom;
}

#endif /* HAVE_BSD */
