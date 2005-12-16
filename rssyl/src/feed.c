/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2004 Hiroyuki Yamamoto
 * This file (C) 2005 Andrej Kacian <andrej@kacian.sk>
 *
 * - various feed parsing functions
 * - this file could use some sorting and/or splitting
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

#include <glib.h>
#include <gtk/gtk.h>

#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <pthread.h>

#include "common/utils.h"
#include "common/sylpheed.h"
#include "codeconv.h"
#include "procmsg.h"
#include "procheader.h"
#include "alertpanel.h"
#include "folder.h"
#include "mainwindow.h"
#include "statusbar.h"

#include "gettext.h"

#include "date.h"
#include "rssyl.h"
#include "rssyl_cb_gtk.h"
#include "feed.h"
#include "feedprops.h"
#include "strreplace.h"
#include "parsers.h"
#include "plugin_version.h"

struct _RSSylThreadCtx {
	const gchar *url;
	gboolean ready;
};

typedef struct _RSSylThreadCtx RSSylThreadCtx;

static void *rssyl_fetch_feed_threaded(void *arg)
{
	RSSylThreadCtx *ctx = (RSSylThreadCtx *)arg;
	CURL *eh = NULL;
	CURLcode res;
	gchar *template = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, RSSYL_DIR,
			G_DIR_SEPARATOR_S, "feedXXXXXX", NULL);
	int fd = mkstemp(template);
	FILE *f;

	if (fd == -1) {
		perror("mkstemp");
		ctx->ready = TRUE;
		g_free(template);
		return NULL;
	}

	f = fdopen(fd, "w");
	if (f == NULL) {
		perror("fdopen");
		ctx->ready = TRUE;
		g_free(template);
		return NULL;
	}

	eh = curl_easy_init();
	if (eh == NULL) {
		g_warning("can't init curl");
		ctx->ready = TRUE;
		g_free(template);
		return NULL;
	}

	curl_easy_setopt(eh, CURLOPT_URL, ctx->url);
	curl_easy_setopt(eh, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(eh, CURLOPT_MUTE, 1);
	curl_easy_setopt(eh, CURLOPT_WRITEDATA, f);
	curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(eh, CURLOPT_MAXREDIRS, 3);
	curl_easy_setopt(eh, CURLOPT_USERAGENT,
		"Sylpheed-Claws RSSyl plugin "PLUGINVERSION
		" (http://claws.sylpheed.org/plugins.php)");

	res = curl_easy_perform(eh);

	curl_easy_cleanup(eh);

	fclose(f);

	ctx->ready = TRUE;

	return template;
}

/* rssyl_fetch_feed()
 *
 * This function utilizes libcurl's easy interface to fetch the feed, pre-parse
 * it for title, create a directory based on the title. It returns a xmlDocPtr
 * for libxml2 to parse further.
 */
xmlDocPtr rssyl_fetch_feed(const gchar *url, gchar **title) {
	gchar *xpath, *rootnode, *dir;
	xmlDocPtr doc;
	xmlNodePtr node, n;
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	RSSylThreadCtx *ctx = g_new0(RSSylThreadCtx, 1);
	void *template = NULL;
#ifdef USE_PTHREAD
	pthread_t pt;
#endif
	gchar * msg = NULL, *tmp;
	gchar *content;
	
	ctx->url = url;
	ctx->ready = FALSE;

	*title = NULL;

	g_return_val_if_fail(url != NULL, NULL);

	debug_print("RSSyl: XML - url is '%s'\n", url);

	msg = g_strdup_printf(_("Fetching '%s'..."), url);
	STATUSBAR_PUSH(mainwin, msg );
	g_free(msg);

	GTK_EVENTS_FLUSH();

#ifdef USE_PTHREAD
	if( pthread_create(&pt, PTHREAD_CREATE_JOINABLE, rssyl_fetch_feed_threaded,
				(void *)ctx) != 0 ) {
		/* Bummer, couldn't create thread. Continue non-threaded */
		template = rssyl_fetch_feed_threaded(ctx);
	} else {
		/* Thread created, let's wait until it finishes */
		debug_print("RSSyl: waiting for thread to finish\n");
		while( !ctx->ready )
			sylpheed_do_idle();
		debug_print("RSSyl: thread finished\n");

		pthread_join(pt, &template);
	}
#else
	debug_print("RSSyl: no pthreads, run blocking fetch\n");
	(gchar *)template = rssyl_fetch_feed_threaded(ctx);
#endif

	g_free(ctx);
	STATUSBAR_POP(mainwin);

	g_return_val_if_fail (template != NULL, NULL);
	
		
	/* Strip ugly \r\n endlines */
	file_strip_crs((gchar *)template);

	doc = xmlParseFile(template);
	g_remove((gchar *)template);
	g_free(template);

	g_return_val_if_fail(doc != NULL, NULL);

	node = xmlDocGetRootElement(doc);

	debug_print("RSSyl: XML - root node is '%s'\n", node->name);

	rootnode = g_ascii_strdown(node->name, -1);

	if( !xmlStrcmp(rootnode, "rss") ) {
		context = xmlXPathNewContext(doc);
		xpath = g_strconcat("/", node->name, "/channel/title",	NULL);
		debug_print("RSSyl: XML - '%s'\n", xpath);
		if( !(result = xmlXPathEvalExpression(xpath, context)) ) {
			debug_print("RSSyl: XML - no result found for '%s'\n", xpath);
			xmlXPathFreeContext(context);
			g_free(rootnode);
			g_free(xpath);
			return NULL;
		}

		if( xmlXPathNodeSetIsEmpty(result->nodesetval) ) {
			debug_print("RSSyl: XML - nodeset empty for '%s'\n", xpath);
			g_free(rootnode);
			g_free(xpath);
			xmlXPathFreeObject(result);
			xmlXPathFreeContext(context);
			return NULL;
		}
		g_free(xpath);

		xmlXPathFreeContext(context);
		node = result->nodesetval->nodeTab[0];
		xmlXPathFreeObject(result);
		content = xmlNodeGetContent(node);
		debug_print("RSSyl: XML - title is '%s'\n", content );
		*title = g_strdup(content);
		xmlFree(content);
		debug_print("RSSyl: XML - our title is '%s'\n", *title);
	} else if( !xmlStrcmp(rootnode, "rdf") ) {
		node = node->children;
		/* find "channel" */
		while( node && xmlStrcmp(node->name, "channel") )
			node = node->next;
		/* now find "title" */
		for( n = node->children; n; n = n->next ) {
			if( !xmlStrcmp(n->name, "title") ) {
				content = xmlNodeGetContent(n);
				*title = g_strdup(content);
				xmlFree(content);
				debug_print("RSSyl: XML - RDF our title is '%s'\n", *title);
			}
		}
	} else if( !xmlStrcmp(rootnode, "feed") ) {
		/* find "title" */
		for( n = node->children; n; n = n->next ) {
			if( !xmlStrcmp(n->name, "title") ) {
				content = xmlNodeGetContent(n);
				*title = g_strdup(content);
				xmlFree(content);
				debug_print("RSSyl: XML - FEED our title is '%s'\n", *title);
			}
		}
	} else {
		g_warning("Unsupported feed type.\n");
		g_free(rootnode);
		return NULL;
	}

	g_return_val_if_fail(*title != NULL, NULL);

	tmp = rssyl_strreplace(*title, "/", "\\");
	dir = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, RSSYL_DIR,
			G_DIR_SEPARATOR_S, tmp, NULL);
	g_free(tmp);
	if( !is_dir_exist(dir) ) {
		if( make_dir(dir) < 0 ) {
			g_warning("couldn't create directory %s\n", dir);
			g_free(rootnode);
			g_free(dir);
			return NULL;
		}
	}

	g_free(rootnode);
	g_free(dir);

	return doc;
}

/* rssyl_parse_folder_item_file()
 *
 * Parse the RFC822-formatted feed item given by "path", and returns a
 * pointer to a RSSylFeedItem struct, which contains all required data.
 */
static RSSylFeedItem *rssyl_parse_folder_item_file(gchar *path)
{
	gchar *contents, **lines, **line, *text;
	GError *error = NULL;
	RSSylFeedItem *fitem;
	gint i = 0;
	gboolean parsing_headers, past_html_tag, past_endhtml_tag;
	gboolean started_author = FALSE, started_subject = FALSE;
	gboolean started_link = FALSE;

	debug_print("RSSyl: parsing '%s'\n", path);

	g_file_get_contents(path, &contents, NULL, &error);

	if( error )
		g_warning("GError: '%s'\n", error->message);

	if( contents ) {
		lines = g_strsplit(contents, "\n", 0);
	} else {
		g_warning("Badly formatted file found, ignoring: '%s'\n", path);
		g_free(contents);
		return NULL;
	}

	fitem = g_new0(RSSylFeedItem, 1); /* free that */
	fitem->date = -1;
	fitem->link = NULL;
	fitem->text = NULL;
	fitem->realpath = g_strdup(path);

	parsing_headers = TRUE;
	past_html_tag = FALSE;
	past_endhtml_tag = FALSE;
	while(lines[i] ) {
		if( parsing_headers && lines[i] && !strlen(lines[i]) && fitem->link ) {
			parsing_headers = FALSE;
			debug_print("RSSyl: finished parsing headers\n");
		}

		if( parsing_headers ) {
			line = g_strsplit(lines[i], ": ", 2);
			if( line[0] && line[1] && strlen(line[0]) && lines[i][0] != ' ') {
				started_author = FALSE;
				started_subject = FALSE;
				started_link = FALSE;
				/* Author */
				if( !strcmp(line[0], "From") ) {
					fitem->author = g_strdup(line[1]);
					debug_print("RSSyl: got author '%s'\n", fitem->author);
					started_author = TRUE;
				}

				/* Date */
				if( !strcmp(line[0], "Date") ) {
					fitem->date = parseRFC822Date(line[1]);
					debug_print("RSSyl: got date \n" );
				}

				/* Title */
				if( !strcmp(line[0], "Subject") ) {
					fitem->title = g_strdup(line[1]);
					debug_print("RSSyl: got title '%s'\n", fitem->title);
					started_subject = TRUE;
				}

				/* Link */
				if( !strcmp(line[0], "X-RSSyl-URL") ) {
					fitem->link = g_strdup(line[1]);
					debug_print("RSSyl: got link '%s'\n", fitem->link);
					started_link = TRUE;
				}
			} else if (lines[i][0] == ' ') {
				gchar *tmp = NULL;
				/* continuation line */
				if (started_author) {
					tmp = g_strdup_printf("%s %s", fitem->author, lines[i]+1);
					g_free(fitem->author);
					fitem->author = tmp;
					debug_print("RSSyl: updated author to '%s'\n", fitem->author);
				} else if (started_subject) {
					tmp = g_strdup_printf("%s %s", fitem->title, lines[i]+1);
					g_free(fitem->title);
					fitem->title = tmp;
					debug_print("RSSyl: updated title to '%s'\n", fitem->title);
				} else if (started_link) {
					tmp = g_strdup_printf("%s%s", fitem->link, lines[i]+1);
					g_free(fitem->link);
					fitem->link = tmp;
					debug_print("RSSyl: updated link to '%s'\n", fitem->link);
				}
			}
			g_strfreev(line);
		} else {
			if( !strcmp(lines[i], RSSYL_TEXT_START) ) {
				debug_print("Leading html tag found at line %d\n", i);
				past_html_tag = TRUE;
				i++;
				continue;
			}
			while( past_html_tag && !past_endhtml_tag && lines[i] ) {
				if( !strcmp(lines[i], RSSYL_TEXT_END) ) {
					debug_print("Trailing html tag found at line %d\n", i);
					past_endhtml_tag = TRUE;
					i++;
					continue;
				}
				if( fitem->text != NULL ) {
					text = g_strdup(fitem->text);
					g_free(fitem->text);
					fitem->text = g_strconcat(text, "\n", lines[i], NULL);
					g_free(text);
				}	else {
					fitem->text = g_strdup(lines[i]);
				}
				i++;
			}

			if( lines[i] == NULL )
				return fitem;
		}

		i++;
	}
	g_strfreev(lines);
	g_free(contents);
	return fitem;
}

/* rssyl_free_feeditem()
 * frees an RSSylFeedItem
 */
void rssyl_free_feeditem(RSSylFeedItem *item)
{
	if (!item)
		return;
	g_free(item->title);
	item->title = NULL;
	g_free(item->text);
	item->text = NULL;
	g_free(item->link);
	item->link = NULL;
	g_free(item->author);
	item->author = NULL;
	g_free(item->realpath);
	item->realpath = NULL;
	g_free(item);
}

/* rssyl_read_existing()
 *
 * Parse all existing folder items, storing their data in memory. Data is
 * later used for checking for duplicate entries.
 */
void rssyl_read_existing(RSSylFolderItem *ritem)
{
	FolderItem *item = &ritem->item;
	RSSylFeedItem *fitem = NULL;
	DIR *dp;
	struct dirent *d;
	gint num;
	gchar *path;

	debug_print("RSSyl: rssyl_read_existing()\n");

	g_return_if_fail(ritem != NULL);

	path = folder_item_get_path(item);
	g_return_if_fail(path != NULL);

	/* create a new GSList, throw away the old one */
	if( ritem->contents != NULL ) {
		GSList *cur;
		for (cur = ritem->contents; cur; cur = cur->next) {
			RSSylFeedItem *olditem = (RSSylFeedItem *)cur->data;
			rssyl_free_feeditem(olditem);
		}
		g_slist_free(ritem->contents); /* leak fix here */
		ritem->contents = NULL;
	}
	ritem->contents = g_slist_alloc();

	if( change_dir(path) < 0 ) {
		g_free(path);
		return;
	}

	if( (dp = opendir(".")) == NULL ) {
		FILE_OP_ERROR(item->path, "opendir");
		g_free(path);
		return;
	}

	while( (d = readdir(dp)) != NULL ) {
		if( (num = to_number(d->d_name)) > 0 && dirent_is_regular_file(d) ) {
			debug_print("RSSyl: starting to parse '%s'\n", d->d_name);
			if( (fitem = rssyl_parse_folder_item_file(d->d_name)) != NULL ) {
				debug_print("Appending '%s'\n", fitem->title);
				g_slist_append(ritem->contents, fitem);
			}
		}
	}
	closedir(dp);
	g_free(path);

	debug_print("RSSyl: rssyl_read_existing() is returning\n");
}

/* rssyl_cb_feed_compare()
 *
 * Callback compare function called by glib2's g_slist_find_custom().
 */
static gint rssyl_cb_feed_compare(const RSSylFeedItem *a,
		const RSSylFeedItem *b)
{
	if( a == NULL || b == NULL )
		return 1;
	
	return strcmp(a->link, b->link);
}

static gboolean rssyl_feed_item_changed(RSSylFeedItem *old_item, RSSylFeedItem *new_item)
{
	if( old_item->text && new_item->text ) {
		if( strcmp(old_item->text, new_item->text) )
			return TRUE;
	} else {
		if( old_item->text || new_item->text )
			return TRUE;
	}

	if( old_item->title && new_item->title ) {
		gchar *old = conv_unmime_header(old_item->title, CS_UTF_8);
		gchar *new = conv_unmime_header(new_item->title, CS_UTF_8);
		if( strcmp(old, new) ) {
			g_free(old);
			g_free(new);
			return TRUE;
		}
		g_free(old);
		g_free(new);
	} else {
		if( old_item->title || new_item->title )
			return TRUE;
	}

	if( old_item->author && new_item->author ) {
		gchar *old = conv_unmime_header(old_item->author, CS_UTF_8);
		gchar *new = conv_unmime_header(new_item->author, CS_UTF_8);
		if( strcmp(old, new) ) {
			g_free(old);
			g_free(new);
			return TRUE;
		}
		g_free(old);
		g_free(new);
	} else {
		if( old_item->author || new_item->author )
			return TRUE;
	}

	return FALSE;
}

/* rssyl_feed_item_exists()
 *
 * Returns 1 if a feed item already exists locally, 2 if there's a changed
 * item with link that already belongs to existing item, 0 if item is new.
 */
static guint rssyl_feed_item_exists(RSSylFolderItem *ritem,
		RSSylFeedItem *fitem, RSSylFeedItem **oldfitem)
{
	GSList *item = NULL;
	RSSylFeedItem *efitem = NULL;
	g_return_val_if_fail(ritem != NULL, FALSE);
	g_return_val_if_fail(fitem != NULL, FALSE);

	if( ritem->contents == NULL || g_slist_length(ritem->contents) == 0 )
		return 0;

	if( (item = g_slist_find_custom(ritem->contents,
					(gconstpointer)fitem, (GCompareFunc)rssyl_cb_feed_compare)) ) {
		efitem = (RSSylFeedItem *)item->data;
		if(  rssyl_feed_item_changed(efitem, fitem) ) {
			*oldfitem = efitem;
			return 2;
		}
		return 1;
	}

	return 0;
}

void rssyl_parse_feed(xmlDocPtr doc, RSSylFolderItem *ritem)
{
	xmlNodePtr node;
	gchar *rootnode;
	MainWindow *mainwin = mainwindow_get_mainwindow();
	gint count;
	gchar *msg;
	
	rssyl_read_existing(ritem);

	node = xmlDocGetRootElement(doc);
	g_return_if_fail(doc != NULL);

	debug_print("RSSyl: XML - root node is '%s'\n", node->name);
	rootnode = g_ascii_strdown(node->name, -1);

	msg = g_strdup_printf(_("Refreshing feed '%s'..."),
				ritem->item.name);
	STATUSBAR_PUSH(mainwin, msg );
	g_free(msg);
	GTK_EVENTS_FLUSH();

	folder_item_update_freeze();

	if( !strcmp(rootnode, "rss") ) {
		debug_print("RSSyl: XML - calling rssyl_parse_rss()\n");
		count = rssyl_parse_rss(doc, ritem);
	} else if( !strcmp(rootnode, "rdf") ) {
		debug_print("RSSyl: XML - calling rssyl_parse_rdf()\n");
		count = rssyl_parse_rdf(doc, ritem);
	} else if( !strcmp(rootnode, "feed") ) {
		debug_print("RSSyl: XML - calling rssyl_parse_atom()\n");
		count = rssyl_parse_atom(doc, ritem);
	} else {
		alertpanel_error(_("This feed format is not supported yet."));
		count = 0;
	}

	ritem->last_count = count;

	folder_item_scan(&ritem->item);
	folder_item_update_thaw();

	STATUSBAR_POP(mainwin);

	g_free(rootnode);
}

gboolean rssyl_add_feed_item(RSSylFolderItem *ritem, RSSylFeedItem *fitem)
{
	MsgFlags *flags;
	gchar *template;
	gchar tmp[512];
	gint d, fd, dif = 0;
	FILE *f;
	RSSylFeedItem *oldfitem = NULL;

	g_return_val_if_fail(ritem != NULL, FALSE);
	g_return_val_if_fail(ritem->item.path != NULL, FALSE);
	g_return_val_if_fail(fitem != NULL, FALSE);

	/* Adjust some fields */
	if( fitem->date <= 0 )
		fitem->date = time(NULL);

	if( !fitem->author )
		fitem->author = g_strdup(_("N/A"));

	/* Skip if the item already exists */
	dif = rssyl_feed_item_exists(ritem, fitem, &oldfitem);
	if( dif == 1 ) {
		debug_print("RSSyl: This item already exists, skipping...\n");
		return FALSE;
	}
	if( dif == 2 && oldfitem != NULL ) {
		debug_print("RSSyl: Item changed, removing old one and adding new...\n");
		g_slist_remove(ritem->contents, oldfitem);
		g_remove(oldfitem->realpath);
		rssyl_free_feeditem(oldfitem);
		oldfitem = NULL;
	}

	debug_print("RSSyl: Adding item '%s'\n", fitem->title);

	g_slist_append(ritem->contents, fitem);

	flags = g_new(MsgFlags, 1);
	template = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, RSSYL_DIR,
			G_DIR_SEPARATOR_S, "feedXXXXXX", NULL);
	fd = mkstemp(template);

	f = fdopen(fd, "w");
	g_return_val_if_fail(f != NULL, FALSE);

	if( fitem->date != -1 ) {
		gchar *tmpdate = createRFC822Date(&fitem->date);
		fprintf(f, "Date: %s\n", tmpdate );
		g_free(tmpdate);
	}

	if( fitem->author ) {
		if (g_utf8_validate(fitem->author, -1, NULL)) {
			conv_encode_header_full(tmp, 511, fitem->author, 
				strlen(fitem->author), TRUE, CS_UTF_8);
			fprintf(f, "From: %s\n", tmp);
		} else
			fprintf(f, "From: %s\n", fitem->author);
	} 
	if( fitem->title ) {
		if (g_utf8_validate(fitem->title, -1, NULL)) {
			conv_encode_header_full(tmp, 511, fitem->title, 
				strlen(fitem->author), FALSE, CS_UTF_8);
			fprintf(f, "Subject: %s\n", tmp);
		} else
			fprintf(f, "Subject: %s\n", fitem->title);
	}

	if( fitem->link )
		fprintf(f, "X-RSSyl-URL: %s\n", fitem->link);

	if (fitem->text && g_utf8_validate(fitem->text, -1, NULL)) {
		/* if it passes UTF-8 validation, specify it. */
		fprintf(f, "Content-Type: text/html; charset=UTF-8\n\n");		
	} else {
		/* make sure Sylpheed-Claws displays it as html */
		fprintf(f, "Content-Type: text/html\n\n");
	}

	if( fitem->link )
		fprintf(f, "URL: <a href=\"%s\">%s</a>\n\n<br><br>\n",
				fitem->link, fitem->link);

	if( fitem->text )
		fprintf(f, "<html><head></head><body>\n"
				RSSYL_TEXT_START"\n"
				"%s\n"
				RSSYL_TEXT_END"\n"
				"</body></html>",
				fitem->text);

	fclose(f);

	flags->perm_flags = MSG_NEW | MSG_UNREAD;
	flags->tmp_flags = 0;

	g_return_val_if_fail(template != NULL, FALSE);
	d = folder_item_add_msg(&ritem->item, template, flags, TRUE);
	g_free(template);

	debug_print("RSSyl: folder_item_add_msg(): %d\n", d);

	return TRUE;
}

MsgInfo *rssyl_parse_feed_item_to_msginfo(gchar *file, MsgFlags flags,
		gboolean a, gboolean b, FolderItem *item)
{
	MsgInfo *msginfo;

	g_return_val_if_fail(item != NULL, NULL);

	msginfo = procheader_parse_file(file, flags, a, b);
	msginfo->folder = item;

	return msginfo;
}

void rssyl_remove_feed_cache(FolderItem *item)
{
	gchar *path;
	DIR *dp;
	struct dirent *d;
	gint num = 0;

	g_return_if_fail(item != NULL);

	debug_print("Removing local cache for '%s'\n", item->name);

	path = folder_item_get_path(item);
	g_return_if_fail(path != NULL);
	if( change_dir(path) < 0 ) {
		g_free(path);
		return;
	}

	debug_print("Emptying '%s'\n", path);

	if( (dp = opendir(".")) == NULL ) {
		FILE_OP_ERROR(item->path, "opendir");
		return;
	}

	while( (d = readdir(dp)) != NULL ) {
		g_remove(d->d_name);
		num++;
	}
	closedir(dp);

	debug_print("Removed %d files\n", num);

	g_remove(path);
	g_free(path);
}

void rssyl_update_feed(RSSylFolderItem *ritem)
{
	gchar *title, *dir, *dir2, *tmp;
	xmlDocPtr doc;

	g_return_if_fail(ritem != NULL);

	if( !ritem->url )
		rssyl_get_feed_props(ritem);
	g_return_if_fail(ritem->url != NULL);

	doc = rssyl_fetch_feed(ritem->url, &title);
	g_return_if_fail(doc != NULL);
	g_return_if_fail(title != NULL);

	tmp = rssyl_strreplace(title, "/", "\\");
	dir = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, RSSYL_DIR,
			G_DIR_SEPARATOR_S, tmp, NULL);
	g_free(tmp);
	if( strcmp(title, (&ritem->item)->name) ) {
		tmp = rssyl_strreplace((&ritem->item)->name, "/", "\\");
		dir2 = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, RSSYL_DIR,
				G_DIR_SEPARATOR_S, tmp,
				NULL);
		g_free(tmp);
		if( g_rename(dir2, dir) == -1 ) {
			g_warning("couldn't rename directory '%s'\n", dir2);
			g_free(dir);
			g_free(dir2);
			g_free(title);
			return;
		}
		g_free(dir2);

		rssyl_props_update_name(ritem, title);

		g_free((&ritem->item)->name);
		(&ritem->item)->name = g_strdup(title);
		folder_item_rename(&ritem->item, title);
	}

	rssyl_parse_feed(doc, ritem);

	rssyl_expire_items(ritem);

	xmlFreeDoc(doc);
	g_free(title);
	g_free(dir);
}

void rssyl_start_refresh_timeout(RSSylFolderItem *ritem)
{
	RSSylRefreshCtx *ctx;
	guint source_id;

	g_return_if_fail(ritem != NULL);

	ctx = g_new0(RSSylRefreshCtx, 1);
	ctx->ritem = ritem;

	source_id = g_timeout_add(ritem->refresh_interval * 60 * 1000,
			(GSourceFunc)rssyl_refresh_timeout_cb, ctx );
	ritem->refresh_id = source_id;
	ctx->id = source_id;

	debug_print("RSSyl: start_refresh_timeout - %d min (id %d)\n",
			ritem->refresh_interval, ctx->id);
}

static void rssyl_find_feed_by_url_func(FolderItem *item, gpointer data)
{
	RSSylFolderItem *ritem;
	RSSylFindByUrlCtx *ctx = (RSSylFindByUrlCtx *)data;

	/* If we've already found a feed with desired URL, don't do anything */
	if( ctx->ritem != NULL )
		return;

	/* Only check rssyl folders */
	if( !IS_RSSYL_FOLDER_ITEM(item) )
		return;

	/* Don't bother with the root folder */
	if( folder_item_parent(item) == NULL )
		return;

	ritem = (RSSylFolderItem *)item;

	if( ritem->url && ctx->url && !strcmp(ritem->url, ctx->url) )
		ctx->ritem = ritem;
}

static RSSylFolderItem *rssyl_find_feed_by_url(gchar *url)
{
	RSSylFindByUrlCtx *ctx = NULL;
	RSSylFolderItem *ritem = NULL;

	g_return_val_if_fail(url != NULL, NULL);

	ctx = g_new0(RSSylFindByUrlCtx, 1);
	ctx->url = url;
	ctx->ritem = NULL;

	folder_func_to_all_folders(
			(FolderItemFunc)rssyl_find_feed_by_url_func, ctx);

	if( ctx->ritem != NULL )
		ritem = ctx->ritem;

	g_free(ctx);

	return ritem;
}

void rssyl_subscribe_new_feed(FolderItem *parent, gchar *url)
{
	gchar *title;
	xmlDocPtr doc;
	FolderItem *new_item;
	RSSylFolderItem *ritem;

	g_return_if_fail(parent != NULL);
	g_return_if_fail(url != NULL);

	if( rssyl_find_feed_by_url(url) != NULL ) {
		alertpanel_error(_("You are already subscribed to this feed."));
		return;
	}

	doc = rssyl_fetch_feed(url, &title);
	if( !title ) {
		alertpanel_error(_("Couldn't fetch URL '%s'."), url);
		return;
	}

	new_item = folder_create_folder(parent, title);
	if( !new_item ) {
		alertpanel_error(_("Can't subscribe feed '%s'."), title);
		return;
	}

	ritem = (RSSylFolderItem *)new_item;
	ritem->url = g_strdup(url);

	rssyl_store_feed_props(ritem);

	folder_write_list();

	rssyl_parse_feed(doc, ritem);
	xmlFreeDoc(doc);

	rssyl_expire_items(ritem);

	rssyl_start_refresh_timeout(ritem);

	folder_item_scan(new_item);
}

static gint rssyl_expire_sort_func(RSSylFeedItem *a, RSSylFeedItem *b)
{
	if( !a || !b )
		return 0;

	return (b->date - a->date);
}

void rssyl_expire_items(RSSylFolderItem *ritem)
{
	int num;
	RSSylFeedItem *fitem;
	GSList *i;

	g_return_if_fail(ritem != NULL);

	rssyl_read_existing(ritem);

	g_return_if_fail(ritem->contents != NULL);

	num = ritem->expired_num;
	if( num == -1 ||
			(num > (g_slist_length(ritem->contents) - ritem->last_count)) )
		return;

	debug_print("RSSyl: rssyl_expire_items()\n");

	ritem->contents = g_slist_sort(ritem->contents,
			(GCompareFunc)rssyl_expire_sort_func);

	debug_print("RSSyl: finished sorting\n");

	while( (i = g_slist_nth(ritem->contents, ritem->last_count + num + 1)) ) {
		fitem = (RSSylFeedItem *)i->data;
		debug_print("RSSyl: expiring '%s'\n", fitem->realpath);
		g_remove(fitem->realpath);
		rssyl_free_feeditem(fitem);
		fitem = NULL;
		ritem->contents = g_slist_remove(ritem->contents, i->data);
	}

	folder_item_scan(&ritem->item);

	debug_print("RSSyl: finished expiring\n");
}
