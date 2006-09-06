/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2004 Hiroyuki Yamamoto & the Sylpheed-Claws team
 * This file (C) 2004 Colin Leroy <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __VCAL_FOLDER_H__
#define __VCAL_FOLDER_H__

#include <glib.h>
#include "folder.h"

extern gboolean manual_update;

FolderClass *vcal_folder_get_class();
void vcal_folder_gtk_init(void);
void vcal_folder_gtk_done(void);
GSList *vcal_folder_get_waiting_events(void);
GSList *vcal_folder_get_webcal_events(void);
void vcal_folder_export(void);

gboolean vcal_curl_put(gchar *url, FILE *fp, gint filesize);
gchar *vcal_curl_read(const char *url, gboolean verbose, 
	void (*callback)(const gchar *url, gchar *data, gboolean verbose, gchar
		*error));
#endif
