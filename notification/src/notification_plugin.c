/* Notification Plugin for Sylpheed-Claws
 * Copyright (C) 2005 Holger Berndt
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

#include "common/sylpheed.h"
#include "common/version.h"
#include "common/utils.h"
#include "common/defs.h"
#include "folder.h"
#include "hooks.h"
#include "procmsg.h"

#include "notification_plugin.h"
#include "notification_core.h"
#include "notification_prefs.h"
#include "notification_banner.h"
#include "notification_popup.h"


static gboolean my_folder_item_update_hook(gpointer, gpointer);
static gboolean my_post_filtering_hook    (gpointer, gpointer);


static guint hook_msgcnt;
static guint hook_pfilter;


#ifdef NOTIFICATION_BANNER
static GSList *banner_collected_msgs;
#endif

static gboolean my_post_filtering_hook(gpointer source, gpointer data)
{
  g_return_val_if_fail(source != NULL, FALSE);

#ifdef NOTIFICATION_POPUP
  notification_popup_msg((MsgInfo*)source);
#endif

  return FALSE;
}

static gboolean my_folder_item_update_hook(gpointer source, gpointer data)
{
  FolderItemUpdateData *update_data = source;

  g_return_val_if_fail(source != NULL, FALSE);

  if(update_data->update_flags & F_ITEM_UPDATE_MSGCNT) {
#ifdef NOTIFICATION_BANNER
    notification_update_banner();
#endif
  }
  return FALSE;
}

gint plugin_init(gchar **error)
{
  gchar *rcpath;

  /* Version check */
  if(sylpheed_get_version() > VERSION_NUMERIC) {
    *error = g_strdup("Your Sylpheed-Claws version is newer than the version "
		      "the notification plugin was built with");
    return -1;
  }
  if(sylpheed_get_version() < MAKE_NUMERIC_VERSION(1, 9, 15, 94)) {
    *error = g_strdup("Your Sylpheed-Claws version is too old for the "
		      "notification plugin");
    return -1;
  }

  /* Check if threading is enabled */
  if(!g_thread_supported()) {
    *error = g_strdup("The notification plugin needs threading support.");
    return -1;
  }

  hook_msgcnt = hooks_register_hook(FOLDER_ITEM_UPDATE_HOOKLIST,
				my_folder_item_update_hook, NULL);
  if(hook_msgcnt == (guint) -1) {
    *error = g_strdup("Failed to register folder item update hook in the "
		      "notification plugin");
    return -1;
  }
  hook_pfilter = hooks_register_hook(MAIL_POSTFILTERING_HOOKLIST,
				     my_post_filtering_hook, NULL);
  if(hook_pfilter == (guint) -1) {
    *error = g_strdup("Failed to register mail postfiltering hook in the "
		      "notification plugin");
    hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_msgcnt);
    return -1;
  }

  /* Configuration */
  prefs_set_default(notify_param);
  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
  prefs_read_config(notify_param, "NotificationPlugin", rcpath, NULL);
  g_free(rcpath);

#ifdef NOTIFICATION_BANNER
  notification_update_banner();
#endif

  notify_gtk_init();
  debug_print("Notification plugin loaded\n");

  return 0;
}

void plugin_done(void)
{
  notify_save_config();

#ifdef NOTIFICATION_BANNER
  notification_collected_msgs_free(banner_collected_msgs);
  banner_collected_msgs = NULL;
  notification_banner_destroy();
#endif

  hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_msgcnt);
  notify_gtk_done();
  debug_print("Notification plugin unloaded\n");
}

const gchar *plugin_name(void)
{
  return "Notification";
}

const gchar *plugin_desc(void)
{
  return "This plugin provides various ways "
    "to notify the user of new and unread email.\nFeedback "
    "to <berndth@gmx.de> is welcome.";
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

#ifdef NOTIFICATION_BANNER
void notification_update_banner(void)
{
  notification_collected_msgs_free(banner_collected_msgs);
  banner_collected_msgs = NULL;
  if(notify_config.banner_show != NOTIFY_BANNER_SHOW_NEVER)
    banner_collected_msgs =
      notification_collect_msgs(notify_config.banner_include_unread);
  notification_banner_show(banner_collected_msgs);
}
#endif
