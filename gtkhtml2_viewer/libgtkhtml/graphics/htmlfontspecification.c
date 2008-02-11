/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 Red Hat Software
   
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


#include <string.h>

#include "htmlfontspecification.h"

static gfloat html_font_size[7];

typedef struct _HtmlFontSpecificationPrivate HtmlFontSpecificationPrivate;

struct _HtmlFontSpecificationPrivate {
	HtmlFontSpecification base;
	gint ref_count;
};

static gboolean html_font_spec_is_initialized = FALSE;

static void
html_font_specification_init_sizes ()
{
	gfloat base_size = 14.0;
	PangoFontDescription *font_desc;
	GtkSettings *settings;
	gchar *font_name;

	settings = gtk_settings_get_default ();
	g_object_get (G_OBJECT (settings), "gtk-font-name", &font_name, NULL);
	font_desc = pango_font_description_from_string (font_name);
	g_free (font_name);
	
	if (font_desc) { 
		base_size = pango_font_description_get_size (font_desc) / PANGO_SCALE;
		pango_font_description_free (font_desc);
		
	}

	html_font_size[0] = base_size * 0.5;
	html_font_size[1] = base_size * 0.65;
	html_font_size[2] = base_size * 0.8;
	html_font_size[3] = base_size;
	html_font_size[4] = base_size * 1.2;
	html_font_size[5] = base_size * 1.4;
	html_font_size[6] = base_size * 1.7;
	html_font_spec_is_initialized = TRUE;
}

HtmlFontSpecification *
html_font_specification_new (gchar *family,
			     HtmlFontStyleType style, 
			     HtmlFontVariantType variant,
			     HtmlFontWeightType weight,
			     HtmlFontStretchType stretch, 
			     HtmlFontDecorationType decoration,
			     gfloat size)
{
	HtmlFontSpecificationPrivate *spec;

	spec = g_new (HtmlFontSpecificationPrivate, 1);
	spec->base.family = g_strdup (family);
	spec->base.style = style;
	spec->base.variant = variant;
	spec->base.weight = weight;
	spec->base.stretch = stretch;
	spec->base.decoration = decoration;
	spec->base.size = size;
	spec->ref_count = 1;

	if (!html_font_spec_is_initialized) html_font_specification_init_sizes();
	return (HtmlFontSpecification *)spec;
}

HtmlFontSpecification *
html_font_specification_ref (HtmlFontSpecification *spec)
{
	HtmlFontSpecificationPrivate *private = (HtmlFontSpecificationPrivate *)spec;

	private->ref_count++;
	return spec;
}

void
html_font_specification_unref (HtmlFontSpecification *spec)
{
	HtmlFontSpecificationPrivate *private = (HtmlFontSpecificationPrivate *)spec;

	if (private->ref_count > 1) {
		private->ref_count--;
		
	} else {
		g_free (spec->family);
		g_free (spec);
	}
}

HtmlFontSpecification *
html_font_specification_dup (HtmlFontSpecification *spec)
{
	HtmlFontSpecificationPrivate *new;
	new = g_new (HtmlFontSpecificationPrivate, 1);
	new->base = *spec;
	new->base.family = g_strdup (spec->family);
	new->ref_count = 1;

	return (HtmlFontSpecification *)new;
}

static PangoStyle pango_style[] = {
	PANGO_STYLE_NORMAL,
	PANGO_STYLE_ITALIC,
	PANGO_STYLE_OBLIQUE,
};

static PangoVariant pango_variant[] = {
	PANGO_VARIANT_NORMAL,
	PANGO_VARIANT_SMALL_CAPS,
};

static PangoWeight pango_weight[] = {
	100,
	200,
	300,
	400,
	500,
	600,
	700,
	800,
	900,
};

static PangoStretch pango_stretch[] = {
	PANGO_STRETCH_NORMAL,
	PANGO_STRETCH_ULTRA_CONDENSED,
	PANGO_STRETCH_EXTRA_CONDENSED,
	PANGO_STRETCH_CONDENSED,
	PANGO_STRETCH_SEMI_CONDENSED,
	PANGO_STRETCH_SEMI_EXPANDED,
	PANGO_STRETCH_EXPANDED,
	PANGO_STRETCH_EXTRA_EXPANDED,
	PANGO_STRETCH_ULTRA_EXPANDED,
};

PangoFontDescription *
html_font_specification_get_pango_font_description (HtmlFontSpecification *spec)
{
	PangoFontDescription *desc;

	desc = pango_font_description_new ();
	
        /* Map between HTML and Pango name for mono-spaced font. */
        if (!strcmp (spec->family, "monospace"))
                pango_font_description_set_family (desc, "mono");
        else
		pango_font_description_set_family (desc, spec->family);

	pango_font_description_set_style (desc, pango_style[spec->style]);
	pango_font_description_set_variant (desc, pango_variant[spec->variant]);
	pango_font_description_set_weight (desc, pango_weight[spec->weight]);
	pango_font_description_set_stretch (desc, pango_stretch[spec->stretch]);
	pango_font_description_set_size (desc, spec->size * PANGO_SCALE);

	return desc;
}

gboolean
html_font_description_equal (HtmlFontSpecification *a, HtmlFontSpecification *b)
{
	if (strcmp (a->family, b->family) != 0)
		return FALSE;

	if (a->size != b->size)
		return FALSE;
	
	if (a->weight != b->weight)
		return FALSE;
	
	if (a->style != b->style)
		return FALSE;
	
	if (a->variant != b->variant)
		return FALSE;
	
	if (a->decoration != b->decoration)
		return FALSE;

	return TRUE;
}


void
html_font_specification_get_extra_attributes (HtmlFontSpecification *spec,
					      PangoAttrList *attrs,
					      gint start_index, gint end_index)
{
	PangoAttribute *attr;
	if (spec->decoration & HTML_FONT_DECORATION_UNDERLINE) {
		attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
		attr->start_index = start_index;
		attr->end_index = end_index;
		pango_attr_list_insert (attrs, attr);
	}
	if (spec->decoration & HTML_FONT_DECORATION_LINETHROUGH) {
		attr = pango_attr_strikethrough_new (TRUE);
		attr->start_index = start_index;
		attr->end_index = end_index;
		pango_attr_list_insert (attrs, attr);
	}
	if (spec->decoration & HTML_FONT_DECORATION_OVERLINE) {
		g_warning ("Overline fonts not supported by pango yet");
	}
}

void
html_font_specification_get_all_attributes (HtmlFontSpecification *spec,
					    PangoAttrList *attrs,
					    gint start_index, gint end_index,
					    gdouble magnification)
{
	PangoFontDescription *desc;
	PangoAttribute *attr;

	desc = html_font_specification_get_pango_font_description (spec);
	pango_font_description_set_size (desc, magnification * pango_font_description_get_size (desc));

	attr = pango_attr_font_desc_new (desc);
	attr->start_index = start_index;
	attr->end_index = end_index;
	pango_font_description_free (desc);

	pango_attr_list_insert (attrs, attr);
	
	html_font_specification_get_extra_attributes (spec, attrs, start_index, end_index);
}

gint
html_font_specification_get_html_size (HtmlFontSpecification *spec)
{
	gint i, best = 3;
	gint diff = ABS(spec->size - html_font_size[3]);
	
	for (i=0;i< 7;i++) {
		if (ABS (spec->size - html_font_size[i]) < diff) {
			best = i;
			diff = ABS (spec->size - html_font_size[i]);
		}
	}
	return best + 1;
}

gfloat
html_font_description_html_size_to_pt (gint font_size) 
{
	if (font_size < 1)
		font_size = 1;
	if (font_size > 7)
		font_size = 7;
	
	return html_font_size [font_size - 1];
}
