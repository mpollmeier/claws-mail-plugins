/*
 * Claws-Mail-- a GTK+ based, lightweight, and fast e-mail client
 * This file (C) 2008 Andrej Kacian <andrej@kacian.sk>
 *
 * - Export feed list to OPML
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


#include <errno.h>
#include <glib.h>

#include <log.h>
#include <folder.h>

#include "date.h"
#include "rssyl.h"

#define RSSYL_OPML_FILE	"rssyl-feedlist.opml"

static gint _folder_depth(FolderItem *item)
{
	gint i;

	for( i = 0; item != NULL; item = folder_item_parent(item), i++ ) {}
	return i;
}

struct _RSSylOpmlCtx {
	FILE *f;
	gint depth;
};

typedef struct _RSSylOpmlCtx RSSylOpmlCtx;

static void rssyl_opml_export_func(FolderItem *item, gpointer data)
{
	RSSylOpmlCtx *ctx = (RSSylOpmlCtx *)data;
	RSSylFolderItem *ritem = (RSSylFolderItem *)item;
	gboolean isfolder = FALSE, err = FALSE;
	gboolean haschildren = FALSE;
	gchar *indent = NULL, *xmlurl = NULL;
	gint depth;

	if( !IS_RSSYL_FOLDER_ITEM(item) )
		return;

	if( folder_item_parent(item) == NULL )
		return;

	/* Check for depth and adjust indentation */
	depth = _folder_depth(item);
	if( depth < ctx->depth ) {
		for( ctx->depth--; depth <= ctx->depth; ctx->depth-- ) {
			indent = g_strnfill(ctx->depth, '\t');
			err |= (fprintf(ctx->f, "%s</outline>\n", indent) < 0);
			g_free(indent);
		}
	}
	ctx->depth = depth;

	if( ritem->url == NULL ) {
		isfolder = TRUE;
	} else {
		xmlurl = g_strdup_printf("xmlUrl=\"%s\"", ritem->url);
	}

	if( g_node_n_children(item->node) )
		haschildren = TRUE;

	indent = g_strnfill(ctx->depth, '\t');

	err |= (fprintf(ctx->f,
				"%s<outline title=\"%s\" text=\"%s\" description=\"%s\" type=\"%s\" %s%s>\n",
				indent, item->name,
				(ritem->official_name ? ritem->official_name : item->name),
				(ritem->official_name ? ritem->official_name : item->name),
				(isfolder ? "folder" : "rss"),
				(xmlurl ? xmlurl : ""), (haschildren ? "" : "/")) < 0);

	g_free(indent);
	g_free(xmlurl);
	
	if( err ) {
		log_warning(LOG_PROTOCOL,
				"Error while writing '%s' to feed export list.\n",
				item->name);
		debug_print("Error while writing '%s' to feed_export list.\n",
				item->name);
	}
}

void rssyl_opml_export(void)
{
	FILE *f;
	gchar *opmlfile, *tmpdate, *indent;
	time_t tt = time(NULL);
	RSSylOpmlCtx *ctx = NULL;
	gboolean err = FALSE;

	opmlfile = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, RSSYL_DIR,
			G_DIR_SEPARATOR_S, RSSYL_OPML_FILE, NULL);

	if( g_file_test(opmlfile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR ) )
		g_remove(opmlfile);
	
	if( (f = g_fopen(opmlfile, "w")) == NULL ) {
		log_warning(LOG_PROTOCOL,
				"Couldn't open file '%s' for feed list exporting: %s\n",
				opmlfile, g_strerror(errno));
		debug_print("Couldn't open feed list export file, returning.\n");
		g_free(opmlfile);
		return;
	}

	tmpdate = createRFC822Date(&tt);

	/* Write OPML header */
	err |= (fprintf(f,
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<opml version=\"1.1\">\n"
				"\t<head>\n"
				"\t\t<title>RSSyl Feed List Export</title>\n"
				"\t\t<dateCreated>%s</dateCreated>\n"
				"\t</head>\n"
				"\t<body>\n",
				tmpdate) < 0);
	g_free(tmpdate);

	ctx = g_new0(RSSylOpmlCtx, 1);
	ctx->f = f;
	ctx->depth = 1;

	folder_func_to_all_folders(
			(FolderItemFunc)rssyl_opml_export_func, ctx);

	for( ctx->depth--; ctx->depth >= 2; ctx->depth-- ) {
		indent = g_strnfill(ctx->depth, '\t');
		err |= (fprintf(ctx->f, "%s</outline>\n", indent) < 0);
		g_free(indent);
	}

	err |= (fprintf(f,
				"\t</body>\n"
				"</opml>\n") < 0);

	if( err ) {
		log_warning(LOG_PROTOCOL, "Error during writing feed export file.\n");
		debug_print("Error during writing feed export file.");
	}

	debug_print("Feed export finished.\n");

	fclose(f);
	g_free(opmlfile);
}
