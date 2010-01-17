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
#include <gdk/gdkkeysyms.h>
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
#include "prefs_common.h"
#include "defs.h"
#include "prefs_gtk.h"

#include "pluginconfig.h"

#define PREFS_BLOCK_NAME "AttRemover"

static struct _AttRemover {
	GtkWidget	*window;
	MsgInfo		*msginfo;
	GtkTreeModel	*model;
	gint		win_width;
	gint		win_height;
} AttRemoverData;

typedef struct _AttRemover AttRemover;

static PrefParam prefs[] = {
        {"win_width", "-1", &AttRemoverData.win_width, P_INT, NULL,
        NULL, NULL},
        {"win_height", "-1", &AttRemoverData.win_height, P_INT, NULL, 
         NULL, NULL},
        {0,0,0,0}
};

enum {
	ATT_REMOVER_INFO,
	ATT_REMOVER_TOGGLE,
	N_ATT_REMOVER_COLUMNS
};

static MimeInfo *find_first_text_part(MimeInfo *partinfo)
{
	while (partinfo && partinfo->type != MIMETYPE_TEXT) {
		partinfo = procmime_mimeinfo_next(partinfo);
	}
	
	return partinfo;
}

static gboolean key_pressed_cb(GtkWidget *widget, GdkEventKey *event,
				AttRemover *attremover)
{
	if (event && event->keyval == GDK_Escape)
		gtk_widget_destroy(attremover->window);

	return FALSE;
}

static void cancelled_event_cb(GtkWidget *widget, AttRemover *attremover)
{
	gtk_widget_destroy(attremover->window);
}

static void size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation)
{
	cm_return_if_fail(allocation != NULL);

	AttRemoverData.win_width = allocation->width;
	AttRemoverData.win_height = allocation->height;
}

static gint save_new_message(MsgInfo *oldmsg, MsgInfo *newmsg, MimeInfo *info,
				gboolean has_attachment)
{
	MsgInfo *finalmsg;
	FolderItem *item = oldmsg->folder;
	MsgFlags flags = {0, 0};
	gint msgnum = -1;
	
	finalmsg = procmsg_msginfo_new_from_mimeinfo(newmsg, info);
	if (!finalmsg) {
		procmsg_msginfo_free(newmsg);
		return -1;
	}

	debug_print("finalmsg %s\n", finalmsg->plaintext_file);
		
	flags.tmp_flags = oldmsg->flags.tmp_flags;
	flags.perm_flags = oldmsg->flags.perm_flags;
	
	if (!has_attachment)
		flags.tmp_flags &= ~MSG_HAS_ATTACHMENT;

	folder_item_remove_msg(item, oldmsg->msgnum);
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
	
	return msgnum;
}

static void remove_attachments_cb(GtkWidget *widget, AttRemover *attremover)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	GtkTreeModel *model = attremover->model;
	GtkTreeIter iter;
	MsgInfo *newmsg;
	MimeInfo *info, *parent, *gparent, *partinfo;
	GNode *child;
	gint att_all = 0, att_removed = 0, msgnum;
	gboolean to_removal, iter_valid=TRUE;
	
	newmsg = procmsg_msginfo_copy(attremover->msginfo);
	info = procmime_scan_message(newmsg);
	
	partinfo = find_first_text_part(info);
	partinfo = procmime_mimeinfo_next(partinfo);
	if (!partinfo || !gtk_tree_model_get_iter_first(model, &iter)) {
		gtk_widget_destroy(attremover->window);
		procmsg_msginfo_free(newmsg);
		return;
	}
	
	while (partinfo && iter_valid) {
		if (partinfo->type == MIMETYPE_MULTIPART) {
    			partinfo = procmime_mimeinfo_next(partinfo);
			continue;
		}
			
		att_all++;
		gtk_tree_model_get(model, &iter, ATT_REMOVER_TOGGLE,
				   &to_removal, -1);
		if (!to_removal) {
			partinfo = procmime_mimeinfo_next(partinfo);
			iter_valid = gtk_tree_model_iter_next(model, &iter);
			continue;
		}
		
		parent = (MimeInfo *) partinfo->node->parent->data;
		gparent = (MimeInfo *) parent->node->parent->data;

		/* multipart/{alternative,mixed,related} make sense
		   only when they have at least 2 nodes, remove them
		   and move their nodes one level up if otherwise  */
		if ((!strcmp(parent->subtype, "alternative") &&
			g_node_n_children(parent->node) <= 2) ||
			(!strcmp(parent->subtype, "mixed") &&
			g_node_n_children(parent->node) <= 2) ||
			(!strcmp(parent->subtype, "related") &&
			g_node_n_children(parent->node) <= 2) ||
			!strcmp(parent->subtype, "signed"))
		{
			while((child = g_node_last_child(parent->node)) != NULL) {
				g_node_unlink(child);
				g_node_prepend(gparent->node, child);
			}

			g_node_unlink(parent->node);
			g_node_destroy(parent->node);
		}
		
		parent = partinfo;
		partinfo = procmime_mimeinfo_next(partinfo);
		iter_valid = gtk_tree_model_iter_next(model, &iter);

		g_node_unlink(parent->node);
		g_node_destroy(parent->node);
		att_removed++;
	}

	partinfo = find_first_text_part(info);
	if (partinfo->node->prev) {
		parent = (MimeInfo *) partinfo->node->parent->data;
		g_node_unlink(partinfo->node);
		g_node_prepend(parent->node, partinfo->node);
	}

	msgnum = save_new_message(attremover->msginfo, newmsg, info,
			 (att_all - att_removed > 0));
	if (msgnum > 0)
		summary_select_by_msgnum(mainwin->summaryview, msgnum);

	gtk_widget_destroy(attremover->window);
	procmsg_msginfo_free(newmsg);
}

static void remove_toggled_cb(GtkCellRendererToggle *cell, gchar *path_str,
				gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean fixed;
	       	               
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, ATT_REMOVER_TOGGLE, &fixed, -1);
	       	                     
	fixed ^= 1;
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, ATT_REMOVER_TOGGLE, fixed, -1);
	       	                             
	gtk_tree_path_free (path);
}

static void fill_attachment_store(GtkTreeView *list_view, MimeInfo *partinfo)
{
	const gchar *name;
	gchar *label, *content_type;
	GtkTreeIter iter;
	GtkListStore *list_store = GTK_LIST_STORE(gtk_tree_view_get_model
					(GTK_TREE_VIEW(list_view)));

	partinfo = find_first_text_part(partinfo);
	partinfo = procmime_mimeinfo_next(partinfo);
	if (!partinfo)
		return;
		
	while (partinfo) {
		if (partinfo->type == MIMETYPE_MULTIPART) {
    			partinfo = procmime_mimeinfo_next(partinfo);
			continue;
		}
			
		content_type = procmime_get_content_type_str(
					partinfo->type, partinfo->subtype);
		
		name = procmime_mimeinfo_get_parameter(partinfo, "filename");
		if (!name)
			name = procmime_mimeinfo_get_parameter(partinfo, "name");
		if (!name)
			name = _("unknown");
		
		label = g_strconcat(_("<b>Type: </b>"), content_type, "   ",
			_("<b>Size: </b>"), to_human_readable((goffset)partinfo->length),
			"\n", _("<b>Filename: </b>"), name, NULL);

		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter,
				   ATT_REMOVER_INFO, label,
				   ATT_REMOVER_TOGGLE, TRUE,
				   -1);
		g_free(label);
		g_free(content_type);
		partinfo = procmime_mimeinfo_next(partinfo);
	}
}

static void remove_attachments_dialog(AttRemover *attremover)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkTreeView *list_view;
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *scrollwin;
	GtkWidget *hbbox;
	GtkWidget *ok_btn;
	GtkWidget *cancel_btn;
	MimeInfo *info = procmime_scan_message(attremover->msginfo);

	static GdkGeometry geometry;

	window = gtkut_window_new(GTK_WINDOW_TOPLEVEL, "AttRemover");
	gtk_container_set_border_width( GTK_CONTAINER(window), VBOX_BORDER);
	gtk_window_set_title(GTK_WINDOW(window), _("Remove attachments"));
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);

	g_signal_connect(G_OBJECT(window), "delete_event",
			  G_CALLBACK(cancelled_event_cb), attremover);
	g_signal_connect(G_OBJECT(window), "key_press_event",
			  G_CALLBACK(key_pressed_cb), attremover);
	g_signal_connect(G_OBJECT(window), "size_allocate",
			 G_CALLBACK(size_allocate_cb), NULL);

	vbox = gtk_vbox_new(FALSE, VBOX_BORDER);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	model = GTK_TREE_MODEL(gtk_list_store_new(N_ATT_REMOVER_COLUMNS,
				  G_TYPE_STRING,
				  G_TYPE_BOOLEAN,
				  -1));
	list_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));
	g_object_unref(model);	
	gtk_tree_view_set_rules_hint(list_view, prefs_common.use_stripes_everywhere);
	
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(remove_toggled_cb), model);
	column = gtk_tree_view_column_new_with_attributes
		(_("Remove"),
		renderer,
		"active", ATT_REMOVER_TOGGLE,
		NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes
		(_("Attachment"),
		 renderer,
		 "markup", ATT_REMOVER_INFO,
		 NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list_view), column);
	fill_attachment_store(list_view, info);

	scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin),
				GTK_SHADOW_ETCHED_OUT);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollwin), GTK_WIDGET(list_view));
	gtk_container_set_border_width(GTK_CONTAINER(scrollwin), 4);
	gtk_box_pack_start(GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0); 

	gtkut_stock_button_set_create(&hbbox, &cancel_btn, GTK_STOCK_CANCEL,
				      &ok_btn, GTK_STOCK_OK,
				      NULL, NULL);
	gtk_box_pack_end(GTK_BOX(vbox), hbbox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbbox), HSPACING_NARROW);
	gtk_widget_grab_default(ok_btn);

	g_signal_connect(G_OBJECT(ok_btn), "clicked",
			 G_CALLBACK(remove_attachments_cb), attremover);
	g_signal_connect(G_OBJECT(cancel_btn), "clicked",
			 G_CALLBACK(cancelled_event_cb), attremover);
			 
	if (!geometry.min_height) {
		geometry.min_width = 450;
		geometry.min_height = 350;
	}

	gtk_window_set_geometry_hints(GTK_WINDOW(window), NULL, &geometry,
				      GDK_HINT_MIN_SIZE);
	gtk_widget_set_size_request(window, attremover->win_width,
					attremover->win_height);

	attremover->window = window;
	attremover->model  = model;

	gtk_widget_show_all(window);
	gtk_widget_grab_focus(ok_btn);
}

static void remove_attachments(GSList *msglist)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = mainwin->summaryview;
	GSList *cur;
	gint msgnum = -1;
	
	if (alertpanel_full(_("Destroy attachments"),
                  _("Do you really want to remove all attachments from "
                  "the selected messages ?\n\n"
		  "The deleted data will be unrecoverable."), 
		  GTK_STOCK_CANCEL, GTK_STOCK_DELETE, NULL,
                  FALSE, NULL, ALERT_QUESTION, G_ALERTALTERNATE) != G_ALERTALTERNATE)
		return;

	main_window_cursor_wait(summaryview->mainwin);
	gtk_cmclist_freeze(GTK_CMCLIST(summaryview->ctree));
	folder_item_update_freeze();
	inc_lock();

	for (cur = msglist; cur; cur = cur->next) {
		MsgInfo *msginfo = (MsgInfo *)cur->data;
		MsgInfo *newmsg = NULL;
		MimeInfo *info = NULL;
		MimeInfo *partinfo = NULL;

		if (!msginfo)
			continue;
		
		newmsg = procmsg_msginfo_copy(msginfo);
		info = procmime_scan_message(newmsg);
	
		if ( !(partinfo = find_first_text_part(info)) ) {
			procmsg_msginfo_free(newmsg);
			continue;
		}
		partinfo->node->next = NULL;
		partinfo->node->children = NULL;
		info->node->children->data = partinfo;

		msgnum = save_new_message(msginfo, newmsg, info, FALSE);		
	}

	inc_unlock();
	folder_item_update_thaw();
	gtk_cmclist_thaw(GTK_CMCLIST(summaryview->ctree));
	main_window_cursor_normal(summaryview->mainwin);

	if (msgnum > 0) {
		summary_select_by_msgnum(summaryview, msgnum);
	}
}

static void remove_attachments_ui(GtkAction *action, gpointer data)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	GSList *msglist = summary_get_selected_msg_list(mainwin->summaryview);
	MimeInfo *info = NULL, *partinfo = NULL;
	
	if (summary_is_locked(mainwin->summaryview) || !msglist) 
		return;

	if (g_slist_length(msglist) == 1) {
		info = procmime_scan_message(msglist->data);
	
		partinfo = find_first_text_part(info);
		partinfo = procmime_mimeinfo_next(partinfo);
		
		if (!partinfo) {
			alertpanel_notice(_("This message doesn't have any attachments."));
			g_slist_free(msglist);
			return;
		}
		
		AttRemoverData.msginfo = msglist->data;
		remove_attachments_dialog(&AttRemoverData);
	} else
		remove_attachments(msglist);

	g_slist_free(msglist);
}

static GtkActionEntry remove_att_main_menu[] = {{
	"Message/RemoveAtt",
	NULL, N_("Remove attachments..."), NULL, NULL, G_CALLBACK(remove_attachments_ui)
}};

static guint context_menu_id = 0;
static guint main_menu_id = 0;

gint plugin_init(gchar **error)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	static gchar *path[3];
	gchar *rcpath;
	
	if( !check_plugin_version(MAKE_NUMERIC_VERSION(3,6,1,27),
				VERSION_NUMERIC, _("AttRemover"), error) )
		return -1;

	gtk_action_group_add_actions(mainwin->action_group, remove_att_main_menu,
			1, (gpointer)mainwin);
	MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menu/Message", "RemoveAtt", 
			  "Message/RemoveAtt", GTK_UI_MANAGER_MENUITEM,
			  main_menu_id)
	MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menus/SummaryViewPopup", "RemoveAtt", 
			  "Message/RemoveAtt", GTK_UI_MANAGER_MENUITEM,
			  context_menu_id)

	path[0] = _("Plugins");
	path[1] = "AttRemover";
	path[2] = NULL;

	prefs_set_default(prefs);
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	prefs_read_config(prefs, PREFS_BLOCK_NAME, rcpath, NULL);
	g_free(rcpath);

	return 0;
}

gboolean plugin_done(void)
{
	MainWindow *mainwin = mainwindow_get_mainwindow();
	SummaryView *summaryview = NULL;
	PrefFile *pref_file;
	gchar *rc_file_path;

	rc_file_path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
				   COMMON_RC, NULL);
	pref_file = prefs_write_open(rc_file_path);
	g_free(rc_file_path);
        
	if (!pref_file || prefs_set_block_label(pref_file, PREFS_BLOCK_NAME) < 0)
        	return TRUE;
        
	if (prefs_write_param(prefs, pref_file->fp) < 0) {
		g_warning("failed to write AttRemover Plugin configuration\n");
		prefs_file_close_revert(pref_file);
		return TRUE;
        }

        if (fprintf(pref_file->fp, "\n") < 0) {
        	FILE_OP_ERROR(rc_file_path, "fprintf");
        	prefs_file_close_revert(pref_file);
	} else
		prefs_file_close(pref_file);

	if (mainwin == NULL)
		return TRUE;

	summaryview = mainwin->summaryview;

	MENUITEM_REMUI_MANAGER(mainwin->ui_manager,mainwin->action_group, "Message/RemoveAtt", main_menu_id);
	main_menu_id = 0;

	MENUITEM_REMUI_MANAGER(mainwin->ui_manager,mainwin->action_group, "Message/RemoveAtt", context_menu_id);
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
