/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2002 Hiroyuki Yamamoto and the Sylpheed-Claws Team
 *
 * LCDProc Plugin - version 0.1.4
 * Copyright (C) 2003 Leandro Pereira <leandro@linuxmag.com.br>
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

#include "plugin.h"
#include "utils.h"
#include "folder.h"
#include "hooks.h"

#include "socket.h"
#include "alertpanel.h"

static const gchar   *server = "127.0.0.1";
static const gushort port    = 13666;
static guint hook_id;

static SockInfo *sock = NULL;

/*
 * Shameless copy from lcdxmms.c by kuro :)
 */
static void lcd_connect()
{
	gint len, count = 0;
	gchar buf[8192];

 	sock = sock_connect(server, port);
 	/*
 	 * Quietly return when a connection fails; next attempt
 	 * will be made when some folder info has been changed.
 	 */
	if (sock == NULL || sock->state == CONN_FAILED)
		return;

	sock_set_nonblocking_mode(sock, TRUE);

	sock_puts(sock, "hello");
	
	for (count = 0; count < 50 && len <= 0; count++) {
		usleep (125000);
		len = sock_read(sock, buf, 8000);
	}

	/*
	 * This might not be a LCDProc server.
	 * FIXME: check LCD size, LCDd version etc
	 */
	if (len <= 0) {
		alertpanel_error("Can't communicate with server! Are you sure that\n"
		                 "there is a LCDProc running on %s:%d?", server, port);
		sock_close(sock);
		return;
	}
		
	sock_puts(sock, "client_set name {LCDSylpheed}");
	sock_puts(sock, "screen_add sylph");
	sock_puts(sock, "screen_set sylph name {Sylpheed Screen}");
	
	sock_puts(sock, "widget_add sylph title title");
	sock_puts(sock, "widget_set sylph title {Sylpheed}");
	sock_puts(sock, "widget_add sylph unread string");
	
	lcd_update_hook(NULL, NULL);
}

gboolean lcd_update_hook(gpointer source, gpointer data)
{
	gint unread, new, trash;
	gchar *buf;
	
	/*
	 * (Re)connect to the server.
	 */
	if (sock == NULL || sock->state == CONN_FAILED) {
		lcd_connect();
		return FALSE;
	}

	folder_count_total_msgs(&new, &unread, &trash, &trash);
	if (unread + new != 0)
		buf = g_strdup_printf("widget_set sylph unread 1 2 {U: %d N: %d}", unread, new);
	else
		buf = g_strdup("widget_set sylph unread 1 2 {No unread msgs!}");
		
	sock_puts(sock, buf);
	
	g_free(buf);	
	return FALSE;
}

gint plugin_init(gchar **error)
{
	hook_id = hooks_register_hook (FOLDER_ITEM_UPDATE_HOOKLIST, lcd_update_hook, NULL);
	if (hook_id == -1) {
		*error = g_strdup("Failed to register LCD update hook");
		return -1;
	}

	lcd_connect();

	return 0;
}

void plugin_done()
{
	if (sock != NULL)
		sock_close(sock);
	
	hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_id);	
}

const gchar *plugin_name()
{
	return "LCDProc Plugin";
}

const gchar *plugin_type()
{
	return "Common";
}

const gchar *plugin_desc()
{
	return "Shows currently unread/new message info in a LCDProc screen.\n"
	       "\n"
	       "Requires a LCDProc server running on localhost, port 13666.\n";
}
