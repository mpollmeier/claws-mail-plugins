/* Notification Plugin for Sylpheed-Claws
 * Copyright (C) 2005 Holger Berndt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
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

#include "pluginconfig.h"

#include "folder.h"

#include "notification_core.h"
#include "notification_banner.h"

typedef struct {
  GSList *collected_msgs;
  gboolean unread_also;
} TraverseCollect;

static gboolean notification_traverse_collect(GNode*, gpointer);

GSList* notification_collect_msgs(gboolean unread_also)
{
  GList *folder_list, *walk;
  Folder *folder;
  TraverseCollect collect_data;

  collect_data.unread_also = unread_also;
  collect_data.collected_msgs = NULL;
  folder_list = folder_get_list();
  for(walk = folder_list; walk != NULL; walk = g_list_next(walk)) {
    folder = walk->data;
    g_node_traverse(folder->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
		    notification_traverse_collect, &collect_data);
  }
  return collect_data.collected_msgs;
}

void notification_collected_msgs_free(GSList *collected_msgs)
{
  if(collected_msgs) {
    GSList *walk;
    for(walk = collected_msgs; walk != NULL; walk = g_slist_next(walk)) {
      CollectedMsg *msg = walk->data;
      if(msg->from)
	g_free(msg->from);
      if(msg->subject)
	g_free(msg->subject);
      if(msg->folderitem_name)
	g_free(msg->folderitem_name);
      g_free(msg);
    }
    g_slist_free(collected_msgs);
  }
}

static gboolean notification_traverse_collect(GNode *node, gpointer data)
{
  TraverseCollect *cdata = data;
  FolderItem *item = node->data;

  if(item->new_msgs || (cdata->unread_also && item->unread_msgs)) {
    GSList *msg_list = folder_item_get_msg_list(item);
    GSList *walk;
    for(walk = msg_list; walk != NULL; walk = g_slist_next(walk)) {
      MsgInfo *msg_info = walk->data;
      CollectedMsg *cmsg;
      if(MSG_IS_NEW(msg_info->flags) ||
	 (MSG_IS_UNREAD(msg_info->flags) && cdata->unread_also)) {
	cmsg = g_new(CollectedMsg, 1);
	cmsg->from = g_strdup(msg_info->from ? msg_info->from : "");
	cmsg->subject = g_strdup(msg_info->subject ? msg_info->subject : "");
	if(msg_info->folder && msg_info->folder->name)
	  cmsg->folderitem_name = g_strdup(msg_info->folder->path);
	else
	  cmsg->folderitem_name = g_strdup("");
	cdata->collected_msgs = g_slist_prepend(cdata->collected_msgs, cmsg);
      }
    }
    procmsg_msg_list_free(msg_list);
  }

  return FALSE;
}
