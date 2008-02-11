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

#include "htmlbox.h"
#include "htmllinebox.h"
#include "htmlboxinline.h"
#include "htmlboxtext.h"
#include "htmlboxblock.h"
#include "htmlboxroot.h"
#include "htmlrelayout.h"

static GMemChunk *html_line_box_mem_chunk;

static GSList *
reorder_items_recurse (GSList *items, int n_items)
{
	GSList *tmp_list, *level_start_node;
	int i, level_start_i;
	int min_level = G_MAXINT;
	GSList *result = NULL;
	
	if (n_items == 0)
		return NULL;
	
	tmp_list = items;
	for (i=0; i<n_items; i++) {
		min_level = MIN (min_level, html_box_get_bidi_level (HTML_BOX (tmp_list->data)));
		
		tmp_list = tmp_list->next;
	}
	
	level_start_i = 0;
	level_start_node = items;
	tmp_list = items;
	for (i=0; i<n_items; i++) {
		if (html_box_get_bidi_level (HTML_BOX (tmp_list->data)) == min_level) {
			if (min_level % 2) {
				if (i > level_start_i)
					result = g_slist_concat (reorder_items_recurse (level_start_node, i - level_start_i), result);
				result = g_slist_prepend (result, tmp_list->data);
			}
			else {
				if (i > level_start_i)
					result = g_slist_concat (result, reorder_items_recurse (level_start_node, i - level_start_i));
				result = g_slist_append (result, tmp_list->data);
			}
			level_start_i = i + 1;
			level_start_node = tmp_list->next;
		}
		tmp_list = tmp_list->next;
	}
  
	if (min_level % 2) {
		if (i > level_start_i)
			result = g_slist_concat (reorder_items_recurse (level_start_node, i - level_start_i), result);
	}
	else {
		if (i > level_start_i)
			result = g_slist_concat (result, reorder_items_recurse (level_start_node, i - level_start_i));
	}
	return result;
}

static GSList *
html_line_box_reorder_items (GSList *logical_items)
{
	return reorder_items_recurse (logical_items, g_slist_length (logical_items));
}

static void
html_line_box_do_vertical_align (HtmlLineBox *line)
{
	HtmlBox *child;
	GSList *list;
	gint max_descent = 0, max_ascent = 0, min_y = G_MAXINT, baseline;

	g_return_if_fail (line != NULL);

	list = line->item_list;
	while (list) {
		child = (HtmlBox *)list->data;

		min_y = MIN (min_y, child->y);
		max_ascent  = MAX(max_ascent, html_box_get_ascent (child));
		max_descent = MAX(max_descent, html_box_get_descent (child));

		list = list->next;
	}
	if (max_ascent + max_descent > line->height)
		line->height = max_ascent + max_descent;

	baseline = min_y + MAX (max_ascent, line->height / 2);

	list = line->item_list;
	while (list) {
		child = (HtmlBox *)list->data;

		switch (HTML_BOX_GET_STYLE (child->parent)->vertical_align) {
			
		case HTML_VERTICAL_ALIGN_BASELINE:
			child->y = baseline - html_box_get_ascent (child);
			break;
			
		case HTML_VERTICAL_ALIGN_TOP:
			break;
			
		case HTML_VERTICAL_ALIGN_MIDDLE:
			child->y += (line->height - child->height) / 2;
			break;
			
		case HTML_VERTICAL_ALIGN_BOTTOM:
			child->y += line->height - child->height;
			break;
		default:
			g_warning ("unhandled vertical_align");
		}
		if (child->y < 0)
			child->y = 0;

		list = list->next;
	}
	
}

static void
html_line_box_layout_boxes (HtmlLineBox *line, gint offset)
{
	GSList *item = line->item_list;
	gint x = offset;

	while (item) {
		HtmlBox *box = HTML_BOX (item->data);

		box->x = x;

		x += box->width;

		item = item->next;
	}
}

#if 0
static void
debug_print_layout_list (GSList *tmp)
{
	int i = 0;

	while (tmp) {
		g_print ("%d: %p, level = %d\n", ++i, tmp->data, html_box_get_bidi_level (HTML_BOX (tmp->data)));
		tmp = tmp->next;
	}
}
#endif

static gboolean
html_line_box_update_geometry (HtmlLineBox *line, HtmlBox *box, HtmlRelayout *relayout, HtmlBox *parent, 
			       gint y, gint left_margin, gint max_width, gint boxwidth)
{
	line->width += box->width;

	/* Set the lineheight if it is specified */
	line->height = MAX (HTML_BOX_GET_STYLE (box)->inherited->line_height, line->height);

	if (box->height > line->height) {
		gint new_max_width, new_left_margin;
		line->height = box->height;

		new_left_margin = html_relayout_get_left_margin_ignore (relayout, parent, boxwidth, line->height, y, parent);
		new_max_width   = html_relayout_get_max_width_ignore   (relayout, parent, boxwidth, line->height, y, parent);

		/* oops, doesn't we fit on here anymore */
		if (left_margin != new_left_margin || new_max_width != max_width)
			return FALSE;
	}
	return TRUE;
}

static gint get_real_max_width (HtmlBox *parent, gint max_width, gint left_margin, gint boxwidth)
{
	if (max_width == -1)
		max_width = boxwidth;
#if 0
		max_width = parent->width - html_box_horizontal_mbp_sum (parent);
#endif
	max_width -= left_margin;

	return max_width;
}

HtmlLineBoxState
html_line_box_add_inlines (HtmlLineBox *line, HtmlRelayout *relayout, HtmlBox *box, HtmlBox **next_box, HtmlBox *parent, GSList **iterator, 
			   gint y, gint left_margin, gint max_width, GSList **float_list, gint boxwidth)
{
	gint real_max_width;

	real_max_width = get_real_max_width (parent, max_width, left_margin, boxwidth);
	relayout->preserve_leading_space = FALSE;
	relayout->line_is_empty = TRUE;
	relayout->line_offset = 0;

	while (1) {

		/* This label is used by the code below to get to the outer while loop, a continue statement
		 * can't be used because there are other while loops inside */
	ugly_label_1:

		while (box == NULL && *iterator) {
			GSList *tmp;
			if ((*iterator)->data)
				box = HTML_BOX ((*iterator)->data);
			else
				box = NULL;
			tmp = *iterator;
			*iterator = (*iterator)->next;
			g_slist_free_1 (tmp);
			
		}
		
		if (box == NULL)
			break;

		if (HTML_BOX_GET_STYLE (box)->position == HTML_POSITION_RELATIVE && !HTML_IS_BOX_TEXT (box))
			html_box_root_add_positioned (HTML_BOX_ROOT (relayout->root), box);

		/* If the box is a lineline block then we will continue inside it, 
		 *  but first save our path so that we can countinue when we return / jb
		*/
		while (HTML_IS_BOX_INLINE (box)) {

			HtmlBox *inline_block;
			/* inline_block is an laout node contining other inline boxes, and the width and height
			   of this block should be updated to the minimal box covering all the child boxes */
			inline_block = box;
			inline_block->width = inline_block->height = 0;

			*iterator = g_slist_prepend (*iterator, box->next);
			/* Now continue with the children */
			box = box->children;

			/* Add the pseudo :before and :after elements to the stack */
			if (html_box_get_after (inline_block))
				*iterator = g_slist_prepend (*iterator, html_box_get_after (inline_block));

			if (html_box_get_before (inline_block)) {
				*iterator = g_slist_prepend (*iterator, box);
				box = html_box_get_before (inline_block);
			}
			/* We need to get to the top of the outer while loop, we can't use continue
			 * because it will only get us to the inner while loop. goto might be the only
			 * solution */
			goto ugly_label_1;
		}		

		/* Ignore these */
		if (HTML_BOX_GET_STYLE (box)->display == HTML_DISPLAY_NONE) {

			box = box->next;
			continue;
		}

		/* Process these later */
		if (HTML_BOX_GET_STYLE (box)->Float != HTML_FLOAT_NONE && !HTML_IS_BOX_TEXT (box)) {
			*float_list = g_slist_append (*float_list, box);
			box = box->next;
			continue;
		}

		/*  Only display: inline boxes should be on the line  */
		if (HTML_BOX_GET_STYLE (box)->display != HTML_DISPLAY_INLINE && 
		    HTML_BOX_GET_STYLE (box)->display != HTML_DISPLAY_INLINE_TABLE && 
		    !HTML_IS_BOX_TEXT (box)) 
			break;
		
		/* We don't have to set box->x it is overridden in html_line_box_layout_line_ltr|trl */
		box->x = left_margin + line->width;
		box->y = y;

		if (relayout->get_max_width)
			relayout->max_width = G_MAXINT;
		else
			relayout->max_width = real_max_width - line->width;

		html_box_relayout (box, relayout);

		/* If this is a PRE or NOWRAP  line, then do a linebreak if the text wants to  */
		if (HTML_IS_BOX_TEXT (box)) {

			/* We have to keep track of the character count for the current line
			 * to be able to calculate the correct tab positions /jb */
			relayout->line_offset += relayout->text_item_length;

			if (HTML_BOX_TEXT (box)->forced_newline) {
				
				/* Does the line still fit? */
				if (html_line_box_update_geometry (line, box, relayout, 
								   parent, y, left_margin, 
								   max_width, boxwidth) == FALSE)
					return HTML_LINE_BOX_DOES_NOT_FIT;
				
				line->item_list = g_slist_append (line->item_list, box);		
				box = box->next;
				break;
			}
		}

		/* Did this box fit on the line, and is it "normal" text (not PRE or nowrap) ? */
		if (real_max_width - line->width < box->width && 
		    HTML_BOX_GET_STYLE (box)->inherited->white_space == HTML_WHITE_SPACE_NORMAL) {

			/* If this isn't the first box and there aren't any floating boxes arround, then break */
			if ((line->width > 0 || (!(left_margin == 0 && max_width == -1))) && 
			    relayout->get_max_width == FALSE) {
				if (line->width == 0) {
					*next_box = box;
					line->width  = box->width;
					line->height = box->height;
					return HTML_LINE_BOX_DOES_NOT_FIT;
				}
				break;
			}
		}

		/* Does the line still fit? */
		if (html_line_box_update_geometry (line, box, relayout, parent, y, left_margin, max_width, boxwidth) == FALSE)
			return HTML_LINE_BOX_DOES_NOT_FIT;
		
		line->item_list = g_slist_append (line->item_list, box);
		relayout->line_is_empty = FALSE;

		/* We check if box is the last textbox slave and if so call html_box_text_free_relayout () 
		   Or else we won't free all memory */
		if (HTML_IS_BOX_TEXT(box) && 
		    (box->next == NULL || !HTML_IS_BOX_TEXT(box->next) || html_box_text_is_master(HTML_BOX_TEXT(box->next))))
			html_box_text_free_relayout (HTML_BOX_TEXT (box));
		
		box = box->next;
	}
	*next_box = box;

	return (*next_box == NULL) ? HTML_LINE_BOX_NOT_FULL : HTML_LINE_BOX_FULL;
}

void
html_line_box_init (HtmlLineBox *line)
{
	line->width = 0;

	if (line->item_list) {

		g_slist_free (line->item_list);
		line->item_list = NULL;
	}
}

void
html_line_box_close (HtmlLineBox *line, HtmlBox *parent, gint left_margin, gint max_width, gint boxwidth)
{
	GSList *tmp;
	gint real_max_width = get_real_max_width (parent, max_width, left_margin, boxwidth);
	

	if (line->type != HTML_INLINE_LINE_BOX_TYPE)
		return;

	tmp = html_line_box_reorder_items (line->item_list);
	g_slist_free (line->item_list);
	line->item_list = tmp;

	switch (HTML_BOX_GET_STYLE (parent)->inherited->text_align) {
	case HTML_TEXT_ALIGN_LEFT:
		html_line_box_layout_boxes (line, left_margin);
		break;
	case HTML_TEXT_ALIGN_RIGHT:
		html_line_box_layout_boxes (line, left_margin + real_max_width - line->width);
		break;
	case HTML_TEXT_ALIGN_CENTER:
		html_line_box_layout_boxes (line, left_margin + ((real_max_width - line->width) / 2));
		break;
	case HTML_TEXT_ALIGN_DEFAULT:
		if (HTML_BOX_GET_STYLE (parent)->inherited->direction == HTML_DIRECTION_LTR)
			html_line_box_layout_boxes (line, left_margin);
		else
			html_line_box_layout_boxes (line, left_margin + real_max_width - line->width);
	default:
		break;
	}
	html_line_box_do_vertical_align (line);
}

void
html_line_box_add_block (HtmlLineBox *line, HtmlRelayout *relayout, HtmlBox *box, 
			 gint y, gboolean force_relayout, gint *old_margin, gint boxwidth)
{
	HtmlBox *cbox = html_box_get_containing_block (box);
	gint width = html_box_get_containing_block_width (box);
	/* Do margin collapsing */
	gint margin_top = html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->margin.top, width);
	gint margin_bottom = html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->margin.bottom, width);
	gint margin_collapse = margin_top + *old_margin - MAX(margin_top, *old_margin);;
	*old_margin = margin_bottom;

	/* If some floats has moved then we need to relayout it just in case */
	if (HTML_IS_BOX_BLOCK (box) && 
	    HTML_BOX_BLOCK (box)->float_magic_value != html_box_block_calculate_float_magic (box, relayout))
		force_relayout = TRUE;

	if (box->is_relayouted == FALSE || force_relayout) {

		if (HTML_IS_BOX_BLOCK (box))
			box->x = 0;
		else {
			box->height = MAX (box->height, 1);
			box->x = html_relayout_get_left_margin_ignore (relayout, cbox, 
								       boxwidth, box->height, y, box);
		}
		
		box->y = y;
		box->y -= margin_collapse;

		html_box_root_mark_floats_unrelayouted (HTML_BOX_ROOT (relayout->root), box);
		html_box_relayout (box, relayout);
	}
	else {

		box->y = y;
		box->y -= margin_collapse;
		html_box_root_mark_floats_relayouted (HTML_BOX_ROOT (relayout->root), box);
	}
	switch (HTML_BOX_GET_STYLE (box->parent)->inherited->text_align) {
	case HTML_TEXT_ALIGN_LEFT:
		break;
	case HTML_TEXT_ALIGN_RIGHT:
		box->x = MAX(0, boxwidth - box->width);
		break;
	case HTML_TEXT_ALIGN_CENTER:
		box->x = MAX(0, (boxwidth - box->width) / 2);
		break;
	default:
		break;
	}

	line->width = box->x + box->width;
	if (HTML_IS_BOX_BLOCK (box))
		line->full_width = HTML_BOX_BLOCK (box)->full_width;
	else
		line->full_width = line->width;

	line->height = box->height - margin_collapse;

	line->item_list = g_slist_append (line->item_list, box);
}

HtmlLineBox *
html_line_box_new (HtmlLineBoxType type)
{
	HtmlLineBox *line;

	if (html_line_box_mem_chunk == NULL) {

		html_line_box_mem_chunk = g_mem_chunk_new ("html_line_box", sizeof (HtmlLineBox), sizeof (HtmlLineBox) * 1000, G_ALLOC_AND_FREE);
		g_return_val_if_fail (html_line_box_mem_chunk, NULL);
	}
	line = g_mem_chunk_alloc0 (html_line_box_mem_chunk);

	line->type = type;

	return line;
}

void
html_line_box_destroy (HtmlLineBox *line)
{
	g_slist_free (line->item_list);

	g_mem_chunk_free (html_line_box_mem_chunk, line);
}


