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

#ifndef __DOM_STYLESHEET_H__
#define __DOM_STYLESHEET_H__

#include <dom/dom-types.h>
#include <css/cssstylesheet.h>

#define DOM_TYPE_STYLE_SHEET             (dom_style_sheet_get_type ())
#define DOM_STYLE_SHEET(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), DOM_TYPE_STYLE_SHEET, DomStyleSheet))
#define DOM_STYLE_SHEET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_STYLE_SHEET, DomStyleSheetClass))
#define DOM_IS_STYLE_SHEET(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object),  DOM_TYPE_STYLE_SHEET))
#define DOM_IS_STYLE_SHEET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_STYLE_SHEET))
#define DOM_STYLE_SHEET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_STYLE_SHEET, DomStyleSheetClass))

struct _DomStyleSheet {
	GObject parent_instance;

	DomNode *owner;
	CssStylesheet *sheet;
};

struct _DomStyleSheetClass {
	GObject parent_class;
};

GType dom_style_sheet_get_type (void);

DomString *dom_StyleSheet__get_type (DomStyleSheet *sheet);
DomBoolean dom_StyleSheet__get_disabled (DomStyleSheet *sheet);
void dom_StyleSheet__set_disabled (DomStyleSheet *sheet, DomBoolean disabled);
DomNode *dom_StyleSheet__get_ownerNode (DomStyleSheet *sheet);


#endif /* __DOM_STYLESHEET_H__ */
