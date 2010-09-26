/* vim: set textwidth=80 tabstop=4: */

/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2008 Michael Rasmussen and the Claws Mail Team
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
 * 
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gtk/gtkutils.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "common/claws.h"
#include "common/version.h"
#include "plugin.h"
#include "utils.h"
#include "prefs.h"
#include "folder.h"
#include "prefs_gtk.h"
#include "foldersel.h"
#include "statusbar.h"
#include "alertpanel.h"
#include "clamd-plugin.h"

/* needs to be generic */
static const gchar* config_dirs[] = { 
	"/etc", 
	"/usr/local/etc",
	"/etc/clamav",
	"/usr/local/etc/clamav",
	NULL };

static const gchar* clamd_tokens[] = {
	"LocalSocket",
	"TCPSocket",
	"TCPAddr",
	NULL };

static Clamd_Socket* Socket = NULL;
static int sock;
static gchar* folder = NULL;

void clamd_create_config(const gchar* path) {
	FILE* conf;
	char buf[1024];
	gchar* key = NULL;
	gchar* value = NULL;

	debug_print("%s : %s\n", folder, path);
	if (folder && strcmp(folder, path) == 0) {
		debug_print("%s : %s - Identical. No need to read again\n", folder, path);
		return;
	}
	g_free(folder);
	folder = g_strdup(path);
	debug_print("Opening %s to parse config file\n", path);
	conf = fopen(path, "r");
	if (!conf) {
		/*g_error("%s: Unable to open", path);*/
		alertpanel_error(_("%s: Unable to open\nclamd will be disabled"), path);
		return;
	}
	while (fgets(buf, sizeof(buf), conf)) {
		g_strstrip(buf);
		if (buf[0] == '#')
			continue;
		const gchar** tokens = clamd_tokens;
		while (*tokens) {
			const gchar* token = *tokens++;
			if ((key = g_strstr_len(buf, strlen(buf), token)) != NULL) {
				gchar* tmp = &(*(key + strlen(token)));
				tmp = g_strchug(tmp);
				gchar* end = index(tmp, '#');
				if (end)
					value = g_strndup(tmp, end - tmp);
				else
					value = g_strdup(g_strchomp(tmp));
				if (strcmp(clamd_tokens[0], token) == 0) {
					/* UNIX socket */
					Socket = (Clamd_Socket *) malloc(sizeof(Clamd_Socket *));
					if (Socket) {
						Socket->socket.path = NULL;
						Socket->socket.host = NULL;
						Socket->socket.port = -1;
						Socket->type = UNIX_SOCKET;
						Socket->socket.path = g_strdup(value);
						g_free(value);
						value = NULL;
						fclose(conf);
						debug_print("clamctl: %s\n", Socket->socket.path);
						return;
					}
				}
				else {
					/* INET socket */
					if (!Socket) {
						Socket = 
							(Clamd_Socket *) malloc(sizeof(Clamd_Socket *));
						Socket->socket.path = NULL;
						Socket->socket.host = NULL;
						Socket->socket.port = -1;
					}
					if (Socket) {
						Socket->type = INET_SOCKET;
						if (strcmp(clamd_tokens[1], token) == 0)
							Socket->socket.port = atoi(value);
						else
							Socket->socket.host = g_strdup(value);
						g_free(value);
						value = NULL;
					}
				}
			}
		}
	}
	if (! Socket && (Socket->socket.port || Socket->socket.path)) {
		/*g_error("%s: Not able to find required information", path);*/
		alertpanel_error(_("%s: Not able to find required information\nclamd will be disabled"), path);
	}
	if (Socket && Socket->type == INET_SOCKET && Socket->socket.host == NULL)
		Socket->socket.host = g_strdup("localhost");
}

gboolean clamd_find_socket() {
	const gchar** config_dir = config_dirs;
	gchar *clamd_conf = NULL;
	
	while (*config_dir) {
		clamd_conf = g_strdup_printf("%s/clamd.conf", *config_dir++);
		debug_print("Looking for %s\n", clamd_conf);
		if (g_file_test(clamd_conf, G_FILE_TEST_EXISTS))
			break;
		g_free(clamd_conf);
		clamd_conf = NULL;
	}
	if (! clamd_conf)
		return FALSE;
	debug_print("Using %s to find configuration\n", clamd_conf);
	folder = g_strdup(clamd_conf);
	clamd_create_config(clamd_conf);
	g_free(clamd_conf);
	return TRUE;
}

gchar* clamd_get_folder() {
	return folder;
}

Clamd_Socket* clamd_get_socket() {
	return Socket;
}

static void close_socket() {
	debug_print("Closing socket: %d\n", sock);
	close(sock);
}

static void create_socket() {
	struct sockaddr_un addr_u;
	struct sockaddr_in addr_i;
	struct hostent *hp;

	if (sock > 0) {
		close_socket();
		sock = 0;
	}
	memset(&addr_u, 0, sizeof(addr_u));
	memset(&addr_i, 0, sizeof(addr_i));
	debug_print("socket->type: %d\n", Socket->type);
	switch (Socket->type) {
		case UNIX_SOCKET:
			debug_print("socket path: %s\n", Socket->socket.path);
			sock = socket(PF_UNIX, SOCK_STREAM, 0);
			debug_print("socket file (create): %d\n", sock);
			if (sock < 0) 
				return;
			addr_u.sun_family = AF_UNIX;
			memcpy(addr_u.sun_path, Socket->socket.path, 
							sizeof(addr_u.sun_path) - 1);
			if (connect(sock, (struct sockaddr *) &addr_u, sizeof(addr_u)) < 0) {
				perror("connect socket");
				close_socket();
				sock = -2;
			}
			debug_print("socket file (connect): %d\n", sock);
			break;
		case INET_SOCKET:
			addr_i.sin_family = AF_INET;
			addr_i.sin_port = htons(Socket->socket.port);
			hp = gethostbyname(Socket->socket.host);
			bcopy((void *)hp->h_addr, (void *)&addr_i.sin_addr, hp->h_length);
			sock = socket(PF_INET, SOCK_STREAM, 0);
			if (sock < 0)
				return;
			if (connect(sock, (struct sockaddr *)&addr_i, sizeof(addr_i)) < 0) {
				perror("connect socket");
				close_socket();
				sock = -2;
			}
			break;
	}
}

static void copy_socket(Clamd_Socket* sock) {
	Socket = (Clamd_Socket *) malloc(sizeof(Clamd_Socket *));
	Socket->socket.path = NULL;
	Socket->socket.host = NULL;
	Socket->type = sock->type;
	if (Socket->type == UNIX_SOCKET) {
		Socket->socket.path = g_strdup(sock->socket.path);
	}
	else {
		Socket->socket.host = g_strdup(sock->socket.host);
		Socket->socket.port = sock->socket.port;
	}
}

Clamd_Stat clamd_init(Clamd_Socket* config) {
	const gchar ping[] = "PING\n";
	gchar buf[BUFSIZ];
	int n_read;
	gboolean connect = FALSE;

	if (config != NULL && Socket != NULL)
		return NO_SOCKET;
	if (config) {
		debug_print("socket: %s\n", config->socket.path);
		copy_socket(config);
	}
	create_socket();
	if (sock < 0) {
		debug_print("no connection\n");
		return NO_CONNECTION;
	}
	if (write(sock, ping, strlen(ping)) == -1) {
		debug_print("no connection\n");
		return NO_CONNECTION;
	}
	memset(buf, '\0', sizeof(buf));
	while ((n_read = read(sock, buf, BUFSIZ)) > 0) {
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		debug_print("Ping result: %s\n", buf);
		if (strcmp("PONG", buf) == 0)
			connect = TRUE;
		/*memset(buf, '\0', sizeof(buf));*/
	}
	close_socket();
	return (connect) ? OK : NO_CONNECTION;
}

Clamd_Stat clamd_verify_email(const gchar* path, response* result) {
	gchar buf[BUFSIZ];
	int n_read;
	gchar* command;
	Clamd_Stat stat;

	if (!result) {
		result = malloc(sizeof(response *));
		memset(result, '\0', sizeof(response *));
	}
	create_socket();
	if (sock < 0) {
		debug_print("no connection\n");
		return NO_CONNECTION;
	}
	command = g_strdup_printf("SCAN %s\n", path);
	debug_print("command: %s\n", command);
	if (write(sock, command, strlen(command)) == -1) {
		debug_print("no connection\n");
		stat = NO_CONNECTION;
	}
	g_free(command);
	memset(buf, '\0', sizeof(buf));
	while ((n_read = read(sock, buf, BUFSIZ)) > 0) {
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
	}
	debug_print("response: %s\n", buf);
	if (strstr(buf, "ERROR")) {
		stat = SCAN_ERROR;
		result->msg = g_strdup(buf);
	}		
	else if (strstr(buf, "FOUND")) {
		stat = VIRUS;
		result->msg = g_strdup(buf);
	}		
	else {
		stat = OK;
		result->msg = NULL;
	}
	close_socket();
	return stat;
}

GSList* clamd_verify_dir(const gchar* path) {
	gchar buf[BUFSIZ];
	int n_read;
	gchar* command;
	GSList *list = NULL;

	create_socket();
	if (sock < 0) {
		debug_print("No socket\n");
		return list;
	}
	command = g_strdup_printf("CONTSCAN %s\n", path);
	debug_print("command: %s\n", command);
	if (write(sock, command, strlen(command)) == -1) {
		debug_print("No socket\n");
		return list;
	}
	g_free(command);
	memset(buf, '\0', sizeof(buf));
	while ((n_read = read(sock, buf, BUFSIZ)) > 0) {
		gchar** tmp = g_strsplit(buf, "\n", 0);
		gchar** head = tmp;
		while (*tmp) {
			gchar* file = *tmp++;
			debug_print("%s\n", file);
			if (strstr(file, "ERROR")) {
				g_warning("%s", file);
				/* dont report files with errors */
			}
			else if (strstr(file, "FOUND")) {
				list = g_slist_append(list, g_strdup(file));
			}
		}
		g_strfreev(head);
	}
	close_socket();
	return list;
}

void clamd_free_gslist(GSList* list) {
	GSList* tmp = list;
	while(tmp) {
		g_free(tmp->data);
		tmp = g_slist_next(tmp);
	}
	g_slist_free(list);
}

gchar* clamd_get_virus_name(gchar* msg) {
	gchar *head, *tail, *name;

	tail = g_strrstr_len(msg, strlen(msg), "FOUND");
	if (! tail)
		return NULL;
	head = g_strstr_len(msg, strlen(msg), ":");
	++head;
	name = g_strndup(head, tail - head);
	g_strstrip(name);
	return name;
}

void clamd_free() {
/*
 * struct _Clamd_Socket {
 *	Type type;
 *	union {
 *		struct {
 *		    gchar*  path;
 *		};
 *		struct {
 *		    gchar*  host;
 *		    int	    port;
 *		};
 *	} socket;
 *  };
 */
	if (sock > 0) {
		close_socket();
		sock = 0;
	}
	if (Socket) {
		switch (Socket->type) {
		    case UNIX_SOCKET:
			if (Socket->socket.path) {
			    g_free(Socket->socket.path);
			    Socket->socket.path = NULL;
			}
			break;
		    case INET_SOCKET:
			if (Socket->socket.host) {
			    g_free(Socket->socket.host);
			    Socket->socket.host = NULL;
			}
			break;
		}
		g_free(Socket);
		Socket = NULL;
	}
	if (folder) {
	    g_free(folder);
	    folder = NULL;
	}
}

