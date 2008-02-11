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

#include "graphics/htmlstylepainter.h"
#include "layout/htmlbox.h"
#include "layout/htmlboxtext.h"
#include "layout/html/htmlboxform.h"
#include "layout/html/htmlboximage.h"
#include "layout/htmlboxinline.h"
#include "layout/htmlboxtablerow.h"
#include "gtkhtmlcontext.h"

static GObjectClass *parent_class = NULL;

HtmlBox *
html_box_get_containing_block (HtmlBox *box)
{
	box = box->parent;

	while (box) {
		/* This is three times less expensive then HTML_IS_BOX_BLOCK  */
		HtmlStyle *style = HTML_BOX_GET_STYLE(box);
		if (style->display == HTML_DISPLAY_BLOCK ||
		    style->display == HTML_DISPLAY_LIST_ITEM ||
		    style->display == HTML_DISPLAY_TABLE_CELL)
			return box;

		box = box->parent;
	}

	return NULL;
}

HtmlBox *
html_box_get_before (HtmlBox *box)
{
	static GQuark before_quark = 0;

	if (before_quark == 0)
		before_quark = g_quark_from_static_string ("HtmlBox->before");

	return (HtmlBox *)g_object_get_qdata (G_OBJECT (box), before_quark);
}


HtmlBox *
html_box_get_after (HtmlBox *box)
{
	static GQuark after_quark = 0;

	if (after_quark == 0)
		after_quark = g_quark_from_static_string ("HtmlBox->after");

	return (HtmlBox *)g_object_get_qdata (G_OBJECT (box), after_quark);
}

void
html_box_set_before (HtmlBox *box, HtmlBox *before)
{
	static GQuark before_quark = 0;
	
	if (before_quark == 0)
		before_quark = g_quark_from_static_string ("HtmlBox->before");
	
	g_object_set_qdata (G_OBJECT (box), before_quark, before);
}


void
html_box_set_after (HtmlBox *box, HtmlBox *after)
{
	static GQuark after_quark = 0;

	if (after_quark == 0)
		after_quark = g_quark_from_static_string ("HtmlBox->after");

	g_object_set_qdata (G_OBJECT (box), after_quark, after);
}

gint
html_box_get_containing_block_height (HtmlBox *box)
{
	box = html_box_get_containing_block (box);

	if (box)
		return box->height - html_box_vertical_mbp_sum (box);
	else 
		return 0;
}

gint
html_box_get_containing_block_width (HtmlBox *box)
{
	HtmlBoxBlock *block = NULL;

	box = html_box_get_containing_block (box);
	
	if (box) {

		block = HTML_BOX_BLOCK (box);
		return block->containing_width;
	}
	else 
		return 0;
}

/**
 * simple_margin:
 * @style: 
 * 
 * Some boxes should have the faster, simpler
 * margin calculation algorithm
 * 
 * Return value: 
 **/
static gboolean
simple_margin (HtmlStyle *style)
{
	if (style->box->width.type == HTML_LENGTH_AUTO || 
	    style->Float != HTML_FLOAT_NONE ||
	    style->position != HTML_POSITION_STATIC ||
	    style->display == HTML_DISPLAY_INLINE ||
	    style->display == HTML_DISPLAY_TABLE_CELL ||
	    style->display == HTML_DISPLAY_TABLE ||
	    style->display == HTML_DISPLAY_INLINE_TABLE ||
	    style->display == HTML_DISPLAY_TABLE_CAPTION)
		return TRUE;
	else 
		return FALSE;
}

static gboolean
need_containing_width (HtmlBox *box, gint width)
{
	if (width > 0)
		return FALSE;

	if (html_box_get_containing_block (box))
		return TRUE;

	return FALSE;
}

gint
html_box_horizontal_mbp_sum (HtmlBox *box)
{
	gint width = 0;

	g_return_val_if_fail (box != NULL, 0);

	if (need_containing_width (box, -1))
		width = html_box_get_containing_block_width (box);

	return html_box_left_mbp_sum (box, width) + html_box_right_mbp_sum (box, width);
}

gint
html_box_vertical_mbp_sum (HtmlBox *box)
{
	gint width = 0;

	g_return_val_if_fail (box != NULL, 0);

	if (need_containing_width (box, -1))
		width = html_box_get_containing_block_width (box);

	return html_box_top_mbp_sum (box, width) + html_box_bottom_mbp_sum (box, width);
}

gint
html_box_left_mbp_sum (HtmlBox *box, gint width)
{
	return HTML_BOX_GET_CLASS(box)->left_mbp_sum (box, width);
}

gint
html_box_right_mbp_sum (HtmlBox *box, gint width)
{
	return HTML_BOX_GET_CLASS(box)->right_mbp_sum (box, width);
}

gint
html_box_top_mbp_sum (HtmlBox *box, gint width)
{
	return HTML_BOX_GET_CLASS(box)->top_mbp_sum (box, width);
}

gint
html_box_bottom_mbp_sum (HtmlBox *box, gint width)
{
	return HTML_BOX_GET_CLASS(box)->bottom_mbp_sum (box, width);
}

gint
html_box_left_padding (HtmlBox *box, gint width)
{
	return html_length_get_value (&HTML_BOX_GET_STYLE(box)->surround->padding.left, width);
}

gint
html_box_right_padding (HtmlBox *box, gint width)
{
	return html_length_get_value (&HTML_BOX_GET_STYLE(box)->surround->padding.right, width);
}

gint
html_box_top_padding (HtmlBox *box, gint width)
{
	return html_length_get_value (&HTML_BOX_GET_STYLE(box)->surround->padding.top, width);
}

gint
html_box_bottom_padding (HtmlBox *box, gint width)
{
	return html_length_get_value (&HTML_BOX_GET_STYLE(box)->surround->padding.bottom, width);
}

gint
html_box_left_border_width (HtmlBox *box)
{
	if (HTML_BOX_GET_STYLE(box)->border->left.border_style != HTML_BORDER_STYLE_NONE &&
	    HTML_BOX_GET_STYLE(box)->border->left.border_style != HTML_BORDER_STYLE_HIDDEN)
		return HTML_BOX_GET_STYLE(box)->border->left.width;
	else
		return 0;
}

gint
html_box_right_border_width (HtmlBox *box)
{
	if (HTML_BOX_GET_STYLE(box)->border->right.border_style != HTML_BORDER_STYLE_NONE &&
	    HTML_BOX_GET_STYLE(box)->border->right.border_style != HTML_BORDER_STYLE_HIDDEN)
		return HTML_BOX_GET_STYLE(box)->border->right.width;
	else
		return 0;
}

gint
html_box_top_border_width (HtmlBox *box)
{
	if (HTML_BOX_GET_STYLE(box)->border->top.border_style != HTML_BORDER_STYLE_NONE &&
	    HTML_BOX_GET_STYLE(box)->border->top.border_style != HTML_BORDER_STYLE_HIDDEN)
		return HTML_BOX_GET_STYLE(box)->border->top.width;
	else
		return 0;
}

gint
html_box_bottom_border_width (HtmlBox *box)
{
	if (HTML_BOX_GET_STYLE(box)->border->bottom.border_style != HTML_BORDER_STYLE_NONE &&
	    HTML_BOX_GET_STYLE(box)->border->bottom.border_style != HTML_BORDER_STYLE_HIDDEN)
		return HTML_BOX_GET_STYLE(box)->border->bottom.width;
	else
		return 0;
}

/* From section "10.6 Computing heighs and margins" in the css2 specification */
gint
html_box_top_margin (HtmlBox *box, gint width)
{
	return html_length_get_value (&HTML_BOX_GET_STYLE(box)->surround->margin.top, width);
}

/* From section "10.6 Computing heights and margins" in the css2 specification */
gint
html_box_bottom_margin (HtmlBox *box, gint width)
{
	return html_length_get_value (&HTML_BOX_GET_STYLE(box)->surround->margin.bottom, width);
}

/* From section "10.3 Computing widths and margins" in the css2 specification */
gint
html_box_left_margin (HtmlBox *box, gint width)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE(box);

	if (simple_margin (style))
		return html_length_get_value (&style->surround->margin.left, width);

	/* If both left and right margin is specified, then we might have to recalculate one of the
	 * values to satisfy the expression: "'left' + 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 
	 'padding-right' + 'border-right-width' + 'margin-right' + 'right' = width of containing block"
	 */
	else if (style->surround->margin.left.type != HTML_LENGTH_AUTO &&
		 style->surround->margin.right.type != HTML_LENGTH_AUTO) {

		if (html_box_get_containing_block (box) && 
		    HTML_BOX_GET_STYLE (html_box_get_containing_block (box)) &&
		    HTML_BOX_GET_STYLE (html_box_get_containing_block (box))->inherited && 
		    HTML_BOX_GET_STYLE (html_box_get_containing_block (box))->inherited->direction == HTML_DIRECTION_RTL) {

			return width - html_length_get_value (&style->box->width, width) -
				html_box_left_padding (box, width) - html_box_right_padding (box, width) -
				html_box_left_border_width (box)   - html_box_right_border_width (box) -
				html_box_right_margin (box, width);
		}
		else
			return html_length_get_value (&style->surround->margin.left, width);
	}
	else if (style->surround->margin.left.type != HTML_LENGTH_AUTO)
		return html_length_get_value (&style->surround->margin.left, width);
	else {
		gint tmp = width - html_length_get_value (&style->box->width, width) -
			html_box_left_padding (box, width) - html_box_right_padding (box, width) -
			html_box_left_border_width (box)   - html_box_right_border_width (box);

		if (style->surround->margin.right.type != HTML_LENGTH_AUTO)
			return tmp - html_box_right_margin (box, width);
		else
			return tmp / 2;
	}
}

/* From section "10.3 Computing widths and margins" in the css2 specification */
gint
html_box_right_margin (HtmlBox *box, gint width)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE(box);

	if (simple_margin (style))
		return html_length_get_value (&style->surround->margin.right, width);

	/* If both left and right margin is specified, then we might have to recalculate one of the
	 * values to satisfy the expression: "'left' + 'margin-left' + 'border-left-width' + 'padding-left' + 'width' + 
	 'padding-right' + 'border-right-width' + 'margin-right' + 'right' = width of containing block"
	 */
	else if (style->surround->margin.left.type != HTML_LENGTH_AUTO &&
		 style->surround->margin.right.type != HTML_LENGTH_AUTO) {

		if (!html_box_get_containing_block (box) ||
		    !HTML_BOX_GET_STYLE (html_box_get_containing_block (box)) ||
		    !HTML_BOX_GET_STYLE (html_box_get_containing_block (box))->inherited ||
		     HTML_BOX_GET_STYLE (html_box_get_containing_block (box))->inherited->direction != HTML_DIRECTION_RTL) {

			return width - html_length_get_value (&style->box->width, width) -
				html_box_left_padding (box, width) - html_box_right_padding (box, width) -
				html_box_left_border_width (box)   - html_box_right_border_width (box) -
				html_box_left_margin (box, width);
		}
		else
			return html_length_get_value (&style->surround->margin.right, width);
	}
	else if (style->surround->margin.right.type != HTML_LENGTH_AUTO)
		return html_length_get_value (&style->surround->margin.right, width);
	else {
		gint tmp = width - html_length_get_value (&style->box->width, width) -
			html_box_left_padding (box, width) - html_box_right_padding (box, width) -
			html_box_left_border_width (box)   - html_box_right_border_width (box);
		
		if (style->surround->margin.left.type != HTML_LENGTH_AUTO)
			return tmp - html_box_left_margin (box, width);
		else
			return tmp / 2;
	}
}

static gint
html_box_real_left_mbp_sum (HtmlBox *box, gint width)
{
	gint margin, padding, border;

	if (need_containing_width (box, width))
		width = html_box_get_containing_block_width (box);

	g_return_val_if_fail (box != NULL, 0);

	margin  = html_box_left_margin (box, width);
	padding = html_box_left_padding (box, width);
	border  = html_box_left_border_width (box);

	return margin + padding + border;
}

static gint
html_box_real_right_mbp_sum (HtmlBox *box, gint width)
{
	gint margin, padding, border;

	if (need_containing_width (box, width))
		width = html_box_get_containing_block_width (box);
	
	g_return_val_if_fail (box != NULL, 0);

	margin  = html_box_right_margin (box, width);
	padding = html_box_right_padding (box, width);
	border  = html_box_right_border_width (box);

	return margin + padding + border;
}

static gint
html_box_real_top_mbp_sum (HtmlBox *box, gint width)
{
	gint margin, padding, border;

	if (need_containing_width (box, width))
		width = html_box_get_containing_block_width (box);
	
	g_return_val_if_fail (box != NULL, 0);
	
	margin  = html_box_top_margin (box, width);
	padding = html_box_top_padding (box, width);
	border  = html_box_top_border_width (box);

	return margin + padding + border;
}

static gint
html_box_real_bottom_mbp_sum (HtmlBox *box, gint width)
{
	gint margin, padding, border;

	if (need_containing_width (box, width))
		width = html_box_get_containing_block_width (box);

	g_return_val_if_fail (box != NULL, 0);
	
	margin  = html_box_bottom_margin (box, width);
	padding = html_box_bottom_padding (box, width);
	border  = html_box_bottom_border_width (box);

	return margin + padding + border;
}

static void
html_box_finalize (GObject *object)
{
	HtmlBox *box = HTML_BOX (object);

	if (html_box_get_before (box))
		g_object_unref (G_OBJECT (html_box_get_before (box)));
	if (html_box_get_after (box))
		g_object_unref (G_OBJECT (html_box_get_after (box)));

	if (box->style)
		html_style_unref (box->style);

	if (box->dom_node)
		g_object_remove_weak_pointer (G_OBJECT (box->dom_node), (gpointer *) &(box->dom_node));

	G_OBJECT_CLASS (parent_class)->finalize (object);
	
}

/**
 * html_box_append_child:
 * @self: the box that the child will be appended on
 * @child: the child to append.
 *
 * This function will append @child as the last child
 * on box @self.
 *
 **/
void
html_box_append_child (HtmlBox *self, HtmlBox *child)
{
	HTML_BOX_GET_CLASS(self)->append_child (self, child);
}

static void
html_box_real_append_child (HtmlBox *self, HtmlBox *child)
{
	HtmlBox *box = self->children;

	g_return_if_fail (HTML_IS_BOX (self));
	g_return_if_fail (HTML_IS_BOX (child));

	if (box) {
		while (box->next)
			box = box->next;
		
		box->next = child;
		child->prev = box;
	}
	else {
		/* First child */
		self->children = child;
		child->prev = NULL;
	}
	child->next = NULL;
	child->parent = self;
}

/**
 * html_box_insert_after:
 * @self: the box to insert
 * @box: the box that @self will be inserted after
 *
 * This function inserts @self after @box
 **/
void
html_box_insert_after (HtmlBox *self, HtmlBox *box)
{
	g_return_if_fail (HTML_IS_BOX (self));
	g_return_if_fail (HTML_IS_BOX (box));

	if (self->next) {
		self->next->prev = box;
	}
	box->next = self->next;
	box->prev = self;
	self->next = box;
	box->parent = self->parent;
}

static void
html_box_real_remove (HtmlBox *self)
{
	if (self->parent != NULL && self->parent->children == self)
		self->parent->children = self->next;

	self->parent = NULL;

	if (self->next != NULL)
		self->next->prev = self->prev;
	if (self->prev != NULL)
		self->prev->next = self->next;

	self->next = self->prev = NULL;
	self->parent = NULL;
}

/**
 * html_box_remove:
 * @self: the box to remove
 *
 * This function will "remove" @self from the layout tree.
 *
 * Note: This function won't free any memory, it will only
 * remove the box from the layout tree.
 *
 * Returs: nothing
 **/
void
html_box_remove (HtmlBox *self)
{
	HTML_BOX_GET_CLASS(self)->remove (self);
}

static gboolean
html_box_real_handles_events (HtmlBox *self)
{
	/* We handle events by default */
	return TRUE;
}

static gint
html_box_real_get_bidi_level (HtmlBox *self)
{
	return HTML_BOX_GET_STYLE (self)->inherited->direction;
}

gboolean
html_box_handles_events (HtmlBox *self)
{
	return HTML_BOX_GET_CLASS(self)->handles_events (self);
}

gint
html_box_get_bidi_level (HtmlBox *self)
{
	return HTML_BOX_GET_CLASS(self)->get_bidi_level (self);
}

/**
 * html_box_relayout:
 * @self: The box to relayout
 * @relayout: the relayout context
 *
 * This function tells the box (@self) to calculate it's size and position of
 itself 
 * and it's children.
 **/
void 
html_box_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	/* These boxes shouldn't be displayed */
	if (HTML_BOX_GET_STYLE (self)->display == HTML_DISPLAY_NONE)
		self->width = self->height = 0;
	else
		HTML_BOX_GET_CLASS(self)->relayout (self, relayout);

	self->is_relayouted = TRUE;
}

void
html_box_apply_positioned_offset (HtmlBox *box, gint *tx, gint *ty)
{
	gint width  = html_box_get_containing_block_width  (box);
	gint height = html_box_get_containing_block_height (box);

	if (HTML_BOX_GET_STYLE (box)->surround->position.left.type != HTML_LENGTH_AUTO)
		*tx += html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.left, width);
	
	else if (HTML_BOX_GET_STYLE (box)->surround->position.right.type != HTML_LENGTH_AUTO) {
		if (HTML_BOX_GET_STYLE (box)->display == HTML_DISPLAY_INLINE)
			*tx -= html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.right, width);
		else
			*tx += width - box->width - html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.right, width);
	}
	if (HTML_BOX_GET_STYLE (box)->surround->position.top.type != HTML_LENGTH_AUTO)
		*ty += html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.top, height);
	
	else if (HTML_BOX_GET_STYLE (box)->surround->position.bottom.type != HTML_LENGTH_AUTO) {
		if (HTML_BOX_GET_STYLE (box)->display == HTML_DISPLAY_INLINE)
			*ty -= html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.bottom, height);
		else
			*ty += height - box->height - html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.bottom, height);
	}
}

gboolean
html_box_should_paint (HtmlBox *box, GdkRectangle *area, gint tx, gint ty)
{
	return HTML_BOX_GET_CLASS(box)->should_paint (box, area, tx, ty);
}

static gboolean
html_box_real_should_paint (HtmlBox *box, GdkRectangle *area, gint tx, gint ty)
{
	if (HTML_BOX_GET_STYLE (box)->position != HTML_POSITION_STATIC)
		return TRUE;
	
	if (HTML_IS_BOX_INLINE (box))
		return TRUE;

	if (HTML_IS_BOX_TABLE_ROW (box))
		return TRUE;

	/* Clipping */
	if (box->y + ty > area->y + area->height || box->y + box->height + ty < area->y ||
	    box->x + tx > area->x + area->width || box->x + box->width + tx < area->x)
		return FALSE;
	
	return TRUE;
}

/**
 * html_box_paint:
 * @self: a box
 * @painter: the painter to use
 * @area: The area that has to be repainted
 * @tx: the x offset
 * @ty: the y offset
 *
 * This function paints the specified box (@self).
 *
 **/
void 
html_box_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);

	if (!style)
		return;

  	if (style->display == HTML_DISPLAY_NONE)
		return;

	if (style->visibility != HTML_VISIBILITY_VISIBLE)
		return;

	if ((style->position == HTML_POSITION_RELATIVE ||
	     style->position == HTML_POSITION_ABSOLUTE) && !HTML_IS_BOX_TEXT (self))
		html_box_apply_positioned_offset (self, &tx, &ty);

	/* Should we paint this one? */
	if (!html_box_should_paint (self, area, tx, ty))
		return;

	/* Draw the background color */
	html_style_painter_draw_background_color (self, painter, area, tx, ty);
	/* Draw the background image */
	html_style_painter_draw_background_image (self, painter, area, tx, ty);
	/* And now draw the border */
	html_style_painter_draw_border (self, painter, area, tx, ty);

	/* Call the real paint function */
	if (HTML_BOX_GET_CLASS (self)->paint)
		HTML_BOX_GET_CLASS(self)->paint (self, painter, area, tx, ty);

	/* And now draw the outline */
	html_style_painter_draw_outline (self, HTML_BOX_GET_STYLE (self), painter, area, tx, ty);

	if (gtk_html_context_get ()->debug_painting) {
		if (self->width > 0 && self->height > 0) {
			HtmlColor *red = html_color_new_from_rgb (255, 0, 0);
			html_painter_set_foreground_color (painter, red);
			html_color_unref (red);
			
			html_painter_draw_rectangle (painter, area, self->x + tx, self->y + ty, 
						     self->width, self->height);
		}
	}
}

static gint 
html_box_real_get_ascent (HtmlBox *self)
{
	return self->height;
}

static gint 
html_box_real_get_descent (HtmlBox *self)
{
	return 0;
}

/**
 * html_box_get_ascent:
 * @self: the box we want to get the ascent value from.
 * 
 * This function returns the ascent value of the box @self.
 * The ascent value is the space in pixels that the box
 * needs to have above the baseline.
 * 
 * Return value: the ascent value in pixels
 **/
gint 
html_box_get_ascent (HtmlBox *self)
{
	return HTML_BOX_GET_CLASS(self)->get_ascent (self);
}

/**
 * html_box_get_descent:
 * @self: the box we want to get the descent value from.
 * 
 * This function returns the descent value of the box @self.
 * The descent value is the space in pixels that the box
 * needs to have under the baseline.
 * 
 * Return value: the descent value in pixels
 **/
gint 
html_box_get_descent (HtmlBox *self)
{
	return HTML_BOX_GET_CLASS(self)->get_descent (self);
}

/**
 * html_box_get_absolute_x:
 * @box: the HtmlBox you want to get the x coordiante from 
 * 
 * This function returns the absolute x coordinate for the box @box
 * 
 * Return value: the absolute x coordinate in pixels.
 **/
gint
html_box_get_absolute_x (HtmlBox *box)
{
        gint boxx = box->x;
        HtmlBox *parent;

	g_return_val_if_fail (box != NULL, 0);

	parent= box->parent;
        while (parent) {
		if (!HTML_IS_BOX_INLINE (parent))
			boxx += parent->x + html_box_left_mbp_sum (parent, -1);
                parent = parent->parent;
        }
        return boxx;
}

/**
 * html_box_get_absolute_y:
 * @box: the HtmlBox you want to get the y coordiante from 
 * 
 * This function returns the absolute y coordinate for the box @box
 * 
 * Return value: the absolute y coordinate in pixels.
 **/
gint
html_box_get_absolute_y (HtmlBox *box)
{
        gint boxy;
        HtmlBox *parent;

	g_return_val_if_fail (box != NULL, 0);
	
	parent = box->parent;
	boxy = box->y;
	
        while (parent) {
		if (!HTML_IS_BOX_INLINE (parent))
			boxy += parent->y + html_box_top_mbp_sum (parent, -1);
                parent = parent->parent;
        }
        return boxy;
}

/**
 * html_box_check_min_max_width_height:
 * @self: the box to check
 * @boxwidth: width
 * @boxheight: height
 * 
 * This function corrects the box width or height, if the values are lower
 * then the min values or greater then the max values.
 **/
void
html_box_check_min_max_width_height (HtmlBox *self, gint *boxwidth, gint *boxheight) 
{
	int tmp;
	gint old_width = *boxwidth;
	gint old_height = *boxheight;
	HtmlBox *parent = html_box_get_containing_block (self);

	if (parent) {
		if (HTML_BOX_GET_STYLE (self)->box->min_width.type != HTML_LENGTH_AUTO) {
			tmp = html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->min_width, parent->width -
						     html_box_horizontal_mbp_sum (parent) - html_box_horizontal_mbp_sum (self));
			if (*boxwidth < tmp)
				*boxwidth = tmp;		

		}
		
		if (HTML_BOX_GET_STYLE (self)->box->max_width.type != HTML_LENGTH_AUTO) {
			tmp = html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->max_width, parent->width -
						     html_box_horizontal_mbp_sum (parent) - html_box_horizontal_mbp_sum (self));
			if (*boxwidth > tmp)
				*boxwidth = tmp;		

		}
		
		if (HTML_BOX_GET_STYLE (self)->box->min_height.type != HTML_LENGTH_AUTO) {
			tmp = html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->min_height, parent->height -
						     html_box_horizontal_mbp_sum (parent) - html_box_horizontal_mbp_sum (self));
			if (*boxheight < tmp)
				*boxheight = tmp;		

		}
		
		if (HTML_BOX_GET_STYLE (self)->box->max_height.type != HTML_LENGTH_AUTO) {
			tmp = html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->max_height, parent->height -
						     html_box_horizontal_mbp_sum (parent) - html_box_horizontal_mbp_sum (self));
			if (*boxheight > tmp)
				*boxheight = tmp;		

		}
	}
	else {
		if (HTML_BOX_GET_STYLE (self)->box->min_width.type != HTML_LENGTH_AUTO)
			if (*boxwidth < html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->min_width, 0))
				*boxwidth = html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->min_width, 0);
				
		if (HTML_BOX_GET_STYLE (self)->box->max_width.type != HTML_LENGTH_AUTO)
			if (*boxwidth > html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->max_width, 0))
				*boxwidth = html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->max_width, 0);
		
		if (HTML_BOX_GET_STYLE (self)->box->min_height.type != HTML_LENGTH_AUTO)
			if (*boxheight < html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->min_height, 0))
				*boxheight = html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->min_height, 0);
		
		if (HTML_BOX_GET_STYLE (self)->box->max_height.type != HTML_LENGTH_AUTO)
			if (*boxheight > html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->max_height, 0))
				*boxheight = html_length_get_value (&HTML_BOX_GET_STYLE (self)->box->max_height, 0);
	}
	
	/* Maintain aspect ratio if it's an image */
	if (HTML_IS_BOX_IMAGE (self)) {
		if ((*boxwidth > old_width) && (*boxheight >= old_height)) {
			*boxheight = *boxheight * (gdouble)(*boxwidth / (gdouble)old_width);
			return;
		}
		else if ((*boxheight > old_height) && (*boxwidth >= old_width)) {
			*boxwidth = *boxwidth * (gdouble)(*boxheight / (gdouble)old_height);
			return;
		}
		else if ((*boxwidth < old_width) && (*boxheight <= old_height)) {
			*boxheight = *boxheight * (gdouble)(*boxwidth / (gdouble)old_width);
			return;
		}
		else if ((*boxheight < old_height) && (*boxwidth <= old_width)) {
			*boxwidth = *boxwidth * (gdouble)(*boxheight / (gdouble)old_height);
			return;
		}
	}
}

static void
html_box_class_init (HtmlBoxClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;
	object_class->finalize = html_box_finalize;

	klass->get_ascent     = html_box_real_get_ascent;
	klass->get_descent    = html_box_real_get_descent;
	klass->get_bidi_level = html_box_real_get_bidi_level;
	klass->handles_events = html_box_real_handles_events;
	klass->append_child   = html_box_real_append_child;
	klass->left_mbp_sum   = html_box_real_left_mbp_sum;
	klass->right_mbp_sum  = html_box_real_right_mbp_sum;
	klass->top_mbp_sum    = html_box_real_top_mbp_sum;
	klass->bottom_mbp_sum = html_box_real_bottom_mbp_sum;
	klass->should_paint   = html_box_real_should_paint;
	klass->remove = html_box_real_remove;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_init (HtmlBox *box)
{
	box->is_relayouted = FALSE;

	/* FIXME: Necessary? */
	box->parent = NULL;
}

GType 
html_box_get_type (void)
{
	static GType html_type = 0;

	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_class_init,
			NULL,
			NULL,
			sizeof (HtmlBox),
			16,
			(GInstanceInitFunc) html_box_init
		};

		html_type = g_type_register_static (G_TYPE_OBJECT, "HtmlBox", &type_info, 0);
	}

	return html_type;
}

/**
 * html_box_set_unrelayouted_up:
 * @self: a layout box
 * 
 * This function marks this box and all of the boxes higher up in the tree as unrelayouted.
 **/
void
html_box_set_unrelayouted_up (HtmlBox *self)
{
	while (self) {
		self->is_relayouted = FALSE;

		self = self->parent;
	}
}

/**
 * html_box_set_unrelayouted_down:
 * @self: a layout box
 * 
 * This function marks this box and all of the boxes higher up in the tree as unrelayouted.
 **/
void
html_box_set_unrelayouted_down (HtmlBox *self)
{
	HtmlBox *box = self->children;
	self->is_relayouted = FALSE;

	while (box) {
		html_box_set_unrelayouted_down (box);
		box = box->next;
	}

}

void
html_box_set_style (HtmlBox *box, HtmlStyle *style)
{
	if (box->style == style)
		return;

	g_assert (box->dom_node == NULL);

	html_style_ref (style);
	
	if (box->style)
		html_style_unref (box->style);
	
	box->style = style;
}

/**
 * html_box_is_parent:
 * @self: a box
 * @_parent: the parent box to search for
 * 
 * This function checks if @_parent is a parent to @self.
 * 
 * Return value: TRUE if @self is a child, grandchild, .... to @_parent, else FALSE.
 **/
gboolean
html_box_is_parent (HtmlBox *self, HtmlBox *parent)
{
	HtmlBox *box = self->parent;

	while (box) {
		if (box == parent)
			return TRUE;

		box = box->parent;
	}
	return FALSE;
}

void
html_box_handle_html_properties (HtmlBox *box, xmlNode *n)
{
	if (HTML_BOX_GET_CLASS(box)->handle_html_properties)
		HTML_BOX_GET_CLASS(box)->handle_html_properties (box, n);
}

