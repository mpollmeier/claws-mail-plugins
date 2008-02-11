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

#ifndef __DOM_CSSSTYLEDECLARATION_H__
#define __DOM_CSSSTYLEDECLARATION_H__

#include <dom/dom-types.h>

#define DOM_TYPE_CSS_STYLE_DECLARATION             (dom_css_style_declaration_get_type ())
#define DOM_CSS_STYLE_DECLARATION(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), DOM_TYPE_CSS_STYLE_DECLARATION, DomCSSStyleDeclaration))
#define DOM_CSS_STYLE_DECLARATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_CSS_STYLE_DECLARATION, DomCSSStyleDeclarationClass))
#define DOM_IS_CSS_STYLE_DECLARATION(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object),  DOM_TYPE_CSS_STYLE_DECLARATION))
#define DOM_IS_CSS_STYLE_DECLARATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_CSS_STYLE_DECLARATION))
#define DOM_CSS_STYLE_DECLARATION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_CSS_STYLE_DECLARATION, DomCSSStyleDeclarationClass))

struct _DomCSSStyleDeclaration {
	GObject parent_instance;
};

struct _DomCSSStyleDeclarationClass {
	GObject parent_class;
};

GType dom_css_style_declaration_get_type (void);

DomString *dom_CSSStyleDeclaration__get_cssText (DomCSSStyleDeclaration *decl);
void dom_CSSStyleDeclaration__set_cssText (DomCSSStyleDeclaration *decl, const DomString *cssText, DomException *exc);
DomString *dom_CSSStyleDeclaration_getPropertyValue (DomCSSStyleDeclaration *decl, const DomString *propertyName);

#endif /* __DOM_CSSSTYLEDECLARATION_H__ */
