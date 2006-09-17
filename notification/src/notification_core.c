/* Notification Plugin for Sylpheed-Claws
 * Copyright (C) 2005-2006 Holger Berndt
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
#include "notification_popup.h"
#include "notification_command.h"

typedef struct {
  GSList *collected_msgs;
  GSList *folder_items;
  gboolean unread_also;
} TraverseCollect;

static gboolean notification_traverse_collect(GNode*, gpointer);
static void     notification_new_unnotified_do_msg(MsgInfo*);
static void     notification_foreach_free_val(gpointer, gpointer, gpointer);
static gboolean notification_traverse_hash_startup(GNode*, gpointer);

#if 0
#if defined debug_print
#undef debug_print
#define debug_print g_print
#endif
#endif

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
   and that still are marked as "new". The keys are the msgnum's,
   the values are lists which contain the folderitems in which the
   msg is. (It would be more intuitive the other way round, but this
   way is supposed to be faster, since folders are much more likely to
   collide than message numbers) */
static GHashTable *notified_hash = NULL;


/* Remove message from the notified_hash if
   - the message flags changed
   - the message is not new
   - the message is in the hash
*/
gboolean notification_notified_hash_msginfo_update(MsgInfoUpdate *msg_update)
{
  g_return_val_if_fail(msg_update != NULL, FALSE);

  if((msg_update->flags & MSGINFO_UPDATE_FLAGS) &&
     !MSG_IS_NEW(msg_update->msginfo->flags)) {
    GSList *folder_list;
    MsgInfo *msg;
    
    msg = msg_update->msginfo;
    
    g_return_val_if_fail(msg != NULL, FALSE);

    folder_list = (GSList*) g_hash_table_lookup(notified_hash,
						GUINT_TO_POINTER(msg->msgnum));
    if(folder_list) {

      folder_list = g_slist_remove(folder_list, msg->folder);

      debug_print("Notification Plugin: Message id number %u is in hash "
		  "table, removing folder %p from list, new list starts "
		  "at %p\n", msg->msgnum, msg->folder,folder_list);

      if(!folder_list) {
	debug_print("Notification Plugin: Removing entry %u from hash table, "
		    "because folder list is now empty\n", msg->msgnum);
	g_hash_table_remove(notified_hash, GUINT_TO_POINTER(msg->msgnum));
      }
      else {
	debug_print("Notification Plugin: Replacing entry %u in hash, because "
		    "list is not yet empty\n", msg->msgnum);
	g_hash_table_replace(notified_hash, GUINT_TO_POINTER(msg->msgnum),
			     folder_list);
      }
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
    notified_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
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
      GSList *folder_list;
      
      /* The message/folder combination can not yet be in the hash table, add
	 it right away. */
      folder_list =
	(GSList*)g_hash_table_lookup(notified_hash,
				     GUINT_TO_POINTER(msg->msgnum));
      folder_list = g_slist_prepend(folder_list,msg->folder);
      /* then replace the old value pointer or insert a new key */
      g_hash_table_insert(notified_hash, GUINT_TO_POINTER(msg->msgnum),
			  folder_list);
      debug_print("Notification Plugin: Init: Adding (%u,%p) to the hash\n",
		  msg->msgnum, msg->folder);

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
    debug_print("Notification Plugin: Destroying hash table...\n");
    g_hash_table_foreach(notified_hash, notification_foreach_free_val, NULL);
    g_hash_table_destroy(notified_hash);
  }
  notified_hash = NULL;
  debug_print("Notification Plugin: Hash table destroyed\n");
}

void notification_new_unnotified_msgs(FolderItemUpdateData *update_data)
{
  GSList *msg_list, *walk;

  if(!notified_hash) {
    notified_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
    debug_print("Notification Plugin: Hash table created\n");
  }

  msg_list = folder_item_get_msg_list(update_data->item);

  for(walk = msg_list; walk; walk = g_slist_next(walk)) {
    MsgInfo *msg;
    msg = (MsgInfo*) walk->data;
    
    if(MSG_IS_NEW(msg->flags)) {
      GSList *walk, *folder_list;
      gboolean found = FALSE;
      debug_print("Notification Plugin: Found msg %u, "
		  "checking if it is in hash... ", msg->msgnum);
      /* Check if message is in hash table */
      folder_list =
	(GSList*) g_hash_table_lookup(notified_hash,
				      GUINT_TO_POINTER(msg->msgnum));
      for(walk = folder_list; walk; walk = g_slist_next(walk)) {
	if(g_direct_equal(walk->data, (gpointer)msg->folder)) {
	  found = TRUE;
	  debug_print("yes, folderlist at %p\n",folder_list);
	  break;
	}
      }

      if(!found) {
	/* Add to hashtable */
	/* First add the new folderitem to the list... */
	folder_list = g_slist_prepend(folder_list,msg->folder);
	debug_print("no, adding folderlist at %p\n",folder_list);
	/* then replace the old value pointer or insert a new key */
	g_hash_table_insert(notified_hash, GUINT_TO_POINTER(msg->msgnum),
			    folder_list);

	/* Do the notification */
	notification_new_unnotified_do_msg(msg);
      }
      
    } /* msg is 'new' */
  } /* for all messages */
  procmsg_msg_list_free(msg_list);
}

static void notification_foreach_free_val(gpointer key, gpointer val,
					  gpointer data)
{
  debug_print("Notification Plugin: Destroying value %p in the hash table\n",
	      val);
  if(val)
    g_slist_free(val);
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

/* if folders is not NULL, then consider only those folder items */
GSList* notification_collect_msgs(gboolean unread_also, GSList *folder_items)
{
  GList *folder_list, *walk;
  Folder *folder;
  TraverseCollect collect_data;

  collect_data.unread_also = unread_also;
  collect_data.collected_msgs = NULL;
  collect_data.folder_items = folder_items;

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
