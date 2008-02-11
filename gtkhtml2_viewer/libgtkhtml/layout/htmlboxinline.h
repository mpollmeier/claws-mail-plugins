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

#ifndef __HTMLBOXINLINE_H__
#define __HTMLBOXINLINE_H__

#include <gdk/gdk.h>
#include "htmlbox.h"

G_BEGIN_DECLS

typedef struct _HtmlBoxInline HtmlBoxInline;
typedef struct _HtmlBoxInlineClass HtmlBoxInlineClass;

#define HTML_TYPE_BOX_INLINE (html_box_inline_get_type ())
#define HTML_BOX_INLINE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), HTML_TYPE_BOX_INLINE, HtmlBoxInline))
#define HTML_BOX_INLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_TYPE_BOX_INLINE, HtmlBoxInlineClasss))
#define HTML_IS_BOX_INLINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), HTML_TYPE_BOX_INLINE))
#define HTML_IS_BOX_INLINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HTML_TYPE_BOX_INLINE))

struct _HtmlBoxInline {
	HtmlBox parent_object;
};

struct _HtmlBoxInlineClass {
	HtmlBoxClass parent_class;
};

GType    html_box_inline_get_type (void);
HtmlBox *html_box_inline_new      (void);

G_END_DECLS

#endif /* __HTMLBOXINLINE_H__ */
