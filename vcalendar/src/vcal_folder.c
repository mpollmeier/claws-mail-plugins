/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto & the Sylpheed-Claws team
 * This file (C) 2004-2005 Colin Leroy <colin@colino.net>
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

#include "defs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "utils.h"
#include "procmsg.h"
#include "procheader.h"
#include "folder.h"
#include "folderview.h"
#include "vcalendar.h"
#include "vcal_folder.h"
#include "vcal_prefs.h"
#include "vcal_manager.h"
#include "vcal_meeting_gtk.h"
#include "prefs_account.h"

#include <gtk/gtk.h>
#include <dirent.h>

#define VCAL_FOLDERITEM(item) ((VCalFolderItem *) item)

typedef struct _VCalFolder VCalFolder;
typedef struct _VCalFolderItem VCalFolderItem;

static Folder *vcal_folder_new(const gchar * name,
				  const gchar * folder);
static void vcal_folder_destroy(Folder * folder);
static void vcal_item_destroy(Folder *folder, FolderItem *_item);
static gchar *vcal_item_get_path(Folder *folder, FolderItem *item);

static gint vcal_scan_tree(Folder * folder);
static FolderItem *vcal_item_new(Folder * folder);
static gint vcal_get_num_list(Folder * folder, FolderItem * item,
				 MsgNumberList ** list,
				 gboolean * old_uids_valid);
static MsgInfo *vcal_get_msginfo(Folder * folder, FolderItem * item,
				    gint num);
static gchar *vcal_fetch_msg(Folder * folder, FolderItem * item,
				gint num);
static gint vcal_add_msg(Folder * folder, FolderItem * _dest,
			    const gchar * file, MsgFlags * flags);
static gint vcal_remove_msg(Folder * folder, FolderItem * _item,
			       gint num);
static FolderItem *vcal_create_folder(Folder * folder,
					 FolderItem * parent,
					 const gchar * name);
static gint vcal_create_tree(Folder *folder);
static gint vcal_remove_folder(Folder *folder, FolderItem *item);
static gboolean vcal_scan_required(Folder *folder, FolderItem *item);
static void vcal_change_flags(Folder *folder, FolderItem *_item, MsgInfo *msginfo, MsgPermFlags newflags);
static void new_meeting_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void export_cal_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void set_sensitivity(GtkItemFactory *factory, FolderItem *item);

FolderClass vcal_class;

static GSList *created_files = NULL;
static GHashTable *hash_uids = NULL;

struct _VCalFolder
{
	Folder folder;
};

struct _VCalFolderItem
{
	FolderItem item;

};

static GtkItemFactoryEntry vcal_popup_entries[] =
{
	{N_("/_New meeting..."), NULL, new_meeting_cb,	0, NULL},
	{N_("/_Export calendar..."), NULL, export_cal_cb,	0, NULL},
	{"/---",	 	 NULL, NULL,    	0, "<Separator>"},
};

static FolderViewPopup vcal_popup =
{
	"vCalendar",
	"<vCalendar>",
	NULL,
	set_sensitivity
};


FolderClass *vcal_folder_get_class()
{
	debug_print("register class\n");
	if (vcal_class.idstr == NULL) {
		vcal_class.type = F_UNKNOWN;
		vcal_class.idstr = "vCalendar";
		vcal_class.uistr = "vCalendar";

		/* Folder functions */
		vcal_class.new_folder = vcal_folder_new;
		vcal_class.destroy_folder = vcal_folder_destroy;
		vcal_class.set_xml = folder_set_xml;
		vcal_class.get_xml = folder_get_xml;
		vcal_class.scan_tree = vcal_scan_tree;
		vcal_class.create_tree = vcal_create_tree;

		/* FolderItem functions */
		vcal_class.item_new = vcal_item_new;
		vcal_class.item_destroy = vcal_item_destroy;
		vcal_class.item_get_path = vcal_item_get_path;
		vcal_class.create_folder = vcal_create_folder;
		vcal_class.remove_folder = vcal_remove_folder;
		vcal_class.rename_folder = NULL;
		vcal_class.scan_required = vcal_scan_required;
		vcal_class.get_num_list = vcal_get_num_list;

		/* Message functions */
		vcal_class.get_msginfo = vcal_get_msginfo;
		vcal_class.fetch_msg = vcal_fetch_msg;
		vcal_class.add_msg = vcal_add_msg;
		vcal_class.copy_msg = NULL;
		vcal_class.remove_msg = vcal_remove_msg;
		vcal_class.change_flags = vcal_change_flags;
		debug_print("registered class for real\n");
	}

	return &vcal_class;
}

static Folder *vcal_folder_new(const gchar * name,
				  const gchar * path)
{
	VCalFolder *folder;		   
	
	debug_print("vcal_folder_new\n");
	folder = g_new0(VCalFolder, 1);
	FOLDER(folder)->klass = &vcal_class;
	folder_init(FOLDER(folder), name);

	return FOLDER(folder);
}

static void vcal_folder_destroy(Folder *_folder)
{
}

static FolderItem *vcal_item_new(Folder *folder)
{
	VCalFolderItem *item;
	item = g_new0(VCalFolderItem, 1);
	return (FolderItem *) item;

}

static void vcal_item_destroy(Folder *folder, FolderItem *_item)
{
	VCalFolderItem *item = (VCalFolderItem *)_item;
	g_return_if_fail(item != NULL);
	g_free(item);
}

static gchar *vcal_item_get_path(Folder *folder, FolderItem *item)
{
	return g_strdup(vcal_manager_get_event_path());
}

static gint vcal_scan_tree(Folder *folder)
{
	g_return_val_if_fail(folder != NULL, -1);

	folder->outbox = NULL;
	folder->draft = NULL;
	folder->queue = NULL;
	folder->trash = NULL;

	debug_print("scanning tree\n");
	vcal_create_tree(folder);

	return 0;
}

static gint vcal_get_num_list(Folder *folder, FolderItem *item,
				 MsgNumberList ** list, gboolean *old_uids_valid)
{
	DIR *dp;
	struct dirent *d;
	int n_msg = 1;
	gchar *snmsg = NULL;
	dp = opendir(vcal_manager_get_event_path());
	
	debug_print("get_num_list\n");
	if (!dp) {
		FILE_OP_ERROR(vcal_manager_get_event_path(), "opendir");
		return 0;
	}

	if (hash_uids != NULL)
		g_hash_table_destroy(hash_uids);
		
	hash_uids = g_hash_table_new_full(g_str_hash, g_str_equal,
					  g_free, g_free);
					  
	while ((d = readdir(dp)) != NULL) {
		VCalEvent *event = NULL;
		if (d->d_name[0] == '.' || strstr(d->d_name, ".bak")) 
			continue;

		snmsg = g_strdup_printf("%d",n_msg);
		g_hash_table_insert(hash_uids, snmsg, g_strdup(d->d_name));
		
		event = vcal_manager_load_event(d->d_name);

		if (event && event->method != ICAL_METHOD_CANCEL) {
			PrefsAccount *account = vcal_manager_get_account_from_event(event);
			enum icalparameter_partstat status = ICAL_PARTSTAT_NEEDSACTION;
			status = account ? vcal_manager_get_reply_for_attendee(event, account->address): ICAL_PARTSTAT_NEEDSACTION;

			if (status != ICAL_PARTSTAT_DECLINED) {
				*list = g_slist_append(*list, GINT_TO_POINTER(n_msg));
				debug_print("add %d:%s\n", n_msg, d->d_name);
				n_msg++;
			}
		}
		if (event)
			vcal_manager_free_event(event);

	}

	closedir(dp);
		
	return g_slist_length(*list);
}

static MsgInfo *vcal_parse_msg(const gchar *file, FolderItem *item, int num)
{
	MsgInfo *msginfo = NULL;
	MsgFlags flags;

	debug_print("parse_msg\n");
	
	flags.perm_flags = 0;
	flags.tmp_flags = 0;
	msginfo = procheader_parse_file(file, flags, TRUE, TRUE);
	
	msginfo->msgnum = num;
	msginfo->folder = item;
	return msginfo;
}

static MsgInfo *vcal_get_msginfo(Folder * folder,
				    FolderItem * item, gint num)
{
	MsgInfo *msginfo = NULL;
	gchar *file;
	gchar *snum = NULL;

	debug_print("get_msginfo\n");
	
	g_return_val_if_fail(item != NULL, NULL);
	g_return_val_if_fail(num > 0, NULL);

	snum = g_strdup_printf("%d", num);

	file = vcal_fetch_msg(folder, item, num);

	if (!file) {
		g_warning("can't fetch message\n");
		g_free(snum);
		return NULL;
	}

	msginfo = vcal_parse_msg(file, item, num);

	if (msginfo) {
		msginfo->flags.perm_flags = 0;
		msginfo->flags.tmp_flags = 0;

		vcal_change_flags(NULL, NULL, msginfo, 0);

		debug_print("  adding %s\n", snum);
	}
	unlink(file);
	g_free(file);

	debug_print("  got msginfo %p\n", msginfo);

	return msginfo;
}

static gchar *vcal_fetch_msg(Folder * folder, FolderItem * item,
				gint num)
{
	gchar *filename = NULL;
	gchar *snum = g_strdup_printf("%d",num);
	gchar *uid = NULL;
	uid = g_hash_table_lookup(hash_uids, snum);

	if (uid) {
		VCalEvent *event = vcal_manager_load_event(uid);
		debug_print("getting %s\n", uid);
		debug_print("got event %p\n", event);
		if (event)
			filename = vcal_manager_event_dump(event, FALSE, TRUE, NULL);
		debug_print("dumped to %s\n", filename);
		if (filename) {
			created_files = g_slist_append(created_files, g_strdup(filename));
		}
		vcal_manager_free_event(event);
	} 

	g_free(snum);
		
	return filename;
}

static gint vcal_add_msg(Folder *folder, FolderItem *_dest, const gchar *file, MsgFlags *flags)
{
	return 0;
}

static gint vcal_remove_msg(Folder *folder, FolderItem *_item, gint num)
{
	return 0;
}

static FolderItem *vcal_create_folder(Folder * folder,
					 FolderItem * parent,
					 const gchar * name)
{
	gchar *path = NULL;
	FolderItem *newitem = NULL;
	debug_print("creating new vcal folder\n");

	path = g_strconcat((parent->path != NULL) ? parent->path : "", ".", name, NULL);
	newitem = folder_item_new(folder, name, path);
	folder_item_append(parent, newitem);
	g_free(path);

	return newitem;
}

static gint vcal_create_tree(Folder *folder)
{
	FolderItem *rootitem, *inboxitem;
	GNode *rootnode, *inboxnode;

	if (!folder->node) {
		rootitem = folder_item_new(folder, folder->name, NULL);
		rootitem->folder = folder;
		rootnode = g_node_new(rootitem);
		folder->node = rootnode;
		rootitem->node = rootnode;
	} else {
		rootitem = FOLDER_ITEM(folder->node->data);
		rootnode = folder->node;
	}

	/* Add inbox folder */
	if (!folder->inbox) {
		inboxitem = folder_item_new(folder, "Meetings", NULL);
		inboxitem->folder = folder;
		inboxitem->stype = F_INBOX;
		inboxnode = g_node_new(inboxitem);
		inboxitem->node = inboxnode;
		folder->inbox = inboxitem;
		g_node_append(rootnode, inboxnode);
	}

	debug_print("created new vcal tree\n");
	return 0;
}

static gint vcal_remove_folder(Folder *folder, FolderItem *item)
{
	return -1;
}

static gboolean vcal_scan_required(Folder *folder, FolderItem *item)
{
	return TRUE;
}

static void vcal_remove_event (Folder *folder, MsgInfo *msginfo)
{
	MimeInfo *mime = procmime_scan_message(msginfo);
	gchar *uid = NULL;

	if (mime)
		mime = procmime_mimeinfo_next(mime);

	debug_print("next mime info:%s\n", mime->subtype);
	if (mime && !strcmp(mime->subtype, "calendar")) {
		uid = vcalviewer_get_uid_from_mimeinfo(mime);
		if (uid) {
			gchar *file = vcal_manager_get_event_file(uid);
			g_free(uid);
			unlink(file);
			g_free(file);
		}
	}

	if (vcalprefs.export_enable) {
		if (vcal_meeting_export_calendar(vcalprefs.export_path)) {
			if (vcalprefs.export_command &&
			    strlen(vcalprefs.export_command))
				execute_command_line(
					vcalprefs.export_command, TRUE);
		}
	}
}

static void vcal_change_flags(Folder *folder, FolderItem *_item, MsgInfo *msginfo, MsgPermFlags newflags)
{
	if (newflags & MSG_DELETED) {
		msginfo->flags.perm_flags |= MSG_DELETED;
		vcal_remove_event(folder, msginfo);
		return;
	}

	if (msginfo->date_t > time(NULL))
		msginfo->flags.perm_flags |= MSG_COLORLABEL_TO_FLAGS(2); /* Red */
	else
		msginfo->flags.perm_flags &= ~MSG_COLORLABEL_TO_FLAGS(2); /* Red */
}

void vcal_folder_gtk_init(void)
{
	guint i, n_entries;

	n_entries = sizeof(vcal_popup_entries) /
		sizeof(vcal_popup_entries[0]);
	for (i = 0; i < n_entries; i++)
		vcal_popup.entries = g_slist_append(vcal_popup.entries, &vcal_popup_entries[i]);

	folderview_register_popup(&vcal_popup);
}

void vcal_folder_gtk_done(void)
{
	GSList *cur = created_files;
	while (cur) {
		gchar *file = (gchar *)cur->data;
		cur = cur->next;
		if (!file)			
			continue;
		debug_print("removing %s\n", file);
		unlink(file);
		g_free(file);
	}
	g_slist_free(created_files);
}

static void set_sensitivity(GtkItemFactory *factory, FolderItem *item)
{

}

static void new_meeting_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	debug_print("new_meeting_cb\n");
	vcal_meeting_create(NULL);
}

GSList * vcal_folder_get_waiting_events(void)
{
	DIR *dp;
	struct dirent *d;
	GSList *list = NULL;

	dp = opendir(vcal_manager_get_event_path());

	if (!dp) {
		FILE_OP_ERROR(vcal_manager_get_event_path(), "opendir");
		return 0;
	}

	while ((d = readdir(dp)) != NULL) {
		VCalEvent *event = NULL;
		if (d->d_name[0] == '.' || strstr(d->d_name, ".bak")) 
			continue;

		event = vcal_manager_load_event(d->d_name);

		if (event && event->method != ICAL_METHOD_CANCEL) {
			PrefsAccount *account = vcal_manager_get_account_from_event(event);
			enum icalparameter_partstat status = ICAL_PARTSTAT_NEEDSACTION;
			status = account ? vcal_manager_get_reply_for_attendee(event, account->address): ICAL_PARTSTAT_NEEDSACTION;

			if (status != ICAL_PARTSTAT_DECLINED) {
				list = g_slist_append(list, event);
			}
		}

	}

	closedir(dp);
		
	return list;
}

static void export_cal_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	vcal_meeting_export_calendar(NULL);
}
