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

#ifndef __HTMLBOXIMAGE_H__
#define __HTMLBOXIMAGE_H__

#include <glib.h>

#include <layout/htmlbox.h>
#include <layout/htmlrelayout.h>
#include <graphics/htmlpainter.h>
#include <view/htmlview.h>

G_BEGIN_DECLS

typedef struct _HtmlBoxImage HtmlBoxImage;
typedef struct _HtmlBoxImageClass HtmlBoxImageClass;

#define HTML_TYPE_BOX_IMAGE (html_box_image_get_type ())
#define HTML_BOX_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), HTML_TYPE_BOX_IMAGE, HtmlBoxImage))
#define HTML_BOX_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_TYPE_BOX_IMAGE, HtmlBoxImageClasss))
#define HTML_IS_BOX_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HTML_TYPE_BOX_IMAGE))
#define HTML_IS_BOX_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HTML_TYPE_BOX_IMAGE))

struct _HtmlBoxImage {
	HtmlBox parent_object;

	gint content_width, content_height;
	HtmlImage *image;
	GdkPixbuf *scaled_pixbuf;
	gboolean updated;
	HtmlView *view;
	
	gint last_width[2], last_height[2];
};

struct _HtmlBoxImageClass {
	HtmlBoxClass parent_class;
};

GType    html_box_image_get_type (void);
HtmlBox *html_box_image_new      (HtmlView *view);

void     html_box_image_updated       (HtmlBoxImage *image);
gboolean html_box_image_area_prepared (HtmlBoxImage *image);
void     html_box_image_set_image     (HtmlBoxImage *box, HtmlImage *image);

G_END_DECLS

#endif /* __HTMLBOXIMAGE_H__ */
