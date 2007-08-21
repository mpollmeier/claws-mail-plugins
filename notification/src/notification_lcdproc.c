/* Notification Plugin for Claws Mail
 * Copyright (C) 2005-2007 Holger Berndt
 *
 * Code inspired by (and partly stolen from) the old GTK1 Sylpheed-Claws
 * LCDProc plugin, version 0.1.4
 * which is copyrighted (C) 2003 by Leandro Pereira <leandro@linuxmag.com.br>
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

#ifdef NOTIFICATION_LCDPROC

#include "notification_lcdproc.h"
#include "notification_prefs.h"
#include "notification_core.h"

#include "common/socket.h"

#include <string.h>

#define NOTIFICATION_LCDPROC_BUFFER_SIZE 8192

void notification_sock_puts(SockInfo*, gchar*);

static SockInfo *sock = NULL;

void notification_lcdproc_connect(void)
{
  gint len, count;
  gchar buf[NOTIFICATION_LCDPROC_BUFFER_SIZE];

  if(sock)
    notification_lcdproc_disconnect();

  sock = sock_connect(notify_config.lcdproc_hostname,
		      notify_config.lcdproc_port);
  /*
   * Quietly return when a connection fails; next attempt
   * will be made when some folder info has been changed.
   */
  if(sock == NULL || sock->state == CONN_FAILED) {
    debug_print("Could not connect to LCDd\n");
    if(sock && sock->state == CONN_FAILED) {
      sock_close(sock);
      sock = NULL;
    }
    return;
  }
  else
    debug_print("Connected to LCDd\n");
  
  sock_set_nonblocking_mode(sock, TRUE);

  /* Friendly people say "hello" first */
  notification_sock_puts(sock, "hello");

  /* FIXME: Ouch. Is this really the way to go? */
  count = 50;
  len = 0;
  while((len <= 0) && (count-- >= 0)) {
    g_usleep(125000);
    len = sock_read(sock, buf, NOTIFICATION_LCDPROC_BUFFER_SIZE);
  }
  
/*
 * This might not be a LCDProc server.
 * FIXME: check LCD size, LCDd version etc
 */
  
  if (len <= 0) {
    debug_print("Notification plugin: Can't communicate with "
		"LCDd server! Are you sure that "
		"there is a LCDd server running on %s:%d?\n",
		notify_config.lcdproc_hostname, notify_config.lcdproc_port);
    notification_lcdproc_disconnect();
    return;
  }
  
  notification_sock_puts(sock, "client_set -name \"{Claws-Mail}\"");

  notification_sock_puts(sock, "screen_add msg_counts");
  notification_sock_puts(sock, "screen_set msg_counts -name {Claws-Mail Message Count}");
  
  notification_sock_puts(sock, "widget_add msg_counts title title");
  notification_sock_puts(sock, "widget_set msg_counts title {Claws-Mail}");
  notification_sock_puts(sock, "widget_add msg_counts line1 string");
  notification_sock_puts(sock, "widget_add msg_counts line2 string");

  notification_update_msg_counts();
}

void notification_lcdproc_disconnect(void)
{
  if(sock) {
    debug_print("Disconnecting from LCDd\n");
    
    if(sock->state != CONN_FAILED) {
      notification_sock_puts(sock, "screen_del msg_counts");
      sock_close(sock);
    }
    
    sock = NULL;
  }
}

void notification_update_lcdproc(guint new_msgs, guint unread_msgs)
{
  gchar *buf;

  if(!notify_config.lcdproc_enabled || !sock)
    return;

  if(!sock || sock->state == CONN_FAILED) {
    notification_lcdproc_connect();
    return;
  }
  

  if((new_msgs + unread_msgs) > 0) {
    buf =
      g_strdup_printf("widget_set msg_counts line1 1 2 {New: %d}",new_msgs);
    notification_sock_puts(sock, buf);
    buf =
      g_strdup_printf("widget_set msg_counts line2 1 3 {Unread: %d}",unread_msgs);
    notification_sock_puts(sock, buf);
  }
  else {
    buf = g_strdup_printf("widget_set msg_counts line1 1 2 {No new messages}");
    notification_sock_puts(sock, buf);
    buf = g_strdup_printf("widget_set msg_counts line2 1 3 {}");
    notification_sock_puts(sock, buf);
  }

  g_free(buf);
}

void notification_sock_puts(SockInfo *socket, gchar *string)
{
  sock_write(socket, string, strlen(string));
  sock_write(socket, "\n", 1);
}


#endif /* NOTIFICATION_LCDPROC */
