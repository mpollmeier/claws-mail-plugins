#ifndef __RSSYL_H
#define __RSSYL_H

#include <glib.h>

#include <folder.h>

/* Name of directory in rcdir where RSSyl will store its data. */
#define RSSYL_DIR		"RSSyl"

/* Default RSSyl mailbox name */
#define RSSYL_DEFAULT_MAILBOX	"My RSS Feeds"

/* Default feed to be added when creating mailbox for the first time */
#define RSSYL_DEFAULT_FEED	"http://planet.sylpheed.org/rss20.xml"

/* Default feed refresh interval in minutes */
#define RSSYL_DEFAULT_REFRESH		180

/* Default number of expired items to keep */
#define RSSYL_DEFAULT_EXPIRED		-1

struct _RSSylFolderItem {
	FolderItem item;
	GSList *contents;
	gint last_count;

	gchar *url;

	gboolean default_refresh_interval;
	gint refresh_interval;

	gboolean default_expired_num;
	gint expired_num;

	guint refresh_id;	
	gboolean fetch_comments;

	struct _RSSylFeedProp *feedprop;
};

typedef struct _RSSylFolderItem RSSylFolderItem;

struct _RSSylRefreshCtx {
	RSSylFolderItem *ritem;
	guint id;
};

typedef struct _RSSylRefreshCtx RSSylRefreshCtx;

void rssyl_init(void);
void rssyl_done(void);

FolderClass *rssyl_folder_get_class(void);

#define IS_RSSYL_FOLDER_ITEM(item) \
	(item->folder->klass == rssyl_folder_get_class())

#endif /* __RSSYL_H */
