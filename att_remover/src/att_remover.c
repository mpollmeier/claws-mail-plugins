/*
 * att_remover -- for Claws Mail
 *
 * Copyright (C) 2005 Colin Leroy <colin@colino.net>
 *
 * Sylpheed is a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto and the Claws Mail Team
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
#include "menu.h"
#include "claws.h"
#include "plugin.h"

#include "pluginconfig.h"

static void remove_attachments(gpointer callback_data, guint callback_action, GtkWidget *widget)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;
	GSList *msglist = summary_get_selected_msg_list(summaryview);
	GSList *cur;
	gint msgnum = -1;

	if (summary_is_locked(summaryview) || !msglist) 
		return;
	
	if (g_slist_length(msglist) == 1) {
		if (alertpanel_full(_("Destroy attachments"),
                        _("Do you really want to remove all attachments from "
                          "the selected message ?\n\n"
			  "The deleted data will be unrecoverable."), 
			  GTK_STOCK_CANCEL, GTK_STOCK_DELETE, NULL,
                          FALSE, NULL, ALERT_QUESTION, G_ALERTALTERNATE) != G_ALERTALTERNATE)
			return;
	} else {
		if (alertpanel_full(_("Destroy attachments"),
                        _("Do you really want to remove all attachments from "
                          "the selected messages ?\n\n"
			  "The deleted data will be unrecoverable."), 
			  GTK_STOCK_CANCEL, GTK_STOCK_DELETE, NULL,
                          FALSE, NULL, ALERT_QUESTION, G_ALERTALTERNATE) != G_ALERTALTERNATE)
			return;
	}

	main_window_cursor_wait(summaryview->mainwin);
	gtk_cmclist_freeze(GTK_CMCLIST(summaryview->ctree));
	folder_item_update_freeze();
	inc_lock();

	for (cur = msglist; cur; cur = cur->next) {
		MsgInfo *msginfo = (MsgInfo *)cur->data;
		MsgInfo *newmsg = NULL;
		MsgInfo *finalmsg = NULL;
		FolderItem *item = NULL;
		MimeInfo *info = NULL;
		MimeInfo *partinfo = NULL;

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
		info->node->children->data = partinfo;

		finalmsg = procmsg_msginfo_new_from_mimeinfo(newmsg, info);

		if (!finalmsg) {
			procmsg_msginfo_free(newmsg);
			continue;
		}

		debug_print("finalmsg %s\n", finalmsg->plaintext_file);
		
		flags.tmp_flags = msginfo->flags.tmp_flags;
		flags.perm_flags = msginfo->flags.perm_flags;
		flags.tmp_flags &= ~MSG_HAS_ATTACHMENT;

		folder_item_remove_msg(item, msginfo->msgnum);
		msgnum = folder_item_add_msg(item, finalmsg->plaintext_file, 
				&flags, TRUE);
		finalmsg->msgnum = msgnum;
		procmsg_msginfo_free(newmsg);
		procmsg_msginfo_free(finalmsg);
		
		newmsg = folder_item_get_msginfo(item, msgnum);
		if (newmsg && item) {
			procmsg_msginfo_unset_flags(newmsg, ~0, ~0);
			procmsg_msginfo_set_flags(newmsg, flags.perm_flags, flags.tmp_flags);
			procmsg_msginfo_free(newmsg);
		}
		
	}
	inc_unlock();
	folder_item_update_thaw();
	gtk_cmclist_thaw(GTK_CMCLIST(summaryview->ctree));
	main_window_cursor_normal(summaryview->mainwin);
	g_slist_free(msglist);
	if (msgnum > 0) {
		summary_select_by_msgnum(summaryview, msgnum);
	}
}

static void remove_attachments_ui(GtkAction *action, gpointer data)
{
	remove_attachments(NULL, 0, NULL);
}

static GtkActionEntry remove_att_main_menu[] = {{
	"Message/RemoveAtt",
	NULL, N_("Remove attachments"), NULL, NULL, G_CALLBACK(remove_attachments_ui)
}};

static GtkActionEntry remove_att_context_menu[] = {{
	"SummaryViewPopup/RemoveAtt",
	NULL, N_("Remove attachments"), NULL, NULL, G_CALLBACK(remove_attachments_ui)
}};

static guint context_menu_id = 0;
static guint main_menu_id = 0;

gint plugin_init(gchar **error)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;

	if( !check_plugin_version(MAKE_NUMERIC_VERSION(3,5,0,47),
				VERSION_NUMERIC, _("AttRemover"), error) )
		return -1;

	gtk_action_group_add_actions(mainwin->action_group, remove_att_main_menu,
			1, (gpointer)mainwin);
	MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menu/Message", "RemoveAtt", 
			  "Message/RemoveAtt", GTK_UI_MANAGER_MENUITEM,
			  main_menu_id)
	gtk_action_group_add_actions(summaryview->action_group, remove_att_context_menu,
			1, (gpointer)summaryview);
	MENUITEM_ADDUI_ID("/Menus/SummaryViewPopup", "RemoveAtt", 
			  "SummaryViewPopup/RemoveAtt", GTK_UI_MANAGER_MENUITEM,
			  context_menu_id)

	return 0;
}

gboolean plugin_done(void)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = NULL;

	if (mainwin == NULL)
		return TRUE;

	summaryview = mainwin->summaryview;

	MENUITEM_REMUI_MANAGER(mainwin->ui_manager,mainwin->action_group, "Message/RemoveAtt", main_menu_id);
	main_menu_id = 0;

	MENUITEM_REMUI(summaryview->action_group, "SummaryViewPopup/ReportSpam", context_menu_id);
	context_menu_id = 0;
	return TRUE;
}

const gchar *plugin_name(void)
{
	return _("AttRemover");
}

const gchar *plugin_desc(void)
{
	return _("This plugin removes attachments from mails.\n\n"
		 "Warning: this operation will be completely "
		 "un-cancellable and the deleted attachments will "
		 "be lost forever, and ever, and ever.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}

const gchar *plugin_licence(void)
{
		return "GPL3+";
}

const gchar *plugin_version(void)
{
	return PLUGINVERSION;
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = 
		{ {PLUGIN_UTILITY, N_("Attachment handling")},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}
