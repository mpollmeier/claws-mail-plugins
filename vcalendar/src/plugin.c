/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2004 Hiroyuki Yamamoto
 * This file (C) 2004 Colin Leroy <colino.net>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stddef.h>

#include "intl.h"

#include "version.h"
#include "vcalendar.h"
#include "sylpheed.h"

gint plugin_init(gchar **error)
{
	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your sylpheed version is newer than the version the plugin was built with");
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(0, 9, 13, 0))) {
		*error = g_strdup("Your sylpheed version is too old");
		return -1;
	}

	vcalendar_init();
	
	return 0;	
}

void plugin_done(void)
{
	vcalendar_done();
}

const gchar *plugin_name(void)
{
	return _("vCalendar");
}

const gchar *plugin_desc(void)
{
	return _("This plugin enables vCalendar message handling like produced "
		 "by Evolution or Outlook.\n\n"
		 "When loaded, it will create a vCalendar mailbox in your folder "
		 "list, which will be populated by meetings that you have accepted "
		 "or created.\n"
		 "Meeting requests that you receive will be presented in an "
		 "appropriate form and you will be able to accept or decline them.\n"
		 "Finally, to create a meeting right-click on the vCalendar or "
		 "Meetings folder and choose \"New meeting...\".");
}

const gchar *plugin_type(void)
{
	return "GTK";
}
