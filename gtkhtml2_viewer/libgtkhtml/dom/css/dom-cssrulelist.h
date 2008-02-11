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

#ifndef __DOM_CSSRULELIST_H__
#define __DOM_CSSRULELIST_H__

#include <dom/dom-types.h>
#include <dom/stylesheets/dom-stylesheet.h>

#define DOM_TYPE_CSS_RULE_LIST             (dom_css_rule_list_get_type ())
#define DOM_CSS_RULE_LIST(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), DOM_TYPE_CSS_RULE_LIST, DomCSSRuleList))
#define DOM_CSS_RULE_LIST_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_CSS_RULE_LIST, DomCSSRuleListClass))
#define DOM_IS_CSS_RULE_LIST(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object),  DOM_TYPE_CSS_RULE_LIST))
#define DOM_IS_CSS_RULE_LIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_CSS_RULE_LIST))
#define DOM_CSS_RULE_LIST_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_CSS_RULE_LIST, DomCSSRuleListClass))

struct _DomCSSRuleList {
	GObject parent_instance;

	GSList **rules;
};

struct _DomCSSRuleListClass {
	GObjectClass parent_class;
};

GType dom_css_rule_list_get_type (void);

gulong dom_CSSRuleList__get_length (DomCSSRuleList *list);
DomCSSRule *dom_CSSRuleList_item (DomCSSRuleList *list, gulong index);


#endif /* __DOM_RULELIST_H__ */
