/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2002 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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
#  include "pluginconfig.h"
#endif
#include "config.h"

#include "common/plugin.h"
#include "common/utils.h"
#include "common/hooks.h"
#include "common/log.h"
#include "common/intl.h"

gint plugin_init(gchar **error)
{
	ghostscript_viewer_prefs_init();
	ghostscript_viewer_init();
	return 0;
}

void plugin_done()
{
	ghostscript_viewer_done();
	ghostscript_viewer_prefs_done();
}

const gchar *plugin_name()
{
	return _("Ghostscript Viewer");
}

const gchar *plugin_desc()
{
	return _("This plugin provides a Viewer for Postscript and PDF documents for Sylpheed's MimeView");
}

const gchar *plugin_type()
{
	return "GTK";
}
