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

#ifndef __HTMLBOXEMBEDDEDBUTTON_H__
#define __HTMLBOXEMBEDDEDBUTTON_H__

typedef struct _HtmlBoxEmbeddedButton HtmlBoxEmbeddedButton;
typedef struct _HtmlBoxEmbeddedButtonClass HtmlBoxEmbeddedButtonClass;

typedef enum {
	HTML_BOX_EMBEDDED_BUTTON_TYPE_SUBMIT = 0,
	HTML_BOX_EMBEDDED_BUTTON_TYPE_RESET
} HtmlBoxEmbeddedButtonType;


#include "layout/html/htmlboxembedded.h"

G_BEGIN_DECLS

#define HTML_TYPE_BOX_EMBEDDED_BUTTON (html_box_embedded_button_get_type ())
#define HTML_BOX_EMBEDDED_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), HTML_TYPE_BOX_EMBEDDED_BUTTON, HtmlBoxEmbeddedButton))
#define HTML_BOX_EMBEDDED_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_TYPE_BOX_EMBEDDED_BUTTON, HtmlBoxEmbeddedButtonClasss))
#define HTML_IS_BOX_EMBEDDED_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), HTML_TYPE_BOX_EMBEDDED_BUTTON))
#define HTML_IS_BOX_EMBEDDED_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HTML_TYPE_BOX_EMBEDDED_BUTTON))

struct _HtmlBoxEmbeddedButton {
	HtmlBoxEmbedded parent_object;
	HtmlBoxEmbeddedButtonType type;
};

struct _HtmlBoxEmbeddedButtonClass {
	HtmlBoxEmbeddedClass parent_class;
};

GType    html_box_embedded_button_get_type (void);
HtmlBox *html_box_embedded_button_new      (HtmlView *view, HtmlBoxEmbeddedButtonType type);

G_END_DECLS

#endif /* __HTMLBOXBUTTON_H__ */
