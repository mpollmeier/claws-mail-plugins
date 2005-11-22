/*
 * acpi_notifier -- for Sylpheed-Claws
 *
 * Copyright (C) 2005 Colin Leroy <colin@colino.net>
 *
 * Sylpheed is a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "mainwindow.h"
#include "summaryview.h"
#include "folder.h"
#include "version.h"
#include "summaryview.h"
#include "procmime.h"
#include "alertpanel.h"
#include "inc.h"
#include "sylpheed.h"

static void remove_attachments(gpointer callback_data, guint callback_action, GtkWidget *widget)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;
	GSList *msglist = summary_get_selected_msg_list(summaryview);
	GSList *cur;
	gboolean reselected = FALSE;

	if (summary_is_locked(summaryview) || !msglist) 
		return;
	
	if (g_slist_length(msglist) == 1) {
		if (alertpanel_full(_("Remove attachments"),
                        _("Do you really want to remove all attachments from "
                          "the selected message ?"), GTK_STOCK_YES, GTK_STOCK_NO, NULL,
                          FALSE, NULL, ALERT_QUESTION, G_ALERTDEFAULT) != G_ALERTDEFAULT)
			return;
	} else {
		if (alertpanel_full(_("Remove attachments"),
                        _("Do you really want to remove all attachments from "
                          "the selected messages ?"), GTK_STOCK_YES, GTK_STOCK_NO, NULL,
                          FALSE, NULL, ALERT_QUESTION, G_ALERTDEFAULT) != G_ALERTDEFAULT)
			return;
	}

	main_window_cursor_wait(summaryview->mainwin);
	gtk_clist_freeze(GTK_CLIST(summaryview->ctree));
	folder_item_update_freeze();
	inc_lock();

	for (cur = msglist; cur; cur = cur->next) {
		MsgInfo *msginfo = (MsgInfo *)cur->data;
		MsgInfo *newmsg = NULL;
		MsgInfo *finalmsg = NULL;
		FolderItem *item = NULL;
		MimeInfo *info = NULL;
		MimeInfo *partinfo = NULL;
		gint msgnum = -1;

		MsgFlags flags = {0, 0};

		if (!msginfo)
			continue;
		
		item = msginfo->folder;
				
		newmsg = procmsg_msginfo_copy(msginfo);
		info = procmime_scan_message(newmsg);
	
		partinfo = info;
		while (partinfo && partinfo->type != MIMETYPE_TEXT) {
			partinfo = procmime_mimeinfo_next(partinfo);
		}
		
		if (!partinfo) {
			procmsg_msginfo_free(newmsg);
			continue;
		}
		partinfo->node->next = NULL;
		partinfo->node->children = NULL;
		
		finalmsg = procmsg_msginfo_new_from_mimeinfo(newmsg, info);

		if (!finalmsg) {
			procmsg_msginfo_free(newmsg);
			continue;
		}

		debug_print("finalmsg %s\n", finalmsg->plaintext_file);
		
		flags.tmp_flags = msginfo->flags.tmp_flags;
		flags.perm_flags = msginfo->flags.perm_flags;
		
		folder_item_remove_msg(item, msginfo->msgnum);
		msgnum = folder_item_add_msg(item, finalmsg->plaintext_file, 
				&flags, TRUE);
		procmsg_msginfo_free(newmsg);
		procmsg_msginfo_free(finalmsg);
		
		if (msgnum > 0 && !reselected) {
			reselected = TRUE;
			summary_select_by_msgnum(summaryview, msgnum);
		}
	}
	inc_unlock();
	folder_item_update_thaw();
	gtk_clist_thaw(GTK_CLIST(summaryview->ctree));
	main_window_cursor_normal(summaryview->mainwin);
	g_slist_free(msglist);
}

static GtkItemFactoryEntry remove_att_menu = {
	N_("/Message/Remove attachments"),
	NULL,
	remove_attachments,
	0,
	NULL
};

static GtkItemFactoryEntry remove_att_context_menu = {
	N_("/Remove attachments"),
	NULL,
	remove_attachments,
	0,
	NULL
};

gint plugin_init(gchar **error)
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;

	if((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your Sylpheed-Claws version is newer than the version "
				"AttRemover was built with");
		return -1;
	}
	if((sylpheed_get_version() < MAKE_NUMERIC_VERSION(1, 9, 13, 0))) {
		*error = g_strdup("Your Sylpheed-Claws version is too old for AttRemover");
		return -1;
	}
	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	gtk_item_factory_create_item(ifactory, &remove_att_menu, mainwin, 1);
	gtk_item_factory_create_item(summaryview->popupfactory, &remove_att_context_menu, summaryview, 1);

	return 0;
}

void plugin_done(void)
{
	GtkItemFactory *ifactory;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;
	GtkWidget *widget;

	ifactory = gtk_item_factory_from_widget(mainwin->menubar);
	widget = gtk_item_factory_get_widget(ifactory, remove_att_menu.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(ifactory, remove_att_menu.path);

	widget = gtk_item_factory_get_widget(summaryview->popupfactory, remove_att_context_menu.path);
	gtk_widget_destroy(widget);
	gtk_item_factory_delete_item(summaryview->popupfactory, remove_att_context_menu.path);
}

const gchar *plugin_name(void)
{
	return _("AttRemover");
}

const gchar *plugin_desc(void)
{
	return _("This plugin removes attachments from mails.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}
