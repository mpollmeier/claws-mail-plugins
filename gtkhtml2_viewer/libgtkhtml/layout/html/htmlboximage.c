/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000-2001 CodeFactory AB
   Copyright (C) 2000-2001 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000-2001 Anders Carlsson <andersca@codefactory.se>
   
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

#include <gtk/gtkicontheme.h>
#include <math.h>
#include "dom/events/dom-event-utils.h"
#include "layout/html/htmlboximage.h"

static GObjectClass *parent_class = NULL;
static GtkIconTheme *icon_theme = NULL;

static void html_box_image_resize_image (HtmlImage *image, HtmlBoxImage *box);
static void html_box_image_repaint_image (HtmlImage *image,
					  gint x, gint y,
					  gint width, gint height,
					  HtmlBoxImage *box);

static void
html_box_image_paint_border (HtmlBox *box, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBoxImage *image = HTML_BOX_IMAGE (box);
	static HtmlColor *dark_grey = NULL, *light_grey = NULL;
	static GdkPixbuf *error_image = NULL, *loading_image = NULL;
	gint x, y, width, height;

	if (icon_theme == NULL)
		icon_theme = gtk_icon_theme_get_default ();

	if (error_image == NULL)
		error_image = gtk_icon_theme_load_icon (icon_theme, "image-missing", 16, 0, NULL);
	if (error_image == NULL)
		error_image = gtk_widget_render_icon (GTK_WIDGET (image->view),
						      GTK_STOCK_MISSING_IMAGE,
						      GTK_ICON_SIZE_MENU,
						      "html");

	if (loading_image == NULL)
		loading_image = gtk_icon_theme_load_icon (icon_theme, "image-loading", 16, 0, NULL);
	if (loading_image == NULL)
		loading_image = gtk_icon_theme_load_icon (icon_theme, "gnome-fs-loading-icon", 16, 0, NULL);

	if (!dark_grey) {
		dark_grey = html_color_new_from_rgb (127, 127, 127);
		light_grey = html_color_new_from_rgb (191, 191, 191);
	}

	width = image->content_width;
	height = image->content_height;
	x = box->x + tx + (box->width - width) / 2;
	y = box->y + ty + (box->height - height) / 2;

	html_painter_set_foreground_color (painter, dark_grey);
	html_painter_draw_line (painter, x, y, x + width - 1, y);
	html_painter_draw_line (painter, x, y, x, y + height - 1);
	
	html_painter_set_foreground_color (painter, light_grey);
	html_painter_draw_line (painter, x + 1, y + height - 1,
				x + width - 1, y + height - 1);
	html_painter_draw_line (painter, x + width - 1, y,
				x + width - 1, y + height - 1);

	if (width  >= gdk_pixbuf_get_width (error_image) + 4 &&
	    height >= gdk_pixbuf_get_height (error_image) + 4) {
		
		if (image->image->broken) {
			html_painter_draw_pixbuf (painter, area, error_image, 0, 0,
						  x + 2, y + 2,
						  gdk_pixbuf_get_width (error_image),
						  gdk_pixbuf_get_height (error_image));
		}
		else if (image->image->loading) {
			html_painter_draw_pixbuf (painter, area, loading_image, 0, 0,
						  x + 2, y + 2,
						  gdk_pixbuf_get_width (loading_image),
						  gdk_pixbuf_get_height (loading_image));
		}
	}
}

static void
html_box_image_update_scaled_pixbuf (HtmlBoxImage *image, gint width, gint height)
{
	if (image->scaled_pixbuf)
		g_object_unref (image->scaled_pixbuf);

	if (width == gdk_pixbuf_get_width (image->image->pixbuf) &&
	    height == gdk_pixbuf_get_height (image->image->pixbuf)) {
		image->scaled_pixbuf = image->image->pixbuf;
		g_object_ref (image->scaled_pixbuf);
	} else {
		/* FIXME: gdk_pixbuf_scale_simple() expects width & height to
		   be > 0, so we use 1 at least. Maybe we need some special
		   handling if the width or height should be 0. */
		image->scaled_pixbuf = gdk_pixbuf_scale_simple (image->image->pixbuf, MAX (1, width), MAX (1, height), GDK_INTERP_NEAREST);
	}
}

static void
html_box_image_paint (HtmlBox *box, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBoxImage *image = HTML_BOX_IMAGE (box);
	gint x, y, width, height;
	
	width = image->content_width;
	height = image->content_height;
	x = box->x + tx + (box->width - width) / 2;
	y = box->y + ty + (box->height - height) / 2;

	if (image->scaled_pixbuf == NULL)
		html_box_image_paint_border (box, painter, area, tx, ty);
	else {
		GdkPixbuf *pixbuf = image->scaled_pixbuf;
		gint width = gdk_pixbuf_get_width (pixbuf);
		gint height = gdk_pixbuf_get_height (pixbuf);
		
		html_painter_draw_pixbuf (painter, area, pixbuf,
					  0, 0, x, y,
					  width, height);
	}
}

static void
html_box_image_relayout (HtmlBox *box, HtmlRelayout *relayout)
{
	HtmlBoxImage *image = HTML_BOX_IMAGE (box);
	GdkPixbuf *pixbuf = image->image->pixbuf;
	HtmlStyleBox *style = HTML_BOX_GET_STYLE (box)->box;
	gint old_width, old_height;
	
	gint width = 20, height = 20;

	old_width = image->content_width;
	old_height = image->content_height;
	
	if (pixbuf) {

		if (style->width.type == HTML_LENGTH_AUTO &&
		    style->height.type == HTML_LENGTH_AUTO) {
			width = gdk_pixbuf_get_width (pixbuf);
			height = gdk_pixbuf_get_height (pixbuf);
		}
		else {
			if (style->width.type != HTML_LENGTH_AUTO) {
				width = html_length_get_value (&style->width, html_box_get_containing_block_width (box));

				if (style->height.type == HTML_LENGTH_AUTO)
					height = (gint)(floor ((gfloat)(width * gdk_pixbuf_get_height (pixbuf))/(gfloat)gdk_pixbuf_get_width (pixbuf)) + 0.5);
			}

			if (style->height.type != HTML_LENGTH_AUTO) {
				height = html_length_get_value (&style->height, html_box_get_containing_block_height (box));

				if (style->width.type == HTML_LENGTH_AUTO)
					width = (gint)(floor ((gfloat)(height * gdk_pixbuf_get_width (pixbuf))/(gfloat)gdk_pixbuf_get_height (pixbuf)) + 0.5);
			}
		}
		
		html_box_check_min_max_width_height (box, &width, &height);

		/* Guard against oscillation - When max-width/height alters the
		 * size of an image, the aspect ratio is maintained, but this
		 * can cause an infinite resizing loop as the size oscillates
		 * between two sizes that alternately require and don't
		 * require a scrollbar.
		 */
		if ((old_width != width || old_height != height) && (width != image->last_width[1] || height != image->last_height[1])) {
			html_box_image_update_scaled_pixbuf (image, width, height);
			image->last_width[1] = image->last_width[0];
			image->last_height[1] = image->last_height[0];
			image->last_width[0] = width;
			image->last_height[0] = height;
		} else {
			image->last_width[1] = image->last_width[0];
			image->last_height[1] = image->last_height[0];
			image->last_width[0] = width;
			image->last_height[0] = height;
			width = old_width;
			height = old_height;
		}
	}
	else {
		if (style->width.type != HTML_LENGTH_AUTO)
			width = html_length_get_value (&HTML_BOX_GET_STYLE (box)->box->width, html_box_get_containing_block_width (box)) - 2;
		
		if (HTML_BOX_GET_STYLE (box)->box->height.type != HTML_LENGTH_AUTO)
			height = html_length_get_value (&HTML_BOX_GET_STYLE (box)->box->height, html_box_get_containing_block_height (box)) - 2;
	}
	
	if (height < 0)
		height = 0;
	if (width < 0)
		width = 0;
	
	box->width = width + html_box_horizontal_mbp_sum (box);
	box->height = height + html_box_vertical_mbp_sum (box);

	/* The content width */
	image->content_width = width;
	image->content_height = height;
}

static void
html_box_image_finalize (GObject *object)
{
	HtmlBoxImage *image = HTML_BOX_IMAGE (object);

	g_signal_handlers_disconnect_by_func (G_OBJECT (image->image),
					      G_CALLBACK (html_box_image_resize_image),
					      image);
	g_signal_handlers_disconnect_by_func (G_OBJECT (image->image),
					      G_CALLBACK (html_box_image_repaint_image),
					      image);

	if (image->scaled_pixbuf)
		g_object_unref (image->scaled_pixbuf);

	parent_class->finalize (object);
}

static void
html_box_image_class_init (HtmlBoxImageClass *klass)
{
	HtmlBoxClass *box_class = (HtmlBoxClass *)klass;
	GObjectClass *object_class = (GObjectClass *)klass;

	box_class->paint = html_box_image_paint;
	box_class->relayout = html_box_image_relayout;

	object_class->finalize = html_box_image_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_image_init (HtmlBoxImage *image)
{
	image->content_width = 20;
	image->content_height = 20;
	image->image = NULL;
	image->scaled_pixbuf = NULL;
	image->last_width[0] = 0;
	image->last_height[0] = 0;
	image->last_width[1] = 0;
	image->last_height[1] = 0;
}

GType
html_box_image_get_type (void)
{
	static GType html_type = 0;

	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxImageClass),
			NULL,
			NULL,
			(GClassInitFunc) html_box_image_class_init,
			NULL,
			NULL,
			sizeof (HtmlBoxImage),
			16,
			(GInstanceInitFunc) html_box_image_init
		};

		html_type = g_type_register_static (HTML_TYPE_BOX, "HtmlBoxImage", &type_info, 0);
	}

	return html_type;
}

HtmlBox *
html_box_image_new (HtmlView *view)
{
	HtmlBoxImage *box = g_object_new (HTML_TYPE_BOX_IMAGE, NULL);

	box->view = view;

	return HTML_BOX (box);
}

static void
html_box_image_resize_image (HtmlImage *image, HtmlBoxImage *box)
{
	g_signal_emit_by_name (G_OBJECT (box->view->document), "relayout_node", HTML_BOX (box)->dom_node);
}

static void
html_box_image_repaint_image (HtmlImage *image, gint x, gint y, gint width, gint height, HtmlBoxImage *box)
{
	gdouble real_x, real_y;
	gdouble real_width, real_height;

	if (box->scaled_pixbuf && image->pixbuf) {

		html_box_image_update_scaled_pixbuf (box, gdk_pixbuf_get_width (box->scaled_pixbuf), gdk_pixbuf_get_height (box->scaled_pixbuf)); 
		real_y = (y * gdk_pixbuf_get_height (box->scaled_pixbuf)) / (gdouble)gdk_pixbuf_get_height (image->pixbuf);
		real_x = (x * gdk_pixbuf_get_width (box->scaled_pixbuf)) / (gdouble)gdk_pixbuf_get_width (image->pixbuf);
		
		real_height = (height * gdk_pixbuf_get_height (box->scaled_pixbuf)) / (gdouble)gdk_pixbuf_get_height (image->pixbuf);
		real_width = (width * gdk_pixbuf_get_width (box->scaled_pixbuf)) / (gdouble)gdk_pixbuf_get_width (image->pixbuf);
		
		gtk_widget_queue_draw_area (GTK_WIDGET (box->view),
					    html_box_get_absolute_x (HTML_BOX (box)), /* + floor (real_x + 0.5),*/
					    html_box_get_absolute_y (HTML_BOX (box)),/* + floor (real_y + 0.5),*/
					    floor (real_width + real_x + 0.5),
					    floor (real_height + real_y + 0.5));
	}
}

void
html_box_image_set_image (HtmlBoxImage *box, HtmlImage *image)
{
	if (box->image)
		g_error ("support image replacing");

	g_signal_connect (G_OBJECT (image), "resize_image",
			  G_CALLBACK (html_box_image_resize_image), box);
	g_signal_connect (G_OBJECT (image), "repaint_image",
			  G_CALLBACK (html_box_image_repaint_image), box);
	box->image = image;
}

