/*
 * mailmbox Plugin -- mbox support for Sylpheed
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

#include "intl.h"
#include "plugin.h"
#include "folder.h"
#include "mailmbox_folder.h"

gint plugin_init(gchar **error)
{
	folder_register_class(mailmbox_get_class());

	return 0;
}

void plugin_done(void)
{
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
	return "Common";
}
