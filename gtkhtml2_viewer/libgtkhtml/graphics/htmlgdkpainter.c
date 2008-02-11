/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>
   
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
#include "htmlgdkpainter.h"
#include "htmlcolor.h"
#include "htmlfontspecification.h"

#define MIN_TILE_SIZE 200

static HtmlPainterClass *parent_class = NULL;

static void set_clip_rectangle (HtmlPainter *painter, gint x, gint y, gint width, gint height);
static void set_foreground_color (HtmlPainter *painter, HtmlColor *color);

static void draw_arc (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height, gint angle1, gint angle2, gboolean fill);

static void draw_rectangle (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height);
static void fill_rectangle (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height);
static void draw_line (HtmlPainter *painter, gint x1, gint y1, gint x2, gint y2);
static void draw_glyphs (HtmlPainter *painter, gint x, gint y, PangoFont *font, PangoGlyphString *glyphs);
static void draw_layout (HtmlPainter *painter, gint x, gint y, PangoLayout *layout);
static void draw_polygon (HtmlPainter *painter, gboolean filled, GdkPoint *points, gint npoints);
static void draw_pixbuf (HtmlPainter *painter, GdkRectangle *area, GdkPixbuf *pixbuf, gint src_x, gint src_y,
			 gint dest_x, gint dest_y, gint width, gint height);

static void
html_gdk_painter_class_init (HtmlGdkPainterClass *klass)
{
	HtmlPainterClass *painter_class = HTML_PAINTER_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);

	painter_class->set_clip_rectangle = set_clip_rectangle;
	painter_class->set_foreground_color = set_foreground_color;

	painter_class->draw_rectangle = draw_rectangle;
	painter_class->fill_rectangle = fill_rectangle;
	painter_class->draw_arc = draw_arc;  
	painter_class->draw_line = draw_line;
	painter_class->draw_glyphs = draw_glyphs;
	painter_class->draw_layout = draw_layout;
	painter_class->draw_polygon = draw_polygon;
	painter_class->draw_pixbuf = draw_pixbuf;
}

static void
html_gdk_painter_init (HtmlGdkPainter *painter)
{
	painter->window = NULL;
	painter->gc = NULL;
}

GType
html_gdk_painter_get_type (void)
{
	static GType html_gdk_painter_type = 0;

	if (!html_gdk_painter_type) {
		static const GTypeInfo html_gdk_painter_info = {
			sizeof (HtmlGdkPainterClass),
			NULL,
			NULL,
			(GClassInitFunc) html_gdk_painter_class_init,
			NULL,
			NULL,
			sizeof (HtmlGdkPainter),
			1,
			(GInstanceInitFunc) html_gdk_painter_init,
		};		
		html_gdk_painter_type = g_type_register_static (HTML_PAINTER_TYPE, "HtmlGdkPainter", 
								&html_gdk_painter_info, 0);
	}

	return html_gdk_painter_type;
}

HtmlPainter *
html_gdk_painter_new (void)
{
	HtmlPainter *painter;

	painter = (HtmlPainter *)g_type_create_instance (HTML_GDK_PAINTER_TYPE);
	
	return painter;
}

static void
draw_pixbuf (HtmlPainter *painter, GdkRectangle *area, GdkPixbuf *pixbuf, gint src_x, gint src_y,
	     gint dest_x, gint dest_y, gint width, gint height)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);
	GdkRectangle dest, pixbuf_area;

	if (pixbuf == NULL)
		return;

	if (width == -1)
		width = gdk_pixbuf_get_width (pixbuf);
	if (height == -1)
		height = gdk_pixbuf_get_height (pixbuf);

	pixbuf_area.x = dest_x;
	pixbuf_area.y = dest_y;
	pixbuf_area.width = width;
	pixbuf_area.height = height;

	/* Don't draw anything if it is outside the clipping area */
	if (gdk_rectangle_intersect (area, &pixbuf_area, &dest) == FALSE)
		return;

	width  = dest.width;
	height = dest.height;

	if (dest.x > dest_x) {
		src_x += dest.x - dest_x;
		dest_x = dest.x;
	}

	if (dest.y > dest_y) {
		src_y += dest.y - dest_y;
		dest_y = dest.y;
	}

	gdk_pixbuf_render_to_drawable_alpha (pixbuf, gdk_painter->window, src_x, src_y,
					     dest_x, dest_y,
					     width, height,
					     GDK_PIXBUF_ALPHA_FULL,
					     0,
					     GDK_RGB_DITHER_NORMAL,
					     0, 0);
}

void
html_gdk_painter_set_window (HtmlGdkPainter *painter, GdkWindow *window)
{
	g_return_if_fail (window != NULL);
	
	if (painter->gc)
		gdk_gc_destroy (painter->gc);
	
	painter->gc = gdk_gc_new (window);
	painter->window = window;

}

static void
draw_rectangle (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);

	/* gdk_draw_rectange, draws an unfilled rectangle one pixel
	   wider and higher than filled ones */
	gdk_draw_rectangle (gdk_painter->window, gdk_painter->gc, FALSE, 
			    x, y, width - 1, height - 1);
}

static void
draw_arc (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height, gint angle1, gint angle2, gboolean fill)
{

	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);

	if (fill)
		gdk_draw_arc (gdk_painter->window, gdk_painter->gc, TRUE, x , y , width , height , angle1, angle2);

	gdk_draw_arc (gdk_painter->window, gdk_painter->gc, FALSE, x , y , width , height , angle1, angle2);

}


static void
fill_rectangle (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);
	GdkRectangle dest, rect;

	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;

	/* Don't draw anything if it is outside the clipping area */
	if (gdk_rectangle_intersect (area, &rect, &dest) == FALSE)
		return;

	gdk_draw_rectangle (gdk_painter->window, gdk_painter->gc, TRUE, 
			    dest.x, dest.y, dest.width, dest.height);
}

static void
draw_line (HtmlPainter *painter, gint x1, gint y1, gint x2, gint y2)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_draw_line (gdk_painter->window, gdk_painter->gc, x1, y1, x2, y2);
}

static void
draw_polygon (HtmlPainter *painter, gboolean filled, GdkPoint *points, gint npoints)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_draw_polygon (gdk_painter->window, gdk_painter->gc, filled, points, npoints);
}

static void
draw_glyphs (HtmlPainter *painter, gint x, gint y, PangoFont *font, PangoGlyphString *glyphs)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_draw_glyphs (gdk_painter->window, gdk_painter->gc, font, x, y, glyphs);
#if 0
	/* Now check for flags */
	if (painter->font->decoration & HTML_FONT_DECORATION_OVERLINE) {
		gdk_draw_line (gdk_painter->window, gdk_painter->gc, x, y, x + get_text_width (painter, text, len), y);
	}
	
	if (painter->font->decoration & HTML_FONT_DECORATION_LINETHROUGH) {
		gdk_draw_line (gdk_painter->window, gdk_painter->gc, x, y + (get_text_height (painter) >> 1),
			       x + get_text_width (painter, text, len), y + (get_text_height (painter) >> 1));
	}
	
	if (painter->font->decoration & HTML_FONT_DECORATION_UNDERLINE) {
		gdk_draw_line (gdk_painter->window, gdk_painter->gc, x, y + gdk_painter->gdk_font->ascent + (gdk_painter->gdk_font->descent >> 1),
			       x + get_text_width (painter, text, len), y + gdk_painter->gdk_font->ascent + (gdk_painter->gdk_font->descent >> 1));
	}
#endif
}

static void
draw_layout (HtmlPainter *painter, gint x, gint y, PangoLayout *layout)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);

	gdk_draw_layout (gdk_painter->window, gdk_painter->gc, x, y, layout);
}

static void
set_clip_rectangle (HtmlPainter *painter, gint x, gint y, gint width, gint height)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);

	GdkRectangle clip_rect;
	
	if (width == 0 || height == 0) {
		gdk_gc_set_clip_rectangle (gdk_painter->gc, NULL);
		return;
	}

	clip_rect.x = x;
	clip_rect.y = y;
	clip_rect.width = width;
	clip_rect.height = height;

	gdk_gc_set_clip_rectangle (gdk_painter->gc, &clip_rect);		
}

static void
set_foreground_color (HtmlPainter *painter, HtmlColor *color)
{
	HtmlGdkPainter *gdk_painter = HTML_GDK_PAINTER (painter);
	GdkColor gdk_color;

	g_return_if_fail (color != NULL);

	gdk_color.red   = (color->red   << 8) | color->red;
	gdk_color.green = (color->green << 8) | color->green;
	gdk_color.blue  = (color->blue  << 8) | color->blue;
	
	gdk_rgb_find_color (gdk_drawable_get_colormap (GDK_DRAWABLE (gdk_painter->window)), &gdk_color);
	gdk_gc_set_foreground (gdk_painter->gc, &gdk_color);

}

