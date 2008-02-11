/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000,2001 CodeFactory AB
   Copyright (C) 2000,2001 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000,2001 Anders Carlsson <andersca@codefactory.se>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <string.h>
#include <libxml/tree.h>

#include "css/cssmatcher.h"
#include "document/htmldocument.h"
#include "document/htmlparser.h"
#include "dom/events/dom-event-utils.h"
#include "dom/html/dom-htmlformelement.h"
#include "dom/html/dom-htmlinputelement.h"
#include "dom/html/dom-htmloptionelement.h"
#include "dom/html/dom-htmltextareaelement.h"
#include "dom/html/dom-htmlanchorelement.h"
#include "util/htmlmarshal.h"
#include "util/htmlstreambuffer.h"
#include "util/htmlglobalatoms.h"
#include "gtkhtmlcontext.h"

enum {
	REQUEST_URL,
	REQUEST_ICON,
	LINK_CLICKED,
	SET_BASE,
	TITLE_CHANGED,
	SUBMIT,
	
	/* DOM change events */
	NODE_INSERTED,
	NODE_REMOVED,
	TEXT_UPDATED,
	STYLE_UPDATED,
	
	RELAYOUT_NODE,
	REPAINT_NODE,

	/* DOM Events */
	DOM_MOUSE_DOWN,
	DOM_MOUSE_UP,
	DOM_MOUSE_CLICK,
	DOM_MOUSE_OVER,
	DOM_MOUSE_OUT,
	
	LAST_SIGNAL
};

typedef struct _RefreshData RefreshData;

struct _RefreshData {
	HtmlDocument *doc;
	gchar *url;
};

static GObjectClass *parent_class = NULL;


static guint document_signals [LAST_SIGNAL] = { 0 };

static HtmlStyleChange 
html_document_restyle_node (HtmlDocument *document, DomNode *node, HtmlAtom *pseudo, gboolean recurse)
{
	HtmlStyle *parent_style, *new_style;
	HtmlStyleChange style_change = HTML_STYLE_CHANGE_NONE, new_style_change;

	if (!node || !node->style ||
	    node->xmlnode->type == XML_HTML_DOCUMENT_NODE ||
	    node->xmlnode->type == XML_DOCUMENT_NODE ||
	    node->xmlnode->type == XML_DTD_NODE)
		return style_change;
	
	if (node->xmlnode->parent)
			parent_style = dom_Node__get_parentNode (node)->style;
		else
			parent_style = NULL;

	if (node->xmlnode->type == XML_TEXT_NODE) {

		if (parent_style != NULL) {
			html_style_ref (parent_style);
			if (node->style)
				html_style_unref (node->style);
			node->style = parent_style;
		}
	}
	else {

		new_style = css_matcher_get_style (document, parent_style, node->xmlnode, pseudo);
		style_change = html_style_compare (node->style, new_style);
		
		if (style_change != HTML_STYLE_CHANGE_NONE) {
			
			html_style_ref (new_style);
			html_style_unref (node->style);
			node->style = new_style;
			
		}
		else
			html_style_unref (new_style);
		
		if (recurse) {
			for (node = dom_Node__get_firstChild (node); node; node = dom_Node__get_nextSibling (node)) {
				new_style_change = html_document_restyle_node (document, node, pseudo, recurse);
				
				if (new_style_change > style_change)
					style_change = new_style_change;
			}
		}
	}
	return style_change;
}

static void
html_document_stylesheet_stream_close (const gchar *buffer, gint len, gpointer data)
{
	CssStylesheet *sheet;
	HtmlDocumentStreamData *stream_data = (HtmlDocumentStreamData *) data;
	HtmlDocument *document = stream_data->document;
	HtmlStyleChange style_change;
	GSList *list;

	if (!buffer)
		return;
	
	sheet = css_parser_parse_stylesheet (buffer, len, (gchar *) stream_data->internal_data);
	g_free(stream_data->internal_data);
	stream_data->internal_data = NULL;

	for (list = sheet->stat; list; list = list->next) {
		CssStatement *statement = list->data;
		HtmlStream *stream;

		switch (statement->type) {
		case CSS_IMPORT_RULE: {
			HtmlDocumentStreamData *stream_data_import;
			gchar *url;

			url = css_value_to_string (statement->s.import_rule.url);
			stream_data_import = g_new (HtmlDocumentStreamData, 1);
			stream_data_import->document = stream_data->document;
			stream_data_import->internal_data = g_strdup(url);
			stream = html_stream_buffer_new (html_document_stylesheet_stream_close, stream_data_import);
			g_signal_emit (G_OBJECT (document), document_signals [REQUEST_URL], 0, url, stream);
			g_free (url);
			break;
		}
		default:
			break;
		}
	}

	g_free (stream_data);
	document->stylesheets = g_slist_append (document->stylesheets, sheet);

	/* Restyle the document */
	style_change = html_document_restyle_node (document, DOM_NODE (dom_Document__get_documentElement (document->dom_document)), NULL, TRUE);
	g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, DOM_NODE (dom_Document__get_documentElement (document->dom_document)), style_change);
}

static gboolean
html_document_refresh_timeout (gpointer data)
{
	RefreshData *rdata = (RefreshData *) data;

	g_signal_emit (G_OBJECT (rdata->doc), document_signals [LINK_CLICKED],
		       0, rdata->url);

	g_free (rdata->url);
	g_object_unref (rdata->doc);
	g_free (rdata);

	return FALSE;
}

static void
html_document_node_inserted_traverser (HtmlDocument *document, DomNode *node)
{
	if (dom_Node_hasChildNodes (node))
		html_document_node_inserted_traverser (document, dom_Node__get_firstChild (node));

	for (; node; node = dom_Node__get_nextSibling (node)) {
		HtmlStyle *style, *parent_style;
		
		/* First, check for some special nodes that may be of interest */
		if (strcasecmp (node->xmlnode->name, "link") == 0) {
			gchar *str = xmlGetProp (node->xmlnode, "rel");
			
			if (str && (strcasecmp (str, "stylesheet") == 0)) {
				gchar *url = xmlGetProp (node->xmlnode, "href");
				if (url) {
					HtmlDocumentStreamData *stream_data;
					HtmlStream *stream;
					
					stream_data = g_new (HtmlDocumentStreamData, 1);
					stream_data->document = document;
					stream_data->internal_data = g_strdup(url);
					
					stream = html_stream_buffer_new (html_document_stylesheet_stream_close, stream_data);

					g_signal_emit (G_OBJECT (document), document_signals [REQUEST_URL], 0, url, stream);
				}
				g_free (url);
			} else if (str && (strcasecmp (str, "icon") == 0)) {
				gchar *url = xmlGetProp (node->xmlnode, "href");
				if (url) {
					g_signal_emit (G_OBJECT (document), document_signals [REQUEST_ICON], 0, url);
				}
				g_free (url);
			}
			g_free (str);
		}
		else if (strcasecmp (node->xmlnode->name, "meta") == 0) {
			gchar *str = xmlGetProp (node->xmlnode, "http-equiv");

			if (str && (strcasecmp (str, "content-type") == 0)) {
			} else if (str && (strcasecmp (str, "refresh") == 0)) {
				RefreshData *rdata;
				gchar *content = xmlGetProp (node->xmlnode, "content");
				gchar *timestr;
				gchar *url;
				gint timeval;

				rdata = g_new0 (RefreshData, 1);
				rdata->doc = g_object_ref (document);

				if (strchr (content, ';')) {
					timestr = g_strndup (content, strlen (content) - strlen (strchr (content, ';')));
				} else {
					timestr = g_strdup (content);
				}
				timeval = atoi (timestr);

				if (strchr (content, '=')) {
					url = g_strndup (content + (strlen (content) - strlen (strchr (content, '=')) + 1), strlen (strchr (content, '=')) + 1);
				} else {
					url = g_strdup ("");
				}
				rdata->url = g_strdup (url);

				g_timeout_add (timeval * 1000, (GSourceFunc) html_document_refresh_timeout, rdata);

				g_free (url);
				g_free (timestr);
				g_free (content);
			}
			g_free (str);
		}
		else if (node->xmlnode->type == XML_TEXT_NODE && node->xmlnode->parent && strcasecmp (node->xmlnode->parent->name, "option") == 0) {
			dom_html_option_element_new_character_data (DOM_HTML_OPTION_ELEMENT (dom_Node__get_parentNode (node)));
		}
		else if (node->xmlnode->type == XML_TEXT_NODE && node->xmlnode->parent && strcasecmp (node->xmlnode->parent->name, "textarea") == 0) {
			dom_HTMLTextAreaElement__set_defaultValue (DOM_HTML_TEXT_AREA_ELEMENT (dom_Node__get_parentNode (node)), node->xmlnode->content);
		}
		else if ((node->xmlnode->type == XML_TEXT_NODE || node->xmlnode->type == XML_COMMENT_NODE) && 
				node->xmlnode->parent && strcasecmp (node->xmlnode->parent->name, "style") == 0) {

			CssStylesheet *ss;
			HtmlStyleChange style_change;
			GSList *list;

			ss = css_parser_parse_stylesheet (node->xmlnode->content, strlen (node->xmlnode->content), NULL);

			for (list = ss->stat; list; list = list->next) {
				CssStatement *statement = list->data;
				HtmlStream *stream;
				
				switch (statement->type) {
				case CSS_IMPORT_RULE: {
					gchar *cssurl;
					HtmlDocumentStreamData *stream_data;
					
					cssurl = css_value_to_string (statement->s.import_rule.url);
					
					stream_data = g_new (HtmlDocumentStreamData, 1);
					stream_data->document = document;
                    			stream_data->internal_data = g_strdup(cssurl);

					stream = html_stream_buffer_new (html_document_stylesheet_stream_close, stream_data);
					g_signal_emit (G_OBJECT (document), document_signals [REQUEST_URL], 0, cssurl, stream);
					g_free (cssurl);
					break;
				}
				default:
					break;
				}
			}

			document->stylesheets = g_slist_append ( document->stylesheets, ss);

			/* Restyle the document */
			style_change = html_document_restyle_node (document, DOM_NODE (dom_Document__get_documentElement ( document->dom_document)), NULL, TRUE);
			g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, DOM_NODE (dom_Document__get_documentElement (document->dom_document)), style_change);
		}
		else if ((node->xmlnode->type == XML_TEXT_NODE || node->xmlnode->type == XML_COMMENT_NODE) && node->xmlnode->parent && strcasecmp (node->xmlnode->parent->name, "script") == 0) {
			/*g_print ("whee, we have a script: len %d\n", strlen (node->xmlnode->content));*/
		}
		else if (node->xmlnode->type == XML_TEXT_NODE && node->xmlnode->parent && strcasecmp (node->xmlnode->parent->name, "title") == 0) {
				g_signal_emit (G_OBJECT (document), document_signals [TITLE_CHANGED], 0, node->xmlnode->content);
		}
		else if (strcasecmp (node->xmlnode->name, "img") == 0) {
			gchar *src;
			
			if ((src = xmlGetProp (node->xmlnode, "src"))) {
				HtmlImage *image;
				
				image = html_image_factory_get_image (document->image_factory, src);
				
				/* FIXME: What about freeing the data? */
				g_object_set_data_full (G_OBJECT (node), "image", image, g_object_unref);
				xmlFree (src);
			}

		}
		else if (strcasecmp (node->xmlnode->name, "base") == 0) {
			gchar *href;

			if ((href = xmlGetProp (node->xmlnode, "href"))) {
				g_signal_emit (G_OBJECT (document), document_signals [SET_BASE], 0, href);

				xmlFree (href);
			}
		}
		
		if (DOM_IS_HTML_ELEMENT (node))
			dom_html_element_parse_html_properties (DOM_HTML_ELEMENT (node), document);

		if (node->xmlnode->type == XML_HTML_DOCUMENT_NODE ||
		    node->xmlnode->type == XML_DOCUMENT_NODE ||
		    node->xmlnode->type == XML_DTD_NODE)
			return;
		
		if (node->xmlnode->parent)
			parent_style = dom_Node__get_parentNode (node)->style;
		else
			parent_style = NULL;

		if (node->xmlnode->type == XML_TEXT_NODE) {

			if (parent_style != NULL) {
				html_style_ref (parent_style);
				if (node->style)
					html_style_unref (node->style);
				node->style = parent_style;
			}

		} else {
			style = css_matcher_get_style (document, parent_style, node->xmlnode, NULL);
			node->style = html_style_ref (style);
		}
	}
	
}

static void
html_document_node_inserted (HtmlDocument *document, DomNode *node)
{
	HtmlStyle *style, *parent_style;
	
	if (dom_Node__get_parentNode (node))
		parent_style = dom_Node__get_parentNode (node)->style;
	else
		parent_style = NULL;
	
	if (node->xmlnode->type == XML_TEXT_NODE) {

		if (parent_style != NULL) {
			html_style_ref (parent_style);
			if (node->style)
				html_style_unref (node->style);
			node->style = parent_style;
		}
	}
	else {
		style = css_matcher_get_style (document, parent_style, node->xmlnode, NULL);
		node->style = html_style_ref (style);
	}	

	if (dom_Node_hasChildNodes (node))
		html_document_node_inserted_traverser (document, dom_Node__get_firstChild (node));
	
	g_signal_emit (G_OBJECT (document), document_signals [NODE_INSERTED], 0, node);
}

static void
html_document_dom_event (DomEventListener *listener, DomEvent *event, HtmlDocument *document)
{
	gchar *type = dom_Event__get_type (event);
	DomNode *node = DOM_NODE (dom_Event__get_target (event));


	if (strcmp (type, "DOMNodeInserted") == 0) {
		html_document_node_inserted (document, node);
	}
	else if (strcmp (type, "DOMNodeRemoved") == 0) {
		g_signal_emit (G_OBJECT (document), document_signals [NODE_REMOVED], 0, node);
	}
	else if (strcmp (type, "DOMCharacterDataModified") == 0) {
		g_signal_emit (G_OBJECT (document), document_signals [TEXT_UPDATED], 0, node);
	}
	else if (strcmp (type, "StyleChanged") == 0) {
		g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, node, dom_StyleEvent__get_styleChange (DOM_STYLE_EVENT (event)));
	}
	else if (strcmp (type, "mousedown") == 0) {
		gboolean return_value = FALSE;
		
		g_signal_emit (G_OBJECT (document), document_signals [DOM_MOUSE_DOWN], 0, event, &return_value);

		if (return_value)
			dom_Event_preventDefault (event);
	}
	else if (strcmp (type, "mouseup") == 0) {
		gboolean return_value = FALSE;
		
		g_signal_emit (G_OBJECT (document), document_signals [DOM_MOUSE_UP], 0, event, &return_value);

		if (return_value)
			dom_Event_preventDefault (event);
	}
	else if (strcmp (type, "click") == 0) {
		gboolean return_value = FALSE;
		
		g_signal_emit (G_OBJECT (document), document_signals [DOM_MOUSE_CLICK], 0, event, &return_value);

		if (return_value)
			dom_Event_preventDefault (event);
	}
	else if (strcmp (type, "mouseover") == 0) {
		gboolean return_value = FALSE;
		
		g_signal_emit (G_OBJECT (document), document_signals [DOM_MOUSE_OVER], 0, event, &return_value);
		
		if (return_value)
			dom_Event_preventDefault (event);

	}
	else if (strcmp (type, "mouseout") == 0) {
		gboolean return_value = FALSE;
		
		g_signal_emit (G_OBJECT (document), document_signals [DOM_MOUSE_OUT], 0, event, &return_value);
		
		if (return_value)
			dom_Event_preventDefault (event);
	}
	else if (strcmp (type, "submit") == 0) {
		gchar *action, *method, *encoding;

		action   = dom_HTMLFormElement__get_action (DOM_HTML_FORM_ELEMENT(node));
		method   = dom_HTMLFormElement__get_method (DOM_HTML_FORM_ELEMENT(node));
		encoding = dom_HTMLFormElement__get_encoding (DOM_HTML_FORM_ELEMENT(node));

		g_signal_emit (G_OBJECT (document), document_signals [SUBMIT], 0, 
			       action, method, encoding);

		if (action)
			xmlFree (action);
		if (method)
			xmlFree (method);
		if (encoding)
			g_free (encoding);
	}

	g_free (type);
}

static void
html_document_parsed_document_node (HtmlParser *parser, DomDocument *dom_document, HtmlDocument *document)
{
	DomEventListener *listener;

	listener = g_object_get_data (G_OBJECT (document), "dom-event-listener");
	if (listener)
		return;

	listener = dom_event_listener_signal_new ();
	g_signal_connect (G_OBJECT (listener), "event",
			  G_CALLBACK (html_document_dom_event), document);
	g_object_set_data (G_OBJECT (document), "dom-event-listener", listener);
	
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "DOMNodeInserted", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "DOMNodeRemoved", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "DOMCharacterDataModified", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "StyleChanged", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "mousedown", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "mouseup", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "click", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "mouseover", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "mouseout", listener, FALSE);
	dom_EventTarget_addEventListener (DOM_EVENT_TARGET (document->dom_document),
					  "submit", listener, FALSE);

}

static void
html_document_done_parsing (HtmlParser *parser, HtmlDocument *document)
{
}

static void
html_document_new_node (HtmlParser *parser, DomNode *node, HtmlDocument *document)
{
	html_document_node_inserted_traverser (document, node);
	
	g_signal_emit (G_OBJECT (document), document_signals [NODE_INSERTED], 0, node);
}

static void
html_document_finalize (GObject *object)
{
	HtmlDocument *document = HTML_DOCUMENT (object);

	html_document_clear (document);

	if (document->parser)
		g_object_unref (G_OBJECT (document->parser));

	parent_class->finalize (object);
}

static void
html_document_class_init (HtmlDocumentClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;
	object_class->finalize = html_document_finalize;

	parent_class = g_type_class_peek_parent (object_class);

	document_signals [REQUEST_URL] =
		g_signal_new ("request_url",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, request_url),
			      NULL, NULL,
			      html_marshal_VOID__STRING_OBJECT,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_STRING,
			      HTML_TYPE_STREAM);

	document_signals [REQUEST_ICON] =
		g_signal_new ("request_icon",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, request_icon),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_STRING);

	document_signals [LINK_CLICKED] =
		g_signal_new ("link_clicked",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, link_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_STRING);
	
	document_signals [SET_BASE] =
		g_signal_new ("set_base",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, set_base),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_STRING);
	
	document_signals [TITLE_CHANGED] =
		g_signal_new ("title_changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, title_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_STRING);
	
	document_signals [SUBMIT] =
		g_signal_new ("submit",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, submit),
			      NULL, NULL,
			      html_marshal_VOID__STRING_STRING_STRING,
			      G_TYPE_NONE,
			      3,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING);
	
	document_signals [NODE_INSERTED] =
		g_signal_new ("node_inserted",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, node_inserted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      DOM_TYPE_NODE);
	
	document_signals [NODE_REMOVED] =
		g_signal_new ("node_removed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, node_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      DOM_TYPE_NODE);

	document_signals [TEXT_UPDATED] =
		g_signal_new ("text_updated",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, text_updated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      DOM_TYPE_NODE);
	
	document_signals [STYLE_UPDATED] =
		g_signal_new ("style_updated",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, style_updated),
			      NULL, NULL,
			      html_marshal_VOID__OBJECT_INT,
			      G_TYPE_NONE, 2,
			      DOM_TYPE_NODE,
			      G_TYPE_INT);
	
	document_signals [RELAYOUT_NODE] =
		g_signal_new ("relayout_node",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, relayout_node),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      DOM_TYPE_NODE);
	
	document_signals [REPAINT_NODE] =
		g_signal_new ("repaint_node",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, repaint_node),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      DOM_TYPE_NODE);

	document_signals [DOM_MOUSE_DOWN] =
		g_signal_new ("dom_mouse_down",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, dom_mouse_down),
			      NULL, NULL,
			      html_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      DOM_TYPE_MOUSE_EVENT);
	
	document_signals [DOM_MOUSE_UP] =
		g_signal_new ("dom_mouse_up",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, dom_mouse_up),
			      NULL, NULL,
			      html_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      DOM_TYPE_MOUSE_EVENT);

	document_signals [DOM_MOUSE_CLICK] =
		g_signal_new ("dom_mouse_click",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, dom_mouse_click),
			      NULL, NULL,
			      html_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      DOM_TYPE_MOUSE_EVENT);
	
	document_signals [DOM_MOUSE_OVER] =
		g_signal_new ("dom_mouse_over",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, dom_mouse_over),
			      NULL, NULL,
			      html_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      DOM_TYPE_MOUSE_EVENT);

	document_signals [DOM_MOUSE_OUT] =
		g_signal_new ("dom_mouse_out",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlDocumentClass, dom_mouse_out),
			      NULL, NULL,
			      html_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      DOM_TYPE_MOUSE_EVENT);

	gtk_settings_install_property (g_param_spec_float ("gtkhtml-minimum-font-size",
							   "Minimum font size",
							   "The minimum font size used when rendering text",
							   0.0,
							   50.0,
							   8.0,
							   G_PARAM_READWRITE));

}

static void
html_document_request_image (HtmlImageFactory *image_factory, const gchar *uri, HtmlStream *stream, HtmlDocument *doc)
{
	g_signal_emit (G_OBJECT (doc), document_signals [REQUEST_URL], 0, uri, stream);
}

static void
html_document_init (HtmlDocument *document)
{
	document->stylesheets = NULL;
	document->image_factory = html_image_factory_new ();

	g_signal_connect (G_OBJECT (document->image_factory), "request_image",
			  G_CALLBACK (html_document_request_image), document);
}

GType
html_document_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (HtmlDocumentClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) html_document_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (HtmlDocument),
			0,   /* n_preallocs */
			(GInstanceInitFunc) html_document_init,
		};
		
		type = g_type_register_static (G_TYPE_OBJECT, "HtmlDocument", &info, 0);
	}

	return type;
}

HtmlDocument *
html_document_new (void)
{
	HtmlDocument *document = HTML_DOCUMENT (g_object_new (HTML_TYPE_DOCUMENT, NULL));
	GtkHtmlContext *context = gtk_html_context_get ();

	context->documents = g_slist_append (context->documents, document);
	
	return document;
}

gboolean
html_document_open_stream (HtmlDocument *document, const gchar *mime_type)
{
	g_return_val_if_fail (document != NULL, FALSE);
	g_return_val_if_fail (HTML_IS_DOCUMENT (document), FALSE);
	g_return_val_if_fail (mime_type != NULL, FALSE);

	html_document_clear (document);
	if (strcasecmp (mime_type, "text/html") == 0) {
		if (document->parser)
			g_object_unref (document->parser);
		
		document->parser = html_parser_new (document, HTML_PARSER_TYPE_HTML);
		document->current_stream = document->parser->stream;
		g_signal_connect (document->parser, "new_node",
				  (GCallback) html_document_new_node,
				  document);
		g_signal_connect (document->parser, "parsed_document_node",
				  (GCallback) html_document_parsed_document_node,
				  document);
		g_signal_connect (document->parser, "done_parsing",
				  (GCallback) html_document_done_parsing,
				  document);

		document->state = HTML_DOCUMENT_STATE_PARSING;
		return TRUE;
	}
	
	return FALSE;
}

void
html_document_write_stream (HtmlDocument *document, const gchar *buffer, gint len)
{
	g_return_if_fail (document != NULL);
	g_return_if_fail (HTML_IS_DOCUMENT (document));
	g_return_if_fail (document->current_stream != NULL);
	g_return_if_fail (buffer != NULL);

	if (len < 0)
		len = strlen (buffer);
	
	html_stream_write (document->current_stream, buffer, len);
}

void
html_document_close_stream (HtmlDocument *document)
{
	g_return_if_fail (document != NULL);
	g_return_if_fail (HTML_IS_DOCUMENT (document));
	g_return_if_fail (document->current_stream != NULL);

	html_stream_close (document->current_stream);
	document->state = HTML_DOCUMENT_STATE_DONE;

	g_signal_emit (G_OBJECT (document), document_signals [RELAYOUT_NODE], 0, DOM_NODE (dom_Document__get_documentElement (document->dom_document)));
}

void
html_document_clear (HtmlDocument *document)
{
	DomNode *node;
	DomNode *tmp_node;
	GSList *ss;
	xmlNodePtr top_node = NULL;
	DomEventListener *listener;
	
	if (!document->dom_document)
		return;

	/* Reset active, focused and hover nodes */
	html_document_update_hover_node (document, NULL);
	html_document_update_active_node (document, NULL);
	html_document_update_focus_element (document, NULL);
	
	listener = g_object_get_data (G_OBJECT (document), "dom-event-listener");
	if (listener) {
		g_object_set_data (G_OBJECT (document), "dom-event-listener", NULL);
	
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "DOMNodeInserted", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "DOMNodeRemoved", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "DOMCharacterDataModified", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "StyleChanged", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "mousedown", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "mouseup", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "click", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "mouseover", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "mouseout", listener, FALSE);
		dom_EventTarget_removeEventListener (DOM_EVENT_TARGET (document->dom_document),
						     "submit", listener, FALSE);
		g_object_unref (listener);
	}

	node = dom_Node__get_firstChild (DOM_NODE (document->dom_document));
	while (node) {
		tmp_node = node;
		
		top_node =  node->xmlnode;
		node = dom_Node__get_nextSibling (node);
		if (G_OBJECT (document)->ref_count != 0)
			/* Not called from html_document_finalize */
			g_signal_emit (G_OBJECT (document), document_signals [NODE_REMOVED], 0, tmp_node);
		dom_Node_removeChild (DOM_NODE (document->dom_document), tmp_node, NULL);
		g_object_unref (tmp_node);
	}

	if (top_node)
		xmlFreeNode (top_node);

	g_object_unref (document->dom_document);
	
	for (ss = document->stylesheets; ss; ss = ss->next) {
		CssStylesheet *sheet = ss->data;

		css_stylesheet_destroy (sheet);
	}

	g_slist_free (document->stylesheets);
	
	document->dom_document = NULL;
	document->stylesheets = NULL;
}

void
html_document_update_active_node (HtmlDocument *document, DomNode *node)
{
	HtmlAtom active_pseudo[] = { HTML_ATOM_ACTIVE, HTML_ATOM_HOVER, HTML_ATOM_FOCUS, 0};
	HtmlAtom hover_pseudo[] = { HTML_ATOM_HOVER, 0};
	HtmlStyleChange new_style_change;
	
	HtmlStyleChange style_change = HTML_STYLE_CHANGE_NONE;
	DomNode *tmp_node, *top_restyled_node = NULL;

	tmp_node = document->active_node;
	
	if (tmp_node) {
		while (tmp_node && tmp_node->style) {
			
			if (tmp_node->style->has_active_style) {
				top_restyled_node = tmp_node;
				style_change = html_document_restyle_node (document, tmp_node, hover_pseudo, TRUE);
			}
			
			tmp_node = dom_Node__get_parentNode (tmp_node);
		}

		if (top_restyled_node)
			g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, top_restyled_node, style_change);
	}

	tmp_node = node;
	top_restyled_node = NULL;
	
	while (tmp_node && tmp_node->style) {

		if (tmp_node->style->has_active_style) {

			top_restyled_node = tmp_node;
			
			if ((new_style_change = html_document_restyle_node (document, tmp_node, active_pseudo, FALSE)) > style_change)
				style_change = new_style_change;
		}
		
		tmp_node = dom_Node__get_parentNode (tmp_node);

	}

	if (top_restyled_node) 
		g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, top_restyled_node, style_change);

	document->active_node = node;
}

void
html_document_update_focus_element (HtmlDocument *document, DomElement *element)
{
	DomNode *tmp_node = NULL, *top_restyled_node = NULL;
	HtmlAtom focus_pseudo[3];
	HtmlStyleChange style_change = HTML_STYLE_CHANGE_NONE;
	HtmlStyleChange new_style_change;

	if (document->focus_element)
		tmp_node = DOM_NODE (document->focus_element);

	focus_pseudo[0] = HTML_ATOM_FOCUS;
	focus_pseudo[1] = 0;
	focus_pseudo[2] = 0;

	if (tmp_node) {
		while (tmp_node && tmp_node->style) {

			if (tmp_node->style->has_focus_style) {
				top_restyled_node = tmp_node;
				style_change = html_document_restyle_node (document, tmp_node, NULL, TRUE);
			}

			tmp_node = dom_Node__get_parentNode (tmp_node);

		}

		if (top_restyled_node)
			g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, top_restyled_node, style_change);
	}

	if (element) {
		tmp_node = DOM_NODE (element);
	}
	top_restyled_node = NULL;

	while (tmp_node && tmp_node->style) {

		if (tmp_node->style->has_focus_style) {

			top_restyled_node = tmp_node;

			if ((new_style_change = html_document_restyle_node (document, tmp_node, focus_pseudo, FALSE)) > style_change)
				style_change = new_style_change;
		}

		tmp_node = dom_Node__get_parentNode (tmp_node);
	}
	
	if (top_restyled_node) {
		if ((new_style_change = html_document_restyle_node (document, top_restyled_node, focus_pseudo, TRUE)) > style_change)
			style_change = new_style_change;
		
		g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, top_restyled_node, style_change);
	}

	document->focus_element = element;
}

void
html_document_update_hover_node (HtmlDocument *document, DomNode *node)
{
	DomNode *tmp_node, *top_restyled_node = NULL;
	HtmlAtom hover_pseudo[] = { HTML_ATOM_HOVER, 0 };
	HtmlStyleChange style_change = HTML_STYLE_CHANGE_NONE;
	HtmlStyleChange new_style_change;
	
	tmp_node = document->hover_node;
	if (tmp_node) {
		while (tmp_node && tmp_node->style) {
			
			if (tmp_node->style->has_hover_style) {
				top_restyled_node = tmp_node;
				style_change = html_document_restyle_node (document, tmp_node, NULL, TRUE);
			}
			
			tmp_node = dom_Node__get_parentNode (tmp_node);
		}

		if (top_restyled_node) 
			g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, top_restyled_node, style_change);
	}
	tmp_node = node;
	top_restyled_node = NULL;

	while (tmp_node && tmp_node->style) {
		
		if (tmp_node->style->has_hover_style) {
			
			top_restyled_node = tmp_node;
			if ((new_style_change = html_document_restyle_node (document, tmp_node, hover_pseudo, FALSE)) > style_change)
				style_change = new_style_change;
		}
		
		tmp_node = dom_Node__get_parentNode (tmp_node);
	}
	if (top_restyled_node) {
		if ((new_style_change = html_document_restyle_node (document, top_restyled_node, hover_pseudo, TRUE)) > 
		    style_change)
				style_change = new_style_change;

		g_signal_emit (G_OBJECT (document), document_signals [STYLE_UPDATED], 0, top_restyled_node, style_change);
	}

	document->hover_node = node;
}

static DomNode *
find_anchor_helper (DomNode *node, const gchar *anchor)
{
	DomNode *child;

	if (DOM_IS_HTML_ANCHOR_ELEMENT (node)) {
	    gchar *name;

	    if (dom_Element_hasAttribute (DOM_ELEMENT (node), "id"))
		name = dom_Element_getAttribute (DOM_ELEMENT (node), "id");
	    else if (dom_Element_hasAttribute (DOM_ELEMENT (node), "name"))
		name = dom_Element_getAttribute (DOM_ELEMENT (node), "name");
	    else
		name = NULL;

	    if (name) {
		if (strcasecmp (name, anchor) == 0) {

			xmlFree (name);
			return node;
		}
		xmlFree (name);
	    }
	}
	child = dom_Node__get_firstChild (node);

	while (child) {

		DomNode *result = find_anchor_helper (child, anchor);

		if (result != NULL)
			return result;

		child = dom_Node__get_nextSibling (child);
	}
	return NULL;
}

DomNode *
html_document_find_anchor (HtmlDocument *doc, const gchar *anchor)
{
	if (doc->dom_document)
		return find_anchor_helper (DOM_NODE (doc->dom_document), anchor);
	else
		return NULL;
}
