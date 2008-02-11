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

#include "cssstylesheet.h"

void
css_selector_calc_specificity (CssSelector *sel)
{

	gint a = 0, b = 0, c = 0;
	gint i;
	gint n_simple = sel->n_simple;

	for (i = 0; i < n_simple; i++) {
		CssSimpleSelector *ss;
		gint j, n_tail;

		ss = sel->simple[i];
		n_tail = ss->n_tail;
		for (j = 0; j < n_tail; j++) {
			CssTailType type = ss->tail[j].type;

			if (type == CSS_TAIL_ID_SEL)
				a++;
			else if (type == CSS_TAIL_ATTR_SEL || type == CSS_TAIL_PSEUDO_SEL || type == CSS_TAIL_CLASS_SEL)
				b++;
		}
		if (!ss->is_star)
			c++;
	}
	
	sel->a = a;
	sel->b = b;
	sel->c = c;
}

void
css_tail_destroy (CssTail *tail)
{
	if (tail->type == CSS_TAIL_ATTR_SEL) {
		if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_STRING)
			g_free (tail->t.attr_sel.val.a.str);
	}
}

void
css_simple_selector_destroy (CssSimpleSelector *ss)
{
	gint i;

	for (i = 0; i < ss->n_tail; i++)
		css_tail_destroy (&ss->tail[i]);
	
	g_free (ss->tail);
	g_free (ss);
}

void
css_selector_destroy (CssSelector *sel)
{
	gint i;

	for (i = 0; i < sel->n_simple; i++)
		css_simple_selector_destroy (sel->simple[i]);

	g_free (sel->simple);
	g_free (sel->comb);
	g_free (sel);
	
}

static void
css_declaration_destroy (CssDeclaration *decl)
{
	css_value_unref (decl->expr);
	
	g_free (decl);
}

void
css_ruleset_destroy (CssRuleset *ruleset)
{
	gint i;

	for (i = 0; i < ruleset->n_sel; i++)
		css_selector_destroy (ruleset->sel[i]);

	for (i = 0; i < ruleset->n_decl; i++)
		css_declaration_destroy (ruleset->decl[i]);

	g_free (ruleset->sel);
	g_free (ruleset->decl);
	g_free (ruleset);
}

static void
css_statement_destroy (CssStatement *stat)
{
	switch (stat->type) {
	case CSS_RULESET:
		css_ruleset_destroy (stat->s.ruleset);
	default:
		break;
	}
	g_free (stat);
	
}

void
css_stylesheet_destroy (CssStylesheet *sheet)
{
	GSList *list;

	for (list = sheet->stat; list; list = list->next)
		css_statement_destroy (list->data);
	
	if (sheet->stat)
		g_slist_free (sheet->stat);
	g_free (sheet);
}
