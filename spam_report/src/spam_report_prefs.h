/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 Colin Leroy <colin@colino.net>
 * and the Claws Mail Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef SPAMREPORTPREFS_H
#define SPAMREPORTPREFS_H

#include <glib.h>

typedef struct _SpamReportPrefs	SpamReportPrefs;

typedef enum {
	INTF_SIGNALSPAM,
	INTF_LAST
} InterfaceId;

typedef enum {
	INTF_HTTP,
	INTF_HTTP_AUTH,
	INTF_NULL
} InterfaceType;

typedef struct _ReportInterface {
	gchar *name;
	InterfaceType type;
	gchar *url;
	gchar *body;
} ReportInterface;

extern ReportInterface spam_interfaces[];

struct _SpamReportPrefs
{
	gchar *user[INTF_LAST];
	gchar *pass[INTF_LAST];
};

extern SpamReportPrefs spamreport_prefs;

void spamreport_prefs_init(void);
void spamreport_prefs_done(void);

#endif