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
#include "graphics/htmlcolor.h"
#include "htmlstyle.h"

void
html_style_border_ref (HtmlStyleBorder *border)
{
	border->refcount++;
}

void
html_style_border_unref (HtmlStyleBorder *border)
{
	if (!border)
		return;

	border->refcount--;

	if (border->refcount <= 0) {
		if (border->top.color)
			html_color_unref (border->top.color);
		if (border->left.color)
			html_color_unref (border->left.color);
		if (border->right.color)
			html_color_unref (border->right.color);
		if (border->bottom.color)
			html_color_unref (border->bottom.color);
		g_free (border);
	}
}

void
html_style_set_style_border (HtmlStyle *style, HtmlStyleBorder *border)
{
	if (style->border == border)
		return;

	if (style->border)
		html_style_border_unref (style->border);

	if (border) {
		style->border = border;
		html_style_border_ref (style->border);
	}
}

HtmlStyleBorder *
html_style_border_new (void)
{
	HtmlStyleBorder *result = g_new0 (HtmlStyleBorder, 1);
	return result;
}

HtmlStyleBorder *
html_style_border_dup (HtmlStyleBorder *border)
{
	HtmlStyleBorder *result;

	result = html_style_border_new ();
	memcpy (result, border, sizeof (HtmlStyleBorder));

	result->refcount = 0;

	if (border->top.color)
		result->top.color = html_color_ref (border->top.color);
	if (border->left.color)
		result->left.color = html_color_ref (border->left.color);
	if (border->right.color)
		result->right.color = html_color_ref (border->right.color);
	if (border->bottom.color)
		result->bottom.color = html_color_ref (border->bottom.color);

	return result;
}

void
html_style_set_border_top_width (HtmlStyle *style, gint width)
{
	if (style->border->top.width != width) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		style->border->top.width = width;
	}
}

void
html_style_set_border_bottom_width (HtmlStyle *style, gint width)
{
	if (style->border->bottom.width != width) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		style->border->bottom.width = width;
	}
}

void
html_style_set_border_left_width (HtmlStyle *style, gint width)
{
	if (style->border->left.width != width) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		style->border->left.width = width;
	}
}

void
html_style_set_border_right_width (HtmlStyle *style, gint width)
{
	if (style->border->right.width != width) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		style->border->right.width = width;
	}
}

void
html_style_set_border_top_style (HtmlStyle *style, HtmlBorderStyleType border_style)
{
	if (style->border->top.border_style != border_style) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		style->border->top.border_style = border_style;
	}
}

void
html_style_set_border_bottom_style (HtmlStyle *style, HtmlBorderStyleType border_style)
{
	if (style->border->bottom.border_style != border_style) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		style->border->bottom.border_style = border_style;
	}
}

void
html_style_set_border_left_style (HtmlStyle *style, HtmlBorderStyleType border_style)
{
	if (style->border->left.border_style != border_style) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		style->border->left.border_style = border_style;
	}
}

void
html_style_set_border_right_style (HtmlStyle *style, HtmlBorderStyleType border_style)
{
	if (style->border->right.border_style != border_style) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		style->border->right.border_style = border_style;
	}
}

void
html_style_set_border_top_color (HtmlStyle *style, HtmlColor *color)
{
	if (!html_color_equal (style->border->top.color, color)) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		if (style->border->top.color)
			html_color_unref (style->border->top.color);
		style->border->top.color = html_color_dup (color);
	}
}

void
html_style_set_border_bottom_color (HtmlStyle *style, HtmlColor *color)
{
	if (!html_color_equal (style->border->bottom.color, color)) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		if (style->border->bottom.color)
			html_color_unref (style->border->bottom.color);
		style->border->bottom.color = html_color_dup (color);
	}
}

void
html_style_set_border_left_color (HtmlStyle *style, HtmlColor *color)
{
	if (!html_color_equal (style->border->left.color, color)) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		if (style->border->left.color)
			html_color_unref (style->border->left.color);
		style->border->left.color = html_color_dup (color);
	}
}

void
html_style_set_border_right_color (HtmlStyle *style, HtmlColor *color) {
	if (!html_color_equal (style->border->right.color, color)) {
		if (style->border->refcount > 1)
			html_style_set_style_border (style, html_style_border_dup (style->border));
		if (style->border->right.color)
			html_color_unref (style->border->right.color);
		style->border->right.color = html_color_dup (color);
	}
}
