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

HtmlStyleOutline *
html_style_outline_ref (HtmlStyleOutline *outline)
{
	outline->refcount++;
	return outline;
}

void
html_style_outline_unref (HtmlStyleOutline *outline)
{
	if (!outline)
		return;

	outline->refcount--;

	if (outline->refcount <= 0) {
		if (outline->color)
			html_color_unref (outline->color);
		g_free (outline);
	}
}

void
html_style_set_style_outline (HtmlStyle *style, HtmlStyleOutline *outline)
{
	if (style->outline == outline)
		return;

	if (style->outline)
		html_style_outline_unref (style->outline);

	if (outline) {
		style->outline = html_style_outline_ref (outline);
	}
}

HtmlStyleOutline *
html_style_outline_new (void)
{
	return g_new0 (HtmlStyleOutline, 1);
}

HtmlStyleOutline *
html_style_outline_dup (HtmlStyleOutline *outline)
{
	HtmlStyleOutline *result = html_style_outline_new ();

	if (outline)
		memcpy (result, outline, sizeof (HtmlStyleOutline));

	result->refcount = 0;

	if (outline->color)
		result->color = html_color_ref (outline->color);

	return result;
}

void
html_style_set_outline_width (HtmlStyle *style, gint width)
{
	if (style->outline->width != width) {
		if (style->outline->refcount > 1)
			html_style_set_style_outline (style, html_style_outline_dup (style->outline));
		style->outline->width = width;
	}
}

void
html_style_set_outline_style (HtmlStyle *style, HtmlBorderStyleType outline_style)
{
	if (style->outline->style != outline_style) {
		if (style->outline->refcount > 1)
			html_style_set_style_outline (style, html_style_outline_dup (style->outline));
		style->outline->style = outline_style;
	}
}

void
html_style_set_outline_color (HtmlStyle *style, HtmlColor *color)
{
	if (!html_color_equal (style->outline->color, color)) {
		if (style->outline->refcount > 1)
			html_style_set_style_outline (style, html_style_outline_dup (style->outline));
		if (style->outline->color)
			html_color_unref (style->outline->color);
		if (color)
			style->outline->color = html_color_dup (color);
		else
			style->outline->color = NULL;
	}
}
