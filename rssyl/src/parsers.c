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
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "date.h"
#include "feed.h"
#include "strreplace.h"
#include "utils.h"
#include "procheader.h"

gint rssyl_parse_rdf(xmlDocPtr doc, RSSylFolderItem *ritem, gchar *parent)
{
	xmlNodePtr rnode, node, n;
	RSSylFeedItem *fitem = NULL;
	gint count = 0;
	gchar *content = NULL;
	g_return_val_if_fail(doc != NULL, 0);
	g_return_val_if_fail(ritem != NULL, 0);
#ifdef RSSYL_DEBUG
	gchar *fetched = NULL;
#endif	/* RSSYL_DEBUG */

	if( ritem->contents == NULL )
		rssyl_read_existing(ritem);

	rnode = xmlDocGetRootElement(doc);

	for( node = rnode->children; node; node = node->next ) {
		if( !xmlStrcmp(node->name, "item") ) {
			/* We've found an "item" tag, let's poke through its contents */
			fitem = g_new0(RSSylFeedItem, 1);
			fitem->date = 0;
#ifdef RSSYL_DEBUG
			fetched = xmlGetProp(rnode, "fetched");
			fitem->debug_fetched = atoll(fetched);
			xmlFree(fetched);
#endif	/* RSSYL_DEBUG */

			for( n = node->children; n; n = n->next ) {
				/* Title */
				if( !xmlStrcmp(n->name, "title") ) {
					content = xmlNodeGetContent(n);
					fitem->title = rssyl_format_string(g_strdup(content), TRUE, TRUE);
					xmlFree(content);
					debug_print("RSSyl: XML - RDF title is '%s'\n", fitem->title);
				}

				/* Text */
				if( !xmlStrcmp(n->name, "description") ) {
					content = xmlNodeGetContent(n);
					fitem->text = rssyl_format_string(g_strdup(content), FALSE, FALSE);
					xmlFree(content);
					debug_print("RSSyl: XML - got RDF text\n");
				}

				/* URL */
				if( !xmlStrcmp(n->name, "link") ) {
					content = xmlNodeGetContent(n);
					fitem->link = rssyl_format_string(g_strdup(content), FALSE, FALSE);
					xmlFree(content);
					debug_print("RSSyl: XML - RDF link is '%s'\n", fitem->link);
				}

				/* Date - rfc822 format */
				if( !xmlStrcmp(n->name, "pubDate") ) {
					content = xmlNodeGetContent(n);
					fitem->date = procheader_date_parse(NULL, content, 0);
					xmlFree(content);
					if( fitem->date > 0 ) {
						debug_print("RSSyl: XML - RDF date found\n" );
					} else
						fitem->date = 0;
				}
				/* Date - ISO8701 format */
				if( !xmlStrcmp(n->name, "date") ) {
					content = xmlNodeGetContent(n);
					fitem->date = parseISO8601Date(content);
					xmlFree(content);
					debug_print("RSSyl: XML - RDF date found\n" );
				}

				/* Author */
				if( !xmlStrcmp(n->name, "creator") ) {
					content = xmlNodeGetContent(n);
					fitem->author = rssyl_format_string(g_strdup(content), TRUE, TRUE);
					xmlFree(content);
					debug_print("RSSyl: XML - RDF author is '%s'\n", fitem->author);
				}
			}
		}

		if( fitem && fitem->link && fitem->title ) {
			if (rssyl_add_feed_item(ritem, fitem) == FALSE) {
				rssyl_free_feeditem(fitem);
				fitem = NULL;
			}
			fitem = NULL;
			count++;
		}
	}

	return count;
}


/* rssyl_parse_rss()
 *
 * This is where we parse the fetched rss document and create a
 * RSSylFolderItem from it. Returns number of parsed items
 */
gint rssyl_parse_rss(xmlDocPtr doc, RSSylFolderItem *ritem, gchar *parent)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;
	xmlNodePtr node, n, rnode;
	gint i, count = 0;
	RSSylFeedItem *fitem = NULL;
	gchar *xpath;
	gboolean got_encoded;
	gchar *rootnode = NULL;
	RSSylFeedItemMedia *media;
	gchar *media_url, *media_type;
	gulong media_size = 0;
#ifdef RSSYL_DEBUG
	gchar *fetched = NULL;
#endif	/* RSSYL_DEBUG */

	g_return_val_if_fail(doc != NULL, 0);
	g_return_val_if_fail(ritem != NULL, 0);

	if( ritem->contents == NULL )
		rssyl_read_existing(ritem);

	rnode = xmlDocGetRootElement(doc);

	rootnode = g_ascii_strdown(rnode->name, -1);
	xpath = g_strconcat("/", rootnode,
				"/channel/item",	NULL);
	g_free(rootnode);
	context = xmlXPathNewContext(doc);
	if( !(result = xmlXPathEvalExpression(xpath, context)) ){
		debug_print("RSSyl: XML - no result found for '%s'\n", xpath);
		xmlXPathFreeContext(context);
		g_free(xpath);
		return 0;
	}

	g_free(xpath);

	for( i = 0; i < result->nodesetval->nodeNr; i++ ) {
		node = result->nodesetval->nodeTab[i];
		n = node->children;
		fitem = g_new0(RSSylFeedItem, 1);
		fitem->media = NULL;
		fitem->date = 0;
#ifdef RSSYL_DEBUG
		fetched = xmlGetProp(rnode, "fetched");
		fitem->debug_fetched = atoll(fetched);
		xmlFree(fetched);
#endif	/* RSSYL_DEBUG */
		fitem->text = NULL;
		if (parent)
			fitem->parent_link = g_strdup(parent);

		got_encoded = FALSE;
		do {
			gchar *content = NULL;

			/* Title */
			if( !strcmp(n->name, "title") ) {
				content = xmlNodeGetContent(n);
				fitem->title = rssyl_format_string(g_strdup(content), TRUE, TRUE);
				xmlFree(content);
				debug_print("RSSyl: XML - item title: '%s'\n", fitem->title);
			}

			/* Text */
			if( !strcmp(n->name, "description") ) {
				if( (fitem->text == NULL) && (got_encoded == FALSE) ) {
					content = xmlNodeGetContent(n);
					debug_print("RSSyl: XML - item text (description) caught\n");
					fitem->text = rssyl_format_string(g_strdup(content), FALSE, FALSE);
					xmlFree(content);
				}
			}
			if( !strcmp(n->name, "encoded") && !strcmp(n->ns->prefix, "content") ) {
				debug_print("RSSyl: XML - item text (content) caught\n");

				if (fitem->text != NULL)
					g_free(fitem->text); /* free "description" */
					
				content = xmlNodeGetContent(n);
				fitem->text = rssyl_format_string(g_strdup(content), FALSE, FALSE);
				xmlFree(content);
				got_encoded = TRUE;
			}

			/* URL link to the original post */
			if( !strcmp(n->name, "link") ) {
				content = xmlNodeGetContent(n);
				fitem->link = rssyl_format_string(g_strdup(content), FALSE, FALSE);
				xmlFree(content);
				debug_print("RSSyl: XML - item link: '%s'\n", fitem->link);
			}

			/* Date - rfc822 format */
			if( !strcmp(n->name, "pubDate") ) {
				content = xmlNodeGetContent(n);
				fitem->date = procheader_date_parse(NULL, content, 0);
				xmlFree(content);
				if( fitem->date > 0 ) {
					debug_print("RSSyl: XML - item date found\n" );
				} else
					fitem->date = 0;
			}
			/* Date - ISO8701 format */
			if( !strcmp(n->name, "date") ) {
				content = xmlNodeGetContent(n);
				fitem->date = parseISO8601Date(content);
				xmlFree(content);
				debug_print("RSSyl: XML - item date found\n" );
			}

			/* Author */
			if( !strcmp(n->name, "creator") ) {
				content = xmlNodeGetContent(n);
				fitem->author = rssyl_format_string(g_strdup(content), TRUE, TRUE);
				xmlFree(content);
				debug_print("RSSyl: XML - item author: '%s'\n", fitem->author);
			}

			/* Media enclosure */
			if( !strcmp(n->name, "enclosure") ) {
				media_url = xmlGetProp(n, "url");
				media_type = xmlGetProp(n, "type");
				media_size = xmlGetProp(n, "length");

				if( media_url != NULL &&
						media_type != NULL &&
						media_size != 0 ) {
					debug_print("RSSyl: XML - enclosure: '%s' [%s] (%ld)\n",
							media_url, media_type, media_size);
					media = g_new(RSSylFeedItemMedia, 1);
					media->url = media_url;
					media->type = media_type;
					media->size = media_size;
					fitem->media = media;
				} else {
					debug_print("RSSyl: XML - enclosure found, but some data is missing\n");
					g_free(media_url);
					g_free(media_type);
				}
			}

			/* Comments */
			if( !strcmp(n->name, "commentRSS") || !strcmp(n->name, "commentRss") ) {
				content = xmlNodeGetContent(n);
				fitem->comments_link = rssyl_format_string(g_strdup(content), FALSE, FALSE);
				xmlFree(content);
				debug_print("RSSyl: XML - comments_link: '%s'\n", fitem->comments_link);
			}
		} while( (n = n->next) != NULL);

		if( fitem->link && fitem->title ) {
			if (rssyl_add_feed_item(ritem, fitem) == FALSE) {
				rssyl_free_feeditem(fitem);
				fitem = NULL;
			}
			count++;
		}
	}

	xmlXPathFreeObject(result);
	xmlXPathFreeContext(context);

	return count;
}

/* rssyl_parse_atom()
 *
 * This is where we parse the fetched atom document and create a
 * RSSylFolderItem from it. Returns number of parsed items
 */
gint rssyl_parse_atom(xmlDocPtr doc, RSSylFolderItem *ritem, gchar *parent)
{
	xmlNodePtr node, n;
	gint count = 0;
	RSSylFeedItem *fitem = NULL;
	gchar *link_type, *link_href;
	gchar *content;

	g_return_val_if_fail(doc != NULL, 0);
	g_return_val_if_fail(ritem != NULL, 0);

	if( ritem->contents == NULL )
		rssyl_read_existing(ritem);

	node = xmlDocGetRootElement(doc);

	if (node == NULL)
		return 0;

	node = node->children;

	for (; node; node = node->next) {
		gboolean got_content = FALSE;
		if (strcmp(node->name, "entry")) {
			continue;
		}
	
		n = node->children;
		fitem = g_new0(RSSylFeedItem, 1);
		fitem->date = 0;
		fitem->date_published = 0;
		fitem->text = NULL;
		
		do {
			/* Title */
			if( !strcmp(n->name, "title") ) {
				content = xmlNodeGetContent(n);
				fitem->title = rssyl_format_string(g_strdup(content), TRUE, TRUE);
				xmlFree(content);
				debug_print("RSSyl: XML - Atom item title: '%s'\n", fitem->title);
			}

			/* ID */
			if( !strcmp(n->name, "guid") ) {
				content = xmlNodeGetContent(n);
				fitem->id = g_strdup(content);
				xmlFree(content);
				debug_print("RSSyl: XML - Atom guid: '%s'\n", fitem->id);
			}

			/* Text */
			if( !strcmp(n->name, "summary") && !got_content ) {
				content = xmlNodeGetContent(n);
				debug_print("RSSyl: XML - Atom item text (summary) caught\n");
				fitem->text = rssyl_format_string(g_strdup(content), FALSE, FALSE);
				xmlFree(content);
			}

			if( !strcmp(n->name, "content") ) {
				debug_print("RSSyl: XML - Atom item text (content) caught\n");
				if (fitem->text)
					g_free(fitem->text);
				content = xmlNodeGetContent(n);
				fitem->text = rssyl_format_string(g_strdup(content), FALSE, FALSE);
				xmlFree(content);
				got_content = TRUE;
			}

			/* URL link to the original post */
			if( !strcmp(n->name, "link") ) {

				link_type = xmlGetProp(n, "type");
				if( !link_type || !strcmp("text/html", link_type) ) {
					link_href = xmlGetProp(n, "href");
					fitem->link = (link_href ? g_strdup(link_href) : NULL);
					debug_print("RSSyl: XML - Atom item link: '%s'\n", fitem->link);
					if (link_href)
						xmlFree(link_href);
				}
				if (link_type)
					xmlFree(link_type);
			}

			/* Date published - ISO8701 format */
			if( !strcmp(n->name, "issued") ) {
				content = xmlNodeGetContent(n);
				fitem->date_published = parseISO8601Date(content);
				xmlFree(content);
				debug_print("RSSyl: XML - Atom item 'issued' date found\n" );
			}

			/* Date modified - ISO8701 format */
			if( !strcmp(n->name, "updated") ) {
				content = xmlNodeGetContent(n);
				fitem->date = parseISO8601Date(content);
				xmlFree(content);
				debug_print("RSSyl: XML - Atom item 'updated' date found\n" );
			}

			/* Author */
			if( !strcmp(n->name, "author") ) {
				xmlNodePtr subnode;
				gchar *name = NULL, *mail = NULL;
				for (subnode = n->children; subnode; subnode = subnode->next) {
					if (!strcmp(subnode->name, "name") && !name)
						name = g_strdup(xmlNodeGetContent(subnode));
					if (!strcmp(subnode->name, "email") && !mail)
						mail = g_strdup(xmlNodeGetContent(subnode));
				}
				fitem->author = rssyl_format_string(g_strdup_printf("%s%s%s%s%s",
							name ? name:"",
							name && mail ? " <":(mail?"<":""),
							mail ? mail:"",
							mail ? ">":"",
							!name && !mail ? "N/A":""), TRUE, TRUE);
				g_free(name);
				g_free(mail);
				debug_print("RSSyl: XML - Atom item author: '%s'\n", fitem->author);
			}
		} while( (n = n->next) != NULL);

		if( fitem->link && fitem->title ) {
			if (rssyl_add_feed_item(ritem, fitem) == FALSE) {
				rssyl_free_feeditem(fitem);
				fitem = NULL;
			}
			count++;
		}
	}

	return count;
}
