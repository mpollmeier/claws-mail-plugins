/* Notification Plugin for Sylpheed-Claws
 * Copyright (C) 2005-2006 Holger Berndt
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


#ifndef NOTIFICATION_FOLDERCHECK_H
#define NOTIFICATION_FOLDERCHECK_H NOTIFICATION_FOLDERCHECK_H

#include <gtk/gtk.h>

#include "pluginconfig.h"

void     notification_foldercheck_sel_folders_cb(GtkButton*, gpointer);
guint    notification_register_folder_specific_list(gchar*);
GSList*  notification_foldercheck_get_list(guint);
void     notification_free_folder_specific_array(void);
void     notification_foldercheck_write_array(void);
gboolean notification_foldercheck_read_array(void);

#endif /* NOTIFICATION_FOLDERCHECK_H */
