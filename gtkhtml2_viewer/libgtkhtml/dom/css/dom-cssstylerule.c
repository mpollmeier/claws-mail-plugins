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

#include "dom-cssstylerule.h"

DomString *
dom_CSSStyleRule__get_selectorText (DomCSSStyleRule *rule)
{
	return NULL;
}

void
dom_CSSStyleRule__set_selectorText (DomCSSStyleRule *rule, const DomString *selectorText, DomException *exc)
{
}

static void
dom_css_style_rule_class_init (DomCSSStyleRuleClass *klass)
{
}

static void
dom_css_style_rule_init (DomCSSStyleRule *doc)
{
}

GType
dom_css_style_rule_get_type (void)
{
	static GType dom_css_style_rule_type = 0;

	if (!dom_css_style_rule_type) {
		static const GTypeInfo dom_css_style_rule_info = {
			sizeof (DomCSSStyleRuleClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_css_style_rule_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomCSSStyleRule),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_css_style_rule_init,
		};

		dom_css_style_rule_type = g_type_register_static (G_TYPE_OBJECT, "DomCSSStyleRule", &dom_css_style_rule_info, 0);
	}

	return dom_css_style_rule_type;
}
