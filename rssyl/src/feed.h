#ifndef __FEED_H
#define __FEED_H

#include <libxml/parser.h>

#include "procmsg.h"

#include "rssyl.h"

#define RSSYL_XPATH_ROOT		"/rssyl"
#define RSSYL_XPATH_TITLE		RSSYL_XPATH_ROOT"/title"
#define RSSYL_XPATH_LINK		RSSYL_XPATH_ROOT"/link"
#define RSSYL_XPATH_TEXT		RSSYL_XPATH_ROOT"/text"

#define RSSYL_TEXT_START		"<!-- RSSyl text start -->"
#define RSSYL_TEXT_END			"<!-- RSSyl text end -->"

struct _RSSylFeedItem {
	gchar *title;
	gchar *text;
	gchar *link;
	gchar *parent_link;
	gchar *comments_link;
	gchar *author;

	gchar *realpath;
	time_t date;
};

typedef struct _RSSylFeedItem RSSylFeedItem;

struct _RSSylFindByUrlCtx {
	gchar *url;
	RSSylFolderItem *ritem;
};

typedef struct _RSSylFindByUrlCtx RSSylFindByUrlCtx;

xmlDocPtr rssyl_fetch_feed(const gchar *url, time_t last_update, gchar **title);
void rssyl_parse_feed(xmlDocPtr doc, RSSylFolderItem *ritem, gchar *parent);
gboolean rssyl_add_feed_item(RSSylFolderItem *ritem, RSSylFeedItem *fitem);
MsgInfo *rssyl_parse_feed_item_to_msginfo(gchar *file, MsgFlags flags,
		gboolean a, gboolean b, FolderItem *item);
void rssyl_remove_feed_cache(FolderItem *item);
void rssyl_update_feed(RSSylFolderItem *ritem);
void rssyl_read_existing(RSSylFolderItem *ritem);

void rssyl_start_refresh_timeout(RSSylFolderItem *ritem);
void rssyl_expire_items(RSSylFolderItem *ritem);

void rssyl_subscribe_new_feed(FolderItem *parent, gchar *url);
void rssyl_free_feeditem(RSSylFeedItem *item);

#endif /* __FEED_H */
