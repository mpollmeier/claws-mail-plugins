/*
 * CacheSaver plugin for claws-mail
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

#include "folder.h"

#include "pluginconfig.h"
#include "plugin.h"

#include <glib/gi18n.h>

static guint tag = 0;

static void save_all_caches(FolderItem *item, gpointer data)
{
	gchar *id = NULL;
	if (!item->cache)
		return;
	id = folder_item_get_identifier(item);
	g_free(id);
	folder_item_write_cache(item);
}

gint save_caches(void *data)
{
	debug_print("CacheSaver: saving caches...\n");
	folder_write_list();
	folder_func_to_all_folders(save_all_caches, NULL);
	debug_print("CacheSaver: caches saved\n");
	return TRUE;
}

gint plugin_init(gchar **error)
{
	if( !check_plugin_version(MAKE_NUMERIC_VERSION(2, 6, 1, 41),
				VERSION_NUMERIC, "Cachesaver", error) )
		return -1;

	if (tag == 0)
		tag = gtk_timeout_add(60*1000, save_caches, NULL);
	return 0;
}

void plugin_done()
{
	if (tag != 0) {
		gtk_timeout_remove(tag);
		tag = 0;
	}
}

const gchar *plugin_name()
{
	return "CacheSaver";
}

const gchar *plugin_desc()
{
	return "This plugin saves the caches every minute.\n"
		"It helps avoiding the loss of metadata on crashes.";
}

const gchar *plugin_type()
{
	return "Common";
}

const gchar *plugin_licence(void)
{
		return "GPL";
}

const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = 
		{ {PLUGIN_UTILITY, N_("Cache saving")},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}
