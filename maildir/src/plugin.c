/*
 * Maildir Plugin -- Maildir++ support for Sylpheed
 * Copyright (C) 2003 Christoph Hohmann
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

#include "common/version.h"
#include "common/plugin.h"
#include "folder.h"

#include "maildir.h"

gint plugin_init(gchar **error)
{
	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your sylpheed version is newer than the version the plugin was built with");
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(0, 9, 4, 15))) {
		*error = g_strdup("Your sylpheed version is too old");
		return -1;
	}

	folder_register_class(maildir_get_class());

	return 0;
}

void plugin_done()
{
	folder_unregister_class(maildir_get_class());
}

const gchar *plugin_name()
{
	return "Maildir++";
}

const gchar *plugin_desc()
{
	return "";
}

const gchar *plugin_type()
{
	return "Common";
}
