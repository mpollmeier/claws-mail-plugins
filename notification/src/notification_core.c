/* Notification plugin for Claws-Mail
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

#include "pluginconfig.h"

#include "folder.h"

#include "notification_prefs.h"
#include "notification_core.h"
#include "notification_banner.h"
#include "notification_popup.h"
#include "notification_command.h"
#include "notification_lcdproc.h"

typedef struct {
  GSList *collected_msgs;
  GSList *folder_items;
  gboolean unread_also;
  gint max_msgs;
  gint num_msgs;
} TraverseCollect;

static gboolean notification_traverse_collect(GNode*, gpointer);
static void     notification_new_unnotified_do_msg(MsgInfo*);
static gboolean notification_traverse_hash_startup(GNode*, gpointer);

void notification_update_msg_counts(void)
{
  guint unread_msgs;
  guint new_msgs;
  guint unread_marked_msgs;
  guint marked_msgs;
  guint total_msgs;

  folder_count_total_msgs(&new_msgs, &unread_msgs, &unread_marked_msgs,
			  &marked_msgs, &total_msgs);

#ifdef NOTIFICATION_LCDPROC
  notification_update_lcdproc(new_msgs, unread_msgs, total_msgs);
#endif
}


/* Replacement for the post-filtering hook:
   Pseudocode by Colin:
hook on FOLDER_ITEM_UPDATE_HOOKLIST
 if hook flags & F_ITEM_UPDATE_MSGCOUNT
  scan mails (folder_item_get_msg_list)
   if MSG_IS_NEW(msginfo->flags) and not in hashtable
    notify()
    add to hashtable
   procmsg_msg_list_free

hook on MSGINFO_UPDATE_HOOKLIST
 if hook flags & MSGINFO_UPDATE_FLAGS
  if !MSG_IS_NEW(msginfo->flags)
   remove from hashtable, it's now useless
*/

/* This hash table holds all mails that we already notified about,
   and that still are marked as "new". The keys are the msgid's,
   the values are just 1's stored in a pointer. */
static GHashTable *notified_hash = NULL;


/* Remove message from the notified_hash if
 *  - the message flags changed
 *  - the message is not new
 *  - the message is in the hash
*/
gboolean notification_notified_hash_msginfo_update(MsgInfoUpdate *msg_update)
{
  g_return_val_if_fail(msg_update != NULL, FALSE);

  if((msg_update->flags & MSGINFO_UPDATE_FLAGS) &&
     !MSG_IS_NEW(msg_update->msginfo->flags)) {

    MsgInfo *msg;
    gchar *msgid;

    msg = msg_update->msginfo;
    if(msg->msgid)
      msgid = msg->msgid;
    else {
      debug_print("Notification Plugin: Message has no message ID!\n");
      msgid = "";
    }
    
    g_return_val_if_fail(msg != NULL, FALSE);

    if(g_hash_table_lookup(notified_hash, msgid) != NULL) {
      
      debug_print("Notification Plugin: Removing message id %s from hash "
		  "table\n", msgid);
      g_hash_table_remove(notified_hash, msgid);
    }
  }
  return FALSE;
}

/* On startup, mark all new mails as already notified
 * (by including them in the hash) */
void notification_notified_hash_startup_init(void)
{
  GList *folder_list, *walk;
  Folder *folder;

  if(!notified_hash) {
    notified_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
					  g_free, NULL);
    debug_print("Notification Plugin: Hash table created\n");
  }

  folder_list = folder_get_list();
  for(walk = folder_list; walk != NULL; walk = g_list_next(walk)) {
    folder = walk->data;
    
    g_node_traverse(folder->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
		    notification_traverse_hash_startup, NULL);
  }
}

static gboolean notification_traverse_hash_startup(GNode *node, gpointer data)
{
  GSList *walk;
  GSList *msg_list;
  FolderItem *item = (FolderItem*) node->data;
  gint new_msgs_left;

  if(!(item->new_msgs))
    return FALSE;

  new_msgs_left = item->new_msgs;
  msg_list = folder_item_get_msg_list(item);
  
  for(walk = msg_list; walk; walk = g_slist_next(walk)) {
    MsgInfo *msg = (MsgInfo*) walk->data;
    if(MSG_IS_NEW(msg->flags)) {
      gchar *msgid;

      if(msg->msgid)
	msgid = msg->msgid;
      else {
	debug_print("Notification Plugin: Message has no message ID!\n");
	msgid = "";
      }

      /* If the message id is not yet in the hash, add it */
      g_hash_table_insert(notified_hash, g_strdup(msgid),
			  GINT_TO_POINTER(1));
      debug_print("Notification Plugin: Init: Added msg id %s to the hash\n",
		  msgid);
      /* Decrement left count and check if we're already done */
      new_msgs_left--;
      if(new_msgs_left == 0)
	break;
    }
  }
  procmsg_msg_list_free(msg_list);
  return FALSE;
}

void notification_notified_hash_free(void)
{
  if(notified_hash) {
    g_hash_table_destroy(notified_hash);
    notified_hash = NULL;
    debug_print("Notification Plugin: Hash table destroyed\n");
  }
}

void notification_new_unnotified_msgs(FolderItemUpdateData *update_data)
{
  GSList *msg_list, *walk;

  g_return_if_fail(notified_hash != NULL);

  msg_list = folder_item_get_msg_list(update_data->item);

  for(walk = msg_list; walk; walk = g_slist_next(walk)) {
    MsgInfo *msg;
    msg = (MsgInfo*) walk->data;
    
    if(MSG_IS_NEW(msg->flags)) {
      gchar *msgid;

      if(msg->msgid)
	msgid = msg->msgid;
      else {
	debug_print("Notification Plugin: Message has not message ID!\n");
	msgid = "";
      }

      debug_print("Notification Plugin: Found msg %s, "
		  "checking if it is in hash...\n", msgid);
      /* Check if message is in hash table */
      if(g_hash_table_lookup(notified_hash, msgid) != NULL)
	debug_print("yes.\n");
      else {
	/* Add to hashtable */
	g_hash_table_insert(notified_hash, g_strdup(msgid),
			    GINT_TO_POINTER(1));
	debug_print("no, added to table.\n");
	
	/* Do the notification */
	notification_new_unnotified_do_msg(msg);
      }
      
    } /* msg is 'new' */
  } /* for all messages */
  procmsg_msg_list_free(msg_list);
}

static void notification_new_unnotified_do_msg(MsgInfo *msg)
{
#ifdef NOTIFICATION_POPUP
  notification_popup_msg(msg);
#endif
#ifdef NOTIFICATION_COMMAND
  notification_command_msg(msg);
#endif
}

/* If folders is not NULL, then consider only those folder items
 * If max_msgs is not 0, stop after collecting msg_msgs messages
 */
GSList* notification_collect_msgs(gboolean unread_also, GSList *folder_items,
				  gint max_msgs)
{
  GList *folder_list, *walk;
  Folder *folder;
  TraverseCollect collect_data;

  collect_data.unread_also = unread_also;
  collect_data.collected_msgs = NULL;
  collect_data.folder_items = folder_items;
  collect_data.max_msgs = max_msgs;
  collect_data.num_msgs = 0;

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
  gchar *folder_id_cur;

  /* Obey global folder type limitations */
  if(!notify_include_folder_type(item->folder->klass->type,
				 item->folder->klass->uistr))
    return FALSE;

  /* If a folder_items list was given, check it first */
  if((cdata->folder_items) && (item->path != NULL) &&
     ((folder_id_cur  = folder_item_get_identifier(item)) != NULL)) {
    FolderItem *list_item;
    GSList *walk;
    gchar *folder_id_list;
    gboolean eq;
    gboolean folder_in_list = FALSE;

    for(walk = cdata->folder_items; walk != NULL; walk = g_slist_next(walk)) {
      list_item = walk->data;
      folder_id_list = folder_item_get_identifier(list_item);
      eq = !strcmp2(folder_id_list,folder_id_cur);
      g_free(folder_id_list);
      if(eq) {
	folder_in_list = TRUE;
	break;
      }
    }
    g_free(folder_id_cur);
    if(!folder_in_list)
      return FALSE;
  }

  if(item->new_msgs || (cdata->unread_also && item->unread_msgs)) {
    GSList *msg_list = folder_item_get_msg_list(item);
    GSList *walk;
    for(walk = msg_list; walk != NULL; walk = g_slist_next(walk)) {
      MsgInfo *msg_info = walk->data;
      CollectedMsg *cmsg;

      if((cdata->max_msgs != 0) && (cdata->num_msgs >= cdata->max_msgs))
	return FALSE;

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
	cdata->num_msgs++;
      }
    }
    procmsg_msg_list_free(msg_list);
  }

  return FALSE;
}

gboolean notify_include_folder_type(FolderType ftype, gchar *uistr)
{
  gboolean retval;

  retval = FALSE;
  switch(ftype) {
  case F_MH:
  case F_MBOX:
  case F_MAILDIR:
  case F_IMAP:
    if(notify_config.include_mail)
      retval = TRUE;
    break;
  case F_NEWS:
    if(notify_config.include_news)
      retval = TRUE;
    break;
  case F_UNKNOWN:
    if(uistr == NULL)
      retval = FALSE;
    else if(!strcmp(uistr, "vCalendar")) {
      if(notify_config.include_calendar)
	retval = TRUE;
    }
    else if(!strcmp(uistr, "RSSyl")) {
      if(notify_config.include_rss)
	retval = TRUE;
    }
    else
      debug_print("Notification Plugin: Unknown folder type %d\n",ftype);
    break;
  default:
    debug_print("Notification Plugin: Unknown folder type %d\n",ftype);
  }

  return retval;
}
