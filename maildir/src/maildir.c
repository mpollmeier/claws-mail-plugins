/*
 * Maildir Plugin -- Maildir++ support for Sylpheed
 * Copyright (C) 2003 Christoph Hohmann
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

#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>
#include <unistd.h>
#include <glib.h>
#include <db.h>

#include "utils.h"
#include "procmsg.h"
#include "procheader.h"
#include "folder.h"
#include "maildir.h"
#include "localfolder.h"

typedef struct _MaildirFolder MaildirFolder;
typedef struct _MaildirFolderItem MaildirFolderItem;
typedef struct _MaildirUID MaildirUID;

static Folder *maildir_folder_new(const gchar * name,
				  const gchar * folder);
static void maildir_folder_destroy(Folder * folder);
static gint maildir_scan_tree(Folder * folder);
static FolderItem *maildir_item_new(Folder * folder);
static void maildir_item_destroy(Folder * folder, FolderItem * item);
static gchar *maildir_item_get_path(Folder * folder, FolderItem * item);
static gint maildir_get_num_list(Folder * folder, FolderItem * item, MsgNumberList ** list,
				 gboolean *old_uids_valid);
static MsgInfo *maildir_get_msginfo(Folder * folder,
				    FolderItem * item, gint num);
static gchar *maildir_fetch_msg(Folder * folder, FolderItem * item,
				gint num);

FolderClass maildir_class;

struct _MaildirFolder
{
	LocalFolder folder;

	DB_ENV *dbenv;
};

struct _MaildirFolderItem
{
	FolderItem item;

	guint lastuid;
	DB *db_uidkey;
	DB *db_uniqkey;
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

		/* FolderItem functions */
		maildir_class.item_new = maildir_item_new;
		maildir_class.item_destroy = maildir_item_destroy;
		maildir_class.item_get_path = maildir_item_get_path;
		maildir_class.get_num_list = maildir_get_num_list;

		/* Message functions */
		maildir_class.get_msginfo = maildir_get_msginfo;
		maildir_class.fetch_msg = maildir_fetch_msg;
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

	db_env_create(&folder->dbenv, 0);
	folder->dbenv->open(folder->dbenv, get_tmp_dir(), DB_INIT_MPOOL | DB_INIT_CDB | DB_CREATE, 0600);

        return FOLDER(folder);
}

static void maildir_folder_destroy(Folder *_folder)
{
	MaildirFolder *folder = (MaildirFolder *) _folder;

	folder->dbenv->close(folder->dbenv, 0);

	folder_local_folder_destroy(LOCAL_FOLDER(folder));
}

int getuniq(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey)
{
	memset(skey, 0, sizeof(DBT));
	skey->data = pdata->data;
	skey->size = strlen((gchar *) pdata->data);

	return 0;
}

static gint open_database(MaildirFolderItem *item)
{
	gint ret;
	gchar *path, *database;

	path = maildir_item_get_path(FOLDER_ITEM(item)->folder, FOLDER_ITEM(item));
	Xstrcat_a(database, path, "/sylpheed_uid.db", return -1);
	g_free(path);

	/* open uid key based database */
	if ((ret = db_create(&item->db_uidkey, ((MaildirFolder *) FOLDER_ITEM(item)->folder)->dbenv, 0)) != 0) {
		debug_print("db_create: %s\n", db_strerror(ret));
		return -1;
	}
	if ((ret = item->db_uidkey->open(item->db_uidkey, NULL, database, "uidkey", DB_BTREE, DB_CREATE, 0600)) != 0) {
		debug_print("DB->open: %s\n", db_strerror(ret));
		item->db_uidkey->close(item->db_uidkey, 0);
		return -1;
	}
	debug_print("UID based database opened\n");

	/* open uniq key based database */
	if ((ret = db_create(&item->db_uniqkey, ((MaildirFolder *) FOLDER_ITEM(item)->folder)->dbenv, 0)) != 0) {
		debug_print("db_create: %s\n", db_strerror(ret));
		return -1;
	}
	if ((ret = item->db_uniqkey->open(item->db_uniqkey, NULL, database, "uniqkey", DB_BTREE, DB_CREATE, 0600)) != 0) {
		debug_print("DB->open: %s\n", db_strerror(ret));
		item->db_uniqkey->close(item->db_uidkey, 0);
		return -1;
	}
	debug_print("Uniq based database opened\n");

	if ((ret = item->db_uidkey->associate(item->db_uidkey, NULL, item->db_uniqkey, getuniq, 0)) != 0) {
		debug_print("DB->associate: %s\n", db_strerror(ret));
		item->db_uidkey->close(item->db_uidkey, 0);
		item->db_uniqkey->close(item->db_uidkey, 0);
		return -1;
	}
	debug_print("Databases associated\n");

	return 0;
}

static void build_tree(GNode *node, glob_t *globbuf)
{
        int i;
	FolderItem *parent = FOLDER_ITEM(node->data);
	gchar *prefix = parent->path ?  parent->path : "";

        for (i = 0; i < globbuf->gl_pathc; i++) {
		FolderItem *newitem;
		GNode *newnode;
		gchar *tmpstr;
		gboolean res;

                if (globbuf->gl_pathv[i][0] == '.' && globbuf->gl_pathv[i][1] == '\0')
                        continue;

                if (strncmp(globbuf->gl_pathv[i], prefix, strlen(prefix)))
                        continue;

                if (globbuf->gl_pathv[i][strlen(prefix)] != '.')
                        continue;

                if (strchr(&(globbuf->gl_pathv[i][strlen(prefix) + 1]), '.') != NULL)
                        continue;

                if (!is_dir_exist(globbuf->gl_pathv[i]))
                        continue;

		tmpstr = g_strconcat(globbuf->gl_pathv[i], "/cur", NULL);
                res = is_dir_exist(tmpstr);
		g_free(tmpstr);
		if (!res)
			continue;

		newitem = folder_item_new(parent->folder, &(globbuf->gl_pathv[i][strlen(prefix) + 1]), globbuf->gl_pathv[i]);
		newitem->folder = parent->folder;

		newnode = g_node_new(newitem);
		newitem->node = newnode;
		g_node_append(node, newnode);

                debug_print("added item %s\n", newitem->path);
                build_tree(newnode, globbuf);
        }
}

static gint maildir_scan_tree(Folder *folder)
{
        FolderItem *rootitem, *inboxitem;
        gchar *rootpath;
	GNode *rootnode, *inboxnode;
        glob_t globbuf;
        
        g_return_if_fail(folder != NULL);
        
        rootitem = folder_item_new(folder, folder->name, NULL);
        rootitem->folder = folder;
	rootnode = g_node_new(rootitem);
        folder->node = rootnode;
	rootitem->node = rootnode;

	/* Add inbox folder */
	inboxitem = folder_item_new(folder, "inbox", "");
	inboxitem->folder = folder;
	inboxitem->stype = F_INBOX;
	inboxnode = g_node_new(inboxitem);
	inboxitem->node = inboxnode;
	g_node_append(rootnode, inboxnode);

	chdir(LOCAL_FOLDER(folder)->rootpath);
	globbuf.gl_offs = 0;
	glob(".*", 0, NULL, &globbuf);
	build_tree(rootnode, &globbuf);

	return 0;
}

static FolderItem *maildir_item_new(Folder *folder)
{
        MaildirFolderItem *item;

        item = g_new0(MaildirFolderItem, 1);
        item->lastuid = 0;
	item->db_uidkey = NULL;
	item->db_uniqkey = NULL;
        
        return (FolderItem *) item;

}

static void maildir_item_destroy(Folder *folder, FolderItem *_item)
{
        MaildirFolderItem *item = (MaildirFolderItem *)_item;

        g_return_if_fail(item != NULL);

	if (item->db_uidkey != NULL)
		item->db_uidkey->close(item->db_uidkey, 0);
	if (item->db_uniqkey != NULL)
		item->db_uniqkey->close(item->db_uniqkey, 0);

        g_free(item);
}

static gchar *maildir_item_get_path(Folder *folder, FolderItem *item)
{
	gchar *folder_path, *path;

	g_return_val_if_fail(folder != NULL, NULL);
	g_return_val_if_fail(item != NULL, NULL);

	folder_path = g_strdup(LOCAL_FOLDER(folder)->rootpath);
	g_return_val_if_fail(folder_path != NULL, NULL);

	if (folder_path[0] == G_DIR_SEPARATOR) {
                if (item->path)
                        path = g_strconcat(folder_path, G_DIR_SEPARATOR_S,
                                           item->path, NULL);
                else
                        path = g_strdup(folder_path);
        } else {
                if (item->path)
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

static void find_lastuid_in_database(MaildirFolderItem *item)
{
	DBC *cursor;
	DBT key, data;
	gint ret;

	if (item->db_uidkey == NULL)
		if (open_database(item) < 0) return;

	ret = item->db_uidkey->cursor(item->db_uidkey, NULL, &cursor, 0);
	if (ret != 0) {
		debug_print("DB->cursor: %s\n", db_strerror(ret));
		return;
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	while ((ret = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0) {
		guint32 uid = *((guint32 *) key.data);

		if (uid > item->lastuid)
			item->lastuid = uid;		

		memset(&key, 0, sizeof(key));
		memset(&data, 0, sizeof(data));
	}

	cursor->c_close(cursor);
}

static guint32 get_uid_for_filename(MaildirFolderItem *item, const gchar *filename)
{
	MaildirUID *maildiruid;
	GSList *elem;
	gchar *tmpfilename, *uniq, *info;
	DBT key, pkey, data;
	gint ret;

	if (item->db_uidkey == NULL)
		if (open_database(item) < 0) return 0;

	g_return_val_if_fail(item->db_uniqkey != NULL, 0);

	Xstrdup_a(tmpfilename, filename, return 0);
	uniq = tmpfilename;
	info = strchr(tmpfilename, ':');
	if (info) {
		*info = '\0';
		info++;
	}

	memset(&key, 0, sizeof(key));
	memset(&pkey, 0, sizeof(pkey));
	memset(&data, 0, sizeof(data));
	key.data = uniq;
	key.size = strlen(uniq);

	switch (ret = item->db_uniqkey->pget(item->db_uniqkey, NULL, &key, &pkey, &data, 0)) {
	case 0:
		return *((guint32 *) pkey.data);
		break;
	case DB_NOTFOUND:
		memset(&key, 0, sizeof(key));
		memset(&data, 0, sizeof(data));

		if (item->lastuid == 0)
			find_lastuid_in_database(item);

		item->lastuid++;
		key.size = sizeof(guint32);
		key.data = &item->lastuid;

		data.size = strlen(uniq) + strlen(info) + 2;
		data.data = alloca(data.size);
		strcpy(data.data, uniq);
		strcpy(data.data + strlen(uniq) + 1, info);

		debug_print("creating new database entry %d:%s\n", item->lastuid, uniq);

		ret = item->db_uidkey->put(item->db_uidkey, NULL, &key, &data, 0);
		if (ret != 0)
			debug_print("DB->put: %s\n", db_strerror(ret));

		/*
		ret = item->db_uidkey->sync(item->db_uidkey, 0);
		ret = item->db_uniqkey->sync(item->db_uniqkey, 0);
		*/

		return item->lastuid;
	default:
		debug_print("unexpected error: %s\n", db_strerror(ret));
	}

	return 0;
}

static gchar *get_filename_for_uid(MaildirFolderItem *item, guint32 uid)
{
	GSList *elem;
	DBT key, data;
	gchar *uniq, *info, *path, *filename;
	glob_t globbuf;
	gint ret;

	if (item->db_uidkey == NULL)
		if (open_database(item) < 0) return NULL;

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.size = sizeof(guint32);
	key.data = &uid;

	if (item->db_uidkey->get(item->db_uidkey, NULL, &key, &data, 0) != 0)
		return NULL;
	uniq = data.data;
	info = data.data + strlen(uniq) + 1;

	path = maildir_item_get_path(FOLDER_ITEM(item)->folder, FOLDER_ITEM(item));
	filename = g_strconcat(path, "/cur/", uniq, ":", info, NULL);
	g_free(path);

	if (is_file_exist(filename))
		return filename;

	debug_print("researching for %s\n", uniq);
	/* delete old entry */
	g_free(filename);
	item->db_uidkey->del(item->db_uidkey, NULL, &key, 0);
	if (ret != 0)
		debug_print("DB->del: %s\n", db_strerror(ret));

	/* try to find file with same uniq and different info */
	path = maildir_item_get_path(FOLDER_ITEM(item)->folder, FOLDER_ITEM(item));
	filename = g_strconcat(path, "/cur/", uniq, ":*", NULL);
	debug_print("search pattern is %s\n", filename);
	g_free(path);
	globbuf.gl_offs = 0;
	glob(filename, 0, NULL, &globbuf);
	g_free(filename);
	
	filename = NULL;
	if (globbuf.gl_pathc > 0)
		filename = g_strdup(globbuf.gl_pathv[0]);
	globfree(&globbuf);

	/* if found: update database and return new entry */
	if (filename != NULL) {
		debug_print("found %s\n", filename);
		info = strrchr(filename, ':') + 1;
		
		memset(&data, 0, sizeof(data));
		data.size = strlen(uniq) + strlen(info) + 2;
		data.data = alloca(data.size);
		strcpy(data.data, uniq);
		strcpy(data.data + strlen(uniq) + 1, info);

		ret = item->db_uidkey->put(item->db_uidkey, NULL, &key, &data, 0);
		if (ret != 0)
			debug_print("DB->put: %s\n", db_strerror(ret));
	}

	return filename;
}

static void cleanup_database(MaildirFolderItem *item, MsgNumberList *list)
{
	DBC *cursor;
	DBT key, data;
	gint ret;

	if (item->db_uidkey == NULL)
		if (open_database(item) < 0) return;

	ret = item->db_uidkey->cursor(item->db_uidkey, NULL, &cursor, DB_WRITECURSOR);
	if (ret != 0) {
		debug_print("DB->cursor: %s\n", db_strerror(ret));
		return;
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	while ((ret = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0) {
		guint32 uid = *((guint32 *) key.data);

		if (g_slist_find(list, GINT_TO_POINTER(uid)) == NULL) {
			cursor->c_del(cursor, 0);
			debug_print("removed old database entry %d:%s\n", uid, data.data);
		}

		memset(&key, 0, sizeof(key));
		memset(&data, 0, sizeof(data));
	}

	cursor->c_close(cursor);
}

static gint maildir_get_num_list(Folder * folder, FolderItem * item,
				 MsgNumberList ** list, gboolean *old_uids_valid)
{
	gchar *path, *cur;
	glob_t globbuf;
	int i;

	*old_uids_valid = TRUE;

	path = maildir_item_get_path(folder, item);
	cur = g_strconcat(path, "/cur", NULL);
	g_free(path);
	chdir(cur);
	g_free(cur);

	globbuf.gl_offs = 0;
	glob("*", 0, NULL, &globbuf);
	for (i = 0; i < globbuf.gl_pathc; i++) {
		guint32 uid;

		uid = get_uid_for_filename((MaildirFolderItem *) item, globbuf.gl_pathv[i]);
		if (uid != 0)
			*list = g_slist_append(*list, GINT_TO_POINTER(uid));
	}
	globfree(&globbuf);

	cleanup_database((MaildirFolderItem *) item, *list);

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

	filename = get_filename_for_uid((MaildirFolderItem *) item, num);
	return filename;
}
