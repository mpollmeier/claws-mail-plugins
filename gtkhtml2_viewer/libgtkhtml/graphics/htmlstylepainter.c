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

#include <gtk/gtk.h>
/* FIXME: Provide functions instead of including htmlgdkpainter.h*/
#include "graphics/htmlgdkpainter.h"
#include "graphics/htmlpainter.h"
#include "graphics/htmlstylepainter.h"
#include "layout/htmlstyle.h"
#include "layout/htmlbox.h"
#include "layout/htmlboxtext.h"
#include "layout/htmlboxroot.h"
#include "layout/htmlboxinline.h"
#include "layout/htmlboxtable.h"

#define DARKER_SCALE 0.5
#define BRIGHTER_SCALE 2.0

static void
set_up_dash_or_dot_array (gint8 *array, gboolean dotted, gint width)
{
	if (dotted) {
		array[0] = width;
		array[1] = width;
	}
	else {
		array[0] = width * 2;
		array[1] = width * 2;
	}
}

static void
paint_background_text (HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty, HtmlBox *self, HtmlStyle *style, HtmlStyle *bg_style)
{
	gint x, y, w, h, width;

	width = html_box_get_containing_block_width (self);
	html_painter_set_foreground_color (painter, &bg_style->background->color);
	
	x = self->x + tx + html_box_left_margin (self, width);
	y = self->y + ty + html_box_top_margin (self, width) - style->border->top.width;
	w = self->width - html_box_right_margin (self, width) - 
		html_box_left_margin (self, width);
	h = self->height - html_box_top_margin (self, width) - 
		html_box_bottom_margin (self, width) +
		style->border->top.width + style->border->bottom.width;	
	html_painter_fill_rectangle (painter, area, x, y, w, h);
}

static void
paint_background (HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty, HtmlBox *self, HtmlStyle *style, HtmlStyle *bg_style)
{
	gint x, y, w, h, width;

	width = html_box_get_containing_block_width (self);
	html_painter_set_foreground_color (painter, &bg_style->background->color);
	
	x = self->x + tx + html_box_left_margin (self, width);
	y = self->y + ty + html_box_top_margin (self, width);
	w = self->width - html_box_right_margin (self, width) - 
		html_box_left_margin (self, width);
	h = self->height - html_box_top_margin (self, width) - 
		html_box_bottom_margin (self, width);	
	html_painter_fill_rectangle (painter, area, x, y, w, h);
}

void
html_style_painter_draw_background_color (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);
	HtmlStyle *bg_style = HTML_BOX_GET_STYLE (self);
	gboolean is_text = FALSE;

	/* Use the parent style if the box is a textbox and the parent is an inline box */
	if (HTML_IS_BOX_ROOT (self)) {
		if (style->background->color.transparent && self->children) {
			bg_style = HTML_BOX_GET_STYLE (self->children);
		}
	}

	/* Use the parent style if the box is a textbox and the parent is an inline box */
	if (HTML_IS_BOX_TEXT (self)) {
		if (HTML_IS_BOX_INLINE (self->parent)) {
			bg_style = HTML_BOX_GET_STYLE (self->parent);
			is_text = TRUE;
		}
		else
			return;
	}

	/* 
	 * This is a special case for table cells, the cell has to draw the background of on of these
	 * cell, rowgroup, row, table in this order, we can't let the table draw it's own background because
	 * if we have an empty cell then there shouldn't be any background at all there
	 */
	if (style->display == HTML_DISPLAY_TABLE_CELL) {
		HtmlBox *box = self;

		while (box && box->parent && bg_style && bg_style->background &&
			bg_style->background->color.transparent) {
			if (HTML_BOX_GET_STYLE (box)->display == HTML_DISPLAY_TABLE)
				break;
			box = box->parent;
			bg_style = box ? HTML_BOX_GET_STYLE (box) : NULL;
		}
	}

	if (!bg_style || bg_style->visibility != HTML_VISIBILITY_VISIBLE)
		return;

	if (bg_style && !bg_style->background->color.transparent) {

		if (is_text)
			paint_background_text (painter, area, tx, ty, self, style, bg_style);

		switch (style->display) {
		case HTML_DISPLAY_BLOCK:
		case HTML_DISPLAY_TABLE:
		case HTML_DISPLAY_TABLE_CELL:
		case HTML_DISPLAY_TABLE_CAPTION:
			paint_background (painter, area, tx, ty, self, style, bg_style);
			break;
		default:
			break;
		}
	}
}

static void
html_style_painter_draw_top_border (HtmlBox *self, HtmlStyle *style, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty,
				    gboolean left_corner, gboolean right_corner)
{
	gint left = 0, right = 0;
	gint width = html_box_get_containing_block_width (self);
	gint x, y, w, h;
	gint double_line_width, double_line_spacing;
	HtmlColor *tmp_color, *dark_color, *light_color;
	GdkPoint points[4];
	gint8 array[2] = {0, 0};
	
	if (style->border->top.border_style == HTML_BORDER_STYLE_NONE ||
	    style->border->top.border_style == HTML_BORDER_STYLE_HIDDEN ||
	    style->border->top.width == 0)
		return;

	if (left_corner)
		left = style->border->left.width;
	if (right_corner)
		right = style->border->right.width;

	if (style->border->top.color)
		tmp_color = style->border->top.color;
	else
		tmp_color = style->inherited->color;
	
	x = html_box_left_margin (self, width) + self->x + tx;
	y = html_box_top_margin (self, width) + self->y + ty;
	w = self->width - html_box_left_margin (self, width) - html_box_right_margin (self, width);
	h = style->border->top.width;
	
	switch (style->border->top.border_style) {
	case HTML_BORDER_STYLE_SOLID:
		html_painter_set_foreground_color (painter, tmp_color);
		break;
	case HTML_BORDER_STYLE_INSET:
		dark_color = html_color_transform (tmp_color, DARKER_SCALE);
		html_painter_set_foreground_color (painter, dark_color);
		html_color_unref (dark_color);
		break;
	case HTML_BORDER_STYLE_OUTSET:
		light_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		html_painter_set_foreground_color (painter, light_color);
		html_color_unref (light_color);
		break;
	case HTML_BORDER_STYLE_DASHED:
	case HTML_BORDER_STYLE_DOTTED:

		html_painter_set_foreground_color (painter, tmp_color);

		set_up_dash_or_dot_array (array, style->border->top.border_style == HTML_BORDER_STYLE_DOTTED, h);

		gdk_gc_set_dashes (HTML_GDK_PAINTER (painter)->gc,
				   0, array, 2);

		gdk_gc_set_line_attributes (HTML_GDK_PAINTER (painter)->gc,
					    h,
					    GDK_LINE_ON_OFF_DASH,
					    GDK_CAP_BUTT,
					    GDK_JOIN_MITER);
		gdk_draw_line (HTML_GDK_PAINTER (painter)->window,
			       HTML_GDK_PAINTER (painter)->gc, x, y + h / 2, x + w, y + h / 2);
		return;
		break;
	case HTML_BORDER_STYLE_DOUBLE:
		double_line_width = style->border->top.width / 3;
		double_line_spacing = style->border->top.width - double_line_width * 2;

		html_painter_set_foreground_color (painter, tmp_color);
		
		points[0].x = x;
		points[0].y = y;
		points[1].x = x + left / 3;
		points[1].y = y + double_line_width;
		points[2].x = x + w - right / 3;
		points[2].y = y + double_line_width;
		points[3].x = x + w;
		points[3].y = y;

		html_painter_draw_polygon (painter, TRUE, points, 4);

		points[0].x = x + (left - left / 3);
		points[0].y = y + h - double_line_width;
		points[1].x = x + left;
		points[1].y = y + h;
		points[2].x = x + w - right;
		points[2].y = y + h;
		points[3].x = x + w - (right - right / 3);
		points[3].y = y + h - double_line_width;
		
		html_painter_draw_polygon (painter, TRUE, points, 4);

		return;
		break;
	case HTML_BORDER_STYLE_GROOVE:
	case HTML_BORDER_STYLE_RIDGE:
		if (style->border->top.border_style == HTML_BORDER_STYLE_GROOVE) {
			dark_color = html_color_transform (tmp_color, DARKER_SCALE);
			light_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		}
		else {
			light_color = html_color_transform (tmp_color, DARKER_SCALE);
			dark_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		}
		html_painter_set_foreground_color (painter, dark_color);
		points[0].x = x;
		points[0].y = y;
		points[1].x = x + left / 2;
		points[1].y = y + h / 2;
		points[2].x = x + w - right / 2;
		points[2].y = y + h / 2;
		points[3].x = x + w;
		points[3].y = y;
		html_painter_draw_polygon (painter, TRUE, points, 4);

		html_painter_set_foreground_color (painter, light_color);
		points[0].x = x + (left / 2);
		points[0].y = y + h / 2;
		points[1].x = x + left;
		points[1].y = y + h;
		points[2].x = x + w - right;
		points[2].y = y + h;
		points[3].x = x + w - (right / 2);
		points[3].y = y + h / 2;

		html_painter_draw_polygon (painter, TRUE, points, 4);
		
		html_color_unref (light_color);
		html_color_unref (dark_color);
		return;
		break;
	default:
		g_warning ("unknown border style");
	}
	
	points[0].x = x;
	points[0].y = y;
	points[1].x = x + left;
	points[1].y = y + h;
	points[2].x = x + w - right;
	points[2].y = y + h;
	points[3].x = x + w;
	points[3].y = y;
	
	html_painter_draw_polygon (painter, TRUE, points, 4);

}

static void
html_style_painter_draw_bottom_border (HtmlBox *self, HtmlStyle *style, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty,
				       gboolean left_corner, gboolean right_corner)
{
	gint left = 0, right = 0;
	gint width = html_box_get_containing_block_width (self);
	gint x, y, w, h;
	gint double_line_width, double_line_spacing;
	HtmlColor *tmp_color, *dark_color, *light_color;
	GdkPoint points[4];
	gint8 array[2] = {0, 0};
	
	if (style->border->bottom.border_style == HTML_BORDER_STYLE_NONE ||
	    style->border->bottom.border_style == HTML_BORDER_STYLE_HIDDEN ||
	    style->border->bottom.width == 0)
		return;

	if (left_corner)
		left = style->border->left.width;
	if (right_corner)
		right = style->border->right.width;

	if (style->border->bottom.color)
		tmp_color = style->border->bottom.color;
	else
		tmp_color = style->inherited->color;

	x = html_box_left_margin (self, width) + self->x + tx;
	y = self->y + ty + self->height - style->border->bottom.width - html_box_bottom_margin (self, width);
	w = self->width - html_box_left_margin (self, width) - html_box_right_margin (self, width);
	h = style->border->bottom.width;

	switch (style->border->bottom.border_style) {
	case HTML_BORDER_STYLE_SOLID:
		html_painter_set_foreground_color (painter, tmp_color);
		break;
	case HTML_BORDER_STYLE_INSET:
		light_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		html_painter_set_foreground_color (painter, light_color);
		html_color_unref (light_color);
		break;
	case HTML_BORDER_STYLE_OUTSET:
		dark_color = html_color_transform (tmp_color, DARKER_SCALE);
		html_painter_set_foreground_color (painter, dark_color);
		html_color_unref (dark_color);
		break;
	case HTML_BORDER_STYLE_DASHED:
	case HTML_BORDER_STYLE_DOTTED:
		html_painter_set_foreground_color (painter, tmp_color);

		set_up_dash_or_dot_array (array, style->border->top.border_style == HTML_BORDER_STYLE_DOTTED, h);

		gdk_gc_set_dashes (HTML_GDK_PAINTER (painter)->gc,
				   0, array, 2);
		gdk_gc_set_line_attributes (HTML_GDK_PAINTER (painter)->gc,
					    h,
					    GDK_LINE_ON_OFF_DASH,
					    GDK_CAP_BUTT,
					    GDK_JOIN_MITER);
		gdk_draw_line (HTML_GDK_PAINTER (painter)->window,
			       HTML_GDK_PAINTER (painter)->gc, x, y + h / 2, x + w, y + h / 2);
		return;
		break;
	case HTML_BORDER_STYLE_DOUBLE:
		double_line_width = style->border->bottom.width / 3;
		double_line_spacing = style->border->bottom.width - double_line_width * 2;

		html_painter_set_foreground_color (painter, tmp_color);

		points[0].x = x + left;
		points[0].y = y;
		points[1].x = x + (left - left / 3);
		points[1].y = y + double_line_width;
		points[2].x = x + w - (right - right / 3);
		points[2].y = y + double_line_width;
		points[3].x = x + w - right;
		points[3].y = y;
		
		html_painter_draw_polygon (painter, TRUE, points, 4);

		points[0].x = x + left / 3;
		points[0].y = y + h - double_line_width;
		points[1].x = x;
		points[1].y = y + h;
		points[2].x = x + w;
		points[2].y = y + h;
		points[3].x = x + w - right / 3;
		points[3].y = y + h - double_line_width;

		html_painter_draw_polygon (painter, TRUE, points, 4);

		return;
		break;
	case HTML_BORDER_STYLE_GROOVE:
	case HTML_BORDER_STYLE_RIDGE:
		if (style->border->bottom.border_style == HTML_BORDER_STYLE_GROOVE) {
			dark_color = html_color_transform (tmp_color, DARKER_SCALE);
			light_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		}
		else {
			light_color = html_color_transform (tmp_color, DARKER_SCALE);
			dark_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		}
		html_painter_set_foreground_color (painter, dark_color);
		points[0].x = x + left;
		points[0].y = y;
		points[1].x = x;
		points[1].y = y + h / 2;
		points[2].x = x + w;
		points[2].y = y + h / 2;
		points[3].x = x + w - right;
		points[3].y = y;
		
		html_painter_draw_polygon (painter, TRUE, points, 4);

		html_painter_set_foreground_color (painter, light_color);
		points[0].x = x + (left / 2);
		points[0].y = y + h / 2;
		points[1].x = x;
		points[1].y = y + h;
		points[2].x = x + w;
		points[2].y = y + h;
		points[3].x = x + w - (right / 2);
		points[3].y = y + h / 2;

		html_painter_draw_polygon (painter, TRUE, points, 4);

		html_color_unref (dark_color);
		html_color_unref (light_color);
		return;
		break;
	default:
		g_print ("unknown border style\n");
	}

	points[0].x = x + left;
	points[0].y = y;
	points[1].x = x;
	points[1].y = y + h;
	points[2].x = x + w;
	points[2].y = y + h;
	points[3].x = x + w - right;
	points[3].y = y;
	
	html_painter_draw_polygon (painter, TRUE, points, 4);
	
}

static void
html_style_painter_draw_left_border (HtmlBox *self, HtmlStyle *style, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty, gint height)
{
	gint top, bottom;
	gint width = html_box_get_containing_block_width (self);
	gint x, y, w, h;
	gint double_line_width, double_line_spacing;
	HtmlColor *tmp_color, *dark_color, *light_color;
	GdkPoint points[4];
	gint8 array[2] = {0, 0};
	
	if (style->border->left.border_style == HTML_BORDER_STYLE_NONE ||
	    style->border->left.border_style == HTML_BORDER_STYLE_HIDDEN ||
	    style->border->left.width == 0)
		return;

	top = style->border->top.width;
	bottom = style->border->bottom.width;

	if (style->border->left.color)
		tmp_color = style->border->left.color;
	else
		tmp_color = style->inherited->color;

	x = html_box_left_margin (self, width) + self->x + tx;
	y = html_box_top_margin  (self, width) + self->y + ty;
	w = style->border->left.width;
	h = height - html_box_bottom_margin (self, width) - html_box_top_margin (self, width);

	switch (style->border->left.border_style) {
	case HTML_BORDER_STYLE_SOLID:
		html_painter_set_foreground_color (painter, tmp_color);
		break;
	case HTML_BORDER_STYLE_INSET:
		dark_color = html_color_transform (tmp_color, DARKER_SCALE);
		html_painter_set_foreground_color (painter, dark_color);
		html_color_unref (dark_color);
		break;
	case HTML_BORDER_STYLE_OUTSET:
		light_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		html_painter_set_foreground_color (painter, light_color);
		html_color_unref (light_color);
		break;
	case HTML_BORDER_STYLE_DASHED:
	case HTML_BORDER_STYLE_DOTTED:

		html_painter_set_foreground_color (painter, tmp_color);

		set_up_dash_or_dot_array (array, style->border->top.border_style == HTML_BORDER_STYLE_DOTTED, w);
		
		gdk_gc_set_dashes (HTML_GDK_PAINTER (painter)->gc,
				   0, array, 2);

		gdk_gc_set_line_attributes (HTML_GDK_PAINTER (painter)->gc,
					    w,
					    GDK_LINE_ON_OFF_DASH,
					    GDK_CAP_BUTT,
					    GDK_JOIN_MITER);
		gdk_draw_line (HTML_GDK_PAINTER (painter)->window,
			       HTML_GDK_PAINTER (painter)->gc, x + w / 2, y, x + w / 2, y + h);
		return;
		break;
	case HTML_BORDER_STYLE_DOUBLE:
		double_line_width = style->border->left.width / 3;
		double_line_spacing = style->border->left.width - double_line_width * 2;

		html_painter_set_foreground_color (painter, tmp_color);		
		points[0].x = x;
		points[0].y = y;
		points[1].x = x + double_line_width;
		points[1].y = y + top / 3;
		points[2].x = x + double_line_width;
		points[2].y = y + h - bottom / 3;
		points[3].x = x;
		points[3].y = y + h;

		html_painter_draw_polygon (painter, TRUE, points, 4);

		points[0].x = x + w - double_line_width;
		points[0].y = y + (top - top / 3);
		points[1].x = x + w;
		points[1].y = y + top;
		points[2].x = x + w;
		points[2].y = y + h - bottom;
		points[3].x = x + w - double_line_width;
		points[3].y = y + h - (bottom - bottom / 3);

		html_painter_draw_polygon (painter, TRUE, points, 4);
		
		return;
		break;
	case HTML_BORDER_STYLE_GROOVE:
	case HTML_BORDER_STYLE_RIDGE:
		if (style->border->left.border_style == HTML_BORDER_STYLE_GROOVE) {
			dark_color = html_color_transform (tmp_color, DARKER_SCALE);
			light_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		}
		else {
			light_color = html_color_transform (tmp_color, DARKER_SCALE);
			dark_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		}
		html_painter_set_foreground_color (painter, dark_color);
		points[0].x = x;
		points[0].y = y;
		points[1].x = x + w / 2;
		points[1].y = y + top / 2;
		points[2].x = x + w / 2;
		points[2].y = y + h - bottom / 2;
		points[3].x = x;
		points[3].y = y + h;
		html_painter_draw_polygon (painter, TRUE, points, 4);

		html_painter_set_foreground_color (painter, light_color);
		points[0].x = x + w / 2;
		points[0].y = y + (top / 2);
		points[1].x = x + w;
		points[1].y = y + top;
		points[2].x = x + w;
		points[2].y = y + h - bottom;
		points[3].x = x + w / 2;
		points[3].y = y + h - bottom / 2;

		html_painter_draw_polygon (painter, TRUE, points, 4);
		
		html_color_unref (dark_color);
		html_color_unref (light_color);
		return;
		break;
	default:
		g_print ("unknown border style\n");
	}

	points[0].x = x;
	points[0].y = y;
	points[1].x = x + w;
	points[1].y = y + top;
	points[2].x = x + w;
	points[2].y = y + h - bottom;
	points[3].x = x;
	points[3].y = y + h;

	html_painter_draw_polygon (painter, TRUE, points, 4);
}

static void
html_style_painter_draw_right_border (HtmlBox *self, HtmlStyle *style, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty, 
				      gint height)
{
	gint top, bottom;
	gint width = html_box_get_containing_block_width (self);
	gint x, y, w, h;
	gint double_line_width, double_line_spacing;
	HtmlColor *tmp_color, *dark_color, *light_color;
	GdkPoint points[4];
	gint8 array[2] = {0, 0};
	
	if (style->border->right.border_style == HTML_BORDER_STYLE_NONE ||
	    style->border->right.border_style == HTML_BORDER_STYLE_HIDDEN ||
	    style->border->right.width == 0)
		return;

	top = style->border->top.width;
	bottom = style->border->bottom.width;

	if (style->border->right.color)
		tmp_color = style->border->right.color;
	else
		tmp_color = style->inherited->color;

	x = self->x + self->width + tx - style->border->right.width - html_box_right_margin (self, width);
	y = html_box_top_margin (self, width) + self->y + ty;
	w = style->border->right.width;
	h = height - html_box_bottom_margin (self, width) - html_box_top_margin (self, width);

	switch (style->border->right.border_style) {
	case HTML_BORDER_STYLE_SOLID:
		html_painter_set_foreground_color (painter, tmp_color);
		break;
	case HTML_BORDER_STYLE_INSET:
		light_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		html_painter_set_foreground_color (painter, light_color);
		html_color_unref (light_color);
		break;
	case HTML_BORDER_STYLE_OUTSET:
		dark_color = html_color_transform (tmp_color, DARKER_SCALE);
		html_painter_set_foreground_color (painter, dark_color);
		html_color_unref (dark_color);
		break;
	case HTML_BORDER_STYLE_DASHED:
	case HTML_BORDER_STYLE_DOTTED:

		html_painter_set_foreground_color (painter, tmp_color);
		
		set_up_dash_or_dot_array (array, style->border->top.border_style == HTML_BORDER_STYLE_DOTTED, w);
		
		gdk_gc_set_dashes (HTML_GDK_PAINTER (painter)->gc,
				   0, array, 2);
		
		gdk_gc_set_line_attributes (HTML_GDK_PAINTER (painter)->gc,
					    w,
					    GDK_LINE_ON_OFF_DASH,
					    GDK_CAP_BUTT,
					    GDK_JOIN_MITER);
		gdk_draw_line (HTML_GDK_PAINTER (painter)->window,
			       HTML_GDK_PAINTER (painter)->gc, x + w / 2, y, x + w / 2, y + h);
		return;
		break;
	case HTML_BORDER_STYLE_DOUBLE:
		double_line_width = style->border->right.width / 3;
		double_line_spacing = style->border->right.width - double_line_width * 2;

		html_painter_set_foreground_color (painter, tmp_color);

		points[0].x = x;
		points[0].y = y + top;
		points[1].x = x + double_line_width;
		points[1].y = y + (top - top / 3);
		points[2].x = x + double_line_width;
		points[2].y = y + h - (bottom - bottom / 3);
		points[3].x = x;
		points[3].y = y + h - bottom;

		html_painter_draw_polygon (painter, TRUE, points, 4);

		points[0].x = x + w - double_line_width;
		points[0].y = y + top / 3;
		points[1].x = x + w;
		points[1].y = y;
		points[2].x = x + w;
		points[2].y = y + h;
		points[3].x = x + w - double_line_width;
		points[3].y = y + h - bottom / 3;

		html_painter_draw_polygon (painter, TRUE, points, 4);

		return;
		break;

	case HTML_BORDER_STYLE_GROOVE:
	case HTML_BORDER_STYLE_RIDGE:
		if (style->border->bottom.border_style == HTML_BORDER_STYLE_GROOVE) {
			dark_color = html_color_transform (tmp_color, DARKER_SCALE);
			light_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		}
		else {
			light_color = html_color_transform (tmp_color, DARKER_SCALE);
			dark_color = html_color_transform (tmp_color, BRIGHTER_SCALE);
		}
		html_painter_set_foreground_color (painter, dark_color);

		points[0].x = x;
		points[0].y = y + top;
		points[1].x = x + w / 2;
		points[1].y = y + (top / 2);
		points[2].x = x + w / 2;
		points[2].y = y + h - bottom / 2;
		points[3].x = x;
		points[3].y = y + h - bottom;

		html_painter_draw_polygon (painter, TRUE, points, 4);

		html_painter_set_foreground_color (painter, light_color);
		points[0].x = x + w / 2;
		points[0].y = y + (top / 2);
		points[1].x = x + w;
		points[1].y = y;
		points[2].x = x + w;
		points[2].y = y + h;
		points[3].x = x + w / 2;
		points[3].y = y + h - (bottom / 2);

		html_painter_draw_polygon (painter, TRUE, points, 4);
		html_color_unref (dark_color);
		html_color_unref (light_color);
		return;
		break;
	default:
		g_print ("unknown border style\n");
	}

	points[0].x = x;
	points[0].y = y + top;
	points[1].x = x + w;
	points[1].y = y;
	points[2].x = x + w;
	points[2].y = y + h;
	points[3].x = x;
	points[3].y = y + h - bottom;

	html_painter_draw_polygon (painter, TRUE, points, 4);
}

void
html_style_painter_draw_outline (HtmlBox *self, HtmlStyle *style, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	gint width, x1, y1, x2, y2, x3, y3, x4, y4, linew, up, down;
	gint8 array[2] = {0, 0};

	if (style->outline->style == HTML_BORDER_STYLE_NONE ||
	    style->outline->style == HTML_BORDER_STYLE_HIDDEN ||
	    style->outline->width == 0)
		return;

	if (self->width == 0 && self->height == 0)
		return;
#if 0
	if (HTML_IS_BOX_TEXT (self))
		return;
#endif
	width = html_box_get_containing_block_width (self);
	
	linew = style->outline->width;

	up = (linew + 1) / 2;
	down = linew / 2;

	x1 = tx + self->x + html_box_left_margin (self, width);
	x3 = x1;
	y1 = ty + self->y + html_box_top_margin (self, width);
	y2 = y1;
	x2 = tx + self->x + self->width - html_box_right_margin (self, width) - 
		html_box_left_margin (self, width);
	x4 = x2;
	y3 = ty + self->y + self->height - html_box_top_margin (self, width) - 
		html_box_bottom_margin (self, width);
	y4 = y3;
	
	if (style->outline->color)
		html_painter_set_foreground_color (painter, style->outline->color);
	else
		gdk_gc_set_function (HTML_GDK_PAINTER (painter)->gc, GDK_INVERT);

	switch (style->outline->style) {

	case HTML_BORDER_STYLE_SOLID:

		gdk_gc_set_line_attributes (HTML_GDK_PAINTER (painter)->gc,
					    linew,
					    GDK_LINE_SOLID,
					    GDK_CAP_BUTT,
					    GDK_JOIN_MITER);

		break;
	case HTML_BORDER_STYLE_DASHED:
	case HTML_BORDER_STYLE_DOTTED:

		set_up_dash_or_dot_array (array, style->outline->style == HTML_BORDER_STYLE_DOTTED, linew);
		
		gdk_gc_set_dashes (HTML_GDK_PAINTER (painter)->gc,
				   0, array, 2);
		
		gdk_gc_set_line_attributes (HTML_GDK_PAINTER (painter)->gc,
					    linew,
					    GDK_LINE_ON_OFF_DASH,
					    GDK_CAP_BUTT,
					    GDK_JOIN_MITER);
		break;
	default:
		g_warning ("unknown outline style");
	}
	gdk_draw_line (HTML_GDK_PAINTER (painter)->window,
		       HTML_GDK_PAINTER (painter)->gc, x1, y1 + down, 
		       x2 - linew, y2 + down);

	gdk_draw_line (HTML_GDK_PAINTER (painter)->window,
		       HTML_GDK_PAINTER (painter)->gc, x1 + down , y1 + linew, 
		       x3 + down, y3 - linew);

	gdk_draw_line (HTML_GDK_PAINTER (painter)->window,
		       HTML_GDK_PAINTER (painter)->gc, x3, y3 - up, 
		       x4 - linew, y4 - up);
	gdk_draw_line (HTML_GDK_PAINTER (painter)->window,
		       HTML_GDK_PAINTER (painter)->gc, x4 - up, y4, 
		       x2 - up, y2);

	
	gdk_gc_set_function (HTML_GDK_PAINTER (painter)->gc, GDK_COPY);
}

void
html_style_painter_draw_border (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	gint height = self->height;

	/* Borders on text boxes are a special case */
	if (HTML_IS_BOX_TEXT (self)) {
		if (HTML_IS_BOX_INLINE (self->parent)) {
			
			if (HTML_BOX_GET_STYLE (self->parent)->visibility != HTML_VISIBILITY_VISIBLE)
				return;
			
			html_style_painter_draw_top_border (self, HTML_BOX_GET_STYLE (self->parent), painter, area, tx, ty - 
							    HTML_BOX_GET_STYLE (self->parent)->border->top.width, self->prev == NULL, self->next == NULL);
			if (self->prev == NULL)
				html_style_painter_draw_left_border (self, HTML_BOX_GET_STYLE (self->parent), painter, area, tx, ty -
								     HTML_BOX_GET_STYLE (self->parent)->border->top.width, height +
								     HTML_BOX_GET_STYLE (self->parent)->border->top.width +
								     HTML_BOX_GET_STYLE (self->parent)->border->bottom.width);
			
			if (self->next == NULL)
				html_style_painter_draw_right_border (self, HTML_BOX_GET_STYLE (self->parent), painter, area, tx, ty -
								      HTML_BOX_GET_STYLE (self->parent)->border->top.width, height +
								      HTML_BOX_GET_STYLE (self->parent)->border->top.width +
								      HTML_BOX_GET_STYLE (self->parent)->border->bottom.width);
			
			html_style_painter_draw_bottom_border (self, HTML_BOX_GET_STYLE (self->parent), painter, area, tx, ty +
							       HTML_BOX_GET_STYLE (self->parent)->border->top.width, self->prev == NULL, self->next == NULL);
		}
		return;
	}

	if (HTML_BOX_GET_STYLE (self)->visibility != HTML_VISIBILITY_VISIBLE)
		return;
	switch (HTML_BOX_GET_STYLE (self)->display) {
	case HTML_DISPLAY_TABLE:
	case HTML_DISPLAY_BLOCK:
	case HTML_DISPLAY_TABLE_ROW:
	case HTML_DISPLAY_TABLE_CELL:
	case HTML_DISPLAY_TABLE_CAPTION:
		html_style_painter_draw_top_border (self, HTML_BOX_GET_STYLE (self), painter, area, tx, ty, TRUE, TRUE);
		html_style_painter_draw_left_border (self, HTML_BOX_GET_STYLE (self), painter, area, tx, ty, height);
		html_style_painter_draw_right_border (self, HTML_BOX_GET_STYLE (self), painter, area, tx, ty, height);
		html_style_painter_draw_bottom_border (self, HTML_BOX_GET_STYLE (self), painter, area, tx, ty, TRUE, TRUE);
		break;
	default:
		break;
	}
}

#define MIN_TILE_SIZE 128

void
html_style_painter_draw_background_image (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlStyle *style = HTML_BOX_GET_STYLE (self);
	gint x_offset, y_offset;
	GdkPixbuf *pixbuf;
	gint width  = self->width;
	gint height = self->height;
	gint real_width, real_height;
	gint x = tx + self->x;
	gint y = ty + self->y;
	gint tile_x, tile_y = 0;
	GdkRectangle rect, paint;
	gboolean unref_pixbuf = FALSE;
	HtmlBackgroundRepeatType repeat = style->background->repeat;

	/* No image to paint */
	if (style->background->image == NULL || style->background->image->pixbuf == NULL)
		return;

	pixbuf = style->background->image->pixbuf;

	if (width == 0 || height == 0)
		return;

	real_width  = gdk_pixbuf_get_width (pixbuf);
	real_height = gdk_pixbuf_get_height (pixbuf);

	if (repeat == HTML_BACKGROUND_REPEAT_REPEAT || 
	    repeat == HTML_BACKGROUND_REPEAT_REPEAT_X || 
	    repeat == HTML_BACKGROUND_REPEAT_REPEAT_Y) {
		rect.x = x;
		rect.y = y;

		switch (repeat) {
		case HTML_BACKGROUND_REPEAT_REPEAT:
			rect.width = width;
			rect.height = height;
			break;
		case HTML_BACKGROUND_REPEAT_REPEAT_X:
			rect.width = width;
			rect.height = MIN (height, real_height);
			break;
		case HTML_BACKGROUND_REPEAT_REPEAT_Y:
			rect.width = MIN (width, real_width);
			rect.height = height;
			break;
		default:
			g_warning("html_style_painter_draw_background_image: impossible\n");
			return;
		}


		if (gdk_rectangle_intersect (area, &rect, &paint) == FALSE)
			return;

		tile_x = (paint.x - x) % real_width;
		tile_y = (paint.y - y) % real_height;
		
		width = (paint.width) + tile_x;
		height = (paint.height) + tile_y;
		
		x = paint.x - tile_x;
		y = paint.y - tile_y;
	}


	if ((repeat == HTML_BACKGROUND_REPEAT_REPEAT || 
	     repeat == HTML_BACKGROUND_REPEAT_REPEAT_X || 
	     repeat == HTML_BACKGROUND_REPEAT_REPEAT_Y) &&
	    ((paint.width > MIN_TILE_SIZE && real_width < MIN_TILE_SIZE) || 
	     (paint.height > MIN_TILE_SIZE && real_height < MIN_TILE_SIZE))) {

		gint new_real_width, new_real_height;
		gint x_times = MIN_TILE_SIZE / real_width;
		gint y_times = MIN_TILE_SIZE / real_height;
		GdkPixbuf *tmp_pixbuf;

		if (x_times < 1)
			x_times = 1;
		if (y_times < 1)
			y_times = 1;

		new_real_width = real_width * x_times;
		new_real_height = real_height * y_times;
		
		tmp_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 
					     gdk_pixbuf_get_has_alpha (pixbuf),
					     gdk_pixbuf_get_bits_per_sample (pixbuf),
					     new_real_width, new_real_height);

		while (y_times--) {
			gint old_x_times = x_times;

			while (x_times--)
				gdk_pixbuf_copy_area (pixbuf, 0, 0, real_width, real_height, tmp_pixbuf, x_times * real_width, y_times * real_height);

			x_times = old_x_times;
		}
		real_width = new_real_width;
		real_height = new_real_height;
		unref_pixbuf = TRUE; /* Tell the code at the end of this function to free this pixbuf */
		pixbuf = tmp_pixbuf;
	}
	switch (repeat) {
	case HTML_BACKGROUND_REPEAT_NO_REPEAT: 
		html_painter_draw_pixbuf (painter, area, pixbuf, 0, 0, x, y,
					  width > real_width ? real_width : width,
					  height > real_height ? real_height : height);
		break;
		
	case HTML_BACKGROUND_REPEAT_SCALE:
		
		if (width == real_width && height == real_height) {
			html_painter_draw_pixbuf (painter, area, pixbuf, 0, 0,
						  x, y, real_width, real_height);
		}
		else {
			GdkPixbuf *tmp_pixbuf;
			tmp_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, 
						     gdk_pixbuf_get_has_alpha (pixbuf),
						     gdk_pixbuf_get_bits_per_sample (pixbuf),
						     width, height);
			
			g_assert (tmp_pixbuf);

			gdk_pixbuf_scale (pixbuf, tmp_pixbuf, 0, 0, width, height, 0.0, 0.0, 
					  (gdouble)width / (gdouble)real_width, (gdouble)height / (gdouble)real_height, GDK_INTERP_BILINEAR);
			
			html_painter_draw_pixbuf (painter, area, tmp_pixbuf, 0, 0,
						  x, y, width, height);

			gdk_pixbuf_unref (tmp_pixbuf);
		}
		break;

	case HTML_BACKGROUND_REPEAT_REPEAT_X:
		x_offset = 0;
		while (width > 0) {
			html_painter_draw_pixbuf (painter, area, pixbuf, 0, 0, x + x_offset, y, 
						  width > real_width ? real_width : width, 
						  height > real_height ? real_height : height);

			x_offset += real_width;
			width -= real_width;
		}
		break;

	case HTML_BACKGROUND_REPEAT_REPEAT_Y:
		y_offset = tile_y;

		while (height > 0) {
			x_offset = 0;
			html_painter_draw_pixbuf (painter, area, pixbuf, 0, 0, x, y + y_offset, 
						  width > real_width ? real_width : width, 
						  height > real_height ? real_height : height);

			y_offset += real_height;
			height -= real_height;
		}
		break;

	case HTML_BACKGROUND_REPEAT_REPEAT:
		y_offset = 0;

		while (height > 0) {
			gint tmp_width = width;
			x_offset = 0;
			while (tmp_width > 0) {
				html_painter_draw_pixbuf (painter, area, pixbuf, 0, 0,
						  x + x_offset, y + y_offset, 
						  tmp_width > real_width ? real_width : tmp_width, 
						  height > real_height ? real_height : height);

				x_offset += real_width;
				tmp_width -= real_width;
			}
			y_offset += real_height;
			height -= real_height;
		}
		break;
	}
	if (unref_pixbuf)
		gdk_pixbuf_unref (pixbuf);
}
