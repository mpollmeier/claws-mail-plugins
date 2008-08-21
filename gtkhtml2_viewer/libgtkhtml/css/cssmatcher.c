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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>

#include "libgtkhtml/dom/core/dom-element.h"
#include "libgtkhtml/document/htmldocument.h"
#include "libgtkhtml/layout/htmlstyle.h"
#include "libgtkhtml/util/htmlstream.h"
#include "libgtkhtml/util/htmlglobalatoms.h"

#include "cssdebug.h"
#include "cssmatcher.h"
#include "cssparser.h"
#include "html.css.h"

static CssStylesheet *default_stylesheet = NULL;

typedef struct _CssDeclarationListEntry CssDeclarationListEntry;

struct _CssDeclarationListEntry {
	gint spec;
	gint type;
	CssDeclaration *decl;
};

enum {
	CSS_STYLESHEET_DEFAULT,
	CSS_STYLESHEET_USER,
	CSS_STYLESHEET_AUTHOR,
	CSS_STYLESHEET_STYLEDECL
};

enum {
	CSS_STYLESHEET_PSEUDO_NONE = 0,
	CSS_STYLESHEET_PSEUDO_HOVER = (1 << 0),
	CSS_STYLESHEET_PSEUDO_ACTIVE = (1 << 1),
	CSS_STYLESHEET_PSEUDO_FOCUS = (1 << 2),
	CSS_STYLESHEET_PSEUDO_BEFORE = (1 << 3),
	CSS_STYLESHEET_PSEUDO_AFTER = (1 << 4)
};

gint current_pseudos, total_pseudos;

static gboolean
css_matcher_match_simple_selector (CssSimpleSelector *simple, xmlNode *node, HtmlAtom *pseudo)
{
	gchar *str, *href;
	gint element_name = -1, element_id = -1; 
	gint i;
	gboolean matched;
	
	/* Don't try to match NULL nodes and nodes that aren't elements */
	if (!node || node->type != XML_ELEMENT_NODE)
		return FALSE;

	element_name = html_atom_list_get_atom (html_atom_list, node->name);
	
	/* Look at the element name */
	if ((!simple->is_star) && (simple->element_name != element_name))
		return FALSE;

	str = xmlGetProp (node, "id");
	if (str) {
		element_id = html_atom_list_get_atom (html_atom_list, str);
		xmlFree (str);
	}

	for (i = 0;i < simple->n_tail; i++) {
		CssTail *tail = &simple->tail[i];

		if (tail->type == CSS_TAIL_ID_SEL &&
		    tail->t.id_sel.id != element_id)
			return FALSE;
		else if (tail->type == CSS_TAIL_CLASS_SEL) {
			gchar *element_class = xmlGetProp (node, "class");

			matched = FALSE;
			
			if (element_class) {
				const gchar *beg, *end, *next, *matchstr;
				gint matchlen;
				
				matchstr = html_atom_list_get_string (html_atom_list, tail->t.class_sel.css_class);
				matchlen = strlen (matchstr);

				for (beg = element_class; beg != NULL; beg = next) {
					end = strchr (beg, ' ');
					if (end == NULL) {
						end = beg + strlen (beg);
						next = NULL;
					}
					else
						next = end + 1;
					if (matchlen == end - beg && strncasecmp (matchstr, beg, matchlen) == 0)
						matched = TRUE;
				}

				xmlFree (element_class);
			}

			if (!matched) {
				return FALSE; 
			}
		}
		else if (tail->type == CSS_TAIL_ATTR_SEL) {
			gchar *str;

			str = xmlGetProp (node, html_atom_list_get_string (html_atom_list, tail->t.attr_sel.att));

			if (!str) {
				return FALSE;
			}

			/*
			 * We don't test for CSS_MATCH_EMPTY, since we've already returned false if the property
			 * doesn't exist.
			 */

			if (tail->t.attr_sel.match == CSS_MATCH_EQ) {
				gchar *val = NULL;

				if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_IDENT) {
					val = html_atom_list_get_string (html_atom_list, tail->t.attr_sel.val.a.id);
				}
				else if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_STRING) {
					val = tail->t.attr_sel.val.a.str;
				}

				if (strcasecmp (val, str) != 0) {
					xmlFree (str);
					return FALSE;
				}
			}
			else if (tail->t.attr_sel.match == CSS_MATCH_INCLUDES) {

				const gchar *beg, *end, *next, *matchstr = NULL;
				gint matchlen;

				matched = FALSE;

				if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_IDENT) {
					matchstr = html_atom_list_get_string (html_atom_list, tail->t.attr_sel.val.a.id);
				}
				else if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_STRING) {
					matchstr = tail->t.attr_sel.val.a.str;
				}

				matchlen = strlen (matchstr);

				for (beg = str; beg != NULL; beg = next) {
					end = strchr (beg, ' ');
					if (end == NULL) {
						end = beg + strlen (beg);
						next = NULL;
					}
					else
						next = end + 1;
					if (matchlen == end - beg && memcmp (matchstr, beg, matchlen) == 0)
						matched = TRUE;
				}
				
				if (!matched) {
					xmlFree (str);
					return FALSE; 
				}
			}
			else if (tail->t.attr_sel.match == CSS_MATCH_DASHMATCH) {
				const gchar *end, *matchstr = NULL;
				gint matchlen;


				if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_IDENT) {
					matchstr = html_atom_list_get_string (html_atom_list, tail->t.attr_sel.val.a.id);
				}
				else if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_STRING) {
					matchstr = tail->t.attr_sel.val.a.str;
				}
				matchlen = strlen (matchstr);

				end = strchr (str, '-');
				if ((end == NULL && strlen (str) != matchlen) ||
				    end - str != matchlen) {
					xmlFree (str);
					return FALSE;
				}
				if (memcmp (matchstr, str, matchlen) != 0) {
					xmlFree (str);
					return FALSE;
				}
			}
			xmlFree (str);
		}
		else if (tail->type == CSS_TAIL_PSEUDO_SEL) {
			switch (tail->t.pseudo_sel.name) {
			case HTML_ATOM_LINK:
				
				break;
				
				/*  This should only match for links that are unvisited, but
				    we don't have any mekanism for knowing this so we
				    match all links / jb */
				if ((href = xmlGetProp (node, "href")))
					xmlFree (href);
				else
					return FALSE;
				break;

			case HTML_ATOM_HOVER:
				current_pseudos |= CSS_STYLESHEET_PSEUDO_HOVER;
				break;

			case HTML_ATOM_ACTIVE:
				current_pseudos |= CSS_STYLESHEET_PSEUDO_ACTIVE;
				break;

			case HTML_ATOM_FOCUS:
				current_pseudos |= CSS_STYLESHEET_PSEUDO_FOCUS;
				break;

			case HTML_ATOM_BEFORE:
				current_pseudos |= CSS_STYLESHEET_PSEUDO_BEFORE;
				break;

			case HTML_ATOM_AFTER:
				current_pseudos |= CSS_STYLESHEET_PSEUDO_AFTER;
				break;
				
			case HTML_ATOM_FIRST_CHILD:
				while (node->prev && node->prev->type != XML_ELEMENT_NODE)
					node = node->prev;
				
				if (node->prev != NULL)
					return FALSE;
				break;
				
			default: 
				matched = FALSE;
				
				if (!pseudo)
					return FALSE;
				
				for (i = 0; pseudo[i]; i++) {
					if (tail->t.pseudo_sel.name == pseudo[i])
						matched = TRUE;
				}
				
				if (!matched)
					return FALSE;
			}
		}
	}

	/* Since we haven't returned FALSE anywhere we must have a selector match */
	return TRUE;
}

static gboolean
css_matcher_match_selector (CssSelector *sel, xmlNode *node, HtmlAtom *pseudo)
{
	CssSimpleSelector *simple = sel->simple [sel->n_simple - 1];
	CssCombinator comb;
	gboolean matched;
	gint i;

	current_pseudos = CSS_STYLESHEET_PSEUDO_NONE;

	if (!css_matcher_match_simple_selector (simple, node, pseudo))
		return FALSE;

	for (i = sel->n_simple - 1; i > 0; i --) {
		simple = sel->simple [i - 1];
		comb = sel->comb [i - 1];

		/* Child selector */
		if (comb == CSS_COMBINATOR_GT) {
			node = node->parent;

			if (!css_matcher_match_simple_selector (simple, node, pseudo))
				return FALSE;
		}
		/* Adjacent sibling selector */
		else if (comb == CSS_COMBINATOR_PLUS) {

			node = node->prev;
			
			while (node && node->type != XML_ELEMENT_NODE)
				node = node->prev;

			if (!css_matcher_match_simple_selector (simple, node, pseudo))
				return FALSE;
		}
		/* Descendant selector */
		else if (comb == CSS_COMBINATOR_EMPTY) {
			matched = FALSE;

			while (node) {

				node = node->parent;
				

				if (css_matcher_match_simple_selector (simple, node, pseudo)) {
					matched = TRUE;

					while (node->parent && css_matcher_match_simple_selector (simple, node->parent, pseudo) &&
					       !simple->is_star)
						node = node->parent;
					break;
				}
				
			}
			if (!matched)
				return FALSE;
			
		}
		else if (comb == CSS_COMBINATOR_TILDE) {
			matched = FALSE;

			while (node) {
				node = node->prev;

				if (css_matcher_match_simple_selector (simple, node, pseudo)) {
					matched = TRUE;

					while (node->prev && css_matcher_match_simple_selector (simple, node->prev, pseudo))
						node = node->prev;
					break;
				}
			}

			if (!matched)
				return FALSE;
		}
	}

	total_pseudos |= current_pseudos;
	
	/* FIXME: This is inefficient */
	if (current_pseudos & CSS_STYLESHEET_PSEUDO_HOVER) {
		matched = FALSE;
		for (i = 0; pseudo && pseudo[i]; i++) {
			if (pseudo[i] == HTML_ATOM_HOVER)
				matched = TRUE;
		}

		if (!matched)
			return FALSE;
	}

	/* FIXME: This is inefficient */
	if (current_pseudos & CSS_STYLESHEET_PSEUDO_ACTIVE) {
		matched = FALSE;
		for (i = 0; pseudo && pseudo[i]; i++) {
			if (pseudo[i] == HTML_ATOM_ACTIVE)
				matched = TRUE;
		}

		if (!matched)
			return FALSE;
	}

	/* FIXME: This is inefficient */
	if (current_pseudos & CSS_STYLESHEET_PSEUDO_FOCUS) {
		matched = FALSE;
		for (i = 0; pseudo && pseudo[i]; i++) {
			if (pseudo[i] == HTML_ATOM_FOCUS)
				matched = TRUE;
		}

		if (!matched)
			return FALSE;
	}

	
	/* FIXME: This is inefficient */
	if (current_pseudos & CSS_STYLESHEET_PSEUDO_BEFORE) {
		matched = FALSE;
		for (i = 0; pseudo && pseudo[i]; i++) {
			if (pseudo[i] == HTML_ATOM_BEFORE)
				matched = TRUE;
		}

		if (!matched)
			return FALSE;
	}


	/* FIXME: This is inefficient */
	if (current_pseudos & CSS_STYLESHEET_PSEUDO_AFTER) {
		matched = FALSE;
		for (i = 0; pseudo && pseudo[i]; i++) {
			if (pseudo[i] == HTML_ATOM_AFTER)
				matched = TRUE;
		}

		if (!matched)
			return FALSE;
	}

	return TRUE;
}

static gint
css_declaration_list_sorter (gconstpointer p1, gconstpointer p2)
{
	CssDeclarationListEntry *entry1 = (CssDeclarationListEntry *)p1;
	CssDeclarationListEntry *entry2 = (CssDeclarationListEntry *)p2;

	if (entry1->type > entry2->type)
		return 1;
	else if (entry1->type < entry2->type)
		return -1;
	else {
		if (entry1->decl->important && !entry2->decl->important)
			return 1;
		else if (!entry1->decl->important && entry2->decl->important)
			return -1;
		else {
	  		if (entry1->spec > entry2->spec)
				return 1;
			else if (entry1->spec < entry2->spec)
				return -1;
			else
				return 1;
		}
	}
}

static gboolean
css_parse_border_width (HtmlFontSpecification *old_font, CssValue *val, gint *width)
{
	HtmlLength length;
	
	if (val->value_type == CSS_IDENT) {
		switch (val->v.atom) {
		case HTML_ATOM_THIN:
			*width = HTML_BORDER_WIDTH_THIN;
			break;
		case HTML_ATOM_MEDIUM:
			*width = HTML_BORDER_WIDTH_MEDIUM;
			break;
		case HTML_ATOM_THICK:
			*width = HTML_BORDER_WIDTH_THICK;
			break;
		default:
			return FALSE;
		}

		return TRUE;
	}
	else if (html_length_from_css_value (old_font, val, &length)) {
		*width = html_length_get_value (&length, 0);
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
css_parse_color (CssValue *val, HtmlColor *color)
{
	HtmlColor *tmp_color = NULL;
	gchar *str = css_value_to_string (val);

	if (str) {
		tmp_color = html_color_new_from_name (str);
		g_free (str);
	}

	if (!tmp_color)
		return FALSE;
	
	if (color) {
		color->refcount = tmp_color->refcount;
		color->red = tmp_color->red;
		color->green = tmp_color->green;
		color->blue = tmp_color->blue;
		color->transparent = tmp_color->transparent;
	}

	html_color_destroy (tmp_color);

	return TRUE;
}

static gboolean
css_parse_border_style (CssValue *val, HtmlBorderStyleType *style)
{
	switch (val->v.atom) {
	case HTML_ATOM_HIDDEN:
		*style = HTML_BORDER_STYLE_HIDDEN;
		return TRUE;
	case HTML_ATOM_DOTTED:
		*style = HTML_BORDER_STYLE_DOTTED;
		return TRUE;
	case HTML_ATOM_DASHED:
		*style = HTML_BORDER_STYLE_DASHED;
		return TRUE;
	case HTML_ATOM_SOLID:
		*style = HTML_BORDER_STYLE_SOLID;
		return TRUE;
	case HTML_ATOM_DOUBLE:
		*style = HTML_BORDER_STYLE_DOUBLE;
		return TRUE;
	case HTML_ATOM_GROOVE:
		*style = HTML_BORDER_STYLE_GROOVE;
		return TRUE;
	case HTML_ATOM_RIDGE:
		*style = HTML_BORDER_STYLE_RIDGE;
		return TRUE;
	case HTML_ATOM_INSET:
		*style = HTML_BORDER_STYLE_INSET;
		return TRUE;
	case HTML_ATOM_OUTSET:
		*style = HTML_BORDER_STYLE_OUTSET;
		return TRUE;
	default:
		return FALSE;
	}
}

static gint
length_to_pixels (const CssValue *val)
{
	if (val->value_type == CSS_PX)
		return val->v.d;
	return 0;
}

static gboolean
handle_background_repeat (HtmlDocument *document, HtmlStyle *style, HtmlStyle *parent_style, CssValue *val)
{
	switch (val->v.atom) {
	case HTML_ATOM_INHERIT:
		html_style_set_background_repeat (style, parent_style->background->repeat);
		break;
	case HTML_ATOM_REPEAT:
		html_style_set_background_repeat (style, HTML_BACKGROUND_REPEAT_REPEAT);
		break;
	case HTML_ATOM_REPEAT_X:
		html_style_set_background_repeat (style, HTML_BACKGROUND_REPEAT_REPEAT_X);
		break;
	case HTML_ATOM_REPEAT_Y:
		html_style_set_background_repeat (style, HTML_BACKGROUND_REPEAT_REPEAT_Y);
		break;
	case HTML_ATOM_NO_REPEAT:
		html_style_set_background_repeat (style, HTML_BACKGROUND_REPEAT_NO_REPEAT);
		break;
	case HTML_ATOM_SCALE:
		html_style_set_background_repeat (style, HTML_BACKGROUND_REPEAT_SCALE);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static gboolean
handle_background_image (HtmlDocument *document, HtmlStyle *style, CssValue *val)
{
	if (val->value_type == CSS_FUNCTION &&
	    val->v.function->name == HTML_ATOM_URL &&
	    val->v.function->args) {
		
		HtmlImage *image = NULL;
		gchar *str = css_value_to_string (val->v.function->args);

		if (str && val->v.function->args->value_type == CSS_STRING) {
			image = html_image_factory_get_image (document->image_factory, val->v.function->args->v.s);
		}
		g_free (str);
		if (image) {
			html_style_set_background_image (style, image);
			g_object_unref (G_OBJECT(image));
			return TRUE;
		}
	} else if (val->value_type == CSS_IDENT &&
		   val->v.atom == HTML_ATOM_NONE) {
		if (style->background->image) {
			g_object_unref (style->background->image);
			style->background->image = NULL;
		}
	}
	return FALSE;
}

void
css_matcher_apply_rule (HtmlDocument *document, HtmlStyle *style, HtmlStyle *parent_style, HtmlFontSpecification *old_font, CssDeclaration *decl)
{
	gint prop = decl->property;
	CssValue *val = decl->expr;
	HtmlBorderStyleType border_style;
	gint border_width;
	HtmlColor color;
	
	if (val->v.atom == HTML_ATOM_INHERIT && !parent_style)
		return;

	/* FIXME: once (if) we have a static atom table we'll change this to switch instead */

	switch (prop) {
	case HTML_ATOM_DISPLAY:
		/* FIXME: use getter functions for this, so we can check against type */

		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			style->display = parent_style->display;
			break;
		case HTML_ATOM_BLOCK:
			style->display = HTML_DISPLAY_BLOCK;
			break;
		case HTML_ATOM_INLINE:
			style->display = HTML_DISPLAY_INLINE;
			break;
		case HTML_ATOM_NONE:
			style->display = HTML_DISPLAY_NONE;
			break;
		case HTML_ATOM_TABLE:
			style->display = HTML_DISPLAY_TABLE;
			break;
		case HTML_ATOM_INLINE_TABLE:
			style->display = HTML_DISPLAY_INLINE_TABLE;
			break;
		case HTML_ATOM_TABLE_ROW:
			style->display = HTML_DISPLAY_TABLE_ROW;
			break;
		case HTML_ATOM_TABLE_CELL:
			style->display = HTML_DISPLAY_TABLE_CELL;
			break;
		case HTML_ATOM_TABLE_CAPTION:
			style->display = HTML_DISPLAY_TABLE_CAPTION;
			break;
		case HTML_ATOM_TABLE_HEADER_GROUP:
			style->display = HTML_DISPLAY_TABLE_HEADER_GROUP;
			break;
		case HTML_ATOM_TABLE_ROW_GROUP:
			style->display = HTML_DISPLAY_TABLE_ROW_GROUP;
			break;
		case HTML_ATOM_TABLE_FOOTER_GROUP:
			style->display = HTML_DISPLAY_TABLE_FOOTER_GROUP;
			break;
		case HTML_ATOM_LIST_ITEM:
			style->display = HTML_DISPLAY_LIST_ITEM;
			break;
		}
		break;

	case HTML_ATOM_CONTENT:

		if (val->v.atom == HTML_ATOM_INHERIT) {
			if (parent_style->content)
				style->content = g_strdup (parent_style->content);
			break;
		}
		else if (val->value_type == CSS_STRING) {
			style->content = g_strdup (val->v.s);
		}
		break;

	case HTML_ATOM_FLOAT:

		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			style->Float = parent_style->Float;
			break;
		case HTML_ATOM_NONE:
			style->Float = HTML_FLOAT_NONE;
			break;
		case HTML_ATOM_LEFT:
			style->Float = HTML_FLOAT_LEFT;
			break;
		case HTML_ATOM_RIGHT:
			style->Float = HTML_FLOAT_RIGHT;
			break;
		case HTML_ATOM_CENTER:
			style->Float = HTML_FLOAT_CENTER;
			break;
		}
		break;

	case HTML_ATOM_TABLE_LAYOUT:
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			style->table_layout = parent_style->table_layout;
			break;
		case HTML_ATOM_AUTO:
			style->table_layout = HTML_TABLE_LAYOUT_AUTO;
			break;
		case HTML_ATOM_FIXED:
			style->table_layout = HTML_TABLE_LAYOUT_FIXED;
			break;
		}
		break;

	case HTML_ATOM_BACKGROUND_REPEAT:
		handle_background_repeat (document, style, parent_style, val);
		break;

	case HTML_ATOM_LIST_STYLE_TYPE:
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			html_style_set_list_style_type (style, parent_style->inherited->list_style_type);
			break;
		case HTML_ATOM_DISC:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_DISC);
			break;
		case HTML_ATOM_CIRCLE:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_CIRCLE);
			break;
		case HTML_ATOM_SQUARE:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_SQUARE);
			break;
		case HTML_ATOM_DECIMAL:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_DECIMAL);
			break;
		case HTML_ATOM_DECIMAL_LEADING_ZERO:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO);
			break;
		case HTML_ATOM_LOWER_ROMAN:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_LOWER_ROMAN);
			break;
		case HTML_ATOM_UPPER_ROMAN:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_UPPER_ROMAN);
			break;
		case HTML_ATOM_LOWER_GREEK:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_LOWER_GREEK);
			break;
		case HTML_ATOM_LOWER_ALPHA:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_LOWER_ALPHA);
			break;
		case HTML_ATOM_LOWER_LATIN:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_LOWER_LATIN);
			break;
		case HTML_ATOM_UPPER_ALPHA:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_UPPER_ALPHA);
			break;
		case HTML_ATOM_UPPER_LATIN:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_UPPER_LATIN);
			break;
		case HTML_ATOM_HEBREW:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_HEBREW);
			break;
		case HTML_ATOM_ARMENIAN:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_ARMENIAN);
			break;
		case HTML_ATOM_GEORGIAN:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_GEORGIAN);
			break;
		case HTML_ATOM_CJK_IDEOGRAPHIC:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC);
			break;
		case HTML_ATOM_HIRAGANA:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_HIRAGANA);
			break;
		case HTML_ATOM_KATAKANA:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_KATAKANA);
			break;
		case HTML_ATOM_HIRAGANA_IROHA:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_HIRAGANA_IROHA);
			break;
		case HTML_ATOM_KATAKANA_IROHA:
			html_style_set_list_style_type (style, HTML_LIST_STYLE_TYPE_KATAKANA_IROHA);
			break;
		}
		break;

	case HTML_ATOM_CLEAR:
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			style->clear = parent_style->clear;
			break;
		case HTML_ATOM_NONE:
			style->clear = HTML_CLEAR_NONE;
			break;
		case HTML_ATOM_LEFT:
			style->clear = HTML_CLEAR_LEFT;
			break;
		case HTML_ATOM_RIGHT:
			style->clear = HTML_CLEAR_RIGHT;
			break;
		case HTML_ATOM_BOTH:
			style->clear = HTML_CLEAR_BOTH;
			break;
		}
		break;

	case HTML_ATOM_POSITION:
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			style->position = parent_style->position;
			break;
		case HTML_ATOM_FIXED:
			style->position = HTML_POSITION_FIXED;
			break;
		case HTML_ATOM_STATIC:
			style->position = HTML_POSITION_STATIC;
			break;
		case HTML_ATOM_ABSOLUTE:
			style->position = HTML_POSITION_ABSOLUTE;
			break;
		case HTML_ATOM_RELATIVE:
			style->position = HTML_POSITION_RELATIVE;
			break;
		}
		break;

	case HTML_ATOM_TOP: {
		HtmlLength length;
		
		if (html_length_from_css_value (old_font, val, &length))
			html_style_set_position_top (style, &length);
		break;
	}

	case HTML_ATOM_RIGHT: {
		HtmlLength length;
		
		if (html_length_from_css_value (old_font, val, &length))
			html_style_set_position_right (style, &length);
		break;
	}

	case HTML_ATOM_BOTTOM: {
		HtmlLength length;
		
		if (html_length_from_css_value (old_font, val, &length))
			html_style_set_position_bottom (style, &length);
		break;
	}

	case HTML_ATOM_LEFT: {
		HtmlLength length;
		
		if (html_length_from_css_value (old_font, val, &length))
			html_style_set_position_left (style, &length);
		break;
	}

	case HTML_ATOM_CAPTION_SIDE: {
		HtmlCaptionSideType type = HTML_CAPTION_SIDE_TOP;
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			type = parent_style->inherited->caption_side;
			break;
		case HTML_ATOM_TOP:
			type = HTML_CAPTION_SIDE_TOP;
			break;
		case HTML_ATOM_RIGHT:
			type = HTML_CAPTION_SIDE_RIGHT;
			break;
		case HTML_ATOM_BOTTOM:
			type = HTML_CAPTION_SIDE_BOTTOM;
			break;
		case HTML_ATOM_LEFT:
			type = HTML_CAPTION_SIDE_LEFT;
			break;
		}
		html_style_set_caption_side (style, type);
		break;
	}

	case HTML_ATOM_VISIBILITY:

		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			style->visibility = parent_style->visibility;
			break;
		case HTML_ATOM_VISIBLE:
			style->visibility = HTML_VISIBILITY_VISIBLE;
			break;
		case HTML_ATOM_HIDDEN:
			style->visibility = HTML_VISIBILITY_HIDDEN;
			break;
		case HTML_ATOM_COLLAPSE:
			style->visibility = HTML_VISIBILITY_COLLAPSE;
			break;
		}
		break;

	case HTML_ATOM_WHITE_SPACE:

		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			html_style_set_white_space (style, parent_style->inherited->white_space);
			break;
		case HTML_ATOM_NORMAL:
			html_style_set_white_space (style, HTML_WHITE_SPACE_NORMAL);
			break;
		case HTML_ATOM_PRE:
			html_style_set_white_space (style, HTML_WHITE_SPACE_PRE);
			break;
		case HTML_ATOM_NOWRAP:
			html_style_set_white_space (style, HTML_WHITE_SPACE_NOWRAP);
			break;
		}
		break;

	case HTML_ATOM_BORDER_SPACING: { /* FIXME: implement inherit */
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			gint horiz, vert;
			
			/* See how many items we have */
			switch (css_value_list_get_length (val)) {
			case 2:
				horiz = length_to_pixels (entry->value);
				entry = entry->next;
				vert = length_to_pixels (entry->value);
				html_style_set_border_spacing (style, horiz, vert);
				break;
			default:
				/* Do nothing */
				break;
			}
		}
		else {
			gint spacing = length_to_pixels (val);
			html_style_set_border_spacing (style, spacing, spacing);
		}
		break;
	}
	
	case HTML_ATOM_WIDTH: {
		HtmlLength width;
		
		if (val->v.atom == HTML_ATOM_INHERIT) {
			html_length_set_value (&width, 100, HTML_LENGTH_PERCENT);
			html_style_set_width (style, &width);
		}
		else if (html_length_from_css_value (old_font, val, &width))
			html_style_set_width (style, &width);

		break;
	}

	case HTML_ATOM_HEIGHT: {
		HtmlLength height;

		if (val->v.atom == HTML_ATOM_INHERIT) {
			html_length_set_value (&height, 100, HTML_LENGTH_PERCENT);
			html_style_set_height (style, &height);
		}
		else if (html_length_from_css_value (old_font, val, &height))
			html_style_set_height (style, &height);

		break;
	}

	case HTML_ATOM_MIN_WIDTH: {
		HtmlLength min_width;

		if (val->v.atom == HTML_ATOM_INHERIT)
			html_style_set_min_width (style, &parent_style->box->min_width);
		else if (html_length_from_css_value (old_font, val, &min_width))
			html_style_set_min_width (style, &min_width);
		break;
	}

	case HTML_ATOM_MAX_WIDTH: {
		HtmlLength max_width;

		if (val->v.atom == HTML_ATOM_INHERIT)
			html_style_set_max_width (style, &parent_style->box->max_width);
		else if (html_length_from_css_value (old_font, val, &max_width))
			html_style_set_max_width (style, &max_width);
		break;
	}

	case HTML_ATOM_MAX_HEIGHT: {
		HtmlLength max_height;

		if (val->v.atom == HTML_ATOM_INHERIT)
			html_style_set_max_height (style, &parent_style->box->max_height);
		if (html_length_from_css_value (old_font, val, &max_height))
			html_style_set_max_height (style, &max_height);
		break;
	}

	case HTML_ATOM_MIN_HEIGHT: {
		HtmlLength min_height;

		if (val->v.atom == HTML_ATOM_INHERIT)
			html_style_set_min_height (style, &parent_style->box->min_height);
		if (html_length_from_css_value (old_font, val, &min_height))
			html_style_set_min_height (style, &min_height);
		break;
	}

	case HTML_ATOM_COLOR:
		if (val->v.atom == HTML_ATOM_INHERIT)
			html_style_set_color (style, parent_style->inherited->color);
		else if (css_parse_color (val, &color)) {
			html_style_set_color (style, &color);
		}
		break;
	
	case HTML_ATOM_MARGIN: {
		if (val->value_type == CSS_VALUE_LIST) {
			HtmlLength margin;
			CssValueEntry *entry = val->v.entry;

			/* First, check that all specified values are correct */
			while (entry) {
				if (!html_length_from_css_value (old_font, entry->value, &margin))
					return;

				entry = entry->next;
			}

			entry = val->v.entry;
			
			/* See how many items we have */
			switch (css_value_list_get_length (val)) {
			case 2:
				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_top (style, &margin);
				html_style_set_margin_bottom (style, &margin);
				entry = entry->next;
				
				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_left (style, &margin);
				html_style_set_margin_right (style, &margin);
				break;
			case 3:
				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_top (style, &margin);
				entry = entry->next;
				
				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_right (style, &margin);
				html_style_set_margin_left (style, &margin);
				entry = entry->next;
				
				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_bottom (style, &margin);
				break;
			case 4:
				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_top (style, &margin);
				entry = entry->next;

				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_right (style, &margin);
				entry = entry->next;

				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_bottom (style, &margin);
				entry = entry->next;

				html_length_from_css_value (old_font, entry->value, &margin);
				html_style_set_margin_left (style, &margin);
				break;
			default:
				/* Do nothing */
				break;
			}
		}
		else {
			HtmlLength margin;

			/* Only one item, so we'll set margins entirely */
			if (html_length_from_css_value (old_font, val, &margin)) {
				
				html_style_set_margin_left (style, &margin);
				html_style_set_margin_right (style, &margin);
				html_style_set_margin_top (style, &margin);
				html_style_set_margin_bottom (style, &margin);
			}
		}
		
		break;
	}
	
	case HTML_ATOM_MARGIN_LEFT: {
		HtmlLength margin_left;

		if (html_length_from_css_value (old_font, val, &margin_left))
			html_style_set_margin_left (style, &margin_left);

		break;
	}
	case HTML_ATOM_MARGIN_RIGHT: {
		HtmlLength margin_right;

		if (html_length_from_css_value (old_font, val, &margin_right))
			html_style_set_margin_right (style, &margin_right);

		break;
	}
	
	case HTML_ATOM_MARGIN_TOP: {
		HtmlLength margin_top;

		if (html_length_from_css_value (old_font, val, &margin_top))
			html_style_set_margin_top (style, &margin_top);

		break;
	}
	
	case HTML_ATOM_MARGIN_BOTTOM: {
		HtmlLength margin_bottom;

		if (html_length_from_css_value (old_font, val, &margin_bottom))
			html_style_set_margin_bottom (style, &margin_bottom);

		break;
	}

	case HTML_ATOM_PADDING: {
		if (val->value_type == CSS_VALUE_LIST) {
			HtmlLength padding;
			CssValueEntry *entry = val->v.entry;

			/* First, check that all specified values are correct */
			while (entry) {
				if (!html_length_from_css_value (old_font, entry->value, &padding))
					return;

				entry = entry->next;
			}

			entry = val->v.entry;
			
			/* See how many items we have */
			switch (css_value_list_get_length (val)) {
			case 2:
				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_top (style, &padding);
				html_style_set_padding_bottom (style, &padding);
				entry = entry->next;
				
				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_left (style, &padding);
				html_style_set_padding_right (style, &padding);
				break;
			case 3:
				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_top (style, &padding);
				entry = entry->next;
				
				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_right (style, &padding);
				html_style_set_padding_left (style, &padding);
				entry = entry->next;
				
				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_bottom (style, &padding);
				
				break;
			case 4:
				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_top (style, &padding);
				entry = entry->next;

				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_right (style, &padding);
				entry = entry->next;

				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_bottom (style, &padding);
				entry = entry->next;

				html_length_from_css_value (old_font, entry->value, &padding);
				html_style_set_padding_left (style, &padding);
				break;
			default:
				/* Do nothing */
				break;
			}
		}
		else {
			HtmlLength padding;

			/* Only one item, so we'll set paddings entirely */
			if (html_length_from_css_value (old_font, val, &padding)) {
				
				html_style_set_padding_left (style, &padding);
				html_style_set_padding_right (style, &padding);
				html_style_set_padding_top (style, &padding);
				html_style_set_padding_bottom (style, &padding);
			}
		}

		break;
	}

	case HTML_ATOM_PADDING_LEFT: {
		HtmlLength padding_left;

		if (html_length_from_css_value (old_font, val, &padding_left))
			html_style_set_padding_left (style, &padding_left);

		break;
	}

	case HTML_ATOM_PADDING_RIGHT: {
		HtmlLength padding_right;

		if (html_length_from_css_value (old_font, val, &padding_right))
			html_style_set_padding_right (style, &padding_right);

		break;
	}

	case HTML_ATOM_PADDING_TOP: {
		HtmlLength padding_top;

		if (html_length_from_css_value (old_font, val, &padding_top))
			html_style_set_padding_top (style, &padding_top);
		
		break;
	}
	
	case HTML_ATOM_PADDING_BOTTOM: {
		HtmlLength padding_bottom;

		if (html_length_from_css_value (old_font, val, &padding_bottom))
			html_style_set_padding_bottom (style, &padding_bottom);
	}
	break;
	/* FIXME: this should support more than just colors, but it is better that nothing */
	case HTML_ATOM_BACKGROUND:

		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;

			/* First, check that all specified values are correct */
			while (entry) {
				if (handle_background_repeat (document, style, parent_style, entry->value)) {
				}
				else if (css_parse_color (entry->value, &color))
					html_style_set_background_color (style, &color);
				else if (entry->value->value_type == CSS_FUNCTION)
					handle_background_image (document, style, entry->value);

				entry = entry->next;
			}
		}
		if (handle_background_repeat (document, style, parent_style, val)) {
		}
		else if (val->value_type == CSS_FUNCTION)
			handle_background_image (document, style, val);
		else if (val->value_type == CSS_IDENT && val->v.atom == HTML_ATOM_NONE) {
			handle_background_image (document, style, val);
			style->background->color.transparent = TRUE;
		} else if (css_parse_color (val, &color))
			html_style_set_background_color (style, &color);

		break;

	case HTML_ATOM_BACKGROUND_IMAGE:
		
		if (val->v.atom == HTML_ATOM_INHERIT)
			html_style_set_background_image (style, parent_style->background->image);
		else
			handle_background_image (document, style, val);
		break;

	case HTML_ATOM_BACKGROUND_COLOR:
		
		if (val->v.atom == HTML_ATOM_INHERIT)
			html_style_set_background_color (style, &parent_style->background->color);
		else if (val->v.atom == HTML_ATOM_NONE)
			style->background->color.transparent = TRUE;
		else if (css_parse_color (val, &color))
			html_style_set_background_color (style, &color);
		break;

	case HTML_ATOM_DIRECTION:
		
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			html_style_set_direction (style, parent_style->inherited->direction);
			break;
		case HTML_ATOM_LTR:
			html_style_set_direction (style, HTML_DIRECTION_LTR);
			break;
		case HTML_ATOM_RTL:
			html_style_set_direction (style, HTML_DIRECTION_RTL);
			break;
		}
		break;

	case HTML_ATOM_TEXT_ALIGN: {
		HtmlTextAlignType type = HTML_TEXT_ALIGN_DEFAULT;
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			type = parent_style->inherited->text_align;
			break;
		case HTML_ATOM_LEFT:
			type = HTML_TEXT_ALIGN_LEFT;
			break;
		case HTML_ATOM_RIGHT:
			type = HTML_TEXT_ALIGN_RIGHT;
			break;
		case HTML_ATOM_CENTER:
			type = HTML_TEXT_ALIGN_CENTER;
			break;
		case HTML_ATOM_JUSTIFY:
			type = HTML_TEXT_ALIGN_JUSTIFY;
			break;
		default:
			g_message ("Unrecognized text-align attribute value");
			break;
		};
		html_style_set_text_align (style, type);
		break;
	}

	case HTML_ATOM_VERTICAL_ALIGN:

		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			style->vertical_align = parent_style->vertical_align;
			break;
		case HTML_ATOM_TOP:
			style->vertical_align = HTML_VERTICAL_ALIGN_TOP;
			break;
		case HTML_ATOM_BOTTOM:
			style->vertical_align = HTML_VERTICAL_ALIGN_BOTTOM;
			break;
		case HTML_ATOM_BASELINE:
			style->vertical_align = HTML_VERTICAL_ALIGN_BASELINE;
			break;
		case HTML_ATOM_MIDDLE:
			style->vertical_align = HTML_VERTICAL_ALIGN_MIDDLE;
			break;
		case HTML_ATOM_SUB:
			style->vertical_align = HTML_VERTICAL_ALIGN_SUB;
			break;
		case HTML_ATOM_SUPER:
			style->vertical_align = HTML_VERTICAL_ALIGN_SUPER;
			break;
		case HTML_ATOM_TEXT_TOP:
			style->vertical_align = HTML_VERTICAL_ALIGN_TEXT_TOP;
			break;
		case HTML_ATOM_TEXT_BOTTOM:
			style->vertical_align = HTML_VERTICAL_ALIGN_TEXT_BOTTOM;
			break;
		}
		/* FIXME: support Length and percentage / jb */
		break;

	
	case HTML_ATOM_UNICODE_BIDI: 
		
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			style->unicode_bidi = parent_style->unicode_bidi;
			break;
		case HTML_ATOM_NORMAL:
			style->unicode_bidi = HTML_UNICODE_BIDI_NORMAL;
			break;
		case HTML_ATOM_EMBED:
			style->unicode_bidi = HTML_UNICODE_BIDI_EMBED;
			break;
		case HTML_ATOM_BIDI_OVERRIDE:
			style->unicode_bidi = HTML_UNICODE_BIDI_OVERRIDE;
			break;
		}
		
		break;
		
	case HTML_ATOM_FONT_FAMILY:
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry;
			gint len = 0;
			gchar *str;
			/* First figure out the length of all the entries */
			entry = val->v.entry;
			while (entry) {
				CssValue *value = entry->value;
				gchar *value_str = css_value_to_string (value);
				if (value_str) {
					len += strlen (value_str);
					g_free (value_str);
				}
				/* Add space for the ',' character */
				if (entry->next)
					len++;
				entry = entry->next;
			}
			/* Allocate a large enough buffer */
			str = g_new (gchar, len + 1);
			str[0] = 0;
			/* Build the family list string. Format: "x,y,z" */
			entry = val->v.entry;
			while (entry) {
				CssValue *value = entry->value;
				gchar *value_str = css_value_to_string (value);
				if (value_str) {
					strcat (str, value_str);
					g_free (value_str);
					if (entry->next)
						strcat (str, ",");
				}
				entry = entry->next;
			}
			html_style_set_font_family (style, str);
			g_free (str);
		}
		else {
			gchar *value_str = css_value_to_string (val);
			if (value_str) {
				html_style_set_font_family (style, value_str);
				g_free (value_str);
			}
		}
		break;

	case HTML_ATOM_TEXT_DECORATION:
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry;
			
			entry = val->v.entry;

			while (entry) {
				CssValue *value = entry->value;

				switch (value->v.atom) {
				case HTML_ATOM_NONE:
					html_style_set_text_decoration (style, HTML_FONT_DECORATION_NONE);
					break;
				case HTML_ATOM_UNDERLINE:
					html_style_set_text_decoration (style, HTML_FONT_DECORATION_UNDERLINE);
					break;
				case HTML_ATOM_OVERLINE:
					html_style_set_text_decoration (style, HTML_FONT_DECORATION_OVERLINE);
					break;
				case HTML_ATOM_LINE_THROUGH:
					html_style_set_text_decoration (style, HTML_FONT_DECORATION_LINETHROUGH);
					break;
				case HTML_ATOM_BLINK:
					style->blink = TRUE;
				}
				entry = entry->next;
			}
		}
		else if (val->value_type == CSS_IDENT) {

			switch (val->v.atom) {
			case HTML_ATOM_INHERIT:
				html_style_set_text_decoration (style, parent_style->inherited->font_spec->decoration);
				break;
			case HTML_ATOM_NONE:
				html_style_set_text_decoration (style, HTML_FONT_DECORATION_NONE);
				break;
			case HTML_ATOM_UNDERLINE:
				html_style_set_text_decoration (style, HTML_FONT_DECORATION_UNDERLINE);
				break;
			case HTML_ATOM_OVERLINE:
				html_style_set_text_decoration (style, HTML_FONT_DECORATION_OVERLINE);
				break;
			case HTML_ATOM_LINE_THROUGH:
				html_style_set_text_decoration (style, HTML_FONT_DECORATION_LINETHROUGH);
				break;
			case HTML_ATOM_BLINK:
				style->blink = TRUE;
			}
		}
		break;

	case HTML_ATOM_FONT_VARIANT:
		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			html_style_set_font_variant (style, parent_style->inherited->font_spec->variant);
			break;
		case HTML_ATOM_SMALL_CAPS:
			html_style_set_font_variant (style, HTML_FONT_VARIANT_SMALL_CAPS);
			break;
		}
		break;

	case HTML_ATOM_FONT_STYLE:

		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			html_style_set_font_style (style, parent_style->inherited->font_spec->style);
			break;
		case HTML_ATOM_NORMAL:
			html_style_set_font_style (style, HTML_FONT_STYLE_NORMAL);
			break;
		case HTML_ATOM_ITALIC:
			html_style_set_font_style (style, HTML_FONT_STYLE_ITALIC);
			break;
		case HTML_ATOM_OBLIQUE:
			break;
		}
		break;
		
	case HTML_ATOM_FONT_WEIGHT:

		if (val->value_type == CSS_NUMBER) {
			if (val->v.d == 100)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_100);
			else if (val->v.d == 200)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_200);
			else if (val->v.d == 300)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_300);
			else if (val->v.d == 400)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_400);
			else if (val->v.d == 500)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_500);
			else if (val->v.d == 600)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_600);
			else if (val->v.d == 700)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_700);
			else if (val->v.d == 800)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_800);
			else if (val->v.d == 900)
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_900);
		}
		else {
			switch (val->v.atom) {
			case HTML_ATOM_INHERIT:
				html_style_set_font_weight (style, parent_style->inherited->font_spec->weight);
				break;
			case HTML_ATOM_NORMAL:
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_NORMAL);
				break;
			case HTML_ATOM_BOLD:
				html_style_set_font_weight (style, HTML_FONT_WEIGHT_BOLD);
				break;
			case HTML_ATOM_BOLDER:
				html_style_set_font_weight_bolder (style);
				break;
			case HTML_ATOM_LIGHTER:
				html_style_set_font_weight_lighter (style);
				break;
			};
		}
		break;

	case HTML_ATOM_FONT_STRETCH:

		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			html_style_set_font_stretch (style, parent_style->inherited->font_spec->stretch);
			break;
		case HTML_ATOM_ULTRA_CONDENSED:
			html_style_set_font_stretch (style, HTML_FONT_STRETCH_ULTRA_CONDENSED);
			break;
		case HTML_ATOM_EXTRA_CONDENSED:
			html_style_set_font_stretch (style, HTML_FONT_STRETCH_EXTRA_CONDENSED);
			break;
		case HTML_ATOM_CONDENSED:
			html_style_set_font_stretch (style, HTML_FONT_STRETCH_CONDENSED);
			break;
		case HTML_ATOM_SEMI_CONDENSED:
			html_style_set_font_stretch (style, HTML_FONT_STRETCH_SEMI_CONDENSED);
			break;
		case HTML_ATOM_SEMI_EXPANDED:
			html_style_set_font_stretch (style, HTML_FONT_STRETCH_SEMI_EXPANDED);
			break;
		case HTML_ATOM_EXPANDED:
			html_style_set_font_stretch (style, HTML_FONT_STRETCH_EXPANDED);
			break;
		case HTML_ATOM_EXTRA_EXPANDED:
			html_style_set_font_stretch (style, HTML_FONT_STRETCH_EXTRA_EXPANDED);
			break;
		case HTML_ATOM_ULTRA_EXPANDED:
			html_style_set_font_stretch (style, HTML_FONT_STRETCH_ULTRA_EXPANDED);
			break;
		}
		break;

	case HTML_ATOM_FONT_SIZE:
		html_style_set_font_size (style, old_font, val);
		break;

	case HTML_ATOM_LINE_HEIGHT:
		html_style_set_line_height (style, old_font, val);
		break;

	case HTML_ATOM_OUTLINE_COLOR:
		if (val->v.atom == HTML_ATOM_INVERT)
			html_style_set_outline_color (style, NULL);
		else if (css_parse_color (val, &color))
			html_style_set_outline_color (style, &color);
		break;
		
	case HTML_ATOM_BORDER_TOP_COLOR:
		if (css_parse_color (val, &color))
			html_style_set_border_top_color (style, &color);
		break;
		
	case HTML_ATOM_BORDER_BOTTOM_COLOR:
		if (css_parse_color (val, &color))
			html_style_set_border_bottom_color (style, &color);
		break;
		
	case HTML_ATOM_BORDER_LEFT_COLOR:
		if (css_parse_color (val, &color))
			html_style_set_border_left_color (style, &color);
		break;
		
	case HTML_ATOM_BORDER_RIGHT_COLOR:
		if (css_parse_color (val, &color))
			html_style_set_border_right_color (style, &color);
		break;
		
	case HTML_ATOM_BORDER_COLOR: 
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			
			/* First, check that all specified values are correct */
			while (entry) {
				if (!css_parse_color (entry->value, NULL))
				    return;

				entry = entry->next;
			}

			entry = val->v.entry;
			
			/* Apply values */
			switch (css_value_list_get_length (val)) {
			case 2:
				css_parse_color (entry->value, &color);
				html_style_set_border_top_color (style, &color);
				html_style_set_border_bottom_color (style, &color);
				
				entry = entry->next;
				css_parse_color (entry->value, &color);
				html_style_set_border_left_color (style, &color);
				html_style_set_border_right_color (style, &color);

				break;
			case 3:
				css_parse_color (entry->value, &color);
				html_style_set_border_top_color (style, &color);
				entry = entry->next;
				
				css_parse_color (entry->value, &color);
				html_style_set_border_right_color (style, &color);
				html_style_set_border_left_color (style, &color);
				entry = entry->next;

				css_parse_color (entry->value, &color);
				html_style_set_border_bottom_color (style, &color);
				break;

			case 4:
				css_parse_color (entry->value, &color);
				html_style_set_border_top_color (style, &color);
				entry = entry->next;

				css_parse_color (entry->value, &color);
				html_style_set_border_right_color (style, &color);
				entry = entry->next;

				css_parse_color (entry->value, &color);
				html_style_set_border_bottom_color (style, &color);
				entry = entry->next;
				
				css_parse_color (entry->value, &color);
				html_style_set_border_left_color (style, &color);
				break;
			default:
				/* Do nothing */
				break;
			}
		}
		else {
			/* Only one item, so we'll set all the border styles */
			if (css_parse_color (val, &color)) {
				html_style_set_border_top_color (style, &color);
				html_style_set_border_right_color (style, &color);
				html_style_set_border_bottom_color (style, &color);
				html_style_set_border_left_color (style, &color);
			}
		}   
		break;
		
	case HTML_ATOM_BORDER_STYLE: 
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			
			/* First, check that all specified values are correct */
			while (entry) {
				if (!css_parse_border_style (entry->value, &border_style))
				    return;

				entry = entry->next;
			}

			entry = val->v.entry;
			
			/* Apply values */
			switch (css_value_list_get_length (val)) {
			case 2:
				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_top_style (style, border_style);
				html_style_set_border_bottom_style (style, border_style);
				
				entry = entry->next;
				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_left_style (style, border_style);
				html_style_set_border_right_style (style, border_style);

				break;
			case 3:
				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_top_style (style, border_style);
				entry = entry->next;

				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_right_style (style, border_style);
				html_style_set_border_left_style (style, border_style);
				entry = entry->next;
				
				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_bottom_style (style, border_style);
				break;

			case 4:
				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_top_style (style, border_style);
				entry = entry->next;

				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_right_style (style, border_style);
				entry = entry->next;

				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_bottom_style (style, border_style);
				entry = entry->next;
				
				css_parse_border_style (entry->value, &border_style);
				html_style_set_border_left_style (style, border_style);
				break;
			default:
				/* Do nothing */
				break;
			}
		}
		else {
			/* Only one item, so we'll set all the border styles */
			if (css_parse_border_style (val, &border_style)) {
				html_style_set_border_top_style (style, border_style);
				html_style_set_border_right_style (style, border_style);
				html_style_set_border_bottom_style (style, border_style);
				html_style_set_border_left_style (style, border_style);
			}
		}   
		break;

	case HTML_ATOM_BORDER_WIDTH:
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			
			/* First, check that all specified values are correct */
			while (entry) {
				if (!css_parse_border_width (old_font, entry->value, &border_width))
				    return;

				entry = entry->next;
			}

			entry = val->v.entry;
			
			/* Apply values */
			switch (css_value_list_get_length (val)) {
			case 2:
				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_top_width (style, border_width);
				html_style_set_border_bottom_width (style, border_width);
				
				entry = entry->next;
				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_left_width (style, border_width);
				html_style_set_border_right_width (style, border_width);

				break;
			case 3:
				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_top_width (style, border_width);
				entry = entry->next;

				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_right_width (style, border_width);
				html_style_set_border_left_width (style, border_width);
				entry = entry->next;
				
				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_bottom_width (style, border_width);
				break;

			case 4:
				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_top_width (style, border_width);
				entry = entry->next;

				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_right_width (style, border_width);
				entry = entry->next;

				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_bottom_width (style, border_width);
				entry = entry->next;
				
				css_parse_border_width (old_font, entry->value, &border_width);
				html_style_set_border_left_width (style, border_width);
				break;
			default:
				/* Do nothing */
				break;
			}
		}
		else {

			if (val->v.atom == HTML_ATOM_INHERIT) {
				html_style_set_border_top_width (style, parent_style->border->top.width);
				html_style_set_border_left_width (style, parent_style->border->left.width);
				html_style_set_border_right_width (style, parent_style->border->right.width);
				html_style_set_border_bottom_width (style, parent_style->border->bottom.width);
			}
			/* Only one item, so we'll set all the border styles */
			else if (css_parse_border_width (old_font, val, &border_width)) {
				html_style_set_border_top_width (style, border_width);
				html_style_set_border_right_width (style, border_width);
				html_style_set_border_bottom_width (style, border_width);
				html_style_set_border_left_width (style, border_width);
			}
		}   

		break;
		
	case HTML_ATOM_OUTLINE_WIDTH:
		if (css_parse_border_width (old_font, val, &border_width))
			html_style_set_outline_width (style, border_width);
		break;

	case HTML_ATOM_BORDER_TOP_WIDTH:
		if (css_parse_border_width (old_font, val, &border_width))
			html_style_set_border_top_width (style, border_width);
		break;

	case HTML_ATOM_BORDER_BOTTOM_WIDTH:
		if (css_parse_border_width (old_font, val, &border_width))
			html_style_set_border_bottom_width (style, border_width);
		break;
		
	case HTML_ATOM_BORDER_LEFT_WIDTH:
		if (css_parse_border_width (old_font, val, &border_width))
			html_style_set_border_left_width (style, border_width);
		break;

	case HTML_ATOM_BORDER_RIGHT_WIDTH:
		if (css_parse_border_width (old_font, val, &border_width))
			html_style_set_border_right_width (style, border_width);
		break;
		
	case HTML_ATOM_OUTLINE_STYLE:

		if (css_parse_border_style (val, &border_style))
			html_style_set_outline_style (style, border_style);
		break;
		
	case HTML_ATOM_BORDER_TOP_STYLE:

		if (css_parse_border_style (val, &border_style))
			html_style_set_border_top_style (style, border_style);
		break;
		
	case HTML_ATOM_BORDER_BOTTOM_STYLE:

		if (css_parse_border_style (val, &border_style))
			html_style_set_border_bottom_style (style, border_style);
		break;
		
	case HTML_ATOM_BORDER_LEFT_STYLE:

		if (css_parse_border_style (val, &border_style))
			html_style_set_border_left_style (style, border_style);
		break;
		
	case HTML_ATOM_BORDER_RIGHT_STYLE:

		if (css_parse_border_style (val, &border_style))
			html_style_set_border_right_style (style, border_style);
		break;

	case HTML_ATOM_OUTLINE:
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			gboolean width_spec = FALSE, style_spec = FALSE, color_spec = FALSE, invert = FALSE;
			
			if (css_value_list_get_length (val) > 3)
				return;

			/* Go through the list and check that all values are valid */
			while (entry) {
				if (!css_parse_border_width (old_font, entry->value, &border_width) &&
				    !css_parse_border_style (entry->value, &border_style) &&
				    !css_parse_color (entry->value, NULL) &&
				    entry->value->v.atom != HTML_ATOM_INVERT)
					return;
				
				entry = entry->next;
			}
			border_width = HTML_BORDER_WIDTH_MEDIUM;
			border_style = HTML_BORDER_STYLE_NONE;
			color.red = 0;
			color.green = 0;
			color.blue = 0;
			entry = val->v.entry;
			
			while (entry) {
				if (entry->value->v.atom == HTML_ATOM_INVERT)
					invert = TRUE;
				else if (css_parse_border_width (old_font, entry->value, &border_width)) {
					if (width_spec)
						return;
					else
						width_spec = TRUE;
				}
				else if (css_parse_border_style (entry->value, &border_style)) {
					if (style_spec)
						return;
					else
						style_spec = TRUE;
				}
				else if (css_parse_color (entry->value, &color)) {
					if (color_spec)
						return;
					else
						color_spec = TRUE;
				}
				entry = entry->next;
			}

			html_style_set_outline_width (style, border_width);
			html_style_set_outline_style (style, border_style);
			if (invert)
				html_style_set_outline_color (style, NULL);
			else
				html_style_set_outline_color (style, &color);
					 
		}
		else {
			if (css_parse_border_width (old_font, val, &border_width))
				html_style_set_outline_width (style, border_width);
			else if (css_parse_border_style (val, &border_style))
				html_style_set_outline_style (style, border_style);
			else if (val->v.atom == HTML_ATOM_INVERT)
				 html_style_set_outline_color (style, NULL);
			else if (css_parse_color (val, &color))
				 html_style_set_outline_color (style, &color);
		}
		break;

	case HTML_ATOM_BORDER_TOP:
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			gboolean width_spec = FALSE, style_spec = FALSE, color_spec = FALSE;
			
			if (css_value_list_get_length (val) > 3)
				return;

			/* Go through the list and check that all values are valid */
			while (entry) {
				if (!css_parse_border_width (old_font, entry->value, &border_width) &&
				    !css_parse_border_style (entry->value, &border_style) &&
				    !css_parse_color (entry->value, NULL))
					return;
				
				entry = entry->next;
			}
			border_width = HTML_BORDER_WIDTH_MEDIUM;
			border_style = HTML_BORDER_STYLE_NONE;
			color.red = 0;
			color.green = 0;
			color.blue = 0;
			entry = val->v.entry;
			
			while (entry) {
				if (css_parse_border_width (old_font, entry->value, &border_width)) {
					if (width_spec)
						return;
					else
						width_spec = TRUE;
				}
				else if (css_parse_border_style (entry->value, &border_style)) {
					if (style_spec)
						return;
					else
						style_spec = TRUE;
				}
				else if (css_parse_color (entry->value, &color)) {
					if (color_spec)
						return;
					else
						color_spec = TRUE;
				}
				
				entry = entry->next;
			}

			html_style_set_border_top_width (style, border_width);
			html_style_set_border_top_style (style, border_style);
			html_style_set_border_top_color (style, &color);
					 
		}
		else {
			if (css_parse_border_width (old_font, val, &border_width))
				html_style_set_border_top_width (style, border_width);
			else if (css_parse_border_style (val, &border_style))
				html_style_set_border_top_style (style, border_style);
			else if (css_parse_color (val, &color))
				 html_style_set_border_top_color (style, &color);
		}
		break;

	case HTML_ATOM_BORDER_RIGHT:
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			gboolean width_spec = FALSE, style_spec = FALSE, color_spec = FALSE;
			
			if (css_value_list_get_length (val) > 3)
				return;

			/* Go through the list and check that all values are valid */
			while (entry) {
				if (!css_parse_border_width (old_font, entry->value, &border_width) &&
				    !css_parse_border_style (entry->value, &border_style) &&
				    !css_parse_color (entry->value, NULL))
					return;
				
				entry = entry->next;
			}
			border_width = HTML_BORDER_WIDTH_MEDIUM;
			border_style = HTML_BORDER_STYLE_NONE;
			color.red = 0;
			color.green = 0;
			color.blue = 0;
			entry = val->v.entry;
			
			while (entry) {
				if (css_parse_border_width (old_font, entry->value, &border_width)) {
					if (width_spec)
						return;
					else
						width_spec = TRUE;
				}
				else if (css_parse_border_style (entry->value, &border_style)) {
					if (style_spec)
						return;
					else
						style_spec = TRUE;
				}
				else if (css_parse_color (entry->value, &color)) {
					if (color_spec)
						return;
					else
						color_spec = TRUE;
				}
				
				entry = entry->next;
			}

			html_style_set_border_right_width (style, border_width);
			html_style_set_border_right_style (style, border_style);
			html_style_set_border_right_color (style, &color);
					 
		}
		else {
			if (css_parse_border_width (old_font, val, &border_width))
				html_style_set_border_right_width (style, border_width);
			else if (css_parse_border_style (val, &border_style))
				html_style_set_border_right_style (style, border_style);
			else if (css_parse_color (val, &color))
				 html_style_set_border_right_color (style, &color);
		}
		break;

	case HTML_ATOM_BORDER_BOTTOM:

		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			gboolean width_spec = FALSE, style_spec = FALSE, color_spec = FALSE;
			
			if (css_value_list_get_length (val) > 3)
				return;

			/* Go through the list and check that all values are valid */
			while (entry) {
				if (!css_parse_border_width (old_font, entry->value, &border_width) &&
				    !css_parse_border_style (entry->value, &border_style) &&
				    !css_parse_color (entry->value, NULL))
					return;
				
				entry = entry->next;
			}
			border_width = HTML_BORDER_WIDTH_MEDIUM;
			border_style = HTML_BORDER_STYLE_NONE;
			color.red = 0;
			color.green = 0;
			color.blue = 0;
			entry = val->v.entry;
			
			while (entry) {
				if (css_parse_border_width (old_font, entry->value, &border_width)) {
					if (width_spec)
						return;
					else
						width_spec = TRUE;
				}
				else if (css_parse_border_style (entry->value, &border_style)) {
					if (style_spec)
						return;
					else
						style_spec = TRUE;
				}
				else if (css_parse_color (entry->value, &color)) {
					if (color_spec)
						return;
					else
						color_spec = TRUE;
				}
				
				entry = entry->next;
			}

			html_style_set_border_bottom_width (style, border_width);
			html_style_set_border_bottom_style (style, border_style);
			html_style_set_border_bottom_color (style, &color);
					 
		}
		else {
			if (css_parse_border_width (old_font, val, &border_width))
				html_style_set_border_bottom_width (style, border_width);
			else if (css_parse_border_style (val, &border_style))
				html_style_set_border_bottom_style (style, border_style);
			else if (css_parse_color (val, &color))
				 html_style_set_border_bottom_color (style, &color);
		}
		break;

	case HTML_ATOM_BORDER_LEFT:
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			gboolean width_spec = FALSE, style_spec = FALSE, color_spec = FALSE;
			
			if (css_value_list_get_length (val) > 3)
				return;

			/* Go through the list and check that all values are valid */
			while (entry) {
				if (!css_parse_border_width (old_font, entry->value, &border_width) &&
				    !css_parse_border_style (entry->value, &border_style) &&
				    !css_parse_color (entry->value, NULL))
					return;
				
				entry = entry->next;
			}
			border_width = HTML_BORDER_WIDTH_MEDIUM;
			border_style = HTML_BORDER_STYLE_NONE;
			color.red = 0;
			color.green = 0;
			color.blue = 0;
			entry = val->v.entry;
			
			while (entry) {
				if (css_parse_border_width (old_font, entry->value, &border_width)) {
					if (width_spec)
						return;
					else
						width_spec = TRUE;
				}
				else if (css_parse_border_style (entry->value, &border_style)) {
					if (style_spec)
						return;
					else
						style_spec = TRUE;
				}
				else if (css_parse_color (entry->value, &color)) {
					if (color_spec)
						return;
					else
						color_spec = TRUE;
				}
				
				entry = entry->next;
			}

			html_style_set_border_left_width (style, border_width);
			html_style_set_border_left_style (style, border_style);
			html_style_set_border_left_color (style, &color);
					 
		}
		else {
			if (css_parse_border_width (old_font, val, &border_width))
				html_style_set_border_left_width (style, border_width);
			else if (css_parse_border_style (val, &border_style))
				html_style_set_border_left_style (style, border_style);
			else if (css_parse_color (val, &color))
				 html_style_set_border_left_color (style, &color);
		}
		break;
		

	case HTML_ATOM_BORDER:
		if (val->value_type == CSS_VALUE_LIST) {
			CssValueEntry *entry = val->v.entry;
			gboolean width_spec = FALSE, style_spec = FALSE, color_spec = FALSE;
			
			if (css_value_list_get_length (val) > 3)
				return;
 
			/* Go through the list and check that all values are valid */
			while (entry) {
				if (!css_parse_border_width (old_font, entry->value, &border_width) &&
				    !css_parse_border_style (entry->value, &border_style) &&
				    !css_parse_color (entry->value, NULL))
					return;
				
				entry = entry->next;
			}
			border_width = HTML_BORDER_WIDTH_MEDIUM;
			border_style = HTML_BORDER_STYLE_NONE;
			color.red = 0;
			color.green = 0;
			color.blue = 0;
			color.transparent = FALSE;
			entry = val->v.entry;
			
			while (entry) {
				if (css_parse_border_width (old_font, entry->value, &border_width)) {
					if (width_spec)
						return;
					else
						width_spec = TRUE;
				}
				else if (css_parse_border_style (entry->value, &border_style)) {
					if (style_spec)
						return;
					else
						style_spec = TRUE;
				}
				else if (css_parse_color (entry->value, &color)) {
					if (color_spec)
						return;
					else
						color_spec = TRUE;
				}
				
				entry = entry->next;
			}

			html_style_set_border_top_width (style, border_width);
			html_style_set_border_right_width (style, border_width);
			html_style_set_border_bottom_width (style, border_width);
			html_style_set_border_left_width (style, border_width);

			html_style_set_border_top_style (style, border_style);
			html_style_set_border_right_style (style, border_style);
			html_style_set_border_bottom_style (style, border_style);
			html_style_set_border_left_style (style, border_style);
			
			html_style_set_border_top_color (style, &color);
			html_style_set_border_right_color (style, &color);
			html_style_set_border_bottom_color (style, &color);
			html_style_set_border_left_color (style, &color);
					 
		}
		else {
			if (css_parse_border_width (old_font, val, &border_width)) {
				html_style_set_border_top_width (style, border_width);
				html_style_set_border_right_width (style, border_width);
				html_style_set_border_bottom_width (style, border_width);
				html_style_set_border_left_width (style, border_width);
			}
			else if (css_parse_border_style (val, &border_style)) {
				html_style_set_border_top_style (style, border_style);
				html_style_set_border_right_style (style, border_style);
				html_style_set_border_bottom_style (style, border_style);
				html_style_set_border_left_style (style, border_style);
			}
			else if (css_parse_color (val, &color)) {
				html_style_set_border_top_color (style, &color);
				html_style_set_border_right_color (style, &color);
				html_style_set_border_bottom_color (style, &color);
				html_style_set_border_left_color (style, &color);
			}
		}
		break;
	case HTML_ATOM_CURSOR:

		switch (val->v.atom) {
		case HTML_ATOM_INHERIT:
			html_style_set_cursor (style, parent_style->inherited->cursor);
			break;
		case HTML_ATOM_AUTO:
			html_style_set_cursor (style, HTML_CURSOR_AUTO);
			break;
		case HTML_ATOM_CROSSHAIR:
			html_style_set_cursor (style, HTML_CURSOR_CROSSHAIR);
			break;
		case HTML_ATOM_DEFAULT:
			html_style_set_cursor (style, HTML_CURSOR_DEFAULT);
			break;
		case HTML_ATOM_POINTER:
			html_style_set_cursor (style, HTML_CURSOR_POINTER);
			break;
		case HTML_ATOM_MOVE:
			html_style_set_cursor (style, HTML_CURSOR_MOVE);
			break;
		case HTML_ATOM_E_RESIZE:
			html_style_set_cursor (style, HTML_CURSOR_E_RESIZE);
			break;
		case HTML_ATOM_NE_RESIZE:
			html_style_set_cursor (style, HTML_CURSOR_NE_RESIZE);
			break;
		case HTML_ATOM_NW_RESIZE:
			html_style_set_cursor (style, HTML_CURSOR_NW_RESIZE);
			break;
		case HTML_ATOM_N_RESIZE:
			html_style_set_cursor (style, HTML_CURSOR_N_RESIZE);
			break;
		case HTML_ATOM_SE_RESIZE:
			html_style_set_cursor (style, HTML_CURSOR_SE_RESIZE);
			break;
		case HTML_ATOM_SW_RESIZE:
			html_style_set_cursor (style, HTML_CURSOR_SW_RESIZE);
			break;
		case HTML_ATOM_S_RESIZE:
			html_style_set_cursor (style, HTML_CURSOR_S_RESIZE);
			break;
		case HTML_ATOM_W_RESIZE:
			html_style_set_cursor (style, HTML_CURSOR_W_RESIZE);
			break;
		case HTML_ATOM_TEXT:
			html_style_set_cursor (style, HTML_CURSOR_TEXT);
			break;
		case HTML_ATOM_WAIT:
			html_style_set_cursor (style, HTML_CURSOR_WAIT);
			break;
		case HTML_ATOM_HELP:
			html_style_set_cursor (style, HTML_CURSOR_HELP);
			break;
		default:
			break;
		}

		break;
		
	default:
		g_print ("Unhandled property: %d %s\n", prop, html_atom_list_get_string (html_atom_list, prop));
	}
}

#if 0
static void
css_matcher_sheet_stream_write (HtmlStream *stream, const gchar *buffer, guint size, CssFetcherContext *context)
{
	if (!context->str)
		context->str = g_string_new_len (buffer, size);
	else
		g_string_append_len (context->str, buffer, size);
}

static void
css_matcher_sheet_stream_close (HtmlStream *stream, CssFetcherContext *context)
{
	CssStylesheet *ss;

	if (html_stream_get_written (stream) != 0) {
		ss = css_parser_parse_stylesheet (context->str->str, context->str->len, NULL);

		context->stat->s.import_rule.fetched = TRUE;
	
		if (ss) {
			context->stat->s.import_rule.sheet = ss;
		}

		g_string_free (context->str, TRUE);

		html_document_restyle_views (context->doc);
	}

	g_free (context);
}

#endif

static void
css_matcher_apply_stylesheet (HtmlDocument *doc, CssStylesheet *ss, xmlNode *node, GList **declaration_list, gint type, HtmlAtom *pseudo)
{
	GSList *list = ss->stat;

	for (list = ss->stat; list; list = list->next) {
		CssStatement *stat = list->data;
		gint j;

		if (stat->type == CSS_IMPORT_RULE) {
			if (stat->s.import_rule.fetched) {
				if (stat->s.import_rule.sheet) {
					css_matcher_apply_stylesheet (doc, stat->s.import_rule.sheet, node, declaration_list, type, pseudo);
				}
			}
			else if (!stat->s.import_rule.fetching) {
#if 0
				HtmlStream *stream;
				CssFetcherContext *context = g_new0 (CssFetcherContext, 1);
					
				/* FIXME: Can we assume this? */
				gchar *str = g_strndup (stat->s.import_rule.url->v.s->buffer,
							stat->s.import_rule.url->v.s->len);
				
				stream = html_stream_new ((HtmlStreamWriteFunc)css_matcher_sheet_stream_write,
							  (HtmlStreamCloseFunc)css_matcher_sheet_stream_close,
							  context);
				
				context->stat = stat;
				context->doc = doc;
				
				stat->s.import_rule.fetching = TRUE;
				
				gtk_signal_emit_by_name (GTK_OBJECT (doc), "url_requested", str, stream);
				
				g_free (str);
#endif			
			}
		}
		
		/* FIXME: We need to support more than just rulesets here */
		if (stat->type != CSS_RULESET)
			continue;
		
		for (j = 0; j < stat->s.ruleset->n_sel; j++) {
			CssSelector *sel = stat->s.ruleset->sel[j];
			
			if (css_matcher_match_selector (sel, node, pseudo)) {
				int i;
				
				for (i = 0; i < stat->s.ruleset->n_decl; i++) {
					CssDeclaration *decl = stat->s.ruleset->decl[i];
					CssDeclarationListEntry *entry = g_new (CssDeclarationListEntry, 1);
					
					entry->spec = sel->a * 1000000 + sel->b * 1000 + sel->c;
					entry->type = type;
					entry->decl = g_new (CssDeclaration, 1);
					entry->decl->property = decl->property;
					entry->decl->expr = css_value_ref (decl->expr);
					entry->decl->important = decl->important;
					*declaration_list = g_list_insert_sorted (*declaration_list, entry, css_declaration_list_sorter);
				}
			}
		}
	}
}

static void
css_matcher_validate_style (HtmlStyle *style)
{
	/* Do This is according to the CSS2 specification section 9.7 */
	if (style->position == HTML_POSITION_ABSOLUTE ||
	    style->position == HTML_POSITION_FIXED) {

		style->display = HTML_DISPLAY_BLOCK;
		style->Float   = HTML_FLOAT_NONE;
	}
	else if (style->Float != HTML_FLOAT_NONE) {

		style->display = HTML_DISPLAY_BLOCK;
	}
}

static void
free_decl_entry (CssDeclarationListEntry *entry, gpointer user_data)
{
	css_value_unref (entry->decl->expr);
	g_free (entry->decl);
	g_free (entry);
}

static void
css_matcher_html_to_css (HtmlDocument *document, HtmlStyle *style, xmlNode *n)
{
	gchar *str;

	if (n->type != XML_ELEMENT_NODE)
		return;


	if (strcasecmp (n->name, "td") == 0 ||
	    strcasecmp (n->name, "th") == 0) {
		DomNode *node = dom_Node_mkref (n);
		
		gint level = 4;

		node = dom_Node__get_parentNode (node);
		
		while (level-- && node) {
			/* FIXME: Check if border is a valid number */
			if (node->style && node->style->display == HTML_DISPLAY_TABLE &&
			    dom_Element_hasAttribute (DOM_ELEMENT (node), "border")) {
				gchar *str = dom_Element_getAttribute (DOM_ELEMENT (node), "border");

				str = g_strchug (str);
				
				if (atoi (str) > 0) {
					html_style_set_border_top_width (style, HTML_BORDER_WIDTH_THIN);
					html_style_set_border_right_width (style, HTML_BORDER_WIDTH_THIN);
					html_style_set_border_bottom_width (style, HTML_BORDER_WIDTH_THIN);
					html_style_set_border_left_width (style, HTML_BORDER_WIDTH_THIN);
					
					html_style_set_border_top_style (style, HTML_BORDER_STYLE_INSET);
					html_style_set_border_left_style (style, HTML_BORDER_STYLE_INSET);
					html_style_set_border_right_style (style, HTML_BORDER_STYLE_INSET);
					html_style_set_border_bottom_style (style, HTML_BORDER_STYLE_INSET);
				}
				xmlFree (str);

				break;
			}
			node = dom_Node__get_parentNode (node);
		}
	}

	if (n->properties == NULL)
		return;
	
	if (strcasecmp (n->name, "table") == 0) {

		if ((str = xmlGetProp (n, "border"))) {
			gint border_width;
			if (*str == 0)
				border_width = 1;
			else
				border_width = atoi (str);
			
			html_style_set_border_top_width (style, border_width);
			html_style_set_border_right_width (style, border_width);
			html_style_set_border_bottom_width (style, border_width);
			html_style_set_border_left_width (style, border_width);
			
			html_style_set_border_top_style (style, HTML_BORDER_STYLE_OUTSET);
			html_style_set_border_left_style (style, HTML_BORDER_STYLE_OUTSET);
			html_style_set_border_right_style (style, HTML_BORDER_STYLE_OUTSET);
			html_style_set_border_bottom_style (style, HTML_BORDER_STYLE_OUTSET);
			
			xmlFree (str);
		}
	}
	if (strcasecmp (n->name, "img") == 0 || 
	    strcasecmp (n->name, "applet") == 0 ||
	    strcasecmp (n->name, "object") == 0) {
		if ((str = xmlGetProp (n, "hspace"))) {
			HtmlLength length;
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);
			html_style_set_padding_left (style, &length);
			html_style_set_padding_right (style, &length);
			xmlFree (str);
		}
		if ((str = xmlGetProp (n, "vspace"))) {
			HtmlLength length;
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);
			html_style_set_padding_top (style, &length);
			html_style_set_padding_bottom (style, &length);
			xmlFree (str);
		}
	}
	if (strcasecmp (n->name, "body") == 0) {
		if ((str = xmlGetProp (n, "text"))) {
			HtmlColor *color = html_color_new_from_name (str);
			if (color) {
				html_style_set_color (style, color);
				html_color_destroy (color);
			}
			xmlFree (str);
		}
	}
	else if (strcasecmp (n->name, "font") == 0) {
		if ((str = xmlGetProp (n, "family"))) {		
			html_style_set_font_family (style, str);
			xmlFree (str);
		}
		if ((str = xmlGetProp (n, "size"))) {		
			gfloat min_size;
			gint font_size;

			/* Relative size */
			if (strchr (str, '+') || strchr (str, '-')) {

				gint value = atoi (str);
				font_size = html_font_specification_get_html_size (style->inherited->font_spec) + value;
			}
			/* Absolute size */
			else
				font_size = atoi (str);

			g_object_get (gtk_settings_get_default (),
				      "gtkhtml-minimum-font-size",
				      &min_size, NULL);
			if (html_font_description_html_size_to_pt (font_size)
			    < min_size) {
				HtmlFontSpecification *min_size_spec;

				min_size_spec = html_font_specification_new ("min_size",
									     HTML_FONT_STYLE_NORMAL,
									     HTML_FONT_VARIANT_NORMAL,
									     HTML_FONT_WEIGHT_100,
									     HTML_FONT_STRETCH_NORMAL,
									     HTML_FONT_DECORATION_NONE,
									     min_size);
				html_style_set_font_size_html (style, html_font_specification_get_html_size (min_size_spec));
				html_font_specification_unref (min_size_spec);
			} else {
				html_style_set_font_size_html (style, font_size);
			}

			xmlFree (str);
		}
	}
	
	/* convert align="left|right|center|justify" to "text-align:x"
	   * Match against tr, td, th, thead, tfoot, tbody, div, p, h1, h2, h3, h4, h5, h6 */
	else if ((strcasecmp ("tr", n->name) == 0) || 
		 (strcasecmp ("td", n->name) == 0) || 
		 (strcasecmp ("th", n->name) == 0) || 
		 (strcasecmp ("thead", n->name) == 0) || 
		 (strcasecmp ("tbody", n->name) == 0) || 
		 (strcasecmp ("tfoot", n->name) == 0) || 
		 (strcasecmp ("div", n->name) == 0) || 
		 (strcasecmp ("p", n->name) == 0) || 
		 (strlen (n->name) == 2 && tolower(n->name[0]) == 'h' && strchr ("123456", n->name[1]))) {

		if ((str = xmlGetProp (n, "align"))) {
			if (strcasecmp (str, "left") == 0)
				html_style_set_text_align (style, HTML_TEXT_ALIGN_LEFT);
			else if (strcasecmp (str, "right") == 0)
				html_style_set_text_align (style, HTML_TEXT_ALIGN_RIGHT);
			else if (strcasecmp (str, "center") == 0)
				html_style_set_text_align (style, HTML_TEXT_ALIGN_CENTER);
			else if (strcasecmp (str, "justify") == 0)
				html_style_set_text_align (style, HTML_TEXT_ALIGN_JUSTIFY);
			xmlFree (str);
		}
	}
	if ((str = xmlGetProp (n, "color"))) {
		HtmlColor *color = html_color_new_from_name (str);
		if (color) {
			html_style_set_color (style, color);
			html_color_destroy (color);
		}
		xmlFree (str);
	}
	if ((str = xmlGetProp (n, "bgcolor"))) {
		HtmlColor *color = html_color_new_from_name (str);
		if (color) {
			html_style_set_background_color (style, color);
			html_color_destroy (color);
		}
		xmlFree (str);
	}
	if ((str = xmlGetProp (n, "background"))) {
		HtmlImage *image;

		image = html_image_factory_get_image (document->image_factory, str);

		html_style_set_background_image (style, image);
		g_object_unref (G_OBJECT(image));
		xmlFree (str);
	}
	if ((str = xmlGetProp (n, "width"))) {
		HtmlLength length;

		str = g_strchug (str);

		if (strchr (str, '%'))
			html_length_set_value (&length, atoi (str), HTML_LENGTH_PERCENT);
		else if (g_ascii_isdigit (*str))
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);

		html_style_set_width (style, &length);

		xmlFree (str);
	}
	if ((str = xmlGetProp (n, "height"))) {
		HtmlLength length;

		str = g_strchug (str);

		if (strchr (str, '%'))
			html_length_set_value (&length, atoi (str), HTML_LENGTH_PERCENT);
		else if (g_ascii_isdigit (*str))
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);

		html_style_set_height (style, &length);

		xmlFree (str);
	}
}

static void
css_matcher_html_to_css_after (HtmlStyle *style, xmlNode *n) 
{
	gchar *str;

	if (n->type != XML_ELEMENT_NODE)
		return;

	if (n->properties == NULL)
		return;

	if (strcasecmp ("body", n->name) == 0) {
		if ((str = xmlGetProp (n, "leftmargin"))) {
			HtmlLength length;
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);
			html_style_set_margin_left (style, &length);
			xmlFree (str);
		}
		if ((str = xmlGetProp (n, "rightmargin"))) {
			HtmlLength length;
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);
			html_style_set_margin_right (style, &length);
			xmlFree (str);
		}
		if ((str = xmlGetProp (n, "topmargin"))) {
			HtmlLength length;
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);
			html_style_set_margin_top (style, &length);
			xmlFree (str);
		}
		if ((str = xmlGetProp (n, "bottommargin"))) {
			HtmlLength length;
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);
			html_style_set_margin_bottom (style, &length);
			xmlFree (str);
		}
		if ((str = xmlGetProp (n, "marginwidth"))) {
			HtmlLength length;
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);
			html_style_set_margin_left (style, &length);
			html_style_set_margin_right (style, &length);
			xmlFree (str);
		}
		if ((str = xmlGetProp (n, "marginheight"))) {
			HtmlLength length;
			html_length_set_value (&length, atoi (str), HTML_LENGTH_FIXED);
			html_style_set_margin_top (style, &length);
			html_style_set_margin_bottom (style, &length);
			xmlFree (str);
		}
	}
	if (strcasecmp ("table", n->name) == 0) {
		if ((style->display != HTML_DISPLAY_BLOCK) && (str = xmlGetProp (n, "align"))) {
			if (strcasecmp (str, "left") == 0)
				style->Float = HTML_FLOAT_LEFT;
			else if (strcasecmp (str, "right") == 0)
			style->Float = HTML_FLOAT_RIGHT;
			xmlFree (str);
		}
		if ((str = xmlGetProp (n, "cellspacing"))) {
			gint spacing = atoi (str);
			html_style_set_border_spacing (style, spacing, spacing);
			xmlFree (str);
		}
	}
}

HtmlStyle *
css_matcher_get_style (HtmlDocument *doc, HtmlStyle *parent_style, xmlNode *node, HtmlAtom *pseudo)
{
	/* We have to send the old font specification separately because all the em and ex values are based on the
	 * old value not the new */
	HtmlFontSpecification *font_spec = parent_style ? parent_style->inherited->font_spec : NULL;
	xmlChar *prop;
	GList *declaration_list = NULL;
	GList *temp;
	GSList *ss;
	HtmlStyle *style;

	total_pseudos = CSS_STYLESHEET_PSEUDO_NONE;
	
	style = html_style_new (parent_style);

	/* First, convert known HTML properties into style properties */
	css_matcher_html_to_css (doc, style, node);
	
	if (!default_stylesheet) {
		default_stylesheet = css_parser_parse_stylesheet (html_css, strlen (html_css), NULL);
	}

	css_matcher_apply_stylesheet (doc, default_stylesheet, node, &declaration_list, CSS_STYLESHEET_DEFAULT, pseudo);
	
	ss = doc->stylesheets;

	while (ss) {
		CssStylesheet *sheet = ss->data;

		css_matcher_apply_stylesheet (doc, sheet, node, &declaration_list, CSS_STYLESHEET_AUTHOR, pseudo);

		ss = ss->next;
	}
		
	/* FIXME: Should we look at namespaces here, yes, but probably global namespaces */
	prop = xmlGetProp (node, "style");
	
	if (prop) {
		CssRuleset *rs = css_parser_parse_style_attr (prop, strlen (prop), NULL);
		gint i;
		
		if (rs) {
			for (i = 0; i < rs->n_decl; i++) {
				CssDeclarationListEntry *entry = g_new (CssDeclarationListEntry, 1);
				CssDeclaration *decl = rs->decl[i];

				entry->type = CSS_STYLESHEET_STYLEDECL;
				entry->decl = g_new (CssDeclaration, 1);
				entry->decl->property = decl->property;
				entry->decl->expr = css_value_ref (decl->expr);
				entry->decl->important = decl->important;
				entry->spec = 0;

				declaration_list = g_list_insert_sorted (declaration_list, entry, css_declaration_list_sorter);
			}
			css_ruleset_destroy (rs);
		}
		xmlFree (prop);
	}

	temp = declaration_list;
	while (declaration_list) {
		CssDeclarationListEntry *entry = declaration_list->data;

		css_matcher_apply_rule (doc, style, parent_style, font_spec, entry->decl);

		declaration_list = declaration_list->next;
	}
	declaration_list = temp;
	g_list_foreach (declaration_list, (GFunc) free_decl_entry, NULL);
	g_list_free (declaration_list);

	if (style->unicode_bidi == HTML_UNICODE_BIDI_EMBED) {
		
		if (parent_style && style->inherited->direction != parent_style->inherited->direction)
			html_style_set_bidi_level (style, style->inherited->bidi_level + 1);
		else if (parent_style == NULL && style->inherited->direction == HTML_DIRECTION_RTL)
			html_style_set_bidi_level (style, HTML_DIRECTION_RTL);
	}

	css_matcher_validate_style (style);
	css_matcher_html_to_css_after (style, node);

	if (total_pseudos & CSS_STYLESHEET_PSEUDO_HOVER)
		style->has_hover_style = TRUE;
	if (total_pseudos & CSS_STYLESHEET_PSEUDO_ACTIVE)
		style->has_active_style = TRUE;
	if (total_pseudos & CSS_STYLESHEET_PSEUDO_FOCUS)
		style->has_focus_style = TRUE;
	if (total_pseudos & CSS_STYLESHEET_PSEUDO_BEFORE)
		style->has_before_style = TRUE;
	if (total_pseudos & CSS_STYLESHEET_PSEUDO_AFTER)
		style->has_after_style = TRUE;
	
	return style;
}

