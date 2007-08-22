/* SynCE plugin -- SynCE Support for Claws Mail
 *
 * Copyright (C) 2005-2007 Holger Berndt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
	if(!check_plugin_version(MAKE_NUMERIC_VERSION(2,9,2,72),
				VERSION_NUMERIC, "SynCE", error))
		return -1;

  synce_gtk_init();

  debug_print("SynCE Plugin loaded\n");
  return 0;	
}

gboolean plugin_done(void)
{
  synce_gtk_done();
  debug_print("SynCE Plugin unloaded\n");
  return TRUE;
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
  return "GPL3+";
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
