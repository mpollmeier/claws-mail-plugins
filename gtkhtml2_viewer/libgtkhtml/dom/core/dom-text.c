/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2000-2001 CodeFactory AB
 * Copyright (C) 2000-2001 Anders Carlsson <andersca@codefactory.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include "dom-text.h"

/**
 * dom_Text_splitText:
 * @text: a DomText
 * @offset: The offset at which to split, starting from 0.
 * @exc: Return location for an exception.
 * 
 * Breaks this node into two nodes at the specified offset, keeping both in the tree as siblings.
 * 
 * Return value: The newly created text node.
 **/
DomText *
dom_Text_splitText (DomText *text, gulong offset, DomException *exc)
{
	gint datalength = g_utf8_strlen (DOM_NODE (text)->xmlnode->content, -1);
	DomString *substring;
	DomNode *new_node;
	
	if (offset > datalength || offset < 0) {
		DOM_SET_EXCEPTION (DOM_INDEX_SIZE_ERR);
		return NULL;
	}

	/* First, substring the data from offset to the end of the string */
	substring = dom_CharacterData_substringData (DOM_CHARACTER_DATA (text), offset, datalength - offset, NULL);

	/* Then delete the data from the node */
	dom_CharacterData_deleteData (DOM_CHARACTER_DATA (text), 0, offset, NULL);

	/* Finally create a new node */
	new_node = dom_Node_mkref (xmlNewDocTextLen (DOM_NODE (text)->xmlnode->doc, substring, strlen (substring)));

	/* And append it */
	xmlAddNextSibling (DOM_NODE (text)->xmlnode, new_node->xmlnode);

	/* Finally, return the new text node */
	return DOM_TEXT (new_node);
}

static void
dom_text_class_init (DomTextClass *klass)
{
}

static void
dom_text_init (DomText *doc)
{
}

GType
dom_text_get_type (void)
{
	static GType dom_text_type = 0;

	if (!dom_text_type) {
		static const GTypeInfo dom_text_info = {
			sizeof (DomTextClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_text_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomText),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_text_init,
		};

		dom_text_type = g_type_register_static (DOM_TYPE_CHARACTER_DATA, "DomText", &dom_text_info, 0);
	}

	return dom_text_type;
}
