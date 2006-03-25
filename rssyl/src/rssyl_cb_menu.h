#ifndef __RSSYL_MENU_CB
#define __RSSYL_MENU_CB

#include <glib.h>
#include <gtk/gtk.h>

#include "folderview.h"

void rssyl_new_feed_cb(FolderView *folderview, guint action,
		GtkWidget *widget);
void rssyl_remove_feed_cb(FolderView *folderview, guint action,
		GtkWidget *widget);
void rssyl_remove_rss_cb(FolderView *folderview, guint action,
		GtkWidget *widget);
void rssyl_refresh_cb(FolderView *folderview, guint action,
		GtkWidget *widget);
void rssyl_prop_cb(FolderView *folderview, guint action,
		GtkWidget *widget);
void rssyl_refresh_all_cb(FolderView *folderview, guint action,
		GtkWidget *widget);
void rssyl_rename_cb(FolderView *folderview, guint action,
		GtkWidget *widget);

#endif /* __RSSYL_MENU_FEED */
