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

#ifndef __HTMLCOLORFACTORY_H__
#define __HTMLCOLORFACTORY_H__

#include <glib.h>
#include <gobject/gobject.h>
#include "paintertypedefs.h"

G_BEGIN_DECLS

#define HTML_COLOR_FACTORY_TYPE (html_color_factory_get_type ())
#define HTML_COLOR_FACTORY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HTML_COLOR_FACTORY_TYPE, HtmlColorFactory))
#define HTML_COLOR_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_COLOR_FACTORY_TYPE, HtmlColorFactoryClass))
#define HTML_COLOR_FACTORY_GET_CLASS(klass) (G_TYPE_INSTANCE_GET_CLASS ((klass), HTML_COLOR_FACTORY_TYPE, HtmlColorFactoryClass))

#define HTML_COLOR_TYPE (html_color_get_type ())
#define HTML_COLOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HTML_COLOR_TYPE, HtmlColor))
#define HTML_COLOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_COLOR_TYPE, HtmlColorClass))
#define HTML_COLOR_GET_CLASS(klass) (G_TYPE_INSTANCE_GET_CLASS ((klass), HTML_COLOR_TYPE, HtmlColorClass))

typedef struct _HtmlColorFactoryClass HtmlColorFactoryClass;
typedef struct _HtmlColorClass HtmlColorClass;

struct _HtmlColorFactory {
	GObject parent;

	GHashTable *colors;
};

struct _HtmlColorFactoryClass {
	GObjectClass parent_class;

	HtmlColor * (* new_color) (HtmlColorFactory *color_factory, gint red, gint green, gint blue);
};

struct _HtmlColorClass {
	GObjectClass parent_class;
	HtmlColor * (* transform) (HtmlColor *color, gfloat ratio);
};

GType html_color_factory_get_type (void);
HtmlColor *html_color_factory_get_color (HtmlColorFactory *color_factory, const gchar *color_name);
HtmlColor *html_color_factory_get_color_rgb (HtmlColorFactory *color_factory, gint red, gint green, gint blue);
HtmlColor *html_color_transform (HtmlColor *color, gfloat ratio);

GType html_color_get_type (void);

G_END_DECLS

#endif /* __HTMLCOLORFACTORY_H__ */




