/* Notification plugin for Claws-Mail
 * Copyright (C) 2005-2009 Holger Berndt and the Claws Mail Team.
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

#include "pluginconfig.h"

#ifdef NOTIFICATION_INDICATOR

#include "notification_indicator.h"
#include "notification_prefs.h"
#include "notification_core.h"

#include "folder.h"
#include "common/utils.h"

#include <libindicate/server.h>
#include <libindicate/indicator.h>
#include <libindicate/indicator-messages.h>

static IndicateServer *server = NULL;
static GHashTable *indicators = NULL;

void notification_indicator_destroy(void)
{
  if(indicators) {
    g_hash_table_destroy(indicators);
    indicators = NULL;
  }
  if(server) {
    indicate_server_hide(server);
    g_object_unref(server);
    server = NULL;
  }
}

static void show_claws_mail(gpointer dummy, gpointer data)
{
  MainWindow *mainwin;
  if((mainwin = mainwindow_get_mainwindow()) == NULL)
    return;

  notification_show_mainwindow(mainwin);
  // TODO: if data is non-NULL, it contains the account name.
  // display first new message in that account
}

static void set_indicator_unread_count(IndicateIndicator *indicator, gint new, gint unread)
{
  gchar *count_str;

  count_str = g_strdup_printf("%d / %d", new, unread);
  indicate_indicator_set_property(indicator, INDICATE_INDICATOR_MESSAGES_PROP_COUNT, count_str);
  g_free(count_str);
}

static void create_indicators(void)
{
  IndicateIndicator *indicator;
  GList *cur_mb;

  indicators = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

  for(cur_mb = folder_get_list(); cur_mb; cur_mb = cur_mb->next) {
    gchar *name;
    Folder *folder = cur_mb->data;

    if(!folder->name) {
      debug_print("Notification plugin: Warning: Ignoring unnamed mailbox in indicator applet\n");
      continue;
    }
    name = g_strdup(folder->name);

    indicator = indicate_indicator_new();
    indicate_indicator_set_property(indicator, INDICATE_INDICATOR_MESSAGES_PROP_NAME, name);
    set_indicator_unread_count(indicator, 0, 0);
    g_object_set_data(G_OBJECT(indicator), "new_msgs", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(indicator), "unread_msgs", GINT_TO_POINTER(0));
    g_signal_connect(indicator, "user-display", G_CALLBACK (show_claws_mail), name);
    indicate_indicator_show(indicator);
    g_hash_table_insert(indicators, name, indicator);
  }
}

void notification_update_indicator(void)
{
  GHashTableIter iter;
  gpointer key, value;

  if(!notify_config.indicator_enabled)
    return;

  if(!server) {
    server = indicate_server_ref_default();
    indicate_server_set_type (server, "message.mail");
    indicate_server_set_desktop_file(server, get_desktop_file());
    g_signal_connect(server, "server-display", G_CALLBACK(show_claws_mail), NULL);
    indicate_server_show(server);
  }

  if(!indicators)
    create_indicators();

  /* check accounts for new/unread counts */
  g_hash_table_iter_init(&iter, indicators);
  while(g_hash_table_iter_next(&iter, &key, &value)) {
    NotificationMsgCount count;
    gint former_new, former_unread;
    gchar *foldername = key;
    IndicateIndicator *indicator = value;

    former_new = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(indicator), "new_msgs"));
    former_unread = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(indicator), "unread_msgs"));

    notification_core_get_msg_count_of_foldername(foldername, &count);

    set_indicator_unread_count(indicator, count.new_msgs, count.unread_msgs);
    indicate_indicator_set_property(indicator, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION,
        (count.new_msgs > former_new) ? "true" : "false");
    g_object_set_data(G_OBJECT(indicator), "new_msgs", GINT_TO_POINTER(count.new_msgs));
    g_object_set_data(G_OBJECT(indicator), "unread_msgs", GINT_TO_POINTER(count.unread_msgs));
  }
}

#endif /* NOTIFICATION_INDICATOR */