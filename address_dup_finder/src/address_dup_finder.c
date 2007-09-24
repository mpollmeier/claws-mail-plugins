/* Address duplicate finder plugin for Claws-Mail
 * Copyright (C) 2007 Holger Berndt
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

#include "adf_prefs.h"
#include "adf_gtk.h"

#include "common/claws.h"
#include "common/version.h"
#include "plugin.h"
#include "defs.h"

#include "gettext.h"

gint plugin_init(gchar **error)
{
  gchar *rcpath;

  bindtextdomain(TEXTDOMAIN, LOCALEDIR);
  bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");
  
  /* Version check */
  if(!check_plugin_version(MAKE_NUMERIC_VERSION(2,9,2,72),
			   VERSION_NUMERIC, _("Addess duplicate finder"), error))
    return -1;

  /* Configuration */
  prefs_set_default(adf_param);
  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
  prefs_read_config(adf_param, "AddressDupesFinderPlugin", rcpath, NULL);
  g_free(rcpath);

  adf_gtk_init();

  debug_print("Address duplicate finder plugin loaded\n");

  return 0;
}

gboolean plugin_done(void)
{
  adf_save_config();
  adf_gtk_done();

  debug_print("Address duplicate finder plugin unloaded\n");
  return TRUE;
}

const gchar *plugin_name(void)
{
  return _("Address duplicate finder");
}

const gchar *plugin_desc(void)
{
  return _("This plugin finds email address duplicates "
	   "in the addressbook.\nFeedback "
	   "to <berndth@gmx.de> is welcome.");
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
    { {PLUGIN_UTILITY, N_("Address duplicates finder")},
      {PLUGIN_NOTHING, NULL}};
  return features;
}
