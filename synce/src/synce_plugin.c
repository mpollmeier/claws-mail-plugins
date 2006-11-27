/*
 * synce_plugin -- SynCE Support for Claws Mail
 *
 * Copyright (C) 2005   Holger Berndt
 *
 * Sylpheed is a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto and the Claws Mail Team
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
#include "pluginconfig.h"

#include "common/version.h"
#include "common/utils.h"

#include "pluginconfig.h"
#include "synce_gtk.h"
#include "plugin.h"

#include <glib/gi18n.h>

gint plugin_init(gchar **error)
{
  /* version check */
  if((claws_get_version() > VERSION_NUMERIC)) {
    *error = g_strdup("Your Claws Mail version is newer than the version "
		      "the plugin was built with");
    return -1;
  }
  if((claws_get_version() < MAKE_NUMERIC_VERSION(1, 0, 3, 4))) {
    *error = g_strdup("Your Claws Mail version is too old");
    return -1;
  }

  synce_gtk_init();

  debug_print("SynCE Plugin loaded\n");
  return 0;	
}

void plugin_done(void)
{
  synce_gtk_done();
  debug_print("SynCE Plugin unloaded\n");
}

const gchar *plugin_name(void)
{
  return "SynCE";
}

const gchar *plugin_desc(void)
{
  return "This plugin is based on the SynCE project "
    "and helps keeping email "
    "addresses of the addressbook on a "
    "mobile device running Windows CE (tm) "
    "in sync with Claws' addressbook.\nFeedback "
    "to <berndth@gmx.de> is welcome.\n";
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
		{ {PLUGIN_UTILITY, N_("SynCE Synchronisation")},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}
