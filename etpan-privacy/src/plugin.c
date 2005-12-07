/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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

#include "defs.h"
#include "version.h"
#include "sylpheed.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "plugin.h"
#include "utils.h"
#include "hooks.h"
#include "inc.h"
#include "prefs.h"
#include "prefs_gtk.h"

#include "pop.h"
#include "privacy.h"
#include "etpan_privacy.h"
#include "prefs_privacy.h"

gint plugin_init(gchar **error)
{
	int r;
	
	if ((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup(_("Your sylpheed version is newer than the version the plugin was built with"));
		return -1;
	}

	if ((sylpheed_get_version() < MAKE_NUMERIC_VERSION(0, 9, 7, 0))) {
		*error = g_strdup(_("Your sylpheed version is too old"));
		return -1;
	}
	
	r = etpan_privacy_init();
	if (r != 0) {
		*error = g_strdup(_("Could not initialize etPan! Privacy plugin"));
		return -1;
	}
	
	etpan_privacy_prefs_init();
	
	debug_print(_("etPan! - Privacy plugin loaded\n"));
	
	return 0;
}

void plugin_done(void)
{
	etpan_privacy_prefs_done();
	etpan_privacy_done();
	debug_print(_("etPan! - Privacy plugin unloaded\n"));
}

const gchar *plugin_name(void)
{
	return _("etPan! Privacy");
}

const gchar *plugin_desc(void)
{
	return _("This plugin adds a driver to decode S/MIME, OpenPGP and armor PGP.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}

const gchar *plugin_licence(void)
{
		return "GPL";
}
