/*
 * Maildir Plugin -- Maildir++ support for Sylpheed
 * Copyright (C) 2003-2004 Christoph Hohmann
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

#include "utils.h"
#include "procmsg.h"
#include "procheader.h"
#include "folder.h"
#include "maildir.h"
#include "localfolder.h"
#include "uiddb.h"

#define MAILDIR_FOLDERITEM(item) ((MaildirFolderItem *) item)

typedef struct _MaildirFolder MaildirFolder;
typedef struct _MaildirFolderItem MaildirFolderItem;

static Folder *maildir_folder_new(const gchar * name,
				  const gchar * folder);
static void maildir_folder_destroy(Folder * folder);
static gint maildir_scan_tree(Folder * folder);
static FolderItem *maildir_item_new(Folder * folder);
static void maildir_item_destroy(Folder * folder, FolderItem * item);
static gchar *maildir_item_get_path(Folder * folder, FolderItem * item);
static gint maildir_get_num_list(Folder * folder, FolderItem * item,
				 MsgNumberList ** list,
				 gboolean * old_uids_valid);
static MsgInfo *maildir_get_msginfo(Folder * folder, FolderItem * item,
				    gint num);
static gchar *maildir_fetch_msg(Folder * folder, FolderItem * item,
				gint num);
static gint maildir_add_msg(Folder * folder, FolderItem * _dest,
			    const gchar * file, MsgFlags * flags);
static gint maildir_copy_msg(Folder * folder, FolderItem * dest,
			     MsgInfo * msginfo);
static gint maildir_remove_msg(Folder * folder, FolderItem * _item,
			       gint num);
static void maildir_change_flags(Folder * folder, FolderItem * item,
				 MsgInfo * msginfo, MsgPermFlags newflags);
static FolderItem *maildir_create_folder(Folder * folder,
					 FolderItem * parent,
					 const gchar * name);
static gint maildir_create_tree(Folder *folder);
static void remove_missing_folder_items(Folder *folder);
static gint maildir_remove_folder(Folder *folder, FolderItem *item);
static gint maildir_rename_folder(Folder *folder, FolderItem *item,
			     const gchar *name);

FolderClass maildir_class;

struct _MaildirFolder
{
	LocalFolder folder;
};

struct _MaildirFolderItem
{
	FolderItem item;

	guint lastuid;
	UIDDB *db;
};

FolderClass *maildir_get_class()
{
	if (maildir_class.idstr == NULL) {
		maildir_class.type = F_MAILDIR;
		maildir_class.idstr = "maildir";
		maildir_class.uistr = "Maildir++";

		/* Folder functions */
		maildir_class.new_folder = maildir_folder_new;
		maildir_class.destroy_folder = maildir_folder_destroy;
		maildir_class.set_xml = folder_local_set_xml;
		maildir_class.get_xml = folder_local_get_xml;
		maildir_class.scan_tree = maildir_scan_tree;
		maildir_class.create_tree = maildir_create_tree;

		/* FolderItem functions */
		maildir_class.item_new = maildir_item_new;
		maildir_class.item_destroy = maildir_item_destroy;
		maildir_class.item_get_path = maildir_item_get_path;
		maildir_class.create_folder = maildir_create_folder;
		maildir_class.remove_folder = maildir_remove_folder;
		maildir_class.rename_folder = maildir_rename_folder;
		maildir_class.get_num_list = maildir_get_num_list;

		/* Message functions */
		maildir_class.get_msginfo = maildir_get_msginfo;
		maildir_class.fetch_msg = maildir_fetch_msg;
		maildir_class.add_msg = maildir_add_msg;
		maildir_class.copy_msg = maildir_copy_msg;
		maildir_class.remove_msg = maildir_remove_msg;
		maildir_class.change_flags = maildir_change_flags;
	}

	return &maildir_class;
}

static Folder *maildir_folder_new(const gchar * name,
				  const gchar * path)
{
        MaildirFolder *folder;                   
        
        folder = g_new0(MaildirFolder, 1);
        FOLDER(folder)->klass = &maildir_class;
        folder_local_folder_init(FOLDER(folder), name, path);

        return FOLDER(folder);
}

static void maildir_folder_destroy(Folder *_folder)
{
	MaildirFolder *folder = (MaildirFolder *) _folder;

	folder_local_folder_destroy(LOCAL_FOLDER(folder));
}

static gint open_database(MaildirFolderItem *item)
{
	gchar *path, *database;

	if (item->db != NULL)
		return 0;

	path = maildir_item_get_path(FOLDER_ITEM(item)->folder, FOLDER_ITEM(item));
	Xstrcat_a(database, path, G_DIR_SEPARATOR_S "sylpheed_uid.db", return -1);
	g_free(path);

	item->db = uiddb_open(database);

	return 0;
}

static FolderItem *maildir_item_new(Folder *folder)
{
        MaildirFolderItem *item;

        item = g_new0(MaildirFolderItem, 1);
        item->lastuid = 0;
	item->db = NULL;
        
        return (FolderItem *) item;

}

static void maildir_item_destroy(Folder *folder, FolderItem *_item)
{
        MaildirFolderItem *item = (MaildirFolderItem *)_item;

        g_return_if_fail(item != NULL);

	if (item->db != NULL)
		uiddb_close(item->db);

        g_free(item);
}

static gchar *maildir_item_get_path(Folder *folder, FolderItem *item)
{
	gchar *folder_path, *path;

	g_return_val_if_fail(folder != NULL, NULL);
	g_return_val_if_fail(item != NULL, NULL);

	folder_path = g_strdup(LOCAL_FOLDER(folder)->rootpath);
	g_return_val_if_fail(folder_path != NULL, NULL);

	if (g_path_is_absolute(folder_path)) {
                if (item->path && strcmp(item->path, "INBOX"))
                        path = g_strconcat(folder_path, G_DIR_SEPARATOR_S,
                                           item->path, NULL);
                else
                        path = g_strdup(folder_path);
        } else {
                if (item->path && strcmp(item->path, "INBOX"))
                        path = g_strconcat(get_home_dir(), G_DIR_SEPARATOR_S,
                                           folder_path, G_DIR_SEPARATOR_S,
                                           item->path, NULL);
                else
                        path = g_strconcat(get_home_dir(), G_DIR_SEPARATOR_S,
                                           folder_path, NULL);
        }
	g_free(folder_path);

	return path;
}

static void build_tree(GNode *node, glob_t *globbuf)
{
        int i;
	FolderItem *parent = FOLDER_ITEM(node->data);
	gchar *prefix = parent->path ?  parent->path : "";
	Folder *folder = parent->folder;

        for (i = 0; i < globbuf->gl_pathc; i++) {
		FolderItem *newitem;
		GNode *newnode;
		gchar *dirname, *tmpstr, *foldername;
		gboolean res;

		dirname = g_basename(globbuf->gl_pathv[i]);
		foldername = &(dirname[strlen(prefix) + 1]);

                if (dirname[0] == '.' && dirname[1] == '\0')
                        continue;

                if (strncmp(dirname, prefix, strlen(prefix)))
                        continue;

                if (dirname[strlen(prefix)] != '.')
                        continue;

                if (strchr(foldername, '.') != NULL)
                        continue;

                if (!is_dir_exist(globbuf->gl_pathv[i]))
                        continue;

		tmpstr = g_strconcat(globbuf->gl_pathv[i], "/cur", NULL);
                res = is_dir_exist(tmpstr);
		g_free(tmpstr);
		if (!res)
			continue;

		/* don't add items that already exist in the tree */
		newitem = folder_find_child_item_by_name(parent, dirname);
		if (newitem == NULL) {
			newitem = folder_item_new(parent->folder, foldername, dirname);
			newitem->folder = folder;

			newnode = g_node_new(newitem);
			newitem->node = newnode;
			g_node_append(node, newnode);

            		debug_print("added item %s\n", newitem->path);
		}

		if (!parent->path) {
			if (!folder->outbox && !strcmp(dirname, "." OUTBOX_DIR)) {
				newitem->stype = F_OUTBOX;
				folder->outbox = newitem;
			} else if (!folder->draft && !strcmp(dirname, "." DRAFT_DIR)) {
				newitem->stype = F_DRAFT;
				folder->draft = newitem;
			} else if (!folder->queue && !strcmp(dirname, "." QUEUE_DIR)) {
				newitem->stype = F_QUEUE;
				folder->queue = newitem;
			} else if (!folder->trash && !strcmp(dirname, "." TRASH_DIR)) {
				newitem->stype = F_TRASH;
				folder->trash = newitem;
			}
		}

                build_tree(newitem->node, globbuf);
        }
}

static gint maildir_scan_tree(Folder *folder)
{
        FolderItem *rootitem, *inboxitem;
	GNode *rootnode, *inboxnode;
        glob_t globbuf;
	gchar *rootpath, *globpat;
        
        g_return_val_if_fail(folder != NULL, -1);

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
		inboxitem = folder_item_new(folder, "inbox", "INBOX");
		inboxitem->folder = folder;
		inboxitem->stype = F_INBOX;
		inboxnode = g_node_new(inboxitem);
		inboxitem->node = inboxnode;
		folder->inbox = inboxitem;
		g_node_append(rootnode, inboxnode);
	}

	rootpath = folder_item_get_path(rootitem);

	/* clear special folders to make sure we don't have invalid references
	   after remove_missing_folder_items */
	folder->outbox = NULL;
	folder->draft = NULL;
	folder->queue = NULL;
	folder->trash = NULL;

	debug_print("scanning tree %s\n", rootpath);
	maildir_create_tree(folder);
	remove_missing_folder_items(folder);

	globpat = g_strconcat(rootpath, G_DIR_SEPARATOR_S ".*", NULL);
	globbuf.gl_offs = 0;
	glob(globpat, 0, NULL, &globbuf);
	g_free(globpat);
	build_tree(rootnode, &globbuf);
	globfree(&globbuf);

	return 0;
}

static gchar *get_filename_for_msgdata(MessageData *msgdata)
{
	gchar *filename;

	if (msgdata->info[0])
		filename = g_strconcat(msgdata->dir, G_DIR_SEPARATOR_S, 
				       msgdata->uniq, ":", msgdata->info, NULL);
	else
		filename = g_strconcat(msgdata->dir, G_DIR_SEPARATOR_S, 
				       msgdata->uniq, NULL);

	return filename;
}

static MessageData *get_msgdata_for_filename(const gchar *filename)
{
	MessageData *msgdata;
	const gchar *tmpfilename;
	gchar **pathsplit, **namesplit;

	tmpfilename = strrchr(filename, G_DIR_SEPARATOR);
	if (tmpfilename == NULL || tmpfilename == filename)
		return NULL;

	tmpfilename--;
	while (tmpfilename > filename && tmpfilename[0] != G_DIR_SEPARATOR)
		tmpfilename--;
	if (tmpfilename[0] == G_DIR_SEPARATOR)
		tmpfilename++;

    	pathsplit = g_strsplit(tmpfilename, G_DIR_SEPARATOR_S, 2);
	if (pathsplit[1] == NULL) {
	        g_strfreev(pathsplit);
		return NULL;
	}

	namesplit = g_strsplit(pathsplit[1], ":", 2);

	msgdata = g_new0(MessageData, 1);

	msgdata->dir = g_strdup(pathsplit[0]);
	msgdata->uniq = g_strdup(namesplit[0]);
	if (namesplit[1] != NULL)
		msgdata->info = g_strdup(namesplit[1]);
	else
		msgdata->info = g_strdup("");

	g_strfreev(namesplit);
	g_strfreev(pathsplit);

	return msgdata;
}

static guint32 get_uid_for_filename(MaildirFolderItem *item, const gchar *filename)
{
	gchar *uniq, *info;
	MessageData *msgdata;
	guint32 uid;

	g_return_val_if_fail(open_database(item) == 0, 0);

	uniq = strrchr(filename, G_DIR_SEPARATOR);
	if (uniq == NULL)
		return 0;
	uniq++;

	Xstrdup_a(uniq, uniq, return 0);
	info = strchr(uniq, ':');
	if (info != NULL)
		*info = '\0';

	msgdata = uiddb_get_entry_for_uniq(item->db, uniq);
	if (msgdata == NULL) {
		msgdata = get_msgdata_for_filename(filename);
		if (msgdata == NULL)
			return 0;
		msgdata->uid = uiddb_get_new_uid(item->db);

		uiddb_insert_entry(item->db, msgdata);
	}

	uid = msgdata->uid;
	uiddb_free_msgdata(msgdata);

	return uid;
}

static MessageData *get_msgdata_for_uid(MaildirFolderItem *item, guint32 uid)
{
	MessageData *msgdata;
	gchar *path, *msgname, *filename;
	glob_t globbuf;
	g_return_val_if_fail(open_database(item) == 0, NULL);

	msgdata = uiddb_get_entry_for_uid(item->db, uid);
	if (msgdata == NULL)
		return NULL;

	path = maildir_item_get_path(FOLDER_ITEM(item)->folder, FOLDER_ITEM(item));

	msgname = get_filename_for_msgdata(msgdata);
	filename = g_strconcat(path, G_DIR_SEPARATOR_S, msgname, NULL);
	g_free(msgname);
	
	if (is_file_exist(filename)) {
		g_free(path);
		return msgdata;
	}

	debug_print("researching for %s\n", msgdata->uniq);
	/* delete old entry */
	g_free(filename);
	uiddb_delete_entry(item->db, uid);

	/* try to find file with same uniq and different info */
	filename = g_strconcat(path, G_DIR_SEPARATOR_S, "new", G_DIR_SEPARATOR_S, msgdata->uniq, NULL);
	if (!is_file_exist(filename)) {
		g_free(filename);

		filename = g_strconcat(path, G_DIR_SEPARATOR_S, "cur", G_DIR_SEPARATOR_S, msgdata->uniq, ":*", NULL);
		glob(filename, 0, NULL, &globbuf);
		g_free(filename);

		g_free(path);
	
		filename = NULL;
		if (globbuf.gl_pathc > 0)
			filename = g_strdup(globbuf.gl_pathv[0]);
		globfree(&globbuf);
	}
	uiddb_free_msgdata(msgdata);
	msgdata = NULL;

	/* if found: update database and return new entry */
	if (filename != NULL) {
		debug_print("found %s\n", filename);
		
		msgdata = get_msgdata_for_filename(filename);
		msgdata->uid = uid;

		uiddb_insert_entry(item->db, msgdata);
	}

	return msgdata;
}

static gchar *get_filepath_for_msgdata(MaildirFolderItem *item, MessageData *msgdata)
{
	gchar *path, *msgname, *filename;

	path = maildir_item_get_path(FOLDER_ITEM(item)->folder, FOLDER_ITEM(item));
	msgname = get_filename_for_msgdata(msgdata);
	filename = g_strconcat(path, G_DIR_SEPARATOR_S, msgname, NULL);
	g_free(msgname);
	g_free(path);
	
	return filename;
}

static gchar *get_filepath_for_uid(MaildirFolderItem *item, guint32 uid)
{
	MessageData *msgdata;
	gchar *filename;

	g_return_val_if_fail(open_database(item) == 0, NULL);

	msgdata = get_msgdata_for_uid(item, uid);
	if (msgdata == NULL)
		return NULL;

	filename = get_filepath_for_msgdata(item, msgdata);
	uiddb_free_msgdata(msgdata);

	return filename;
}

static gint maildir_get_num_list(Folder *folder, FolderItem *item,
				 MsgNumberList ** list, gboolean *old_uids_valid)
{
	gchar *path, *globpattern;
	glob_t globbuf;
	int i;

        g_return_val_if_fail(open_database(MAILDIR_FOLDERITEM(item)) == 0, -1);

	*old_uids_valid = TRUE;

	globbuf.gl_offs = 0;
	path = maildir_item_get_path(folder, item);

	globpattern = g_strconcat(path, G_DIR_SEPARATOR_S, "cur", G_DIR_SEPARATOR_S, "*", NULL);
	glob(globpattern, 0, NULL, &globbuf);
	g_free(globpattern);

	globpattern = g_strconcat(path, G_DIR_SEPARATOR_S, "new", G_DIR_SEPARATOR_S, "*", NULL);
	glob(globpattern, GLOB_APPEND, NULL, &globbuf);
	g_free(globpattern);

	g_free(path);

	for (i = 0; i < globbuf.gl_pathc; i++) {
		guint32 uid;

		uid = get_uid_for_filename((MaildirFolderItem *) item, globbuf.gl_pathv[i]);
		if (uid != 0)
			*list = g_slist_append(*list, GINT_TO_POINTER(uid));
	}
	globfree(&globbuf);

	uiddb_delete_entries_not_in_list(((MaildirFolderItem *) item)->db, *list);

	return g_slist_length(*list);
}

static MsgInfo *maildir_parse_msg(const gchar *file, FolderItem *item)
{
	struct stat s;
	MsgInfo *msginfo;
	MsgFlags flags;

	flags.perm_flags = MSG_NEW|MSG_UNREAD;
	flags.tmp_flags = 0;

	g_return_val_if_fail(item != NULL, NULL);
	g_return_val_if_fail(file != NULL, NULL);

	if (item->stype == F_QUEUE) {
		MSG_SET_TMP_FLAGS(flags, MSG_QUEUED);
	} else if (item->stype == F_DRAFT) {
		MSG_SET_TMP_FLAGS(flags, MSG_DRAFT);
	}

	msginfo = procheader_parse_file(file, flags, FALSE, FALSE);
	if (!msginfo) return NULL;

	msginfo->msgnum = atoi(file);
	msginfo->folder = item;

	if (stat(file, &s) < 0) {
		FILE_OP_ERROR(file, "stat");
		msginfo->size = 0;
		msginfo->mtime = 0;
	} else {
		msginfo->size = s.st_size;
		msginfo->mtime = s.st_mtime;
	}

	return msginfo;
}

static MsgInfo *maildir_get_msginfo(Folder * folder,
				    FolderItem * item, gint num)
{
	MsgInfo *msginfo;
	gchar *file;

	g_return_val_if_fail(item != NULL, NULL);
	g_return_val_if_fail(num > 0, NULL);

	file = maildir_fetch_msg(folder, item, num);
	if (!file) return NULL;

	msginfo = maildir_parse_msg(file, item);
	if (msginfo)
		msginfo->msgnum = num;

	g_free(file);

	return msginfo;
}

static gchar *maildir_fetch_msg(Folder * folder, FolderItem * item,
				gint num)
{
	gchar *filename;

	filename = get_filepath_for_uid((MaildirFolderItem *) item, num);
	return filename;
}

static gchar *generate_uniq()
{
	gchar hostname[32], *strptr;
	static gint q = 1;
	struct timeval tv;

	gethostname(hostname, 32);
	hostname[31] = '\0';

	strptr = &hostname[0];
	while (*strptr != '\0') {
		if (*strptr == '/')
			*strptr = '\057';
		if (*strptr == ':')
			*strptr = '\072';
		strptr++;
	}

	gettimeofday(&tv, NULL);

	return g_strdup_printf("%d.P%dQ%dM%d.%s", (int) tv.tv_sec, getpid(), q++, (int) tv.tv_usec, hostname);
}

static gchar *get_infostr(MsgPermFlags permflags)
{
	if (permflags & MSG_NEW)
		return g_strdup("");

	return g_strconcat("2,",
		  permflags & MSG_MARKED    ? "F" : "",
		  permflags & MSG_FORWARDED ? "P" : "",
		  permflags & MSG_REPLIED   ? "R" : "",
		!(permflags & MSG_UNREAD)   ? "S" : "",
		NULL);
}

static gint add_file_to_maildir(MaildirFolderItem *item, const gchar *file, MsgFlags *flags)
{
	MessageData *msgdata;
	gchar *tmpname, *destname;
	gint uid = 0;

	g_return_val_if_fail(item != NULL, -1);
        g_return_val_if_fail(open_database(MAILDIR_FOLDERITEM(item)) == 0, -1);

	msgdata = g_new0(MessageData, 1);
	msgdata->uniq = generate_uniq();
	if (flags != NULL)
		msgdata->info = get_infostr(flags->perm_flags);
	else
		msgdata->info = g_strdup("");
	msgdata->uid = uiddb_get_new_uid(item->db);

	msgdata->dir = "tmp";
	tmpname = get_filepath_for_msgdata(item, msgdata);

	if (flags != NULL)
		msgdata->dir = g_strdup(flags->perm_flags & MSG_NEW ? "new" : "cur");
	else
		msgdata->dir = g_strdup("new");

	if (copy_file(file, tmpname, FALSE) < 0) {
		uiddb_free_msgdata(msgdata);
		g_free(tmpname);
		return -1;
	}

	destname = get_filepath_for_msgdata(item, msgdata);
	if (rename(tmpname, destname) < 0) {
		uiddb_free_msgdata(msgdata);
		g_free(tmpname);
		g_free(destname);
		return -1;
	}

	uiddb_insert_entry(item->db, msgdata);

	uid = msgdata->uid;
	uiddb_free_msgdata(msgdata);
	
	return uid;
}

static gint maildir_add_msg(Folder *folder, FolderItem *_dest, const gchar *file, MsgFlags *flags)
{
	MaildirFolderItem *dest = MAILDIR_FOLDERITEM(_dest);

	g_return_val_if_fail(folder != NULL, -1);
	g_return_val_if_fail(dest != NULL, -1);
	g_return_val_if_fail(file != NULL, -1);

	return add_file_to_maildir(dest, file, flags);
}

static gint maildir_copy_msg(Folder *folder, FolderItem *dest, MsgInfo *msginfo)
{
	gchar *srcfile;
	gint ret = -1;
	gboolean delsrc = FALSE;

	g_return_val_if_fail(folder != NULL, -1);
	g_return_val_if_fail(dest != NULL, -1);
	g_return_val_if_fail(msginfo != NULL, -1);

	srcfile = procmsg_get_message_file(msginfo);
	if (srcfile == NULL)
		return -1;

	if ((MSG_IS_QUEUED(msginfo->flags) || MSG_IS_DRAFT(msginfo->flags))
	    && dest->stype != F_QUEUE && dest->stype != F_DRAFT) {
		gchar *tmpfile;

		tmpfile = get_tmp_file();
		if (procmsg_remove_special_headers(srcfile, tmpfile) != 0) {
			g_free(srcfile);
			g_free(tmpfile);
			return -1;
		}		
		g_free(srcfile);
		srcfile = tmpfile;
		delsrc = TRUE;
	}

	ret = add_file_to_maildir(MAILDIR_FOLDERITEM(dest), srcfile, &msginfo->flags);

	if (delsrc)
		unlink(srcfile);
	g_free(srcfile);

	return ret;
}

static gint maildir_remove_msg(Folder *folder, FolderItem *_item, gint num)
{
	MaildirFolderItem *item = MAILDIR_FOLDERITEM(_item);
	gchar *filename;
	gint ret;

	g_return_val_if_fail(folder != NULL, -1);
	g_return_val_if_fail(item != NULL, -1);
	g_return_val_if_fail(num > 0, -1);

	filename = get_filepath_for_uid(item, num);
	if (filename == NULL)
		return -1;

	ret = unlink(filename);	
	if (ret == 0)
		uiddb_delete_entry(item->db, num);

	g_free(filename);
	return ret;
}

static void maildir_change_flags(Folder *folder, FolderItem *_item, MsgInfo *msginfo, MsgPermFlags newflags)
{
	MaildirFolderItem *item = MAILDIR_FOLDERITEM(_item);
	MessageData *msgdata;
	gchar *oldname, *newinfo, *newdir;
	gboolean renamefile = FALSE;

	msgdata = get_msgdata_for_uid(item, msginfo->msgnum);
	if (msgdata == NULL)
		return;

	oldname = get_filepath_for_msgdata(item, msgdata);

	newinfo = get_infostr(newflags);
	if (strcmp(msgdata->info, newinfo)) {
		g_free(msgdata->info);
		msgdata->info = newinfo;
		renamefile = TRUE;
	} else
		g_free(newinfo);

	newdir = g_strdup(newflags & MSG_NEW ? "new" : "cur");
	if (strcmp(msgdata->dir, newdir)) {
		g_free(msgdata->dir);
		msgdata->dir = newdir;
		renamefile = TRUE;
	} else
		g_free(newdir);

	if (renamefile) {
		gchar *newname;

		newname = get_filepath_for_msgdata(item, msgdata);
		if (rename(oldname, newname) == 0) {
			uiddb_delete_entry(item->db, msgdata->uid);
			uiddb_insert_entry(item->db, msgdata);
			msginfo->flags.perm_flags = newflags;
		}
		g_free(newname);
	} else {
		msginfo->flags.perm_flags = newflags;
	}

	g_free(oldname);
	uiddb_free_msgdata(msgdata);
}

static gboolean setup_new_folder(const gchar * path, gboolean subfolder)
{
	gchar *curpath, *newpath, *tmppath, *maildirfolder;
	gboolean failed = FALSE;

	g_return_val_if_fail(path != NULL, TRUE);

	curpath = g_strconcat(path, G_DIR_SEPARATOR_S, "cur", NULL);
	newpath = g_strconcat(path, G_DIR_SEPARATOR_S, "new", NULL);
	tmppath = g_strconcat(path, G_DIR_SEPARATOR_S, "tmp", NULL);

	if (!is_dir_exist(path))
		if (mkdir(path, 0777) != 0)
			failed = TRUE;
	if (!is_dir_exist(curpath))
		if (mkdir(curpath, 0777) != 0)
			failed = TRUE;
	if (!is_dir_exist(newpath))
		if (mkdir(newpath, 0777) != 0)
			failed = TRUE;
	if (!is_dir_exist(tmppath))
		if (mkdir(tmppath, 0777) != 0)
			failed = TRUE;

	if (subfolder) {
		int res;
		maildirfolder = g_strconcat(path, G_DIR_SEPARATOR_S, "maildirfolder", NULL);
		res = open(maildirfolder, O_WRONLY | O_CREAT | O_NONBLOCK | O_NOCTTY,
			   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (res != -1)
			close(res);
		else
			failed = TRUE;
	}

	if (failed) {
		rmdir(tmppath);
		rmdir(newpath);
		rmdir(curpath);
		rmdir(path);
	}
	    
	g_free(tmppath);
	g_free(newpath);
	g_free(curpath);

	return failed;
}

static FolderItem *maildir_create_folder(Folder * folder,
					 FolderItem * parent,
					 const gchar * name)
{
	gchar *folder_path, *path;
	FolderItem *newitem = NULL;
	gboolean failed = FALSE;

	g_return_val_if_fail(folder != NULL, NULL);
	g_return_val_if_fail(parent != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);

	folder_path = g_strdup(LOCAL_FOLDER(folder)->rootpath);
	g_return_val_if_fail(folder_path != NULL, NULL);

	if (g_path_is_absolute(folder_path)) {
		path = g_strconcat(folder_path, G_DIR_SEPARATOR_S,
                                   parent->path != NULL ? parent->path : "",
				   ".", name, NULL);
        } else {
                path = g_strconcat(get_home_dir(), G_DIR_SEPARATOR_S,
                                   folder_path, G_DIR_SEPARATOR_S,
                                   parent->path != NULL ? parent->path : "",
				   ".", name, NULL);
        }
	g_free(folder_path);

	debug_print("creating new maildir folder: %s\n", path);

	failed = setup_new_folder(path, TRUE);
	g_free(path);

	if (failed)
		return NULL;

	path = g_strconcat((parent->path != NULL) ? parent->path : "", ".", name, NULL);
	newitem = folder_item_new(folder, name, path);
	folder_item_append(parent, newitem);
	g_free(path);

	return newitem;
}

static gint maildir_create_tree(Folder *folder)
{
	gchar *rootpath, *folder_path;

	g_return_val_if_fail(folder != NULL, -1);

	folder_path = g_strdup(LOCAL_FOLDER(folder)->rootpath);
	g_return_val_if_fail(folder_path != NULL, -1);

	if (g_path_is_absolute(folder_path)) {
		rootpath = g_strdup(folder_path);
        } else {
                rootpath = g_strconcat(get_home_dir(), G_DIR_SEPARATOR_S,
                                   folder_path, NULL);
        }
	g_free(folder_path);

	debug_print("creating new maildir tree: %s\n", rootpath);
	if (!is_dir_exist(rootpath)) {
		if (is_file_exist(rootpath)) {
			g_warning("File `%s' already exists.\n"
				    "Can't create folder.", rootpath);
			return -1;
		}
		if (make_dir(rootpath) < 0)
			return -1;
	}

	if (setup_new_folder(rootpath, FALSE)) /* create INBOX */
		return -1;
	if (folder->outbox == NULL)
		if (maildir_create_folder(folder, folder->node->data, OUTBOX_DIR) == NULL)
			return -1;
	if (folder->queue == NULL)
		if (maildir_create_folder(folder, folder->node->data, QUEUE_DIR) == NULL)
			return -1;
	if (folder->draft == NULL)
		if (maildir_create_folder(folder, folder->node->data, DRAFT_DIR) == NULL)
			return -1;
	if (folder->trash == NULL)
		if (maildir_create_folder(folder, folder->node->data, TRASH_DIR) == NULL)
			return -1;

	return 0;
}

static gboolean remove_missing_folder_items_func(GNode *node, gpointer data)
{
	FolderItem *item;
	gchar *path;

	g_return_val_if_fail(node->data != NULL, FALSE);

	if (G_NODE_IS_ROOT(node))
		return FALSE;

	item = FOLDER_ITEM(node->data);

	if (item->stype == F_INBOX)
		return FALSE;

	path = folder_item_get_path(item);
	if (!is_dir_exist(path)) {
		debug_print("folder '%s' not found. removing...\n", path);
		folder_item_remove(item);
	}
	g_free(path);

	return FALSE;
}

static void remove_missing_folder_items(Folder *folder)
{
	g_return_if_fail(folder != NULL);

	debug_print("searching missing folders...\n");

	g_node_traverse(folder->node, G_POST_ORDER, G_TRAVERSE_ALL, -1,
			remove_missing_folder_items_func, folder);
}

static gboolean remove_folder_func(GNode *node, gpointer data)
{
	FolderItem *item;
	gchar *path;

	g_return_val_if_fail(node->data != NULL, FALSE);

	if (G_NODE_IS_ROOT(node))
		return FALSE;

	item = FOLDER_ITEM(node->data);

	if (item->stype != F_NORMAL)
		return FALSE;

	path = folder_item_get_path(item);
	debug_print("removing directory %s\n", path);
	if (remove_dir_recursive(path) < 0) {
		g_warning("can't remove directory `%s'\n", path);
		g_free(path);
		*(gint*)data = -1;
		return TRUE;
	}
	g_free(path);

	folder_item_remove(item);

	return FALSE;
}

static gint maildir_remove_folder(Folder *folder, FolderItem *item)
{
	gchar *folder_path, *path, *globpat;
	glob_t	globbuf;
	gint res=0;

	g_return_val_if_fail(folder != NULL, -1);
	g_return_val_if_fail(item != NULL, -1);
	g_return_val_if_fail(item->path != NULL, -1);
	g_return_val_if_fail(item->stype == F_NORMAL, -1);

	debug_print("removing folder %s\n", item->path);

	g_node_traverse(item->node, G_POST_ORDER, G_TRAVERSE_ALL, -1,
			remove_folder_func, &res);

	return res;
}

struct RenameData
{
	gint	 oldprefixlen;
	gchar	*newprefix;
};

static gboolean rename_folder_func(GNode *node, gpointer data)
{
	FolderItem *item;
	gchar *oldpath, *newpath, *newitempath;
	gchar *suffix;
	struct RenameData *renamedata = data;

	g_return_val_if_fail(node->data != NULL, FALSE);

	if (G_NODE_IS_ROOT(node))
		return FALSE;

	item = FOLDER_ITEM(node->data);

	if (item->stype != F_NORMAL)
		return FALSE;

	suffix = item->path + renamedata->oldprefixlen;

	oldpath = folder_item_get_path(item);
	newitempath = g_strconcat(renamedata->newprefix, suffix, NULL);
	newpath = g_strconcat(LOCAL_FOLDER(item->folder)->rootpath, G_DIR_SEPARATOR_S, newitempath, NULL);

	debug_print("renaming directory %s to %s\n", oldpath, newpath);

	if (rename(oldpath, newpath) < 0) {
		FILE_OP_ERROR(oldpath, "rename");
		g_free(newitempath);
	} else {
		g_free(item->path);
		item->path = newitempath;
	}

	g_free(oldpath);
	g_free(newpath);

	return FALSE;
}

static gint maildir_rename_folder(Folder *folder, FolderItem *item,
			     const gchar *name)
{
	struct RenameData renamedata;
	gchar *p;

	g_return_val_if_fail(folder != NULL, -1);
	g_return_val_if_fail(item != NULL, -1);
	g_return_val_if_fail(item->path != NULL, -1);
	g_return_val_if_fail(name != NULL, -1);

	debug_print("renaming folder %s to %s\n", item->path, name);

	g_free(item->name);
	item->name = g_strdup(name);

	renamedata.oldprefixlen = strlen(item->path);
	p = strrchr(item->path, '.');
	if (p)
		p = g_strndup(item->path, p - item->path + 1);
	else
		p = g_strdup(".");
	renamedata.newprefix = g_strconcat(p, name, NULL);
	g_free(p);

	g_node_traverse(item->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
			rename_folder_func, &renamedata);

	g_free(renamedata.newprefix);

	return 0;
}
