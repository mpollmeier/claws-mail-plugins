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

#ifndef NOTIFICATION_CORE_H
#define NOTIFICATION_CORE_H NOTIFICATION_CORE

#include "folder.h"

typedef struct {
  gchar *from;
  gchar *subject;
  FolderItem *folder_item;
  gchar *folderitem_name;
} CollectedMsg;


GSList* notification_collect_msgs(gboolean, GSList*);
void    notification_collected_msgs_free(GSList*);

#endif /* NOTIFICATION_CORE */
