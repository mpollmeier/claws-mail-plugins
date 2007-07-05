/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 Colin Leroy <colin@colino.net>
 * and the Claws Mail Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>

#include <glib.h>
#include "gettext.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "common/claws.h"
#include "common/version.h"
#include "main.h"
#include "plugin.h"
#include "utils.h"
#include "prefs_common.h"
#include "spam_report_prefs.h"
#include "statusbar.h"
#include "procmsg.h"
#include "log.h"
#include "inc.h"
#include "plugin.h"
#include "menu.h"
#include "defs.h"

#ifdef USE_PTHREAD
#include <pthread.h>
#endif

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/curlver.h>
#endif

/* this interface struct is probably not enough for the various available 
 * reporting places/methods. It'll be extended as necessary. */

ReportInterface spam_interfaces[] = {
	{ "Signal-Spam.fr", INTF_HTTP_AUTH, "https://www.signal-spam.fr/api/signaler",
		"message=%claws_mail_body_b64%"},
	{ NULL, INTF_NULL, NULL, NULL}
};

/* From RSSyl. This should be factorized to the core... */
static gchar *spamreport_strreplace(gchar *source, gchar *pattern,
		gchar *replacement)
{
	gchar *new, *w_new, *c;
	guint count = 0, final_length;

	if( source == NULL || pattern == NULL ) {
		debug_print("source or pattern is NULL!!!\n");
		return NULL;
	}

	if( !g_utf8_validate(source, -1, NULL) ) {
		debug_print("source is not an UTF-8 encoded text\n");
		return NULL;
	}

	if( !g_utf8_validate(pattern, -1, NULL) ) {
		debug_print("pattern is not an UTF-8 encoded text\n");
		return NULL;
	}

	c = source;
	while( ( c = g_strstr_len(c, strlen(c), pattern) ) ) {
		count++;
		c += strlen(pattern);
	}

	final_length = strlen(source)
		- ( count * strlen(pattern) )
		+ ( count * strlen(replacement) );

	new = malloc(final_length + 1);
	w_new = new;
	memset(new, '\0', final_length + 1);

	c = source;

	while( *c != '\0' ) {
		if( !memcmp(c, pattern, strlen(pattern)) ) {
			gboolean break_after_rep = FALSE;
			int i;
			if (*(c + strlen(pattern)) == '\0')
				break_after_rep = TRUE;
			for (i = 0; i < strlen(replacement); i++) {
				*w_new = replacement[i];
				w_new++;
			}
			if (break_after_rep)
				break;
			c = c + strlen(pattern);
		} else {
			*w_new = *c;
			w_new++;
			c++;
		}
	}
	return new;
}


static void report_spam(gint id, ReportInterface *intf, MsgInfo *msginfo, gchar *contents)
{
	gchar *reqbody = NULL, *tmp = NULL, *auth = NULL, *b64 = NULL;
	CURL *curl;
	CURLcode res;
	long response;
	
	debug_print("reporting via %s\n", intf->name);
	tmp = spamreport_strreplace(intf->body, "%claws_mail_body%", contents);
	b64 = g_base64_encode(contents, strlen(contents));
	reqbody = spamreport_strreplace(tmp, "%claws_mail_body_b64%", b64);
	g_free(b64);
	g_free(tmp);
	
	if (spamreport_prefs.user[id] && *(spamreport_prefs.user[id])) {
		auth = g_strdup_printf("%s:%s", spamreport_prefs.user[id], spamreport_prefs.pass[id]);

		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, intf->url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reqbody);
		curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
		curl_easy_setopt(curl, CURLOPT_USERAGENT,
                	"Claws Mail SpamReport plugin "
	                "(" PLUGINS_URI ")");
		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
		curl_easy_cleanup(curl);
		switch (response) {
		case 400: /* Bad Request */
			log_error(LOG_PROTOCOL, "%s: Bad Request\n", intf->url);
			break;
		case 401: /* Not Authorized */
			log_error(LOG_PROTOCOL, "%s: Wrong login or password\n", intf->url);
			break;
		case 404: /* Not Authorized */
			log_error(LOG_PROTOCOL, "%s: Not found\n", intf->url);
			break;
		}


		g_free(auth);
	}
	g_free(reqbody);
}

static void report_spam_cb(gpointer callback_data, guint callback_action, GtkWidget *widget)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;
	GSList *msglist = summary_get_selected_msg_list(summaryview);
	GSList *cur;
	gint curnum=0, total=0;
	if (summary_is_locked(summaryview) || !msglist) {
		if (msglist)
			g_slist_free(msglist);
		return;
	}
	main_window_cursor_wait(summaryview->mainwin);
	gtk_clist_freeze(GTK_CLIST(summaryview->ctree));
	folder_item_update_freeze();
	inc_lock();

	STATUSBAR_PUSH(mainwin, _("Reporting spam..."));
	total = g_slist_length(msglist);

	for (cur = msglist; cur; cur = cur->next) {
		MsgInfo *msginfo = (MsgInfo *)cur->data;
		gchar *file = procmsg_get_message_file(msginfo);
		gchar *contents = NULL;
		int i = 0;
		if (!file)
			continue;
		debug_print("reporting message %d (%s)\n", msginfo->msgnum, file);
		statusbar_progress_all(curnum, total, 1);
		GTK_EVENTS_FLUSH();
		curnum++;

		contents = file_read_to_str(file);
		
		for (i = 0; i < INTF_LAST; i++)
			report_spam(i, &(spam_interfaces[i]), msginfo, contents);
		
		g_free(contents);
		g_free(file);
	}

	statusbar_progress_all(0,0,0);
	STATUSBAR_POP(mainwin);
	inc_unlock();
	folder_item_update_thaw();
	gtk_clist_thaw(GTK_CLIST(summaryview->ctree));
	main_window_cursor_normal(summaryview->mainwin);
	g_slist_free(msglist);
}

static GtkItemFactoryEntry spamreport_menu = {
	N_("/Message/Report spam online"),
	NULL,
	report_spam_cb,
	0,
	NULL
};

static GtkItemFactoryEntry spamreport_context_menu = {
	N_("/Report spam online"),
	NULL,
	report_spam_cb,
	0,
	NULL
};

gint plugin_init(gchar **error)
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;

	bindtextdomain(TEXTDOMAIN, LOCALEDIR);
	bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");

	if (!check_plugin_version(MAKE_NUMERIC_VERSION(2,9,2,72),
				VERSION_NUMERIC, _("SpamReport"), error))
		return -1;

	spamreport_prefs_init();
#ifdef HAVE_LIBCURL
	curl_global_init(CURL_GLOBAL_DEFAULT);
#endif

	spamreport_menu.path = _(spamreport_menu.path);
	spamreport_context_menu.path = _(spamreport_context_menu.path);
	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	gtk_item_factory_create_item(ifactory, &spamreport_menu, mainwin, 1);
	gtk_item_factory_create_item(summaryview->popupfactory, &spamreport_context_menu, summaryview, 1);

	return 0;
}

gboolean plugin_done(void)
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = NULL;
	GtkWidget *widget;

	if (mainwin == NULL)
		return TRUE;

	summaryview = mainwin->summaryview;
	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	widget = gtk_item_factory_get_widget(ifactory, spamreport_menu.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(ifactory, spamreport_menu.path);

	widget = gtk_item_factory_get_widget(summaryview->popupfactory, spamreport_context_menu.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(summaryview->popupfactory, spamreport_context_menu.path);
#ifdef HAVE_LIBCURL
	spamreport_prefs_done();
#endif
	return TRUE;
}

const gchar *plugin_name(void)
{
	return _("SpamReport");
}

const gchar *plugin_desc(void)
{
	return _("This plugin reports spam to various places. Currently "
		 "only spam-signal.fr is supported.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}

const gchar *plugin_licence(void)
{
	return "GPL";
}

const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = 
		{ {PLUGIN_UTILITY, N_("Spam reporting")},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}
