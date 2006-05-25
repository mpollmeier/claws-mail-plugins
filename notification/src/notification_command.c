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

#ifdef NOTIFICATION_COMMAND

#include <string.h>
#include <glib.h>
#include "common/utils.h"
#include "notification_command.h"
#include "notification_prefs.h"

typedef struct {
  gboolean blocked;
  guint timeout_id;
} NotificationCommand;

static gboolean command_timeout_fun(gpointer data);

static NotificationCommand command;

G_LOCK_DEFINE_STATIC(command);

void notification_command_msg(MsgInfo *msginfo)
{
  gchar *ret_str, *buf;
  gsize by_read = 0, by_written = 0;

  if(!notify_config.command_enabled || !MSG_IS_NEW(msginfo->flags))
    return;

  buf = g_strdup(notify_config.command_line);

  G_LOCK(command);

  if(!command.blocked) {
    /* execute command */
    command.blocked = TRUE;
    ret_str = g_locale_from_utf8(buf,strlen(buf),&by_read,&by_written,NULL);
    if(ret_str && by_written) {
      g_free(buf);
      buf = ret_str;
    }
    execute_command_line(buf, TRUE);
    g_free(buf);
  }

  /* block further execution for some time,
     no matter if it was blocked or not */
  if(command.timeout_id)
    g_source_remove(command.timeout_id);
  command.timeout_id = g_timeout_add(notify_config.command_timeout,
				     command_timeout_fun, NULL);    
  G_UNLOCK(command);
}

static gboolean command_timeout_fun(gpointer data)
{
  G_LOCK(command);
  command.timeout_id = 0;
  command.blocked = FALSE;
  G_UNLOCK(command);
  return FALSE;
}

#endif /* NOTIFICATION_COMMAND */
