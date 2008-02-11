/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>
   
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
#include <libxml/parser.h>
#include <libxml/SAX.h>
#include "document/htmldocument.h"
#include <string.h>

enum {
	NEW_NODE,
	DONE_PARSING,
	PARSED_DOCUMENT_NODE,
	LAST_SIGNAL
};

static guint parser_signals [LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;

static void
html_parser_emit_new_node (HtmlParser *parser, DomNode *node)
{
	g_signal_emit (G_OBJECT (parser), parser_signals [NEW_NODE], 0, node);
}

static void 
html_comment (void *ctx, const xmlChar *ch)
{
	HtmlParser *parser = HTML_PARSER(ctx);
	DomNode *node;

	xmlSAX2Comment (parser->xmlctxt, ch);

	node = dom_Node_mkref (xmlGetLastChild (parser->xmlctxt->node));
	if (node)
		html_parser_emit_new_node (parser, node);
}

static void
html_characters (void *ctx, const xmlChar *ch, int len)
{
	HtmlParser *parser = HTML_PARSER (ctx);
	DomNode *node;

	xmlSAX2Characters (parser->xmlctxt, ch, len);
	
	node = dom_Node_mkref (xmlGetLastChild (parser->xmlctxt->node));
	
	html_parser_emit_new_node (parser, node);
}

static void
html_startElement(void *ctx, const xmlChar *name, const xmlChar **atts)
{
	HtmlParser *parser = HTML_PARSER (ctx);
	DomNode *node;

	xmlSAX2StartElement (parser->xmlctxt, name, atts);

	node = dom_Node_mkref (parser->xmlctxt->node);

	html_parser_emit_new_node (parser, node);
}

static void
html_endElement (void *ctx, const xmlChar *name)
{
	HtmlParser *parser = HTML_PARSER (ctx);

	xmlSAX2EndElement (parser->xmlctxt, name);
}

static void
html_startDocument (void *ctx)
{
	HtmlParser *parser = HTML_PARSER (ctx);
	
	xmlSAX2StartDocument (parser->xmlctxt);

	if (parser->document->dom_document) {
		g_warning ("DomDocument leaked in html_startDocument");
	}
	parser->document->dom_document = DOM_DOCUMENT (dom_Node_mkref ((xmlNode *)parser->xmlctxt->myDoc));

	/* Emit document node parsed signal */
	g_signal_emit (G_OBJECT (parser), parser_signals [PARSED_DOCUMENT_NODE], 
		       0, DOM_DOCUMENT (parser->document->dom_document));	

}

static void
html_endDocument (void *ctx)
{
	HtmlParser *parser = HTML_PARSER (ctx);
	
	xmlSAX2EndDocument (parser->xmlctxt);
}

xmlSAXHandler SAXHandlerStruct = {
    NULL, /* internalSubset */
    NULL, /* isStandalone */
    NULL, /* hasInternalSubset */
    NULL, /* hasExternalSubset */
    NULL, /* resolveEntity */
    NULL, /* getEntity */
    NULL, /* entityDecl */
    NULL, /* notationDecl */
    NULL, /* attributeDecl */
    NULL, /* elementDecl */
    NULL, /* unparsedEntityDecl */
    NULL, /* setDocumentLocator */
    html_startDocument, /* startDocument */
    html_endDocument, /* endDocument */
    html_startElement, /* startElement */
    html_endElement, /* endElement */
    NULL, /* reference */
    html_characters, /* characters */
    NULL, /* ignorableWhitespace */
    NULL, /* processingInstruction */
    html_comment, /* comment */
    NULL, /* xmlParserWarning */
    NULL, /* xmlParserError */
    NULL, /* xmlParserError */
    NULL, /* getParameterEntity */
    html_characters, /* cdataBlock; */
};

xmlSAXHandlerPtr SAXHandler = &SAXHandlerStruct;

static void
html_parser_stream_write (HtmlStream *stream, const gchar *buffer,
			  guint size, gpointer user_data)
{
	HtmlParser *parser;

	if (!user_data)
		return;

	parser = HTML_PARSER (user_data);

	if (parser->parser_type == HTML_PARSER_TYPE_HTML) {
		htmlParseChunk (parser->xmlctxt, buffer, size, 0);
	}
	else {
		xmlParseChunk (parser->xmlctxt, buffer, size, 0);
	}
}

static void
html_parser_stream_close (HtmlStream *stream, gpointer user_data)
{
	HtmlParser *parser;

	if (!user_data)
		return;

	parser = HTML_PARSER (user_data);

	if (parser->parser_type == HTML_PARSER_TYPE_HTML) {
		htmlParseChunk (parser->xmlctxt, NULL, 0, 1);
	}
	else {
		xmlParseChunk (parser->xmlctxt, NULL, 0, 1);
	}

	/* Emit done loading signal */
	g_signal_emit (G_OBJECT (parser), parser_signals [DONE_PARSING], 0);
}

static void
html_parser_finalize (GObject *object)
{
	HtmlParser *parser = HTML_PARSER (object);
printf("finalizing parser\n");
	if (parser->xmlctxt) {
		xmlDocPtr doc;

		doc = parser->xmlctxt->myDoc;
		if (parser->parser_type == HTML_PARSER_TYPE_HTML)
			htmlFreeParserCtxt (parser->xmlctxt);
		else
			xmlFreeParserCtxt (parser->xmlctxt);
		if (doc)
			xmlFreeDoc (doc);
	}

	parent_class->finalize (object);
}

static void
html_parser_class_init (HtmlParserClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;
	object_class->finalize = html_parser_finalize;

	parent_class = g_type_class_peek_parent (object_class);

	parser_signals [NEW_NODE] =
		g_signal_new ("new_node",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlParserClass, new_node),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      DOM_TYPE_NODE);

	parser_signals [PARSED_DOCUMENT_NODE] =
		g_signal_new ("parsed_document_node",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlParserClass, parsed_document_node),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      DOM_TYPE_DOCUMENT);

	parser_signals [DONE_PARSING] =
		g_signal_new ("done_parsing",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlParserClass, done_parsing),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

}

static void
html_parser_init (HtmlParser *parser)
{
}

GtkType
html_parser_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (HtmlParserClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) html_parser_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (HtmlParser),
			16,   /* n_preallocs */
			(GInstanceInitFunc) html_parser_init,
		};

		type = g_type_register_static (G_TYPE_OBJECT, "HtmlParser", &info, 0);
		
	}

	return type;
}

static void
html_parser_set_type (HtmlParser *parser, HtmlParserType parser_type)
{
	parser->parser_type = parser_type;

	/* FIXME: Free parser if existing */
	if (parser_type == HTML_PARSER_TYPE_HTML) {
		parser->xmlctxt = htmlCreatePushParserCtxt (SAXHandler, parser,
							    parser->chars, parser->res, NULL, 0);
	}
	else {
		parser->xmlctxt = xmlCreatePushParserCtxt (SAXHandler, parser,
							   parser->chars, parser->res, NULL);
	}


}

HtmlParser *
html_parser_new (HtmlDocument *document, HtmlParserType parser_type)
{
	HtmlParser *parser;

	parser = g_object_new (HTML_PARSER_TYPE, NULL);

	parser->document = document;

	parser->stream = html_stream_new (html_parser_stream_write,
					  html_parser_stream_close,
					  parser);

	html_parser_set_type (parser, parser_type);
	
	return parser;
}
