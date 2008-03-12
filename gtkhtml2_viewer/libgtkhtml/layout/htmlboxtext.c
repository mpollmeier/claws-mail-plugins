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

#include <gtk/gtk.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

/* FIXME: Don't include htmlgdkpainter.h */
#include "graphics/htmlgdkpainter.h"
#include "graphics/htmlpainter.h"
#include "graphics/htmlfontspecification.h"
#include "htmlbox.h"
#include "htmlstyle.h"
#include "htmlboxtext.h"
#include "htmlboxinline.h"
#include "htmlrelayout.h"


static HtmlBoxClass *parent_class = NULL;

struct _HtmlBoxTextMaster {
	guchar *text; /* UTF-8 original text. */
	guchar *canon_text;
	
	int n_items;
	HtmlBoxTextItemData *items;
	
	guint must_relayout : 1;
	guint preserved_leading_space : 1;
	HtmlFontSpecification *font_spec;
	guint white_space:2;
};

struct _HtmlBoxTextItemData {
	PangoLogAttr *log_attrs;
	PangoGlyphUnit *log_widths;
	PangoItem *item;
};

void
html_box_text_free_relayout (HtmlBoxText *box)
{
	while (box) {
		if (!HTML_IS_BOX_TEXT (box))
			break;

		/* free unneeded data from box */
		
		if (html_box_text_is_master (box))
			break;

		box = HTML_BOX_TEXT (HTML_BOX (box)->prev);
	}
}

static void
html_box_text_destroy_slaves (HtmlBox *self)
{
	HtmlBox *box, *tmp_box;

	box = self->next;
	
	while (box) {
		if (!HTML_IS_BOX_TEXT (box))
			break;

		tmp_box = box;
		
		if (html_box_text_is_master (HTML_BOX_TEXT (box)))
			break;

		box = box->prev;
		/* Now remove the box link */
		html_box_remove (tmp_box);
		box = box->next;
		/* Free the memory */
		g_object_unref (G_OBJECT (tmp_box));
	}

}

static void
html_box_text_remove (HtmlBox *box)
{
	html_box_text_destroy_slaves (box);

	parent_class->remove (box);
}

static void
html_box_text_free_master (HtmlBoxTextMaster *master)
{
	int i;

	if (master->canon_text != master->text)
		g_free (master->canon_text);
	master->canon_text = NULL;
	
	for (i = 0;i < master->n_items; i++) {
		HtmlBoxTextItemData *data = &master->items[i];
		
		if (data->item)
			pango_item_free (data->item);
		
		g_free (data->log_attrs);
		data->log_attrs = NULL;
		g_free (data->log_widths);
		data->log_widths = NULL;
	}
	g_free (master->items);
	master->items = NULL;
	master->n_items = 0;

	if (master->font_spec)
		html_font_specification_unref (master->font_spec);
	master->font_spec = NULL;
	
}

static void
html_box_text_finalize (GObject *self)
{
	HtmlBoxText *text = HTML_BOX_TEXT (self);
	HtmlBoxTextMaster *master;

	master = text->master;
	if (master) {
		html_box_text_destroy_slaves (HTML_BOX(text));
		html_box_text_free_master (master);
		g_free (master);
		text->master = NULL;
	}

	if (text->glyphs) {
		pango_glyph_string_free (text->glyphs);
		text->glyphs = NULL;
	}
	
	G_OBJECT_CLASS (parent_class)->finalize (self);
}


/* FIXME: This should really take letter_spacing and word_spacing into account */
static gint
glyphs_width (PangoGlyphString *glyphs)
{
	int i;
	gint width;
	
	width = 0;
	for (i=0;i<glyphs->num_glyphs;i++) {
		width += glyphs->glyphs[i].geometry.width;
	}

	return PANGO_PIXELS (width);

}

static gboolean
is_newline (guchar ch)
{
	return ch == '\n' || ch == '\r';
}

static gboolean
is_white (guchar ch)
{
	return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

/* Note: The canonical text is not null-terminated */
static guchar *
html_box_text_canonicalize (guchar *src, guchar *dst, gint src_len, gboolean preserve_leading_space)
{
	guchar *src_end = src + src_len;
	guchar *start;
	int i;
	gint bytes;
	gboolean seen_newline;

	start = src;
	seen_newline = FALSE;

	if (!preserve_leading_space) {
		while ((src != src_end) && is_white (*src)) {
			if ((*src == '\n') || (*src == '\r'))
				seen_newline = TRUE;
			src++;
		}
#if 0
		if ((src != start))
			*dst++ = (seen_newline)?'\n':' ';
#endif
	}	

	while (src != src_end) {
		if (is_white (*src)) {
			seen_newline = FALSE;
			while ((src != src_end) && is_white (*src)) {
				if ((*src == '\n') || (*src == '\r'))
					seen_newline = TRUE;
				src++;
			}
			*dst++ = (seen_newline)?'\n':' ';
		} else {
			bytes = g_utf8_skip[(unsigned int)*src]; 
			for (i=0;i<bytes;i++)
				*dst++ = *src++;
		}
	}
	return dst;
}

static gboolean
pre_has_strange_whitespace (guchar *text, int length)
{
	guchar *end;

	end = text + length;

	while (text != end) {
		if (is_white (*text) && *text != ' ')
			return TRUE;
		text += g_utf8_skip[(unsigned int)*text]; 
	}
	return FALSE;
}

static guchar *
pre_convert_whitespace (guchar *text, int length, gint line_offset, gint *out_len)
{
	guchar *end;
	guchar *dst, *res, *ptr;
	gint bytes, i;
	gint num_tabs = 0;

	end = text + length;
	/* Count the number of tabs */
	ptr = text;
	while (ptr != end) {
		if (*ptr++ == '\t')
			num_tabs++;
	}
	res = g_malloc (length + (8 * num_tabs));
	dst = res;

	while (text != end) {
		if (*text == '\t') {
			gint num = 8 - ((line_offset + dst - res) % 8);
			for (i=0;i<num;i++)
				*dst++ = ' ';
			text++;
		}
		else if (is_white (*text)) {
			*dst++ = ' ';
			text++;
		}
		else {
			bytes = g_utf8_skip[(unsigned int)*text]; 
			for (i=0;i<bytes;i++)
				*dst++ = *text++;
		}
	}
	*out_len = dst - res;
	return res;
}

static gboolean
can_break_at (HtmlBox *self, PangoLogAttr attr)
{
	if (HTML_BOX_GET_STYLE (self->parent)->inherited->white_space == HTML_WHITE_SPACE_NORMAL) 
		return attr.is_line_break;
	else 
		return attr.is_mandatory_break;
}

static void
strip_newlines (guchar *start, guchar *end)
{
	while (start < end) {
		if (*start == '\n')
			*start = ' ';
		start += g_utf8_skip[(unsigned int)*start]; 
	}
}

static void
html_box_text_get_extents (HtmlBox *self, PangoRectangle *log_rect)
{
	HtmlBoxText *text = HTML_BOX_TEXT (self);
	
	if (text->glyphs) {
		pango_glyph_string_extents (text->glyphs, text->item_data->item->analysis.font, NULL, log_rect);
	} else {
		log_rect->x = 0;
		log_rect->y = 0;
		log_rect->width = 0;
		log_rect->height = 0;
	}
		
	if (HTML_BOX_GET_STYLE (self)->inherited->font_spec->decoration & HTML_FONT_DECORATION_UNDERLINE) {
		log_rect->height = MAX (log_rect->height, 2 * PANGO_SCALE - log_rect->y);
 	}
	if (HTML_BOX_GET_STYLE (self)->inherited->font_spec->decoration & HTML_FONT_DECORATION_OVERLINE) {
		log_rect->height += 2 * PANGO_SCALE;
		log_rect->y -= 2 * PANGO_SCALE;
	} 
}

/* TODO:
 * Set must_relayout if font_spec or white_space in the box style is changed.
 */

static void
html_box_text_recalc_items (HtmlBoxText *text,
			    HtmlFontSpecification *font_spec,
			    HtmlWhiteSpaceType white_space,
			    HtmlRelayout *relayout)
{
	HtmlBoxTextMaster *master;
	GList *items, *l;
	PangoAttrList *attrs;
	static PangoContext *context = NULL;
	HtmlBoxTextItemData *data;
	guchar *canon_text, *canon_end;
	PangoGlyphString *glyphs;
	int orig_len;
	char *ctype;

	master = text->master;

	html_box_text_free_master (master);
	orig_len = strlen (master->text);
	
	/* Split up the canonicalized text into PangoItems and
	   create slave textboxes for each of them */
	attrs = pango_attr_list_new ();
	html_font_specification_get_all_attributes (font_spec, attrs, 0, orig_len,
						    relayout->magnification);
	
	/* FIXME: The context should be stored somewhere in the view tree */
	if (context == NULL) {
		context = gdk_pango_context_get ();

		/* FIXME: This should be set from the document */

		/* modified by phill.zhang@sun.com, borrow form fontconfig */
                ctype = setlocale (LC_CTYPE, NULL);
                if (!ctype || !strcmp (ctype, "C"))
                {
                    ctype = getenv ("LC_ALL");
                    if (!ctype)
                    {
                        ctype = getenv ("LC_CTYPE");
                        if (!ctype)
                            ctype = getenv ("LANG");
                    }
                }
                if (!ctype || !strcmp(ctype, "C") || !strcmp(ctype, "POSIX"))
		    pango_context_set_language (context, pango_language_from_string ("en")); 
                else
		    pango_context_set_language (context, pango_language_from_string (ctype)); 
	}
	
	if (white_space == HTML_WHITE_SPACE_PRE) {
		canon_text = master->text;
		canon_end = master->text + orig_len;
	} else {
		canon_text = g_malloc (orig_len);
		canon_end = html_box_text_canonicalize (master->text,
							canon_text,
							orig_len,
							relayout->preserve_leading_space);
	}

	master->canon_text = canon_text;

	items = NULL;
	if (canon_end > canon_text)
	    items = pango_itemize (context,
				   canon_text,
				   0,
				   canon_end - canon_text,
				   attrs,
				   NULL);
	
	pango_attr_list_unref (attrs);

	if (items) {
		master->n_items = g_list_length (items);
		master->items = g_new (HtmlBoxTextItemData, master->n_items);
	} else {
		master->n_items = 0;
		master->items = NULL;
	}
	data = master->items;
	l = items;
	while (l) {
		PangoItem *item = (PangoItem *) l->data;
		guint num_chars;
		guchar *canon;
		gint canon_len;
		
		
		data->item = item;
		canon = canon_text + item->offset;
		canon_len = item->length;

		if (white_space != HTML_WHITE_SPACE_PRE) {
			num_chars = g_utf8_pointer_to_offset (canon, canon + canon_len);
			
			data->log_attrs = g_new (PangoLogAttr, num_chars + 1);
			pango_break (canon, canon_len, &data->item->analysis, data->log_attrs, num_chars + 1);
			/* After breaking, convert all '\n' to ' ' to avoid rendering them.
			 * They were only useful to get is_mandatory_break in log_attrs. */
			strip_newlines (canon, canon + canon_len);
			
			glyphs = pango_glyph_string_new ();
			
			pango_shape (canon, canon_len, &data->item->analysis, glyphs);
			data->log_widths = g_new (PangoGlyphUnit, num_chars);
			pango_glyph_string_get_logical_widths (glyphs,
							       canon, canon_len,
							       data->item->analysis.level,
							       data->log_widths);
			
			pango_glyph_string_free (glyphs);
		} else {
			/* pre whitespace */
			data->log_attrs = NULL;
			data->log_widths = NULL;
		}
		
		l = g_list_next (l);
		data++;
	}
	g_list_free (items);
	master->must_relayout = FALSE;
	master->preserved_leading_space = relayout->preserve_leading_space;
	master->font_spec = html_font_specification_dup (font_spec);
	master->white_space = white_space;
}
     
/* Tries to make the box fit in relayout->max_width by breaking the text.
 * If the text cannot be broken to fit, return the smallest box it can.
 * This will use an extra box at the start of all lines, but it works.
 */
static void
html_box_text_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	HtmlBox *box_child;
	HtmlBoxText *text = HTML_BOX_TEXT (self);
	guchar *ptr = NULL;
	guchar *end;
	HtmlBoxTextMaster *master;
	gint len, width = 0, height;
	gboolean create_no_slave = FALSE;
	PangoRectangle log_rect;
	guchar *canon_text, *canon_end;
	int i;

	relayout->text_item_length = text->length;

	text->forced_newline = FALSE;

	master = text->master;
	if (master) {
		HtmlFontSpecification *font_spec;
		HtmlWhiteSpaceType white_space;

		font_spec = HTML_BOX_GET_STYLE (self)->inherited->font_spec;
		white_space = HTML_BOX_GET_STYLE (self->parent)->inherited->white_space;
		if ((master->font_spec &&
		     !html_font_description_equal (master->font_spec, font_spec)) ||
		    (relayout->preserve_leading_space != master->preserved_leading_space) ||
		    (white_space != master->white_space) ||
		    relayout->magnification_modified)
			master->must_relayout = TRUE;

		if (master->must_relayout)
			html_box_text_recalc_items (text, font_spec, 
						    white_space, relayout);
		
		/* Remove the old slaves */
		html_box_text_destroy_slaves (self);

		/* if no items */
		text->canon_text = NULL; 
		text->length = 0;
		text->item_data = NULL;
		text->item_offset = 0;

		for (i = master->n_items - 1; i >= 0; i--) {
			HtmlBoxTextItemData *data = &master->items[i];
			PangoItem *item = data->item;
			HtmlBoxText *child_text;

			if (i > 0) {
				box_child = html_box_text_new (FALSE);
				child_text = HTML_BOX_TEXT (box_child);
				if (self->dom_node) {
					box_child->dom_node = self->dom_node;
					g_object_add_weak_pointer (G_OBJECT (self->dom_node), (gpointer *) &(box_child->dom_node));
				}
				else {
					html_box_set_style (box_child, HTML_BOX_GET_STYLE (self));
				}


				html_box_insert_after (self, box_child);
			} else {
				child_text = text;
			}
			child_text->canon_text = master->canon_text + item->offset;
			child_text->length = item->length;
			child_text->item_data = data;
			child_text->item_offset = 0;
			child_text->generated_content = text->generated_content;
		}
	}

	/* Break the item */
	len = 0;
	width = 0;

	if (text->glyphs == NULL)
		text->glyphs = pango_glyph_string_new ();
	
	switch (HTML_BOX_GET_STYLE (self->parent)->inherited->white_space) {
	case HTML_WHITE_SPACE_PRE:
		/* Find the first place to break the string at */
		ptr = text->canon_text;
		end = text->canon_text + text->length;

		while (ptr < end && *ptr != '\n' && *ptr != '\r') 
			ptr += g_utf8_skip[(unsigned int)*ptr];

		if (ptr != text->canon_text) {
			guchar *str;
			gint len;

			if (pre_has_strange_whitespace (text->canon_text, ptr - text->canon_text)) {
				str = pre_convert_whitespace (text->canon_text, 
							      ptr - text->canon_text, 
							      relayout->line_offset, &len);
				relayout->text_item_length = len;
			}
			else {
				str = text->canon_text;
				len = ptr - text->canon_text;
			}
			pango_shape (str, len, &text->item_data->item->analysis, text->glyphs);
			
			if (str != text->canon_text)
				g_free (str);
		} else {
			/* Only an empty line */
			pango_shape (" ", 1, &text->item_data->item->analysis, text->glyphs);
		}
		width = glyphs_width (text->glyphs);

		/* Skip the newline */
		if (ptr < end && *ptr) {
			guchar last;
			
			last = *ptr;
			ptr++;
			if (last == '\r' && ptr < end && *ptr == '\n')
				ptr++;
		}
			
		if (ptr == end)
			create_no_slave = TRUE;
		break;

	case HTML_WHITE_SPACE_NOWRAP:
	case HTML_WHITE_SPACE_NORMAL:
		{
		gint new_width = 0;
		gint num_chars;
		guchar *can, *last_can;
		HtmlBoxTextItemData *item_data;
		int pos;

		canon_text = text->canon_text;
		canon_end = text->canon_text + text->length;

		if (canon_text == NULL) { /* Empty text */
			pango_glyph_string_free (text->glyphs);
			text->glyphs = NULL;
			width = 0;
			create_no_slave = TRUE;
			break;
		}

		num_chars = g_utf8_pointer_to_offset (canon_text, canon_end);

		width = 0;
		can = canon_text;
		pos = text->item_offset;
		item_data = text->item_data;
		do {
			width += new_width;
			new_width = 0;
			last_can = can;

			if (can != canon_end) {
				/* Never break at the first position, and
				 * skip the possible break point we checked last
				 * iteration. */
				can += g_utf8_skip[(unsigned int)*can];
				new_width += item_data->log_widths[pos];
				pos++;
			}

			/* Find the first place to try break the string at */
			while (can != canon_end && (!can_break_at (self, item_data->log_attrs[pos]))) {
				can += g_utf8_skip[(unsigned int)*can];
				new_width += item_data->log_widths[pos];
				pos++;
			}
		} while (can < canon_end && PANGO_PIXELS (width + new_width) <= relayout->max_width);

		/* Check if the last segment fit too */
		if ( (can == canon_end) && (PANGO_PIXELS (width + new_width) <= relayout->max_width)) {
			create_no_slave = TRUE;
			width += new_width;
			last_can = can;
		}
		
		if (width == 0) { /* We couldn't find a place to break to make the text small enough */

                        if (can == canon_end || relayout->line_is_empty == FALSE)
				create_no_slave = TRUE;
			width = new_width;
			last_can = can;
		} 

#if 0
		g_print ("broke line of width: %d (target: %d) from %p to %p (%d bytes).\nText: \"", PANGO_PIXELS (width), relayout->max_width, canon_text, last_can, last_can - canon_text);
		{gchar *c = canon_text; while (c < last_can) g_print ("%c", *c++);}
		g_print ("\"\n");
#endif
				
		/* Shape the resulting line */
		pango_shape (canon_text, last_can - canon_text, &item_data->item->analysis, text->glyphs);

		ptr = last_can;
		width = PANGO_PIXELS (width);
		}
		break;
	default:
		break;
	}

	if (create_no_slave == FALSE) {
		HtmlBoxText *child_text;
		gint first_len;

		/* Create a new text block with the remaining text */
		box_child = html_box_text_new (FALSE);
		child_text = HTML_BOX_TEXT (box_child);
		if (self->dom_node) {

			box_child->dom_node = self->dom_node;
			g_object_add_weak_pointer (G_OBJECT (self->dom_node), (gpointer *) &(box_child->dom_node));
		}
		else {
			html_box_set_style (box_child, HTML_BOX_GET_STYLE (self));
		}
		
		child_text->canon_text = ptr;
		first_len = ptr - text->canon_text;
		child_text->length = text->length - first_len;
		text->length = first_len;
		child_text->item_offset = text->item_offset + g_utf8_pointer_to_offset (text->canon_text, ptr);
		child_text->item_data = text->item_data;
		child_text->generated_content = text->generated_content;

		html_box_insert_after (self, box_child);
	} 
	
	/* We need to know if the last character in this textbox was a whitespace */
	if (text->canon_text) {
		relayout->preserve_leading_space = is_white (*(text->canon_text + text->length - 1)) ? FALSE : TRUE;
		if (HTML_BOX_GET_STYLE (self->parent)->inherited->white_space != HTML_WHITE_SPACE_NORMAL)
			text->forced_newline = is_newline (*(text->canon_text + text->length - 1)) ? TRUE : FALSE;
	} else
		relayout->preserve_leading_space = TRUE;

	html_box_text_get_extents (self, &log_rect);
	text->ascent = PANGO_PIXELS (PANGO_ASCENT (log_rect));
	text->descent = PANGO_PIXELS (PANGO_DESCENT (log_rect));
	height = PANGO_PIXELS (log_rect.height);
	
	self->width = width;
	self->height = height;

	/* If this is the first box in an inline block width a border, then add the border width to the width */
	if (self->prev == NULL && HTML_IS_BOX_INLINE (self->parent))
		self->width += html_box_left_border_width (self->parent);
	/* If this is the first box in an inline block width a border, then add the border width to the width */
	if (self->next == NULL && HTML_IS_BOX_INLINE (self->parent))
		self->width += html_box_right_border_width (self->parent);
}

gint
html_box_text_get_len (HtmlBoxText *box)
{
	g_return_val_if_fail (box != NULL, 0);
	g_return_val_if_fail (HTML_IS_BOX_TEXT (box), 0);
	
	return box->length;
}

gchar *
html_box_text_get_text (HtmlBoxText *box, int *text_len)
{
	g_return_val_if_fail (box != NULL, NULL);

	if (text_len)
		*text_len = box->length;
	return box->canon_text;
}

gboolean
html_box_text_is_master (HtmlBoxText *box)
{
	g_return_val_if_fail (box != NULL, FALSE);

	return box->master != NULL;
}

void
html_box_text_set_text (HtmlBoxText *box, gchar *txt)
{
	g_return_if_fail (box != NULL);
	g_return_if_fail (box->master != NULL);

	box->master->text = txt;
	box->master->must_relayout = TRUE;
}

void
html_box_text_set_generated_content (HtmlBoxText *text, gchar *txt)
{
	g_return_if_fail (text != NULL);

	html_box_text_set_text (text, txt);
	text->generated_content = TRUE;
}

static gint
html_box_text_get_ascent (HtmlBox *self)
{
	HtmlBoxText *text = HTML_BOX_TEXT (self);
	return text->ascent;
}

static gint
html_box_text_get_descent (HtmlBox *self)
{
	HtmlBoxText *text = HTML_BOX_TEXT (self);
	return text->descent;
}

static void
html_box_text_paint_selection (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBoxText *text = HTML_BOX_TEXT (self);
	HtmlBoxTextSelection selection = text->selection;

	if (selection != HTML_BOX_TEXT_SELECTION_NONE) {

		gint width, start_pos = 0, width1, width2;
		gint start_index = text->sel_start_index;
		gint end_index = text->sel_end_index;

		/* If the text is RTL, then we have to swap some values
		 * to draw the selection correct */
		if (html_box_get_bidi_level (self) % 2) {
			if (selection == HTML_BOX_TEXT_SELECTION_START) {
				selection = HTML_BOX_TEXT_SELECTION_END;
				end_index = start_index;
			}
			else if (selection == HTML_BOX_TEXT_SELECTION_END) {
				selection = HTML_BOX_TEXT_SELECTION_START;
				start_index = end_index;
			}
		}

		gdk_gc_set_function (HTML_GDK_PAINTER (painter)->gc, GDK_INVERT);

		switch (selection) {
		case HTML_BOX_TEXT_SELECTION_START:
			pango_glyph_string_index_to_x (text->glyphs,
						       text->canon_text,
						       text->length,
						       &text->item_data->item->analysis,
						       start_index,
						       FALSE,
						       &width);
			
			width /= PANGO_SCALE;
			start_pos = tx + self->x + width;
			width = self->width - width;
			break;

		case HTML_BOX_TEXT_SELECTION_END:
			start_pos = tx + self->x;
			pango_glyph_string_index_to_x (text->glyphs,
						       text->canon_text,
						       text->length,
						       &text->item_data->item->analysis,
						       end_index,
						       FALSE,
						       &width);
			
			width /= PANGO_SCALE;
			break;

		case HTML_BOX_TEXT_SELECTION_BOTH:
			pango_glyph_string_index_to_x (text->glyphs,
						       text->canon_text,
						       text->length,
						       &text->item_data->item->analysis,
						       start_index,
						       FALSE,
						       &width1);
			pango_glyph_string_index_to_x (text->glyphs,
						       text->canon_text,
						       text->length,
						       &text->item_data->item->analysis,
						       end_index,
						       FALSE,
						       &width2);
			
			width1 /= PANGO_SCALE;
			width2 /= PANGO_SCALE;
			start_pos = tx + self->x + MIN (width1, width2);
			width = ABS (width1 - width2);
			break;

		case HTML_BOX_TEXT_SELECTION_FULL:
			start_pos = tx + self->x;
			width = self->width;
			break;
		default:
			g_warning("html_box_text_paint_selection: impossible!");
			gdk_gc_set_function (HTML_GDK_PAINTER (painter)->gc, GDK_COPY);
			return;
		}
			
		html_painter_fill_rectangle (painter, area, start_pos, ty + self->y, width, self->height);

		gdk_gc_set_function (HTML_GDK_PAINTER (painter)->gc, GDK_COPY);
	}
}

static void
html_box_text_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBoxText *text = HTML_BOX_TEXT (self);
	gint ascent;
	
	if (HTML_BOX_GET_STYLE (self->parent)->visibility != HTML_VISIBILITY_VISIBLE)
		return;

	/* If this is the first box in an inline block width a border, then add the border width to the offset */
	if (self->prev == NULL && HTML_IS_BOX_INLINE (self->parent))
	    tx += html_box_left_border_width (self->parent);

	/* Set color */
	html_painter_set_foreground_color (painter, HTML_BOX_GET_STYLE (self)->inherited->color);
	
	ascent = html_box_text_get_ascent (self);
	if (text->glyphs)
		html_painter_draw_glyphs (painter, self->x + tx, self->y + ascent + ty,
					  text->item_data->item->analysis.font, text->glyphs);

	if (HTML_BOX_GET_STYLE (self)->inherited->font_spec->decoration & HTML_FONT_DECORATION_UNDERLINE) {
		/* FIXME: Perhaps make line width depend on font size? */
		gdk_gc_set_line_attributes (HTML_GDK_PAINTER (painter)->gc,
					    1, GDK_LINE_SOLID,
					    GDK_CAP_BUTT,
					    GDK_JOIN_MITER);
		html_painter_draw_line (painter,
					self->x + tx, self->y + ascent + ty + 2,
					self->x + tx + self->width, self->y + ascent + ty + 2);
	}
	if (HTML_BOX_GET_STYLE (self)->inherited->font_spec->decoration & HTML_FONT_DECORATION_OVERLINE) {
		html_painter_draw_line (painter,
					self->x + tx, self->y + ty,
					self->x + tx + self->width, self->y + ty);
	}
	if (HTML_BOX_GET_STYLE (self)->inherited->font_spec->decoration & HTML_FONT_DECORATION_LINETHROUGH) {
		html_painter_draw_line (painter,
					self->x + tx, self->y + ascent/2 + ty,
					self->x + tx + self->width, self->y + ascent/2 + ty);
	} 
	html_box_text_paint_selection (self, painter, area, tx, ty);
}

gint
html_box_text_get_index (HtmlBoxText *text, gint x_pos)
{
	gint index, trailing;

	pango_glyph_string_x_to_index (text->glyphs,
				       text->canon_text,
				       text->length,
				       &text->item_data->item->analysis,
				       x_pos * PANGO_SCALE,
				       &index,
				       &trailing);

	return index;
}

void
html_box_text_get_character_extents (HtmlBoxText *text, gint index, GdkRectangle *rect)
{
	if (rect) {
		gint width1, width2;
		HtmlBox *box;

		box = HTML_BOX (text);
		pango_glyph_string_index_to_x (text->glyphs,
					       text->canon_text,
					       text->length,
					       &text->item_data->item->analysis,
					       index,
					       FALSE,
					       &width1);
		pango_glyph_string_index_to_x (text->glyphs,
					       text->canon_text,
					       text->length,
					       &text->item_data->item->analysis,
					       index + 1,
					       FALSE,
					       &width2);
		width1 /= PANGO_SCALE;
		width2 /= PANGO_SCALE;
		rect->x = box->x + width1;
		rect->width = width2 - width1;
		rect->y = box->y;
		rect->height = box->height;
	}
}

static gint
html_box_text_get_bidi_level (HtmlBox *box)
{
	HtmlBoxText *text = HTML_BOX_TEXT (box);
	gint level;

	/* FIXME: This is only temporary, this should be removed when pango has support for RLO / jb */
	if (HTML_BOX_GET_STYLE (box->parent)->unicode_bidi == HTML_UNICODE_BIDI_OVERRIDE)
		level = HTML_BOX_GET_STYLE (box)->inherited->direction;
	else {
		if (text->item_data && text->item_data->item)
			level = text->item_data->item->analysis.level;
		else
			return 0;
	}

	if ((HTML_BOX_GET_STYLE (box)->inherited->bidi_level % 2) != level)
		return HTML_BOX_GET_STYLE (box)->inherited->bidi_level + 1;
	else
		return HTML_BOX_GET_STYLE (box)->inherited->bidi_level;
}

static gboolean
html_box_text_should_paint (HtmlBox *box, GdkRectangle *area, gint tx, gint ty)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (box);

	gint x = box->x, y = box->y, width = box->width, height = box->height;

	if (HTML_BOX_GET_STYLE (box)->position != HTML_POSITION_STATIC)
		return TRUE;

	if (style->border->top.border_style != HTML_BORDER_STYLE_NONE &&
	    style->border->top.border_style != HTML_BORDER_STYLE_HIDDEN) {
		y -= style->border->top.width;
		height += style->border->top.width;
	}

	if (style->border->bottom.border_style != HTML_BORDER_STYLE_NONE &&
	    style->border->bottom.border_style != HTML_BORDER_STYLE_HIDDEN) {
		height += style->border->bottom.width;
	}

	/* Clipping */
	if (y + ty > area->y + area->height || y + height + ty < area->y ||
	    x + tx > area->x + area->width || x + width + tx < area->x)
		return FALSE;

	return TRUE;
}

static AtkObject*
html_box_text_get_accessible (HtmlBoxText *text)
{
	AtkObject *obj;

	if (html_box_text_get_len (text) == 0)
		return NULL;

	obj = atk_gobject_accessible_for_object (G_OBJECT (text));
	/* Accessibility is not enabled */
	if (ATK_IS_NO_OP_OBJECT (obj))
		return NULL;
	return obj;
}

void 
html_box_text_set_selection (HtmlBoxText *text, HtmlBoxTextSelection mode, gint start_index, gint end_index)
{
	AtkObject *obj;

	if (text->selection == mode &&
	    text->sel_start_index == start_index &&
            text->sel_end_index == end_index)
		return;
 
	text->selection = mode;

	if (start_index >= 0)
		text->sel_start_index = start_index;
	if (end_index >= 0)
		text->sel_end_index = end_index;

	obj = html_box_text_get_accessible (text);
	if (obj)
		g_signal_emit_by_name (obj, "text-selection-changed");
}

static void
html_box_text_class_init (HtmlBoxClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;
	object_class->finalize = html_box_text_finalize;
	
	klass->paint = html_box_text_paint;
	klass->remove = html_box_text_remove;
	klass->relayout = html_box_text_relayout;
	klass->get_ascent = html_box_text_get_ascent;
	klass->get_descent = html_box_text_get_descent;
	klass->get_bidi_level = html_box_text_get_bidi_level;
	klass->should_paint = html_box_text_should_paint;

	parent_class = g_type_class_peek_parent (klass);
}


static void
html_box_text_init (HtmlBox *box)
{
	HtmlBoxText *text = HTML_BOX_TEXT (box);

	text->master = NULL;
	
	text->generated_content = FALSE;
	text->forced_newline = FALSE;
	
	text->canon_text = NULL;
	text->length = 0;
	
	text->item_data = NULL;
	text->item_offset = 0;
	
	text->glyphs = NULL;
	text->ascent = 0;
	text->descent = 0;
}

GType
html_box_text_get_type (void)
{
       static GType html_type = 0;

       if (!html_type) {
               static GTypeInfo type_info = {
		       sizeof (HtmlBoxTextClass),
		       NULL,
		       NULL,
		       (GClassInitFunc) html_box_text_class_init,
		       NULL,
		       NULL,
		       sizeof (HtmlBoxText),
		       16,
                       (GInstanceInitFunc) html_box_text_init
               };

               html_type = g_type_register_static (HTML_TYPE_BOX, "HtmlBoxText", &type_info, 0);
       }
       
       return html_type;
}

HtmlBox *
html_box_text_new (gboolean is_master)
{
	HtmlBoxText *box;
	HtmlBoxTextMaster *master;

	box = g_object_new (HTML_TYPE_BOX_TEXT, NULL);

	if (is_master) {
		master = g_new (HtmlBoxTextMaster, 1);
		box->master = master;

		master->text = NULL;
		master->canon_text = NULL;
		master->n_items = 0;
		master->items = NULL;
		master->must_relayout = TRUE;
		master->preserved_leading_space = TRUE;
		master->font_spec = NULL;
		master->white_space = HTML_WHITE_SPACE_NORMAL;
	}
	return HTML_BOX (box);
}

