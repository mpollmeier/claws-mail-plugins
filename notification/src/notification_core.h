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

#ifndef NOTIFICATION_CORE_H
#define NOTIFICATION_CORE_H NOTIFICATION_CORE_H

#include "folder.h"

typedef struct {
  gchar *from;
  gchar *subject;
  FolderItem *folder_item;
  gchar *folderitem_name;
} CollectedMsg;


/* Collect new and possibly unread messages in all folders */
GSList*  notification_collect_msgs(gboolean, GSList*, gint);
void     notification_collected_msgs_free(GSList*);

/* New mail notification */
void     notification_update_msg_counts(void);
void     notification_new_unnotified_msgs(FolderItemUpdateData*);
void     notification_notified_hash_free(void);
gboolean notification_notified_hash_msginfo_update(MsgInfoUpdate*);
void     notification_notified_hash_startup_init(void);

/* folder type specific settings */
gboolean notify_include_folder_type(FolderType, gchar*);

#endif /* NOTIFICATION_CORE_H */
