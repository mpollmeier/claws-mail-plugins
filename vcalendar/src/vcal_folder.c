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
#  include "pluginconfig.h"
#endif

#include "defs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <unistd.h>
#include "gettext.h"
#include <curl/curl.h>
#include <curl/curlver.h>

#include "account.h"
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
#include "prefs_common.h"
#include "inputdialog.h"
#include "xml.h"
#include "alertpanel.h"
#include "log.h"
#include "mainwindow.h"
#include "statusbar.h"

#include <gtk/gtk.h>
#include <dirent.h>

#define VCAL_FOLDERITEM(item) ((VCalFolderItem *) item)

#ifdef USE_PTHREAD
#include <pthread.h>
#endif

typedef struct _thread_data {
	const gchar *url;
	gchar *result;
	gchar *error;
	gboolean done;
} thread_data;


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
static void subscribe_cal_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void check_subs_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void unsubscribe_cal_cb(FolderView *folderview, guint action, GtkWidget *widget);
static void set_sensitivity(GtkItemFactory *factory, FolderItem *item);
static void update_subscription(const gchar *uri, gboolean verbose);
static void rename_cb(FolderView *folderview, guint action, GtkWidget *widget);

gboolean vcal_subscribe_uri(Folder *folder, const gchar *uri);

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
	gchar *uri;
	gchar *feed;
	icalcomponent *cal;
	GSList *numlist;
	GSList *evtlist;
};

static char *vcal_popup_labels[] =
{
	N_("/_New meeting..."),
	N_("/_Export calendar..."),
	"/---",
	N_("/_Subscribe to webCal..."),
	N_("/_Unsubscribe..."),
	"/---",
	N_("/_Rename..."),
	"/---",
	N_("/U_pdate subscriptions"),
	"/---",
	NULL
};			
static GtkItemFactoryEntry vcal_popup_entries[] =
{
	{NULL, NULL, new_meeting_cb,	0, NULL},
	{NULL, NULL, export_cal_cb,	0, NULL},
	{NULL, NULL, NULL,    	0, "<Separator>"},
	{NULL, NULL, subscribe_cal_cb,	0, NULL},
	{NULL, NULL, unsubscribe_cal_cb,	0, NULL},
	{NULL, NULL, NULL,    	0, "<Separator>"},
	{NULL, NULL, rename_cb, 0, NULL},
	{NULL, NULL, NULL,    	0, "<Separator>"},
	{NULL, NULL, check_subs_cb, 0, NULL},
	{NULL, NULL, NULL,  	0, "<Separator>"}
};

static void vcal_fill_popup_menu_labels(void)
{
	gint i;

	for( i = 0; vcal_popup_labels[i] != NULL; i++ ) {
		(vcal_popup_entries[i]).path = _(vcal_popup_labels[i]);
	}
}

static FolderViewPopup vcal_popup =
{
	"vCalendar",
	"<vCalendar>",
	NULL,
	set_sensitivity
};

static void vcal_item_set_xml(Folder *folder, FolderItem *item, XMLTag *tag)
{
	GList *cur;
	folder_item_set_xml(folder, item, tag);
	
	for (cur = tag->attr; cur != NULL; cur = g_list_next(cur)) {
		XMLAttr *attr = (XMLAttr *) cur->data;

		if (!attr || !attr->name || !attr->value) continue;
		if (!strcmp(attr->name, "uri")) {
			if (((VCalFolderItem *)item)->uri != NULL)
				g_free(((VCalFolderItem *)item)->uri);
			((VCalFolderItem *)item)->uri = g_strdup(attr->value);
		} 
	}
}

static XMLTag *vcal_item_get_xml(Folder *folder, FolderItem *item)
{
	static gchar *folder_item_stype_str[] = {"normal", "inbox", "outbox",
						 "draft", "queue", "trash"};
	static gchar *sort_key_str[] = {"none", "number", "size", "date",
					"from", "subject", "score", "label",
					"mark", "unread", "mime", "to", 
					"locked"};
	XMLTag *tag;
	gchar *value;

	tag = folder_item_get_xml(folder, item);

	if (((VCalFolderItem *)item)->uri)
		xml_tag_add_attr(tag, xml_attr_new("uri", ((VCalFolderItem *)item)->uri));

	return tag;
}

static gint vcal_rename_folder(Folder *folder, FolderItem *item,
			     const gchar *name)
{
	if (!name)
		return -1;
	g_free(item->name);
	item->name = g_strdup(name);
	return 0;
}

FolderClass *vcal_folder_get_class()
{
	if (vcal_class.idstr == NULL) {
		debug_print("register class\n");
		vcal_class.type = F_UNKNOWN;
		vcal_class.idstr = "vCalendar";
		vcal_class.uistr = "vCalendar";

		/* Folder functions */
		vcal_class.new_folder = vcal_folder_new;
		vcal_class.destroy_folder = vcal_folder_destroy;
		vcal_class.set_xml = folder_set_xml;
		vcal_class.get_xml = folder_get_xml;
		vcal_class.item_set_xml = vcal_item_set_xml;
		vcal_class.item_get_xml = vcal_item_get_xml;
		vcal_class.scan_tree = vcal_scan_tree;
		vcal_class.create_tree = vcal_create_tree;

		/* FolderItem functions */
		vcal_class.item_new = vcal_item_new;
		vcal_class.item_destroy = vcal_item_destroy;
		vcal_class.item_get_path = vcal_item_get_path;
		vcal_class.create_folder = vcal_create_folder;
		vcal_class.remove_folder = vcal_remove_folder;
		vcal_class.rename_folder = vcal_rename_folder;
		vcal_class.scan_required = vcal_scan_required;
		vcal_class.get_num_list = vcal_get_num_list;

		/* Message functions */
		vcal_class.get_msginfo = vcal_get_msginfo;
		vcal_class.fetch_msg = vcal_fetch_msg;
		vcal_class.add_msg = vcal_add_msg;
		vcal_class.copy_msg = NULL;
		vcal_class.remove_msg = vcal_remove_msg;
		vcal_class.change_flags = vcal_change_flags;
		vcal_class.subscribe = vcal_subscribe_uri;

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
	VCalFolderItem *fitem = (VCalFolderItem *)item;
	if (fitem->uri == NULL)
		return g_strdup(vcal_manager_get_event_path());
	else {
		gchar *path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
					"vcalendar", G_DIR_SEPARATOR_S,
					item->path, NULL);
		return path;
	}
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

gboolean manual_update = TRUE;

static gint feed_fetch(FolderItem *fitem, MsgNumberList ** list, gboolean *old_uids_valid)
{
	VCalFolderItem *item = (VCalFolderItem *)fitem;
	MsgNumberList *msglist = NULL;
	icalcomponent *evt = NULL;
	icalcomponent_kind type = ICAL_VEVENT_COMPONENT;
	gint num = 1;

	debug_print("fetching\n");

	if (!item->uri) {
		debug_print("no uri!\n");
		return -1;
	}

	update_subscription(item->uri, TRUE);

	*old_uids_valid = FALSE;
	*list = NULL;

	if (item->cal) {
		evt = icalcomponent_get_first_component(
			item->cal, ICAL_VEVENT_COMPONENT);
		if (evt == NULL) {
			evt = icalcomponent_get_first_component(
				item->cal, ICAL_VTODO_COMPONENT);
			if (evt != NULL)
				type = ICAL_VTODO_COMPONENT;
		}
	} else
		debug_print("no cal!\n");

	if (evt == NULL)
		debug_print("no event\n");

	if (item->numlist) {
		g_slist_free(item->numlist);
		item->numlist = NULL;
	}

	if (item->evtlist) {
		g_slist_free(item->evtlist);
		item->evtlist = NULL;
	}

	while (evt) {
		icalproperty *prop = icalcomponent_get_first_property(evt, ICAL_UID_PROPERTY);

		if (prop) {
			gchar *uid = g_strdup(icalproperty_get_uid(prop));
			item->numlist = g_slist_append(item->numlist, GINT_TO_POINTER(num));
			item->evtlist = g_slist_append(item->evtlist, evt);
			debug_print("add %d : %s\n", num, uid);
			g_free(uid);
			num++;
		} else {
			debug_print("no uid!\n");
		}
		evt = icalcomponent_get_next_component(
			item->cal, type);
	}
	*list = item->numlist ? g_slist_copy(item->numlist) : NULL;
	debug_print("return %d\n", num);
	return num;
}
static gint vcal_get_num_list(Folder *folder, FolderItem *item,
				 MsgNumberList ** list, gboolean *old_uids_valid)
{
	DIR *dp;
	struct dirent *d;
	int n_msg = 1;
	gchar *snmsg = NULL;
	debug_print(" num for %s\n", ((VCalFolderItem *)item)->uri);
	
	*old_uids_valid = FALSE;
	
	if (((VCalFolderItem *)item)->uri) 
		return feed_fetch(item, list, old_uids_valid);
	
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
		if (d->d_name[0] == '.' || strstr(d->d_name, ".bak")
		||  !strcmp(d->d_name, "internal.ics")
		||  !strcmp(d->d_name, "multisync")) 
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

	g_free(snum);

	debug_print("  got msginfo %p\n", msginfo);

	return msginfo;
}

static gchar *feed_fetch_item(FolderItem * fitem, gint num)
{
	gchar *filename = NULL;
	gchar *snum = NULL;
	gchar *uid = NULL;
	VCalFolderItem *item = (VCalFolderItem *)fitem;
	GSList *ncur, *ecur;
	int i = 1;
	icalcomponent *event = NULL;
	icalproperty *prop = NULL;
	gchar *title = NULL;

	if (!item->numlist) {
		debug_print("numlist null\n");
		return NULL;
	}

	ncur = item->numlist;
	ecur = item->evtlist;
	
	while (i < num) {
		if (!ncur || !ecur) {
			debug_print("list short end (%d to %d) %d,%d\n", i, num, ncur!=NULL, ecur!=NULL);
			return NULL;
		}
		ncur = ncur->next;
		ecur = ecur->next;
		i++;
	}
	
	event = (icalcomponent *)ecur->data;
	
	if (!event) {
		debug_print("no event\n");
		return NULL;
	}
	
	filename = vcal_manager_icalevent_dump(event, fitem->name, NULL);
	debug_print("feed item dump to %s\n", filename);
	return filename;
}

static gchar *vcal_fetch_msg(Folder * folder, FolderItem * item,
				gint num)
{
	gchar *filename = NULL;
	gchar *snum = NULL;
	gchar *uid = NULL;
	
	debug_print(" fetch for %s %d\n", ((VCalFolderItem *)item)->uri, num);
	if (((VCalFolderItem *)item)->uri) 
		return feed_fetch_item(item, num);

	snum = g_strdup_printf("%d",num);
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

static gint vcal_remove_folder(Folder *folder, FolderItem *fitem)
{
	VCalFolderItem *item = (VCalFolderItem *)fitem;
	if (!item->uri)
		return -1;
	else {
		if (item->feed)
			g_free(item->feed);
		if (item->uri)
			g_free(item->uri);
		item->feed = NULL;
		item->uri = NULL;
		folder_item_remove(fitem);
		return 0;
	}
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
	
	debug_print("next mime info:%s\n", mime?mime->subtype:"(nil)");
	if (mime && !strcmp(mime->subtype, "calendar")) {
		uid = vcalviewer_get_uid_from_mimeinfo(mime);
		if (uid) {
			gchar *file = vcal_manager_get_event_file(uid);
			g_free(uid);
			unlink(file);
			g_free(file);
		}
	}

	if (vcal_meeting_export_calendar(vcalprefs.export_path, TRUE)) {
		if (vcalprefs.export_enable &&
		    vcalprefs.export_command &&
		    strlen(vcalprefs.export_command))
			execute_command_line(
				vcalprefs.export_command, TRUE);
	}
}

static void vcal_change_flags(Folder *folder, FolderItem *_item, MsgInfo *msginfo, MsgPermFlags newflags)
{
	if (newflags & MSG_DELETED) {
		/* delete the stuff */
		msginfo->flags.perm_flags |= MSG_DELETED;
		vcal_remove_event(folder, msginfo);
		return;
	}

	/* accept the rest */
	msginfo->flags.perm_flags = newflags;

	/* but not color */
	msginfo->flags.perm_flags &= ~MSG_CLABEL_FLAG_MASK;
	
	if (msginfo->date_t > time(NULL))
		msginfo->flags.perm_flags |= MSG_COLORLABEL_TO_FLAGS(2); /* Red */
}

void vcal_folder_gtk_init(void)
{
	guint i, n_entries;

	vcal_fill_popup_menu_labels();
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

static void set_sensitivity(GtkItemFactory *factory, FolderItem *fitem)
{
	VCalFolderItem *item = (VCalFolderItem *)fitem;

#define SET_SENS(name, sens) \
	menu_set_sensitive(factory, name, sens)

	SET_SENS(_("/New meeting..."), item->uri == NULL);
	SET_SENS(_("/Export calendar..."), TRUE);
	SET_SENS(_("/Subscribe to webCal..."), item->uri == NULL);
	SET_SENS(_("/Unsubscribe..."), item->uri != NULL);
	SET_SENS(_("/Rename..."), folder_item_parent(fitem) != NULL);
	SET_SENS(_("/Update subscriptions"), TRUE);
	
#undef SET_SENS
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
		if (d->d_name[0] == '.' || strstr(d->d_name, ".bak")
		||  !strcmp(d->d_name, "internal.ics") 
		||  !strcmp(d->d_name, "multisync")) 
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

typedef struct _get_webcal_data {
	GSList *list;
} GetWebcalData;

static gboolean get_webcal_events_func(GNode *node, gpointer user_data)
{
	FolderItem *item = node->data;
	GetWebcalData *data = user_data;
	gboolean dummy = FALSE;
	GSList *list = NULL, *cur = NULL;

	feed_fetch(item, &list, &dummy);

	
	for (cur = ((VCalFolderItem *)item)->evtlist; cur; cur = cur->next)
		data->list = g_slist_append(data->list, cur->data);

	return FALSE;
}

GSList * vcal_folder_get_webcal_events(void)
{
	GetWebcalData *data = g_new0(GetWebcalData, 1);
	Folder *folder = folder_find_from_name ("vCalendar", vcal_folder_get_class());
	GSList *list = NULL;
	g_node_traverse(folder->node, G_PRE_ORDER,
			G_TRAVERSE_ALL, -1, get_webcal_events_func, data);

	list = data->list;
	g_free(data);

	return list;
}

static void export_cal_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	vcal_meeting_export_calendar(NULL, FALSE);
}

struct CBuf {
	gchar *str;
};

static int curl_recv(void *buf, size_t size, size_t nmemb, void *stream)
{
	struct CBuf *buffer = (struct CBuf *)stream;
	gchar *tmp = NULL;
	gchar tmpbuf[size*nmemb + 1];

	memcpy(tmpbuf, buf, size*nmemb);
	tmpbuf[size*nmemb] = '\0';

	if (buffer->str) {
		tmp = g_strconcat(buffer->str, tmpbuf, NULL);
		g_free(buffer->str);
		buffer->str = tmp;
	} else {
		buffer->str = g_strdup(tmpbuf);
	}

	return size*nmemb;
}

void *url_read_thread(void *data)
{
	thread_data *td = (thread_data *)data;
	CURLcode res;
	CURL *curl_ctx = NULL;
	long response_code;
	struct CBuf buffer = { NULL };

#ifdef USE_PTHREAD
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif
	
	curl_ctx = curl_easy_init();
	
	curl_easy_setopt(curl_ctx, CURLOPT_URL, td->url);
	curl_easy_setopt(curl_ctx, CURLOPT_WRITEFUNCTION, curl_recv);
	curl_easy_setopt(curl_ctx, CURLOPT_WRITEDATA, &buffer);
#ifndef USE_PTHREAD
	curl_easy_setopt(curl_ctx, CURLOPT_TIMEOUT, prefs_common.io_timeout_secs);
#endif
#if LIBCURL_VERSION_NUM >= 0x070a00
	curl_easy_setopt(curl_ctx, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl_ctx, CURLOPT_SSL_VERIFYHOST, 0);
#endif
	curl_easy_setopt(curl_ctx, CURLOPT_USERAGENT, 
		"Sylpheed-Claws vCalendar plugin "
		"(http://claws.sylpheed.org/plugins.php)");
	curl_easy_setopt(curl_ctx, CURLOPT_FOLLOWLOCATION, 1);
	res = curl_easy_perform(curl_ctx);
	
	if (res != 0) {
		printf("res %d %s\n", res, curl_easy_strerror(res));
		td->error = g_strdup(curl_easy_strerror(res));
	}

	curl_easy_getinfo(curl_ctx, CURLINFO_RESPONSE_CODE, &response_code);
	if( response_code >= 400 && response_code < 500 ) {
		debug_print("VCalendar: got %d\n", response_code);
		switch(response_code) {
			case 401: 
				td->error = g_strdup(_("401 (Authorisation required)"));
				break;
			case 403:
				td->error = g_strdup(_("403 (Unauthorised)"));
				break;
			case 404:
				td->error = g_strdup(_("404 (Not found)"));
				break;
			default:
				td->error = g_strdup_printf(_("Error %d"), response_code);
				break;
		}
	}
	curl_easy_cleanup(curl_ctx);
	if (buffer.str) {
		td->result = g_strdup(buffer.str);
		g_free(buffer.str);
	}

	td->done = TRUE; /* let the caller thread join() */
	return GINT_TO_POINTER(0);
}

static void url_read(const char *url, gboolean verbose, 
	void (*callback)(const gchar *url, gchar *data, gboolean verbose, gchar *error))
{
	gchar *result;
	thread_data *td;
#ifdef USE_PTHREAD
	pthread_t pt;
#endif
	gchar *msg;
	void *res;
	time_t start_time;
	gboolean killed;
	gchar *error = NULL;
	result = NULL;
	td = g_new0(thread_data, 1);
	msg = NULL;
	res = NULL;
	start_time = time(NULL);
	killed = FALSE;
	
	td->url  = url;
	td->result  = NULL;
	td->done = FALSE;
	
	msg = g_strdup_printf(_("Fetching '%s'..."), url);
	
	STATUSBAR_PUSH(mainwindow_get_mainwindow(), msg);
	
	g_free(msg);

#if (defined USE_PTHREAD && defined __GLIBC__ && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 3)))
	if (pthread_create(&pt, PTHREAD_CREATE_JOINABLE, 
			url_read_thread, td) != 0) {
		url_read_thread(td);	
	}
	while (!td->done)  {
 		sylpheed_do_idle();
		if (time(NULL) - start_time > prefs_common.io_timeout_secs) {
			log_error(_("Timeout (%d seconds) connecting to %s\n"), prefs_common.io_timeout_secs, url);
			pthread_cancel(pt);
			td->done = TRUE;
			killed = TRUE;
		}
	}
 
	pthread_join(pt, &res);
#else
	url_read_thread(td);
#endif
	
	result = td->result;
	error = td->error;
	g_free(td);
	
	STATUSBAR_POP(mainwindow_get_mainwindow());

	if (callback)
		callback(url, killed?NULL:result, verbose, error);
}

static gboolean folder_item_find_func(GNode *node, gpointer data)
{
	FolderItem *item = node->data;
	gpointer *d = data;
	const gchar *uri = d[0];

	if (!uri || !((VCalFolderItem *)item)->uri
	||  strcmp(uri, ((VCalFolderItem *)item)->uri))
		return FALSE;

	d[1] = item;

	return TRUE;
}

static FolderItem *get_folder_item_for_uri(const gchar *uri)
{
	FolderItem *tmp = NULL;
	Folder *root = folder_find_from_name ("vCalendar", vcal_folder_get_class());
	gpointer d[2];
	
	if (root == NULL)
		return NULL;
	
	d[0] = (gpointer)uri;
	d[1] = NULL;
	g_node_traverse(root->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
			folder_item_find_func, d);
	return d[1];
}

static gchar *feed_get_title(const gchar *str)
{
	gchar *title = NULL;
	if (strstr(str, "X-WR-CALNAME:")) {
		title = g_strdup(strstr(str, "X-WR-CALNAME:")+strlen("X-WR-CALNAME:"));
		if (strstr(title, "\n"))
			*(strstr(title, "\n")) = '\0';
		if (strstr(title, "\r"))
			*(strstr(title, "\r")) = '\0';		
	} else if (strstr(str, "X-WR-CALDESC:")) {
		title = g_strdup(strstr(str, "X-WR-CALDESC:")+strlen("X-WR-CALDESC:"));
		if (strstr(title, "\n"))
			*(strstr(title, "\n")) = '\0';
		if (strstr(title, "\r"))
			*(strstr(title, "\r")) = '\0';		
	}
	
	return title;
}

static void update_subscription_finish(const gchar *uri, gchar *feed, gboolean verbose, gchar *error)
{
	Folder *root = folder_find_from_name ("vCalendar", vcal_folder_get_class());
	FolderItem *item = NULL;
	icalcomponent *cal = NULL;
	
	if (root == NULL) {
		g_warning("can't get root folder\n");
		g_free(feed);
		if (error)
			g_free(error);
		return;
	}

	if (feed == NULL) {
		if (verbose && manual_update) {
			gchar *buf = g_strdup_printf(_("Could not retrieve the Webcal URL:\n%s\n%s"),
					uri, error ? error:_("Unknown error"));
			alertpanel_error(buf);
			g_free(buf);
		} else  {
			gchar *buf = g_strdup_printf(_("Could not retrieve the Webcal URL:\n%s\n%s\n"),
					uri, error ? error:_("Unknown error"));
			log_error(buf);
			g_free(buf);
		}
		main_window_cursor_normal(mainwindow_get_mainwindow());
		g_free(feed);
		if (error)
			g_free(error);
		return;
	}
	if (strncmp(feed, "BEGIN:VCALENDAR", strlen("BEGIN:VCALENDAR"))) {
		if (verbose && manual_update) {
			gchar *buf = g_strdup_printf(
					_("This URL does not look like a WebCal URL:\n%s\n%s"),
					uri, error ? error:_("Unknown error"));
			alertpanel_error(buf);
			g_free(buf);
		} else  {
			gchar *buf = g_strdup_printf(
					_("This URL does not look like a WebCal URL:\n%s\n%s\n"),
					uri, error ? error:_("Unknown error"));
			log_error(buf);
			g_free(buf);
		}
		g_free(feed);
		main_window_cursor_normal(mainwindow_get_mainwindow());
		if (error)
			g_free(error);
		return;
	}
	
	if (error)
		g_free(error);
	item = get_folder_item_for_uri(uri);
	if (item == NULL) {
		gchar *title = feed_get_title(feed);
		if (title == NULL) {
			if (strstr(uri, "://"))
				title = g_strdup(strstr(uri,"://")+3);
			else
				title = g_strdup(uri);
			subst_for_filename(title);
			if (strlen(title) > 32) {
				title[29]=title[30]=title[31]='.';
				title[32]='\0';
			}
		}
		item = folder_create_folder(root->node->data, title);
		debug_print("item done %s\n", title);
		((VCalFolderItem *)item)->uri = g_strdup(uri);
		((VCalFolderItem *)item)->feed = feed;
		g_free(title);
	} else {
		if (((VCalFolderItem *)item)->feed)
			g_free(((VCalFolderItem *)item)->feed);

		((VCalFolderItem *)item)->feed = feed;
		/* if title differs, update it */
	}
	cal = icalparser_parse_string(feed);
	
	if (((VCalFolderItem *)item)->cal)
		icalcomponent_free(((VCalFolderItem *)item)->cal);

	((VCalFolderItem *)item)->cal = cal;
	
	main_window_cursor_normal(mainwindow_get_mainwindow());
}

static void update_subscription(const gchar *uri, gboolean verbose)
{
	if (prefs_common.work_offline) {
		if (!verbose || 
		!inc_offline_should_override(
		   _("Sylpheed-Claws needs network access in order "
		     "to update the Webcal feed.")))
			return;
	}
	
	main_window_cursor_wait(mainwindow_get_mainwindow());
	url_read(uri, verbose, update_subscription_finish);
}

static void check_subs_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	Folder *root = folder_find_from_name ("vCalendar", vcal_folder_get_class());

	if (prefs_common.work_offline && 
	    !inc_offline_should_override(_("Sylpheed-Claws needs network access in order "
		     "to update the subscription.")))
		return;

	folderview_check_new(root);
}

static void subscribe_cal_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	gchar *uri = NULL;
	gchar *tmp = NULL;

	tmp = input_dialog(_("Subscribe to WebCal"), _("Enter the WebCal URL:"), NULL);
	if (tmp == NULL)
		return;
	
	if (!strncmp(tmp, "http://", 7)) {
		uri = tmp;
	} else if (!strncmp(tmp, "https://", 8)) {
		uri = tmp;
	} else if (!strncmp(tmp, "webcal://", 9)) {
		uri = g_strconcat("http://", tmp+9, NULL);
		g_free(tmp);
	} else {
		alertpanel_error(_("Could not parse the URL."));
		g_free(tmp);
		return;
	}
	debug_print("uri %s\n", uri);
	
	update_subscription(uri, TRUE);	
	folder_write_list();
	g_free(uri);
}

static void unsubscribe_cal_cb(FolderView *folderview, guint action, GtkWidget *widget)
{
	GtkCTree *ctree = GTK_CTREE(folderview->ctree);
	FolderItem *item;
	gchar *message, *name;
	AlertValue avalue;
	gchar *old_path;
	gchar *old_id;

	if (!folderview->selected) return;

	item = gtk_ctree_node_get_row_data(ctree, folderview->selected);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->path != NULL);
	g_return_if_fail(item->folder != NULL);

	message = g_strdup_printf
		(_("Do you really want to unsubscribe?"));
	avalue = alertpanel_full(_("Delete folder"), message,
		 		 GTK_STOCK_CANCEL, GTK_STOCK_DELETE, NULL, 
				 FALSE, NULL, ALERT_WARNING, G_ALERTDEFAULT);
	g_free(message);
	if (avalue != G_ALERTALTERNATE) return;

	Xstrdup_a(old_path, item->path, return);
	old_id = folder_item_get_identifier(item);

	if (folderview->opened == folderview->selected ||
	    gtk_ctree_is_ancestor(ctree,
				  folderview->selected,
				  folderview->opened)) {
		summary_clear_all(folderview->summaryview);
		folderview->opened = NULL;
	}

	if (item->folder->klass->remove_folder(item->folder, item) < 0) {
		folder_item_scan(item);
		alertpanel_error(_("Can't remove the folder '%s'."), name);
		g_free(old_id);
		return;
	}

	folder_write_list();

	prefs_filtering_delete_path(old_id);
	g_free(old_id);
}

gboolean vcal_subscribe_uri(Folder *folder, const gchar *uri)
{
	gchar *tmp = NULL;
	if (folder->klass != vcal_folder_get_class())
		return FALSE;

	if (!strncmp(uri, "webcal://", 9)) {
		tmp = g_strconcat("http://", uri+9, NULL);
	} else {
		return FALSE;
	}
	debug_print("uri %s\n", tmp);
	
	update_subscription(tmp, FALSE);
	folder_write_list();
	return TRUE;
}

static void rename_cb(FolderView *folderview, guint action,
			     GtkWidget *widget)
{
	FolderItem *item;
	gchar *new_folder;
	gchar *name;
	gchar *message;

	item = folderview_get_selected_item(folderview);
	g_return_if_fail(item != NULL);
	g_return_if_fail(item->path != NULL);
	g_return_if_fail(item->folder != NULL);

	name = trim_string(item->name, 32);
	message = g_strdup_printf(_("Input new name for '%s':"), name);
	new_folder = input_dialog(_("Rename folder"), message, name);
	g_free(message);
	g_free(name);
	if (!new_folder) return;
	AUTORELEASE_STR(new_folder, {g_free(new_folder); return;});

	if (strchr(new_folder, G_DIR_SEPARATOR) != NULL) {
		alertpanel_error(_("'%c' can't be included in folder name."),
				 G_DIR_SEPARATOR);
		return;
	}

	if (folder_find_child_item_by_name(folder_item_parent(item), new_folder)) {
		name = trim_string(new_folder, 32);
		alertpanel_error(_("The folder '%s' already exists."), name);
		g_free(name);
		return;
	}

	if (folder_item_rename(item, new_folder) < 0) {
		alertpanel_error(_("The folder could not be renamed.\n"
				   "The new folder name is not allowed."));
		return;
	}

	folder_item_prefs_save_config(item);
	folder_write_list();
}
