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
#include "notification_command.h"
#include "notification_foldercheck.h"
#include "notification_pixbuf.h"
#include "plugin.h"

#if HAVE_LIBNOTIFY
#  include <libnotify/notify.h>
#endif


static gboolean my_folder_item_update_hook(gpointer, gpointer);
static gboolean my_msginfo_update_hook(gpointer, gpointer);

static guint hook_f_item;
static guint hook_m_info;

#ifdef NOTIFICATION_BANNER
static GSList* banner_collected_msgs;
#endif


static gboolean my_folder_item_update_hook(gpointer source, gpointer data)
{
  FolderItemUpdateData *update_data = source;
  FolderType ftype;
  gchar *uistr;

  g_return_val_if_fail(source != NULL, FALSE);  

  /* Check if the folder types is to be notified about */
  ftype = update_data->item->folder->klass->type;
  uistr = update_data->item->folder->klass->uistr;
  if(!notify_include_folder_type(ftype, uistr))
    return FALSE;

  if(update_data->update_flags & F_ITEM_UPDATE_MSGCNT) {
#if NOTIFICATION_BANNER
    notification_update_banner();
#endif
#if defined(NOTIFICATION_POPUP) || defined(NOTIFICATION_COMMAND)
    notification_new_unnotified_msgs(update_data);
#endif
  }
  return FALSE;
}

static gboolean my_msginfo_update_hook(gpointer source, gpointer data)
{
  return notification_notified_hash_msginfo_update((MsgInfoUpdate*)source);
}

gint plugin_init(gchar **error)
{
  gchar *rcpath;

  /* Version check */
  if(sylpheed_get_version() > VERSION_NUMERIC) {
    *error = g_strdup("Your Sylpheed-Claws version is newer than the version "
		      "the Notification plugin was built with");
    return -1;
  }
  if(sylpheed_get_version() < MAKE_NUMERIC_VERSION(1, 9, 15, 94)) {
    *error = g_strdup("Your Sylpheed-Claws version is too old for the "
		      "Notification plugin");
    return -1;
  }

  /* Check if threading is enabled */
  if(!g_thread_supported()) {
    *error = g_strdup("The Notification plugin needs threading support.");
    return -1;
  }

  hook_f_item = hooks_register_hook(FOLDER_ITEM_UPDATE_HOOKLIST,
				    my_folder_item_update_hook, NULL);
  if(hook_f_item == (guint) -1) {
    *error = g_strdup("Failed to register folder item update hook in the "
		      "Notification plugin");
    return -1;
  }

  hook_m_info = hooks_register_hook(MSGINFO_UPDATE_HOOKLIST,
				    my_msginfo_update_hook, NULL);
  if(hook_m_info == (guint) -1) {
    *error = g_strdup("Failed to register msginfo update hook in the "
		      "Notification plugin");
    hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
    return -1;
  }

  /* Configuration */
  prefs_set_default(notify_param);
  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
  prefs_read_config(notify_param, "NotificationPlugin", rcpath, NULL);
  g_free(rcpath);

  /* Folder specific stuff */
  notification_foldercheck_read_array();

#ifdef NOTIFICATION_BANNER
  notification_update_banner();
#endif

  notification_notified_hash_startup_init();

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

  notification_notified_hash_free();

  hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
  hooks_unregister_hook(MSGINFO_UPDATE_HOOKLIST, hook_m_info);
  notify_gtk_done();

  /* foldercheck cleanup */
  notification_foldercheck_write_array();
  notification_free_folder_specific_array();

#ifdef HAVE_LIBNOTIFY
  if(notify_is_initted())
    notify_uninit();
#endif

  notification_pixbuf_free_all();

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

struct PluginFeature *plugin_provides(void)
{
  static struct PluginFeature features[] = 
    { {PLUGIN_NOTIFIER, "Various tools"},
      {PLUGIN_NOTHING, NULL}};
  return features;
}

#ifdef NOTIFICATION_BANNER
void notification_update_banner(void)
{
  notification_collected_msgs_free(banner_collected_msgs);
  banner_collected_msgs = NULL;

  if(notify_config.banner_show != NOTIFY_BANNER_SHOW_NEVER) {
    guint id;
    GSList *folder_list = NULL;

    if(notify_config.banner_folder_specific) {
      id = notification_register_folder_specific_list
	(BANNER_SPECIFIC_FOLDER_ID_STR);
      folder_list = notification_foldercheck_get_list(id);
    }

    if(!(notify_config.banner_folder_specific && (folder_list == NULL)))
      banner_collected_msgs =
	notification_collect_msgs(notify_config.banner_include_unread,
				  notify_config.banner_folder_specific ? 
				  folder_list : NULL);
  }
  
  notification_banner_show(banner_collected_msgs);
}
#endif
