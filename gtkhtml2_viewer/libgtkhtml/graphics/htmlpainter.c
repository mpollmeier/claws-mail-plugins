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
#include <gtk/gtkicontheme.h>
#include <graphics/htmlpainter.h>

static GObjectClass *parent_class = NULL;

static void html_painter_finalize (GObject *object);

static void
html_painter_class_init (HtmlPainterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = html_painter_finalize;
}

static void
html_painter_init (HtmlPainter *painter)
{
	GtkIconTheme *icon_theme;

	icon_theme = gtk_icon_theme_get_default ();

	painter->debug = FALSE;

	if (gtk_icon_theme_has_icon (icon_theme, "image-loading"))
		painter->loading_picture = gtk_icon_theme_load_icon (icon_theme, "image-loading", 16, 0, NULL);
	else
		painter->loading_picture = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-loading-icon", 16, 0, NULL);
}

GType
html_painter_get_type (void)
{
	static GType html_painter_type = 0;

	if (!html_painter_type) {
		static const GTypeInfo html_painter_info = {
			sizeof (HtmlPainterClass),
			NULL,
			NULL,
			(GClassInitFunc) html_painter_class_init,
			NULL,
			NULL,
			sizeof (HtmlPainter),
			1,
			(GInstanceInitFunc) html_painter_init,
		};
		
		html_painter_type = g_type_register_static (G_TYPE_OBJECT, "HtmlPainter", &html_painter_info, 
							    0);
	}

	return html_painter_type;
}

void
html_painter_draw_pixbuf (HtmlPainter *painter, GdkRectangle *area, GdkPixbuf *pixbuf, gint src_x, gint src_y,
			  gint dest_x, gint dest_y, gint width, gint height)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->draw_pixbuf (painter, area, pixbuf, 
						   src_x, src_y, 
						   dest_x, dest_y,
						   width, height);
}

void
html_painter_draw_rectangle (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->draw_rectangle (painter, area, x, y, width, height);
}

void
html_painter_draw_arc (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height, gint				 angle1, gint angle2, gboolean fill)
{
	g_return_if_fail (painter != NULL);

        HTML_PAINTER_GET_CLASS (painter)->draw_arc (painter, area, x, y, width, height, angle1, angle2, fill);
}


void
html_painter_fill_rectangle (HtmlPainter *painter, GdkRectangle *area, gint x, gint y, gint width, gint height)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->fill_rectangle (painter, area, x, y,
						      width, height);
}

void
html_painter_draw_line (HtmlPainter *painter, gint x1, gint y1, gint x2, gint y2)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->draw_line (painter, x1, y1, x2, y2);
}

void
html_painter_draw_polygon (HtmlPainter *painter, gboolean filled, GdkPoint *points, gint npoints)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->draw_polygon (painter, filled, points, npoints);
}

void
html_painter_draw_glyphs (HtmlPainter *painter, gint x, gint y, PangoFont *font, PangoGlyphString *glyphs)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->draw_glyphs (painter, x, y, font, glyphs);
}

void
html_painter_draw_layout (HtmlPainter *painter, gint x, gint y, PangoLayout *layout)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->draw_layout (painter, x, y, layout);
}

void
html_painter_set_clip_rectangle (HtmlPainter *painter, gint x, gint y, gint width, gint height)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->set_clip_rectangle (painter, x, y, width, height);
}

void
html_painter_set_foreground_color (HtmlPainter *painter, HtmlColor *color)
{
	g_return_if_fail (painter != NULL);

	HTML_PAINTER_GET_CLASS (painter)->set_foreground_color (painter, color);
}

static void
html_painter_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}










