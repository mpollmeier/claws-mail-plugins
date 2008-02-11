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
#include <glib.h>
#include <gdk/gdk.h>

#include "css/cssvalue.h"
#include "layout/htmlstyle.h"
#include "util/htmlglobalatoms.h"

void
html_style_inherited_ref (HtmlStyleInherited *inherited)
{
	inherited->refcount++;
}

void
html_style_inherited_unref (HtmlStyleInherited *inherited)
{
	if (!inherited)
		return;

	inherited->refcount--;

	if (inherited->refcount <= 0) {

		if (inherited->font_spec)
			html_font_specification_unref (inherited->font_spec);
		if (inherited->color)
			html_color_unref (inherited->color);

		g_free (inherited);
	}
}

void
html_style_set_style_inherited (HtmlStyle *style, HtmlStyleInherited *inherited)
{
	if (style->inherited == inherited)
		return;

	if (style->inherited)
		html_style_inherited_unref (style->inherited);

	if (inherited) {
		style->inherited = inherited;
		html_style_inherited_ref (style->inherited);
	}
}

HtmlStyleInherited *
html_style_inherited_new (void)
{
	HtmlStyleInherited *result = g_new0 (HtmlStyleInherited, 1);
	return result;
}

HtmlStyleInherited *
html_style_inherited_dup (HtmlStyleInherited *inherited)
{
	HtmlStyleInherited *result = html_style_inherited_new ();

	if (inherited)
		memcpy (result, inherited, sizeof (HtmlStyleInherited));

	result->refcount = 0;

	/* FIXME: can font_spec be NULL here? */
	if (result->font_spec)
		html_font_specification_ref (result->font_spec);
	if (inherited->color)
		result->color = html_color_ref (inherited->color);
	
	return result;
}

void
html_style_set_color (HtmlStyle *style, HtmlColor *color)
{
	if (!html_color_equal (style->inherited->color, color)) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));

		if (style->inherited->color)
			html_color_unref (style->inherited->color);

		style->inherited->color = html_color_dup (color);
	}
}

void
html_style_set_direction (HtmlStyle *style, HtmlDirectionType direction)
{
	if (style->inherited->direction != direction) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->direction = direction;		
	}
}

void
html_style_set_bidi_level (HtmlStyle *style, int level)
{
	if (style->inherited->bidi_level != level) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->bidi_level = level;
	}
}

void
html_style_set_text_decoration (HtmlStyle *style, HtmlFontDecorationType decoration)
{
	if (!(style->inherited->font_spec->decoration & decoration) ||
	    decoration == HTML_FONT_DECORATION_NONE) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));

		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		if (decoration == HTML_FONT_DECORATION_NONE)
			style->inherited->font_spec->decoration = decoration;
		else
			style->inherited->font_spec->decoration |= decoration;
	}
}

void
html_style_set_font_variant (HtmlStyle *style, HtmlFontVariantType variant)
{
  	if ((style->inherited->font_spec->variant != variant)) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));

		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		style->inherited->font_spec->variant = variant;
	}
}

void
html_style_set_font_weight (HtmlStyle *style, HtmlFontWeightType weight)
{
  	if ((style->inherited->font_spec->weight != weight)) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));

		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		style->inherited->font_spec->weight = weight;
	}
}

void
html_style_set_font_weight_bolder (HtmlStyle *style)
{
  	if ((style->inherited->font_spec->weight != HTML_FONT_WEIGHT_900)) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));

		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		style->inherited->font_spec->weight++;
	}
}

void
html_style_set_font_weight_lighter (HtmlStyle *style)
{
  	if ((style->inherited->font_spec->weight != HTML_FONT_WEIGHT_100)) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));

		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		style->inherited->font_spec->weight--;
	}
}

void
html_style_set_font_style (HtmlStyle *style, HtmlFontStyleType font_style)
{
  	if ((style->inherited->font_spec->style != font_style)) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));

		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		style->inherited->font_spec->style = font_style;
	}
}

void
html_style_set_font_family (HtmlStyle *style, const gchar *family)
{
  	if (strcasecmp (style->inherited->font_spec->family, family) != 0) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));

		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		g_free (style->inherited->font_spec->family);
		style->inherited->font_spec->family = g_strdup (family);
	}
}

void
html_style_set_font_stretch (HtmlStyle *style, HtmlFontStretchType type)
{
	if ((style->inherited->font_spec->stretch != type)) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		
		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		style->inherited->font_spec->stretch = type;
	}
}

static gboolean
get_new_font_size (HtmlStyle *style, CssValue *val, HtmlFontSpecification *font_spec, gint *size)
{
	gint old_size = font_spec ? font_spec->size : 0;
	/* FIXME: Should probably be xdpi and ydpi */
	static gdouble dpi = 0.0;

	if (dpi == 0.0)
		dpi = (((double) gdk_screen_width () * 25.4) / 
		       ((double) gdk_screen_width_mm ()));
	
	/* "font-size: inherit" */
	if (val->v.atom == HTML_ATOM_INHERIT) {
		*size = old_size;
		return TRUE;
	}

	switch (val->value_type) {
	case CSS_PX:
		*size = (gint) (72 * val->v.d / dpi);
		return TRUE;
	case CSS_PT:
		*size = (gint) val->v.d;
		return TRUE;
	case CSS_PERCENTAGE:
		*size = (gint) (val->v.d * old_size / 100);
		return TRUE;
	case CSS_NUMBER:
		*size = (gint) val->v.d;
		return TRUE;
	case CSS_EMS:
	case CSS_EXS:
		*size = (gint) (val->v.d * old_size);
		return TRUE;
	default:
		switch (val->v.atom) {
		/* FIXME: These values are copy/pasted from html.css.h, X_SMALL is guessed */
		case HTML_ATOM_XX_SMALL:
			*size = (gint) (0.67 * old_size);
			return TRUE;
		case HTML_ATOM_X_SMALL:
			*size = (gint) (0.75 * old_size);
			return TRUE;
		case HTML_ATOM_SMALL:
			*size = (gint) (0.83 * old_size);
			return TRUE;
		case HTML_ATOM_MEDIUM:
			*size = (gint) (1 * old_size);
			return TRUE;
		case HTML_ATOM_LARGE:
			*size = (gint) (1.17 * old_size);
			return TRUE;
		case HTML_ATOM_X_LARGE:
			*size = (gint) (1.5 * old_size);
			return TRUE;
		case HTML_ATOM_XX_LARGE:
			*size = (gint) (2 * old_size);
			return TRUE;
		/* FIXME: These are absolutely guessed. Size should always be one of the above seven. */
                case HTML_ATOM_LARGER:
                        *size = (gint) (0.8 * old_size);
                        return TRUE;
                case HTML_ATOM_SMALLER:
                        *size = (gint) (1.2 * old_size);
                        return TRUE;
		}
		return FALSE;
	}
}

static void
html_style_set_font_size_common (HtmlStyle *style, gint size)
{
	if ((style->inherited->font_spec->size != size)) {
		HtmlFontSpecification *spec = style->inherited->font_spec;
		
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		
		style->inherited->font_spec = html_font_specification_dup (spec);
		html_font_specification_unref (spec);
		
		style->inherited->font_spec->size = size;
	}
}

void
html_style_set_font_size_html (HtmlStyle *style, gint htmlsize)
{
	gint size = html_font_description_html_size_to_pt (htmlsize);

	html_style_set_font_size_common (style, size);
}

void
html_style_set_font_size (HtmlStyle *style, HtmlFontSpecification *font_spec, CssValue *val)
{
	gint size;
	/* FIXME: implement this: */
#if 0
	if (val->v.atom == HTML_ATOM_INHERIT)
		size = style->parent->inherited->font_spec->size;
#endif
	if (!get_new_font_size (style, val, font_spec, &size))
		return;

	html_style_set_font_size_common (style, size);
}

void
html_style_set_line_height (HtmlStyle *style, HtmlFontSpecification *font_spec, CssValue *val)
{
	gint size;
	/* FIXME: implement this: */
#if 0
	if (val->v.atom == HTML_ATOM_INHERIT)
		size = style->parent->inherited->line_height;
#endif
	if (!get_new_font_size (style, val, font_spec, &size))
		return;

	if ((style->inherited->line_height != size)) {
		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->line_height = size;
	}
}

void
html_style_set_cursor (HtmlStyle *style, HtmlCursorType cursor)
{
	if (style->inherited->cursor != cursor) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->cursor = cursor;		
	}
}

void
html_style_set_border_spacing (HtmlStyle *style, gushort horiz, gushort vert)
{
	if (style->inherited->border_spacing_horiz != horiz || style->inherited->border_spacing_vert  != vert) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->border_spacing_horiz = horiz;
		style->inherited->border_spacing_vert = vert;
	}
}

void
html_style_set_caption_side (HtmlStyle *style, HtmlCaptionSideType type)
{
	if (style->inherited->caption_side != type) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->caption_side = type;
	}
}

void
html_style_set_text_align (HtmlStyle *style, HtmlTextAlignType type)
{
	if (style->inherited->text_align != type) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->text_align = type;
	}
}

void
html_style_set_white_space (HtmlStyle *style, HtmlWhiteSpaceType type)
{
	if (style->inherited->white_space != type) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->white_space = type;
	}
}

void
html_style_set_list_style_type (HtmlStyle *style, HtmlListStyleTypeType type)
{
	if (style->inherited->list_style_type != type) {

		if (style->inherited->refcount > 1)
			html_style_set_style_inherited (style, html_style_inherited_dup (style->inherited));
		style->inherited->list_style_type = type;
	}
}
