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

#include "htmlstyle.h"

HtmlStyleBackground *
html_style_background_ref (HtmlStyleBackground *background)
{
	background->refcount++;
	return background;
}

void
html_style_background_unref (HtmlStyleBackground *background)
{
	if (!background)
		return;

	background->refcount--;

	if (background->refcount <= 0) {
		if (background->image)
			g_object_unref (G_OBJECT (background->image));
		g_free (background);
	}
}

void
html_style_set_style_background (HtmlStyle *style, HtmlStyleBackground *background)
{
	if (style->background == background)
		return;

	if (style->background)
		html_style_background_unref (style->background);

	if (background) {
		style->background = background;
		html_style_background_ref (style->background);
	}
}

HtmlStyleBackground *
html_style_background_new (void)
{
	HtmlStyleBackground *result = g_new0 (HtmlStyleBackground, 1);
	result->color.transparent = TRUE;
	return result;
}

HtmlStyleBackground *
html_style_background_dup (HtmlStyleBackground *background)
{
	HtmlStyleBackground *result = html_style_background_new ();

	if (background) {
 		memcpy (result, background, sizeof (HtmlStyleBackground));

		result->refcount = 0;

		if (background->image)
		    result->image = g_object_ref (background->image);
	}
	return result;
}

void
html_style_set_background_color (HtmlStyle *style, HtmlColor *color)
{
	if (!html_color_equal (&style->background->color, color)) {
		if (style->background->refcount > 1)
			html_style_set_style_background (style, html_style_background_dup (style->background));
		style->background->color.transparent = color->transparent;
		style->background->color.red = color->red;
		style->background->color.green = color->green;
		style->background->color.blue = color->blue;
	}
}

void
html_style_set_background_image (HtmlStyle *style, HtmlImage *image)
{
	if (style->background->image != image) {

		if (style->background->refcount > 1)
			html_style_set_style_background (style, html_style_background_dup (style->background));
		style->background->image = g_object_ref (G_OBJECT (image));
	}
}

void
html_style_set_background_repeat (HtmlStyle *style, HtmlBackgroundRepeatType repeat)
{
	if (style->background->repeat != repeat) {

		if (style->background->refcount > 1)
			html_style_set_style_background (style, html_style_background_dup (style->background));
		style->background->repeat = repeat;
	}
}
