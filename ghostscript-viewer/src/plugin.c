/*
 * Sylpheed-Claws Ghostscript Viewer Plugin
 * Copyright (C) 2003-2004 Christoph Hohmann
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

#include "common/version.h"
#include "common/plugin.h"
#include "common/utils.h"
#include "common/hooks.h"
#include "common/log.h"
#include "common/intl.h"

gint plugin_init(gchar **error)
{
	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup(_("Your sylpheed version is newer than the version the plugin was built with"));
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(0, 9, 12, 45))) {
		*error = g_strdup(_("Your sylpheed version is too old"));
		return -1;
	}

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
