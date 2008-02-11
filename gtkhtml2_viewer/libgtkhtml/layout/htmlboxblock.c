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

#include "htmlboxblock.h"
#include "htmlboxtext.h"
#include "htmlrelayout.h"
#include "htmllinebox.h"
#include "htmlboxroot.h"

static HtmlBoxClass *parent_class = NULL;

static void
free_lineboxes (HtmlBoxBlock *block) 
{
	HtmlLineBox *line = block->line;

	while (line) {
		HtmlLineBox *tmp = line;
		line = line->next;
		html_line_box_destroy (tmp);
	}
	
	block->line = NULL;
}

static void
html_box_block_handle_positioned (HtmlBox *self, HtmlRelayout *relayout, HtmlBox *box)
{
	/* Temporary move the box away, so it doesn't collide with some floating box */
	box->x = 0;
	box->y = 100000;
	html_box_relayout (box, relayout);
	box->y = 0;
	html_box_root_add_positioned (HTML_BOX_ROOT (relayout->root), box);
}

static void
html_box_block_handle_float (HtmlBox *self, HtmlRelayout *relayout, HtmlBox *box, gint y, gint *boxwidth)
{
	int width;

	g_return_if_fail (self != NULL);
	g_return_if_fail (box  != NULL);

	/* Temporary move the box away, so it doesn't collide with other floating objects */
	box->x = 0;
	box->y = 100000;

	html_box_relayout (box, relayout);

	box->y = y;

	switch (HTML_BOX_GET_STYLE (box)->Float) {
	case HTML_FLOAT_LEFT:
	case HTML_FLOAT_CENTER:
		box->x = html_relayout_get_left_margin_ignore (relayout, self, *boxwidth, box->height, y, box);
		html_relayout_make_fit_left (self, relayout, box, *boxwidth, y);
		break;
	case HTML_FLOAT_RIGHT:
		width = html_relayout_get_max_width_ignore (relayout, self, *boxwidth, box->height, y, box);
		if (width == -1)
			width = self->width - html_box_horizontal_mbp_sum (self);
	 	box->x = MAX(0, width - box->width);
		html_relayout_make_fit_right (self, relayout, box, *boxwidth, y);
		break;
	default:
		g_warning("html_box_block_handle_float: impossible!\n");
		return;
	}
	html_box_root_add_float (HTML_BOX_ROOT (relayout->root), box);
}

static void
html_box_block_update_geometry (HtmlBox *self, HtmlRelayout *relayout, HtmlLineBox *line, gint *y, gint *boxwidth, gint *boxheight) 
{
	HTML_BOX_BLOCK_GET_CLASS(self)->update_geometry (self, relayout, line, y, boxwidth, boxheight);
}

static void
html_real_box_block_update_geometry (HtmlBox *self, HtmlRelayout *relayout, HtmlLineBox *line, gint *y, gint *boxwidth, gint *boxheight) 
{
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);
	gint full_width;

#if 0
	if (relayout->get_min_width || relayout->get_max_width) {
#endif

		/* Only expand the width of the block box if the width is of type "auto" */
		if (HTML_BOX_GET_STYLE (self)->box->width.type == HTML_LENGTH_AUTO && line->width > *boxwidth) {

			*boxwidth   = line->width;
			block->containing_width = line->width;
			self->width = *boxwidth + html_box_horizontal_mbp_sum (self);
			block->force_relayout = TRUE;
		}
#if 0
	}
#endif
	full_width = MAX (line->width, line->full_width);

	if (full_width > block->full_width)
		block->full_width = full_width + html_box_left_mbp_sum (self, -1);

	*y += line->height;
	/* Only expand the height of the block box if the height is of type "auto" */
	if (HTML_BOX_GET_STYLE (self)->box->height.type == HTML_LENGTH_AUTO && *y > *boxheight) {
		*boxheight = *y;
		self->height = *boxheight + html_box_vertical_mbp_sum (self);
	}
}

static HtmlLineBoxType
html_box_block_get_line_type (HtmlBox *box)
{
	if (HTML_IS_BOX_TEXT(box))
		return HTML_INLINE_LINE_BOX_TYPE;

	switch (HTML_BOX_GET_STYLE (box)->display) {
	case HTML_DISPLAY_BLOCK:
	case HTML_DISPLAY_TABLE:
	case HTML_DISPLAY_TABLE_CELL:
	case HTML_DISPLAY_TABLE_ROW:
	case HTML_DISPLAY_LIST_ITEM:
		return HTML_BLOCK_LINE_BOX_TYPE;
	default:
		return HTML_INLINE_LINE_BOX_TYPE;
	}
}

static HtmlLineBox *
html_box_block_create_inline_lines (HtmlBox *self, HtmlRelayout *relayout, HtmlBox **box, GSList **iterator, 
				    gint *boxwidth, gint *boxheight, gint *y)
{	
	gint left_margin, max_width;
	HtmlBox *next_box;
	HtmlLineBox *line;
	HtmlLineBoxState state;
	GSList *old_iterator, *float_list = NULL, *tmp;

	line = html_line_box_new (HTML_INLINE_LINE_BOX_TYPE);

	/* We can't have 0 height, give it a temporary height */
	line->height = 1;
	/* Loop here until we have found some place where the line fits */

	do {
		html_line_box_init (line);
		old_iterator = g_slist_copy (*iterator);
		
		left_margin = html_relayout_get_left_margin_ignore (relayout, self, *boxwidth, line->height, *y, self);
		max_width   = html_relayout_get_max_width_ignore   (relayout, self, *boxwidth, line->height, *y, self);

		/* Fill it with boxes */
		state = html_line_box_add_inlines (line, relayout, *box, &next_box, self, iterator, *y, left_margin, max_width, &float_list, *boxwidth);
		if (state == HTML_LINE_BOX_DOES_NOT_FIT) {
			gint new_y;

			new_y = html_relayout_next_float_offset (relayout, self, *y, *boxwidth, line->height);
			if (new_y != -1)
				*y = new_y;
			
			g_slist_free (*iterator);
			*iterator = old_iterator;
		}
	}
	while (state == HTML_LINE_BOX_DOES_NOT_FIT);
	
	g_slist_free (old_iterator);
	html_line_box_close (line, self, left_margin, max_width, *boxwidth);
	/*
	 * Bandaid for bug 98577.
	 */
	*box = *box == next_box ? NULL : next_box;

	/* If the line is empty, then it has 0 height */
	if (line->item_list == NULL)
		line->height = 0;

	html_box_block_update_geometry (self, relayout, line, y, boxwidth, boxheight);
	
	/* Now add all the floats and positioned boxes found on this line */
	tmp = float_list;
	while (tmp) {
		html_box_block_handle_float (self, relayout, HTML_BOX (tmp->data), *y, boxwidth);
		tmp = tmp->next;
	}
	g_slist_free (float_list);
	float_list = NULL;

	return line;
}

static void
do_clear (HtmlBox *self, HtmlRelayout *relayout, HtmlBox *box, gint boxwidth, gint *y)
{
	switch (HTML_BOX_GET_STYLE (box)->clear) {
	case HTML_CLEAR_LEFT:
		while (html_relayout_get_left_margin_ignore (relayout, self, boxwidth, 1, *y, self) != 0)
			*y = html_relayout_next_float_offset (relayout, self, *y, boxwidth, 1);
		break;
	case HTML_CLEAR_RIGHT:
		while (html_relayout_get_max_width_ignore (relayout, self, boxwidth, 1, *y, self) != -1)
			*y = html_relayout_next_float_offset (relayout, self, *y, boxwidth, 1);
		break;
	case HTML_CLEAR_BOTH:
		while ((html_relayout_get_left_margin_ignore (relayout, self, boxwidth, 1, *y, self) != 0) ||
		       (html_relayout_get_max_width_ignore (relayout, self, boxwidth, 1, *y, self) != -1))
			*y = html_relayout_next_float_offset (relayout, self, *y, boxwidth, 1);
		break;
	case HTML_CLEAR_NONE:
		break;
	}
}

static HtmlLineBox *
html_box_block_create_block_line (HtmlBox *self, HtmlRelayout *relayout, HtmlBox *box, gint *boxwidth, gint *boxheight, gint *y, gint *old_margin)
{
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);
	HtmlLineBox *line;

	/* If it is a floatbox, just add it */
	if (HTML_BOX_GET_STYLE (box)->Float != HTML_FLOAT_NONE) {
		html_box_block_handle_float (self, relayout, box, *y, boxwidth);
		return NULL;
	}
	/* If it is positioned, just add it */
	if (HTML_BOX_GET_STYLE (box)->position == HTML_POSITION_ABSOLUTE ||
	    HTML_BOX_GET_STYLE (box)->position == HTML_POSITION_FIXED) {
		html_box_block_handle_positioned (self, relayout, box);
		return NULL;
	}
	if (HTML_BOX_GET_STYLE (box)->clear != HTML_CLEAR_NONE) {
		*old_margin = 0;
		do_clear (self, relayout, box, *boxwidth, y);
	}

	line = html_line_box_new (HTML_BLOCK_LINE_BOX_TYPE);
	html_line_box_add_block (line, relayout, box, *y, block->force_relayout, old_margin, *boxwidth);
	html_box_block_update_geometry (self, relayout, line, y, boxwidth, boxheight);

	return line;
}

static void
html_box_block_add_line (HtmlBoxBlock *block, HtmlLineBox *line)
{
	HtmlLineBox *tmp = block->line;

	if (block->line == NULL) {
		block->line = line;
		return;
	}
	while (tmp->next)
		tmp = tmp->next;
	tmp->next = line;
}

/**
 * html_box_block_update_lineboxes:
 * @self: A block box
 * @relayout: the relayout context
 * @boxwidth: the current width of this block
 * @boxheight: the current height of this block
 * 
 * This function does a full relayout of this block, all lineboxes are created from scratch.
 **/
static void
html_box_block_create_lines (HtmlBox *self, HtmlRelayout *relayout, gint *boxwidth, gint *boxheight)
{
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);
	HtmlBox *box = self->children;
	GSList *iterator = NULL;
	gint old_margin = 0;   /* This is the previous margin, used to do margin collapsing */
	gint y = 0;

	free_lineboxes (block);

	/* Add the :before and :after pseudo elements to the queue if any */
	if (html_box_get_after (self))
		iterator = g_slist_prepend (iterator, html_box_get_after (self));

	if (html_box_get_before (self)) {
		iterator = g_slist_prepend (iterator, box);
		box = html_box_get_before (self);
	}

	while (box || iterator) {
		HtmlLineBox *line = NULL;

		if (box == NULL) {
			line = html_box_block_create_inline_lines (self, relayout, &box, &iterator, boxwidth, boxheight, &y);
			old_margin = 0;
		}
		else {
			switch (html_box_block_get_line_type (box)) {
			case HTML_BLOCK_LINE_BOX_TYPE:
				line = html_box_block_create_block_line (self, relayout, box, boxwidth, boxheight, &y, &old_margin);
				box = box->next;
				break;
			case HTML_INLINE_LINE_BOX_TYPE:
				line = html_box_block_create_inline_lines (self, relayout, &box, &iterator, boxwidth, boxheight, &y);
				old_margin = 0;
				break;
			default:
				g_warning("html_box_block_create_lines: impossible\n");
				return;
			};
		}
		if (line)
			html_box_block_add_line (block, line);
	}
}

static void
html_box_block_real_get_boundaries (HtmlBox *self, HtmlRelayout *relayout, gint *boxwidth, gint *boxheight)
{
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);
	gint new_width, new_height;

	if (block->last_was_min_max) {

		block->force_relayout = TRUE;
		block->last_was_min_max = FALSE;
	}

	if (relayout->get_min_width || relayout->get_max_width) {
		
		block->last_was_min_max = TRUE;
		block->force_relayout = TRUE;
		*boxwidth = html_length_get_value (&style->box->width, html_box_get_containing_block_width (self));
		*boxheight = html_length_get_value (&style->box->height, html_box_get_containing_block_width (self));
		block->containing_width = *boxwidth;
		block->full_width = *boxwidth;
		self->width = *boxwidth + html_box_horizontal_mbp_sum (self);
		self->height = *boxheight + html_box_vertical_mbp_sum (self);
		return;
	}

	/* Get the prefered width */
	/* If the width wasn't specified by CSS, use the width of the containing box (parent) */
	if (html_box_get_containing_block (self)) {

		if (style->Float != HTML_FLOAT_NONE)
			new_width = html_length_get_value (&style->box->width, html_box_get_containing_block_width (self));
		else if (style->position != HTML_POSITION_STATIC && style->box->width.type != HTML_LENGTH_AUTO)
				new_width = html_length_get_value (&style->box->width, html_box_get_containing_block_width (self));
		else
			new_width = html_box_get_containing_block_width (self) -  html_box_horizontal_mbp_sum (self);

			new_height = html_length_get_value (&style->box->height, html_box_get_containing_block_height (self));
	}
	else {
		new_width  = html_length_get_value (&style->box->width, 0);
		new_height = html_length_get_value (&style->box->height, 0);
	}
	html_box_check_min_max_width_height (self, &new_width, &new_height);

	if (*boxwidth < 0)
		*boxwidth = 0;
	if (*boxheight < 0)
		*boxheight = 0;

	if (new_width != *boxwidth) {
		*boxwidth = new_width;
		block->force_relayout = TRUE;
	}

	if (new_height != *boxheight)
		*boxheight = new_height;

	block->containing_width = *boxwidth;

	self->width = *boxwidth + html_box_horizontal_mbp_sum (self);
	self->height = *boxheight + html_box_vertical_mbp_sum (self);

	block->full_width = *boxwidth;

	html_box_check_min_max_width_height (self, boxwidth, boxheight);
}

static void
html_box_block_get_boundaries (HtmlBox *self, HtmlRelayout *relayout, gint *boxwidth, gint *boxheight)
{
	HTML_BOX_BLOCK_GET_CLASS(self)->get_boundaries (self, relayout, boxwidth, boxheight);
}

static void
html_box_block_restore_geometry (HtmlBox *self, gint *boxwidth, gint *boxheight)
{
	*boxwidth  = self->width  - html_box_horizontal_mbp_sum (self);
#if 0
	*boxheight = self->height - html_box_vertical_mbp_sum   (self);
#endif
}

static gint
calculate_float_magic_helper (GSList *list, HtmlBox *self, gint boxx, gint boxy)
{
	gint magic = 0;

	while (list) {
		HtmlBox *Float = (HtmlBox *) list->data;
		gint Floatx = html_box_get_absolute_x (Float);
		gint Floaty = html_box_get_absolute_y (Float);

		if (Float->is_relayouted && Floaty < boxy + self->height && Floaty + Float->height > boxy &&
		    Floatx < boxx + self->width  && Floatx + Float->width  > boxx)
			magic += Floatx - boxx + Float->width + Floaty - boxy + Float->height;

		list = list->next;
	}
	return magic;
}

/**
 * html_box_block_calculate_float_magic:
 * @self: a layout box
 * @relayout: a relayout context
 * 
 * This function calculates a "magic" number which can be used to check if any floating
 * boxes (that intersects with this box) has moved or been added.
 * 
 * Return value: 
 **/
gint
html_box_block_calculate_float_magic (HtmlBox *self, HtmlRelayout *relayout)
{
	gint boxx, boxy;

	boxx = html_box_get_absolute_x (self);
	boxy = html_box_get_absolute_y (self);

	return calculate_float_magic_helper (html_box_root_get_float_left_list  (HTML_BOX_ROOT (relayout->root)), self, boxx, boxy) + 
		calculate_float_magic_helper (html_box_root_get_float_right_list (HTML_BOX_ROOT (relayout->root)), self, boxx, boxy); 
}

static void
html_box_block_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);
	gint boxheight = 0, boxwidth = 0;

	block->full_width = 0;

#if 1
	block->force_relayout = FALSE;

	if (relayout->type == HTML_RELAYOUT_INCREMENTAL)
		html_box_block_restore_geometry (self, &boxwidth, &boxheight);
#else
	block->force_relayout = TRUE;
#endif
	html_box_block_get_boundaries (self, relayout, &boxwidth, &boxheight);
	html_box_block_create_lines (self, relayout, &boxwidth, &boxheight);
	block->float_magic_value = html_box_block_calculate_float_magic (self, relayout);
}

static void
html_box_block_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBox *box;

	tx += html_box_left_mbp_sum (self, -1);
	ty += html_box_top_mbp_sum (self, -1);

	box = html_box_get_before (self);
	while (box) {
		html_box_paint (box, painter, area, self->x + tx, self->y + ty);
		box = box->next;
	}

	box = html_box_get_after (self);
	while (box) {
		html_box_paint (box, painter, area, self->x + tx, self->y + ty);
		box = box->next;
	}

	box = self->children;
	while (box) {
		HtmlStyle *style = HTML_BOX_GET_STYLE (box);
		/*
		 * Don't paint floating, relative or absolute boxes,
		 * they get painted in html_box_root_paint ()
		 */
		if ((style->position != HTML_POSITION_FIXED &&
		     style->position != HTML_POSITION_ABSOLUTE &&
		     style->Float == HTML_FLOAT_NONE) || HTML_IS_BOX_TEXT (box))
			html_box_paint (box, painter, area, self->x + tx, self->y + ty);

		box = box->next;
	}

}

static void
html_box_block_finalize (GObject *object)
{

	free_lineboxes (HTML_BOX_BLOCK (object));

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
html_box_block_handles_events (HtmlBox *self)
{
	/* Normal inline boxes don't handle events */
	return FALSE;
}

static gboolean
html_box_block_should_paint (HtmlBox *box, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBoxBlock *block = HTML_BOX_BLOCK (box);
	gint width = MAX (box->width, block->full_width);

	if (HTML_BOX_GET_STYLE (box)->position != HTML_POSITION_STATIC)
		return TRUE;

	/* Clipping */
	if (box->y + ty > area->y + area->height || box->y + box->height + ty < area->y ||
	    box->x + tx > area->x + area->width || box->x + width + tx < area->x)
		return FALSE;

	return TRUE;
}

static void
html_box_block_class_init (HtmlBoxBlockClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	HtmlBoxClass *box_class = (HtmlBoxClass *) klass;
	
	object_class->finalize = html_box_block_finalize;

	box_class->relayout = html_box_block_relayout;
	box_class->paint = html_box_block_paint;
	box_class->handles_events = html_box_block_handles_events;
	box_class->should_paint = html_box_block_should_paint;
	klass->get_boundaries = html_box_block_real_get_boundaries;
	klass->update_geometry = html_real_box_block_update_geometry;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_block_init (HtmlBox *box)
{
}

GType
html_box_block_get_type (void)
{
       static GType html_type = 0;

       if (!html_type) {
               static GTypeInfo type_info = {
		       sizeof (HtmlBoxBlockClass),
		       NULL,
		       NULL,
                       (GClassInitFunc) html_box_block_class_init,
		       NULL,
		       NULL,
                       sizeof (HtmlBoxBlock),
		       16,
                       (GInstanceInitFunc) html_box_block_init
               };

               html_type = g_type_register_static (HTML_TYPE_BOX, "HtmlBoxBlock", &type_info, 0);
       }
       
       return html_type;
}

HtmlBox *
html_box_block_new (void)
{
	return g_object_new (HTML_TYPE_BOX_BLOCK, NULL);
}
