/*
 * mailmbox Plugin -- mbox support for Sylpheed
 * Copyright (C) 2003 Christoph Hohmann
 * Copyright (C) 2003-2005 Hoa v. Dinh, Alfons Hoogervorst
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

#include <glib.h>
#include <glib/gi18n.h>
#include "plugin.h"
#include "folder.h"
#include "mailmbox_folder.h"
#include "common/version.h"
#include "plugin_gtk.h"
#include "pluginconfig.h"
#include "plugin.h"
#include "main.h"

gint plugin_init(gchar **error)
{
	if (!check_plugin_version(MAKE_NUMERIC_VERSION(2, 6, 1, 75),
				VERSION_NUMERIC, "Mailmbox", error))
		return -1;

	folder_register_class(claws_mailmbox_get_class());
	plugin_gtk_init(error);
	return 0;
}

void plugin_done(void)
{
	plugin_gtk_done();
	if (!claws_is_exiting())
		folder_unregister_class(claws_mailmbox_get_class());
}

const gchar *plugin_name(void)
{
	return _("mailmbox folder (etPan!)");
}

const gchar *plugin_desc(void)
{
	return _("This is a plugin to handle mailboxes in mbox format.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
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
		{ {PLUGIN_FOLDERCLASS, N_("MBOX")},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}
