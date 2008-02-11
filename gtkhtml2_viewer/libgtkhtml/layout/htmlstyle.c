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

#include <glib.h>
#include <string.h>

#include "css/cssvalue.h"
#include "view/htmlview.h"
#include "layout/htmlbox.h"
#include "layout/htmlstyle.h"
#include "util/htmlglobalatoms.h"

static HtmlStyle *default_style = NULL;

static GtkStyle *
html_style_get_gtk_style (void)
{
  	GtkStyle *style;

	style = gtk_rc_get_style_by_paths (gtk_settings_get_default(),
                                           "GtkTextView",
                                           "TextView",
                                           HTML_TYPE_VIEW);
	if (!style) {
		style = gtk_rc_get_style_by_paths (gtk_settings_get_default(),
                                                   "GtkHtml",
                                                   "HtmlView",
                                                   GTK_TYPE_TEXT_VIEW);
	}

	if (!style) {
		style = gtk_style_new ();
	} else {
		g_object_ref (style);
        }

        return style;
}

static PangoFontDescription*
html_style_get_gtk_font_desc (GtkStyle *style)
{
	return style->font_desc;
}

static HtmlColor*
html_style_get_gtk_text_color (GtkStyle *style)
{
	GdkColor  text_color;
	GdkColor  base_color;
	gushort linkred;
	gushort linkgreen;
	guint red_green;


	base_color = style->base[GTK_STATE_NORMAL];

	/*
	 * Heuristic to pick more visible link color
	 */
	red_green = base_color.red + base_color.green;
	if (red_green < 65535) {
		linkred = 153;
		linkgreen = 204;
	} else {
		linkred = 0;
		linkgreen = 0;
	}

	html_color_set_linkblue (linkred, linkgreen);
	text_color = style->text[GTK_STATE_NORMAL];

	return html_color_new_from_rgb (text_color.red,
					text_color.green,
					text_color.blue);
}

void
html_length_set_value (HtmlLength *length, gint value, HtmlLengthType type)
{
	length->type = type;
	length->value = value;
}

gint
html_length_get_value (const HtmlLength *length, gint base)
{
	switch (length->type) {
	case HTML_LENGTH_FIXED:
		return length->value;
	case HTML_LENGTH_PERCENT:
		return (length->value * base) / 100;
	default:
	case HTML_LENGTH_AUTO:
		return 0;
	}
}

gboolean
html_length_from_css_value (HtmlFontSpecification *font_spec, CssValue *val, HtmlLength *length)
{
	/* FIXME: Should probably be xdpi and ydpi */
	static gdouble dpi = 0.0;

	if (dpi == 0.0)
		dpi = (((double) gdk_screen_width () * 25.4) / 
		       ((double) gdk_screen_width_mm ()));
	
	if (val->v.atom == HTML_ATOM_AUTO) {
		length->value = 0;
		length->type = HTML_LENGTH_AUTO;
	}
	/* FIXME: This is not the correct way way to do this */
	else if (font_spec && 
		 (val->value_type == CSS_EMS ||
		  val->value_type == CSS_EXS)) {
		length->value = (gint) (val->v.d * font_spec->size);
		length->type = HTML_LENGTH_FIXED;
	}
	else if (val->value_type == CSS_PX ||
		 val->value_type == CSS_NUMBER) {
		length->value = (gint) val->v.d;
		length->type = HTML_LENGTH_FIXED;
	}
	else if (val->value_type == CSS_PERCENTAGE) {
		length->value = (gint) val->v.d;
		length->type = HTML_LENGTH_PERCENT;
	}
	else if (val->value_type == CSS_PT) {
		length->value = (gint) (val->v.d * dpi / 72);
		length->type = HTML_LENGTH_FIXED;
	}
	else if (val->value_type == CSS_PC) {
		length->value = (gint) (val->v.d * dpi * 12 / 72);
		length->type = HTML_LENGTH_FIXED;
	}
	else if (val->value_type == CSS_IN) {
		length->value = (gint) (val->v.d * dpi);
		length->type = HTML_LENGTH_FIXED;
	}
	else if (val->value_type == CSS_CM) {
		length->value = (gint) (val->v.d * dpi / 2.54);
		length->type = HTML_LENGTH_FIXED;
	}
	else if (val->value_type == CSS_MM) {
		length->value = (gint) (val->v.d * dpi / 25.4);
		length->type = HTML_LENGTH_FIXED;
	}
	else
		return FALSE;

	return TRUE;
}

gboolean
html_length_equals (const HtmlLength *length1, const HtmlLength *length2)
{
	if (length1->type == length2->type) {

		if (length1->type == HTML_LENGTH_AUTO || length1->value == length2->value)
			return TRUE;
	}
	return FALSE;
}

void
html_length_set (HtmlLength *length, const HtmlLength *length2)
{
	length->type = length2->type;
	length->value = length2->value;
}

HtmlStyle *
html_style_ref (HtmlStyle *style)
{
	style->refcount++;
	return style;
}

void
html_style_unref (HtmlStyle *style)
{
	if (style == NULL)
		return;

	style->refcount--;

	if (style->refcount <= 0) {

		/* FIXME: We do nothing here until I figure out the default style case /ac */
		html_style_box_unref (style->box);
		html_style_surround_unref (style->surround);
		html_style_inherited_unref (style->inherited);
		html_style_background_unref (style->background);
		html_style_outline_unref (style->outline);
		html_style_border_unref (style->border);
		/* FIXME, this should have a refcount */
		g_free (style->visual);
		if (style->content)
			g_free (style->content);
		g_free (style);
	}
}

HtmlStyle *
html_default_style_new (void)
{
	HtmlStyle *result = g_new0 (HtmlStyle, 1);
	HtmlStyleBox *box = html_style_box_new ();
	HtmlStyleSurround *surround = html_style_surround_new ();
	HtmlStyleInherited *inherited = html_style_inherited_new ();
	HtmlStyleBackground *background = html_style_background_new ();
	HtmlStyleBorder *border = html_style_border_new ();
	HtmlStyleOutline *outline = html_style_outline_new ();
	GtkStyle *gtk_style;
	PangoFontDescription *font_desc;
	const gchar *family;
	gfloat size;
	HtmlColor *color;

	/* Set refcount to 1 so it never will be deallocated */
	result->refcount = 1;
	
	html_style_set_style_box (result, box);
	html_style_set_style_inherited (result, inherited);
	html_style_set_style_surround (result, surround);
	html_style_set_style_background (result, background);
	html_style_set_style_border (result, border);
	html_style_set_style_outline (result, outline);

	html_style_set_border_top_width (result, HTML_BORDER_WIDTH_MEDIUM);
	html_style_set_border_bottom_width (result, HTML_BORDER_WIDTH_MEDIUM);
	html_style_set_border_left_width (result, HTML_BORDER_WIDTH_MEDIUM);
	html_style_set_border_right_width (result, HTML_BORDER_WIDTH_MEDIUM);
	
	html_style_set_outline_width (result, HTML_BORDER_WIDTH_MEDIUM);

	gtk_style = html_style_get_gtk_style ();
	font_desc = html_style_get_gtk_font_desc (gtk_style);
        
	family = pango_font_description_get_family (font_desc);
	size = pango_font_description_get_size (font_desc) / (gfloat)PANGO_SCALE;
        
	inherited->font_spec = 
                html_font_specification_new ((gchar *) family, 
                                             HTML_FONT_STYLE_NORMAL, 
                                             HTML_FONT_VARIANT_NORMAL, 
                                             HTML_FONT_WEIGHT_NORMAL, 
                                             HTML_FONT_STRETCH_NORMAL,
                                             HTML_FONT_DECORATION_NONE, 
                                             size);
	
	/*
	 * Set the default color to match the GTK+ theme  
	 * and free the HtmlColor
	 */
	color = html_style_get_gtk_text_color (gtk_style);
	html_style_set_color (result, color);
	html_color_unref (color);
	g_object_unref (gtk_style);
	
	return result;
}

static void
html_style_notify_settings (GObject *obj, GParamSpec *pspec)
{
	if (strcmp (pspec->name, "gtk-theme-name") == 0) {
		GtkStyle *gtk_style;
		HtmlColor *color;

		/*
		 * Update the default color
		 */
		gtk_style = html_style_get_gtk_style ();
		color =  html_style_get_gtk_text_color (gtk_style);
		g_object_unref (gtk_style);
		default_style->inherited->color->red =  color->red;
		default_style->inherited->color->green =  color->green;
		default_style->inherited->color->blue =  color->blue;
		g_free (color);
	}
}
                            
HtmlStyle *
html_style_new (HtmlStyle *parent)
{
	HtmlStyle *style = g_new0 (HtmlStyle, 1);

	if (!default_style) {
		default_style = html_default_style_new ();
		g_signal_connect (gtk_settings_get_default (),
				  "notify",
				  G_CALLBACK (html_style_notify_settings),
				  NULL);
	}

	/* FIXME: Eeeek, very ugly */
	
	/* FIXME: We should inherit these */
	style->visual = g_new0 (HtmlStyleVisual, 1);
	/*	style->box = g_new0 (HtmlStyleBox, 1);*/

	if (parent) {
		style->blink = parent->blink;
		html_style_set_style_inherited (style, parent->inherited);
	}
	else {
		html_style_set_style_inherited (style, default_style->inherited);
	}
	
	html_style_set_style_surround (style, default_style->surround);
	html_style_set_style_background (style, default_style->background);
	html_style_set_style_border (style, default_style->border);
	html_style_set_style_outline (style, default_style->outline);
	html_style_set_style_box (style, default_style->box);

	return style;
}

static void
html_debug_print_length (HtmlLength *length)
{
	if (length->type == HTML_LENGTH_AUTO)
		g_print ("auto");
	else
		g_print ("%d", length->value);
}

void
html_debug_print_style (HtmlStyle *style)
{
        g_print ("\n------------\n");


	g_print ("display: ");
	switch (style->display) {
	case HTML_DISPLAY_TABLE:
		g_print ("table;"); break;
	case HTML_DISPLAY_BLOCK:
		g_print ("block;"); break;
	case HTML_DISPLAY_INLINE:
		g_print ("inline;");break;
	case HTML_DISPLAY_NONE:
		g_print ("none;"); break;
	default:
		g_warning ("unhandled display property %d", style->display);
	}
	g_print ("\n");
	
	g_print ("visibility: ");
	switch (style->visibility) {
	case HTML_VISIBILITY_VISIBLE:
		g_print ("visible;");
		break;
	case HTML_VISIBILITY_HIDDEN:
		g_print ("hidden;");
		break;
	case HTML_VISIBILITY_COLLAPSE:
		g_print ("collapse;");
		break;
	}
	g_print ("\n");
	
	g_print ("width: ");
	html_debug_print_length (&style->box->width);
	g_print (";\n");

	g_print ("height: ");
	html_debug_print_length (&style->box->height);
	g_print (";\n");

	g_print ("max-width: ");
	html_debug_print_length (&style->box->max_width);
	g_print (";\n");

	g_print ("min-width: ");
	html_debug_print_length (&style->box->min_width);
	g_print (";\n");

	g_print ("max-height: ");
	html_debug_print_length (&style->box->max_height);
	g_print (";\n");

	g_print ("min-height: ");
	html_debug_print_length (&style->box->min_height);
	g_print (";\n");
}

#define COMPARE_RELAYOUT(x) if (s1->x != s2->x) return HTML_STYLE_CHANGE_RELAYOUT;
#define COMPARE_RELAYOUT_LENGTH(x) if (html_length_equals (&s1->x, &s2->x) == FALSE) return HTML_STYLE_CHANGE_RELAYOUT;
#define COMPARE_REPAINT_COLOR(x) if (html_color_equal (s1->x, s2->x) == FALSE) return HTML_STYLE_CHANGE_REPAINT;
#define COMPARE_REPAINT(x) if (s1->x != s2->x) return HTML_STYLE_CHANGE_REPAINT;

HtmlStyleChange
html_style_compare (const HtmlStyle *s1, const HtmlStyle *s2)
{
	/* RECREATE begin */
	if (s1->display != s2->display)
		return HTML_STYLE_CHANGE_RECREATE;
	/* RECREATE end */

	/* RELAYOUT begin */
	COMPARE_RELAYOUT (vertical_align);
	COMPARE_RELAYOUT (position);
	COMPARE_RELAYOUT (Float);
	COMPARE_RELAYOUT (overflow);
	COMPARE_RELAYOUT (text_transform);
	COMPARE_RELAYOUT (clear);
	COMPARE_RELAYOUT (unicode_bidi);
	COMPARE_RELAYOUT (table_layout);
	COMPARE_RELAYOUT (blink);
	COMPARE_RELAYOUT_LENGTH (box->width);
	COMPARE_RELAYOUT_LENGTH (box->min_width);
	COMPARE_RELAYOUT_LENGTH (box->max_width);
	COMPARE_RELAYOUT_LENGTH (box->height);
	COMPARE_RELAYOUT_LENGTH (box->min_height);
	COMPARE_RELAYOUT_LENGTH (box->max_height);
	COMPARE_RELAYOUT_LENGTH (visual->clip.top);
	COMPARE_RELAYOUT_LENGTH (visual->clip.left);
	COMPARE_RELAYOUT_LENGTH (visual->clip.right);
	COMPARE_RELAYOUT_LENGTH (visual->clip.bottom);
	COMPARE_RELAYOUT_LENGTH (surround->margin.top);
	COMPARE_RELAYOUT_LENGTH (surround->margin.left);
	COMPARE_RELAYOUT_LENGTH (surround->margin.right);
	COMPARE_RELAYOUT_LENGTH (surround->margin.bottom);
	COMPARE_RELAYOUT_LENGTH (surround->padding.top);
	COMPARE_RELAYOUT_LENGTH (surround->padding.left);
	COMPARE_RELAYOUT_LENGTH (surround->padding.right);
	COMPARE_RELAYOUT_LENGTH (surround->padding.bottom);
	COMPARE_RELAYOUT_LENGTH (surround->position.top);
	COMPARE_RELAYOUT_LENGTH (surround->position.left);
	COMPARE_RELAYOUT_LENGTH (surround->position.right);
	COMPARE_RELAYOUT_LENGTH (surround->position.bottom);

	COMPARE_RELAYOUT (border->top.width);
	COMPARE_RELAYOUT (border->left.width);
	COMPARE_RELAYOUT (border->right.width);
	COMPARE_RELAYOUT (border->bottom.width);
	COMPARE_RELAYOUT (border->top.border_style);
	COMPARE_RELAYOUT (border->left.border_style);
	COMPARE_RELAYOUT (border->right.border_style);
	COMPARE_RELAYOUT (border->bottom.border_style);
	COMPARE_RELAYOUT (inherited->line_height);
	COMPARE_RELAYOUT (inherited->word_spacing);
	COMPARE_RELAYOUT (inherited->letter_spacing);
	COMPARE_RELAYOUT (inherited->cursor);
	COMPARE_RELAYOUT (inherited->border_spacing_horiz);
	COMPARE_RELAYOUT (inherited->border_spacing_vert);
	COMPARE_RELAYOUT (inherited->direction);
	COMPARE_RELAYOUT (inherited->bidi_level);
	COMPARE_RELAYOUT (inherited->caption_side);
	COMPARE_RELAYOUT (inherited->white_space);
	COMPARE_RELAYOUT (inherited->list_style_type);

	COMPARE_RELAYOUT (inherited->font_spec->size);
	COMPARE_RELAYOUT (inherited->font_spec->weight);
	COMPARE_RELAYOUT (inherited->font_spec->style);
	COMPARE_RELAYOUT (inherited->font_spec->variant);
	COMPARE_RELAYOUT (inherited->font_spec->stretch);

	COMPARE_RELAYOUT_LENGTH (inherited->text_indent);
	if (strcmp (s1->inherited->font_spec->family, s2->inherited->font_spec->family) != 0)
		return HTML_STYLE_CHANGE_RELAYOUT;
#if 0
	if (html_font_description_equal (s1->inherited->font_spec, s2->inherited->font_spec) == FALSE)
		return HTML_STYLE_CHANGE_RELAYOUT;
#endif
	/* RELAYOUT end */

	/* REPAINT begin */
	if (html_color_equal (&s1->background->color, &s2->background->color) == FALSE)
		return HTML_STYLE_CHANGE_REPAINT;
#if 0
	COMPARE_REPAINT (background->image);
#endif
	COMPARE_REPAINT (background->repeat);
	COMPARE_REPAINT_COLOR (inherited->color);
	COMPARE_REPAINT_COLOR (border->top.color);
	COMPARE_REPAINT_COLOR (border->left.color);
	COMPARE_REPAINT_COLOR (border->right.color);
	COMPARE_REPAINT_COLOR (border->bottom.color);
	COMPARE_REPAINT (inherited->font_spec->decoration);

	COMPARE_REPAINT (outline->style);
	COMPARE_REPAINT (outline->width);
	COMPARE_REPAINT_COLOR (outline->color);

	/* REPAINT end */

	return HTML_STYLE_CHANGE_NONE;
}
