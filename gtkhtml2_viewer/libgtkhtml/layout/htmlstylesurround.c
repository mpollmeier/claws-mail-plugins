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

void
html_style_surround_ref (HtmlStyleSurround *surround)
{
	surround->refcount++;
}

void
html_style_surround_unref (HtmlStyleSurround *surround)
{
	if (!surround)
		return;

	surround->refcount--;

	if (surround->refcount <= 0)
		g_free (surround);
}

void
html_style_set_style_surround (HtmlStyle *style, HtmlStyleSurround *surround)
{
	if (style->surround == surround)
		return;

	if (style->surround)
		html_style_surround_unref (style->surround);

	if (surround) {
		style->surround = surround;
		html_style_surround_ref (style->surround);
	}
}

HtmlStyleSurround *
html_style_surround_new (void)
{
	HtmlStyleSurround *result = g_new0 (HtmlStyleSurround, 1);
	return result;
}

HtmlStyleSurround *
html_style_surround_dup (HtmlStyleSurround *surround)
{
	HtmlStyleSurround *result = html_style_surround_new ();

	if (surround)
		memcpy (result, surround, sizeof (HtmlStyleSurround));

	result->refcount = 0;

	return result;
}

void
html_style_set_position_top (HtmlStyle *style, const HtmlLength *length)
{
	if (!html_length_equals (&style->surround->position.top, length)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->position.top, length);
	}
}

void
html_style_set_position_right (HtmlStyle *style, const HtmlLength *length)
{
	if (!html_length_equals (&style->surround->position.right, length)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->position.right, length);
	}
}

void
html_style_set_position_bottom (HtmlStyle *style, const HtmlLength *length)
{
	if (!html_length_equals (&style->surround->position.bottom, length)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->position.bottom, length);
	}
}

void
html_style_set_position_left (HtmlStyle *style, const HtmlLength *length)
{
	if (!html_length_equals (&style->surround->position.left, length)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->position.left, length);
	}
}

void
html_style_set_margin_top (HtmlStyle *style, const HtmlLength *margin)
{
	if (!html_length_equals (&style->surround->margin.top, margin)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->margin.top, margin);
	}
}

void
html_style_set_margin_bottom (HtmlStyle *style, const HtmlLength *margin)
{
	if (!html_length_equals (&style->surround->margin.bottom, margin)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->margin.bottom, margin);
	}
}

void
html_style_set_margin_left (HtmlStyle *style, const HtmlLength *margin)
{
	if (!html_length_equals (&style->surround->margin.left, margin)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		if (margin->value >= 0)
			html_length_set (&style->surround->margin.left, margin);
	}
}

void
html_style_set_margin_right (HtmlStyle *style, const HtmlLength *margin)
{
	if (!html_length_equals (&style->surround->margin.right, margin)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->margin.right, margin);
	}
}

void
html_style_set_padding_left (HtmlStyle *style, const HtmlLength *padding)
{
	if (!html_length_equals (&style->surround->padding.left, padding)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->padding.left, padding);
	}
}

void
html_style_set_padding_right (HtmlStyle *style, const HtmlLength *padding)
{
	if (!html_length_equals (&style->surround->padding.right, padding)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->padding.right, padding);
	}
}

void
html_style_set_padding_top (HtmlStyle *style, const HtmlLength *padding)
{
	if (!html_length_equals (&style->surround->padding.top, padding)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->padding.top, padding);
	}
}

void
html_style_set_padding_bottom (HtmlStyle *style, const HtmlLength *padding)
{
	if (!html_length_equals (&style->surround->padding.bottom, padding)) {
		if (style->surround->refcount > 1)
			html_style_set_style_surround (style, html_style_surround_dup (style->surround));
		html_length_set (&style->surround->padding.bottom, padding);
	}
}
