/*
 * CacheSaver plugin for sylpheed-claws
 * Copyright (C) 2004 Colin Leroy
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

gint plugin_init(gchar **error)
{
	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your sylpheed version is newer than the version the plugin was built with");
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(0, 9, 12, 0))) {
		*error = g_strdup("Your sylpheed version is too old");
		return -1;
	}

	/* init stuff */

	/* ... */

	printf("Skeleton loaded\n");

	return 0;
}

void plugin_done()
{
	/* clean up stuff */

	/* ... */

	printf("Skeleton unloaded\n");
}

const gchar *plugin_name()
{
	return "Skeleton";
}

const gchar *plugin_desc()
{
	return "This plugin does this and that.";
}

const gchar *plugin_type()
{
	return "Common";
}
