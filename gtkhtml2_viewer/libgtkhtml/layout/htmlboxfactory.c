/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgstr\366m <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <libxml/parser.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "document/htmldocument.h"
#include "css/cssmatcher.h"
#include "util/htmlglobalatoms.h"
#include "htmlbox.h"
#include "htmlboxtext.h"
#include "htmlboxroot.h"
#include "htmlboxinline.h"
#include "htmlboxblock.h"
#include "htmlboxtable.h"
#include "htmlboxlistitem.h"
#include "htmlboxtablerow.h"
#include "htmlboxtablerowgroup.h"
#include "htmlboxtablecell.h"
#include "htmlboxtablecaption.h"
#include "htmlboxfactory.h"
#include "html/htmlboximage.h"
#include "html/htmlboxform.h"
#include "html/htmlboxembeddedimage.h"
#include "html/htmlboxembeddedbutton.h"
#include "html/htmlboxembeddedentry.h"
#include "html/htmlboxembeddedcheckbox.h"
#include "html/htmlboxembeddedradio.h"
#include "html/htmlboxembeddedtextarea.h"
#include "html/htmlboxembeddedselect.h"
#include "html/htmlboxembeddedobject.h"

static void
add_before_and_after_elements (HtmlDocument *document, HtmlBox *box, HtmlStyle *boxstyle, HtmlStyle *parentstyle, xmlNode *n)
{
	HtmlAtom before_pseudo[] = { HTML_ATOM_BEFORE, 0 };	
	HtmlAtom after_pseudo[] = { HTML_ATOM_AFTER, 0 };
	HtmlStyle *style;

	if (boxstyle->has_before_style) {
		style = css_matcher_get_style (document, parentstyle, n, before_pseudo);
		
		if (style->content) {
			HtmlBox *before;
			
			before = html_box_text_new (TRUE);
			before->parent = box;
			html_box_set_style (before, style);
			/* We  have to hardcode these values or else we might inherit something else */
			HTML_BOX_GET_STYLE (before)->display = HTML_DISPLAY_INLINE;
			html_box_text_set_generated_content (HTML_BOX_TEXT (before), style->content);
			
			html_box_set_before (box, before);
		}
		else
			html_style_unref (style);
	}
	
	if (boxstyle->has_after_style) {
		style = css_matcher_get_style (document, parentstyle, n, after_pseudo);
		
		if (style->content) {
			HtmlBox *before;
			
			before = html_box_text_new (TRUE);
			before->parent = box;
			html_box_set_style (before, style);
			/* We  have to hardcode these values or else we might inherit something else */
			HTML_BOX_GET_STYLE (before)->display = HTML_DISPLAY_INLINE;
			html_box_text_set_generated_content (HTML_BOX_TEXT (before), style->content);
			
			html_box_set_after (box, before);
		}
		else
			html_style_unref (style);
	}
}


HtmlBox *
html_box_factory_new_box (HtmlView *view, DomNode *node, gboolean force_new)
{
	HtmlBox *box = NULL, *parent_box;
	HtmlStyle *style = node->style, *parent_style = NULL;
	gboolean make_new_text_box = TRUE;
	gchar *str;

	parent_box = html_view_find_layout_box (view, dom_Node__get_parentNode (node), FALSE);
	if (parent_box)
		parent_style = HTML_BOX_GET_STYLE (parent_box);
	
	switch (node->xmlnode->type) {
	case XML_TEXT_NODE:

		g_return_val_if_fail (parent_box != NULL, NULL);

		box = parent_box->children;

		while (box && !force_new) {
			if (HTML_IS_BOX_TEXT (box) && box->dom_node == node) {
				
				html_box_text_set_text (HTML_BOX_TEXT (box), node->xmlnode->content);
				box = NULL;
				make_new_text_box = FALSE;
				
				return NULL;
			}
			box = box->next;
		}
		
		if (make_new_text_box) {
			box = html_box_text_new (TRUE);
			html_box_text_set_text (HTML_BOX_TEXT (box), node->xmlnode->content);
#if 0
			html_box_set_style (box, HTML_BOX_GET_STYLE (parent_box));
			box->dom_node = node;
#endif
		}
		
		return box;
		
	case XML_ELEMENT_NODE:
		if (xmlDocGetRootElement (node->xmlnode->doc) == node->xmlnode) {
			box = html_box_root_new ();
			break;
		}
		switch (html_atom_list_get_atom (html_atom_list, node->xmlnode->name)) {
		case HTML_ATOM_FORM:
			box = html_box_form_new ();
			break;
		case HTML_ATOM_TEXTAREA:
			box = html_box_embedded_textarea_new (view, node);
			break;
		case HTML_ATOM_SELECT:
			box = html_box_embedded_select_new (view, node);
			break;
		case HTML_ATOM_OBJECT:
			box = html_box_embedded_object_new (view, node);
			break;
		case HTML_ATOM_INPUT:
			if ((str = xmlGetProp (node->xmlnode, "type"))) {
				switch (html_atom_list_get_atom (html_atom_list, str)) {
				case HTML_ATOM_SUBMIT:
					box = html_box_embedded_button_new (view, HTML_BOX_EMBEDDED_BUTTON_TYPE_SUBMIT);
					break;
				case HTML_ATOM_RESET:
					box = html_box_embedded_button_new (view, HTML_BOX_EMBEDDED_BUTTON_TYPE_RESET);
					break;
				case HTML_ATOM_TEXT:
					box = html_box_embedded_entry_new (view, HTML_BOX_EMBEDDED_ENTRY_TYPE_TEXT);
					break;
				case HTML_ATOM_PASSWORD:
					box = html_box_embedded_entry_new (view, HTML_BOX_EMBEDDED_ENTRY_TYPE_PASSWORD);
					break;
				case HTML_ATOM_CHECKBOX:
					box = html_box_embedded_checkbox_new (view);
					break;
				case HTML_ATOM_RADIO:
					box = html_box_embedded_radio_new (view);
					break;
				case HTML_ATOM_HIDDEN:
					xmlFree (str);
					return NULL;
				case HTML_ATOM_IMAGE:
					if (xmlHasProp (node->xmlnode, "src")) {
						
						HtmlImage *image = g_object_get_data (G_OBJECT (node), "image");
						box = html_box_embedded_image_new (view);
						html_box_embedded_image_set_image (HTML_BOX_EMBEDDED_IMAGE (box), image);
					}
					break;
				default:
					/* FIXME: not sure about this one */
					box = html_box_embedded_entry_new (view, HTML_BOX_EMBEDDED_ENTRY_TYPE_TEXT);
					break;
				}
				xmlFree (str);
			}
			else
				box = html_box_embedded_entry_new (view, HTML_BOX_EMBEDDED_ENTRY_TYPE_TEXT);
			break;
		case HTML_ATOM_IMG:
			if (xmlHasProp (node->xmlnode, "src")) {
				
				HtmlImage *image = g_object_get_data (G_OBJECT (node), "image");
				box = html_box_image_new (view);
				html_box_image_set_image (HTML_BOX_IMAGE (box), image);
			}
			break;
		default:
			switch (style->display) {
			case HTML_DISPLAY_NONE:
				return NULL;
				break;
			case HTML_DISPLAY_BLOCK:
				box = html_box_block_new ();
				add_before_and_after_elements (view->document, box, style, parent_style, node->xmlnode);
				break;
			case HTML_DISPLAY_INLINE:
				box = html_box_inline_new ();
				add_before_and_after_elements (view->document, box, style, parent_style, node->xmlnode);
				break;
			case HTML_DISPLAY_LIST_ITEM:
				box = html_box_list_item_new ();
				break;
			case HTML_DISPLAY_TABLE:
			case HTML_DISPLAY_INLINE_TABLE:
				box = html_box_table_new ();
				break;
			case HTML_DISPLAY_TABLE_ROW:
				box = html_box_table_row_new ();
				break;
			case HTML_DISPLAY_TABLE_CAPTION:
				box = html_box_table_caption_new ();
				break;
			case HTML_DISPLAY_TABLE_CELL:
				box = html_box_table_cell_new ();
				add_before_and_after_elements (view->document, box, style, parent_style, node->xmlnode);
				break;
			case HTML_DISPLAY_TABLE_ROW_GROUP:
			case HTML_DISPLAY_TABLE_HEADER_GROUP:
			case HTML_DISPLAY_TABLE_FOOTER_GROUP:
				box = html_box_table_row_group_new (style->display);
				break;
			default:
				g_error ("unknown style: %d", style->display);
			}
			break;
		}
		break;
	default:
		return NULL;
	}
#if 0
	html_box_set_style (box, style);
	box->dom_node = node;
#endif
	
	return box;
}
