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

#include "cssvalue.h"
#include "cssparser.h"

static const gchar *css_dimensions[] = {
	NULL,
	"",
	"%",
	"em",
	"ex",
	"px",
	"cm",
	"mm",
	"in",
	"pt",
	"pc",
	"deg",
	"rad",
	"grad",
	"ms",
	"s",
	"Hz",
	"kHz"
};


void
css_debug_print_value (CssValue *val)
{
	CssValueEntry *entry;
	
	if (!val)
		g_print ("NULL");
	
	switch (val->value_type) {
	case CSS_NUMBER:
	case CSS_CM:
	case CSS_EMS:
	case CSS_PX:
	case CSS_PT:
	case CSS_DEG:
	case CSS_PERCENTAGE:
	case CSS_EXS:
	case CSS_MM:
	case CSS_PC:
	case CSS_IN:
		g_print ("%g%s", val->v.d, css_dimensions [val->value_type]);
		break;
	case CSS_IDENT:
		g_print ("%s", html_atom_list_get_string (html_atom_list, val->v.atom));
		break;
	case CSS_STRING:
		g_print ("\"%s\"", val->v.s);
		break;
	case CSS_FUNCTION:
		g_print ("%s(", html_atom_list_get_string (html_atom_list, val->v.function->name));
		css_debug_print_value (val->v.function->args);
		g_print (")");
		break;
		
	case CSS_VALUE_LIST:
		entry = val->v.entry;

		while (entry) {
			css_debug_print_value (entry->value);

			if (entry->next)
				g_print ("%c", entry->list_sep);
			entry = entry->next;
		}
		break;
	default:
		g_warning ("css_debug_get_value: Unknown value type %d", val->value_type);
	}
}

void
css_debug_print_simple_selector (CssSimpleSelector *ss)
{
	gint i;
 
	if (ss->is_star) {
		if (ss->n_tail == 0) 
			g_print ("*");

	}
	else
		g_print ("%s", html_atom_list_get_string (html_atom_list, ss->element_name));

	for (i = 0; i < ss->n_tail; i++) {
		CssTail *tail = &ss->tail[i];

		if (tail->type == CSS_TAIL_ATTR_SEL) {
			g_print ("[%s", html_atom_list_get_string (html_atom_list, tail->t.attr_sel.att));

			if (tail->t.attr_sel.match != CSS_MATCH_EMPTY) {
				if (tail->t.attr_sel.match == CSS_MATCH_EQ)
					g_print ("=");
				else if (tail->t.attr_sel.match == CSS_MATCH_INCLUDES)
					g_print ("~=");
				else if (tail->t.attr_sel.match == CSS_MATCH_DASHMATCH)
					g_print ("|=");
				else if (tail->t.attr_sel.match == CSS_MATCH_PREFIXMATCH)
					g_print ("^=");
				else if (tail->t.attr_sel.match == CSS_MATCH_SUFFIXMATCH)
					g_print ("$=");
				else if (tail->t.attr_sel.match == CSS_MATCH_SUBSTRINGMATCH)
					g_print ("*=");
				else
					g_print (" ??? ");
				if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_IDENT) {
					g_print ("%s", html_atom_list_get_string (html_atom_list,
										  tail->t.attr_sel.val.a.id));
				}
				else if (tail->t.attr_sel.val.type == CSS_ATTR_VAL_STRING) {
					
					/* FIXME: escape string contents (see also
					   css_value_to_string). */
					g_print ("\"%s\"", tail->t.attr_sel.val.a.str);
				}
			}
			g_print ("]");
		}
		else if (tail->type == CSS_TAIL_ID_SEL) {
			g_print ("#%s", html_atom_list_get_string (html_atom_list, tail->t.id_sel.id));
		}
		else if (tail->type == CSS_TAIL_CLASS_SEL) {
			g_print (".%s", html_atom_list_get_string (html_atom_list, tail->t.id_sel.id));
		}
		else if (tail->type == CSS_TAIL_PSEUDO_SEL) {
			g_print (":");
			g_print ("%s", html_atom_list_get_string (html_atom_list, tail->t.pseudo_sel.name));

#if 0
			/* FIXME: Handle multiple arguments in functions */
			g_print (":");
			if (tail->t.pseudo.is_function) {
				g_print ("%s(", html_atom_list_get_string (html_atom_list, tail->t.pseudo.func_id));
			}
			g_print ("%s", html_atom_list_get_string (html_atom_list, tail->t.pseudo.id));

			if (tail->t.pseudo.is_function)
				g_print (")");
#endif
		}
		else {
			g_print ("< ??? >");
		}
	}
}


void
css_debug_print_selector (CssSelector *s)
{
	gint i;

	for (i = 0; i < s->n_simple; i++) {
		css_debug_print_simple_selector (s->simple[i]);

		if (i != s->n_simple - 1) {
			switch (s->comb[i]) {
			case CSS_COMBINATOR_PLUS:
				g_print (" + ");
				break;
			case CSS_COMBINATOR_GT:
				g_print (" > ");
				break;
			case CSS_COMBINATOR_EMPTY:
				g_print (" ");
				break;
			case CSS_COMBINATOR_TILDE:
				g_print (" ~ ");
				break;
			}
		}
	}
}

void
css_debug_print_declaration (CssDeclaration *decl)
{
	g_print ("%s: ", html_atom_list_get_string (html_atom_list, decl->property));

	css_debug_print_value (decl->expr);

	if (decl->important)
		g_print (" !important");
}

void
css_debug_print_ruleset (CssRuleset *rs)
{
	gint i;

	for (i = 0; i < rs->n_sel; i++) {
		css_debug_print_selector (rs->sel[i]);
		if (i != rs->n_sel - 1)
			g_print (", ");
	}

	g_print (" {\n");


	for (i = 0; i < rs->n_decl; i++) {
		g_print ("  ");
		css_debug_print_declaration (rs->decl[i]);
		g_print (";\n");
	}

	g_print ("}\n\n");
}

void
css_debug_print_statement (CssStatement *stat)
{
	gint i;
	
	switch (stat->type) {
	case CSS_MEDIA_RULE:
		g_print ("@media ");
		css_debug_print_value (stat->s.media_rule.media_list);
		g_print (" {\n");
		for (i = 0; i < stat->s.media_rule.n_rs; i++) {
			g_print ("  ");
			css_debug_print_ruleset (stat->s.media_rule.rs[i]);
		}
		g_print ("}\n");
//		g_free (str);
		break;
	case CSS_PAGE_RULE:
		g_print ("@page ");
		if (stat->s.page_rule.name != -1)
			g_print ("%s ", html_atom_list_get_string (html_atom_list, stat->s.page_rule.name));

		if (stat->s.page_rule.pseudo != -1)
			g_print (":%s ", html_atom_list_get_string (html_atom_list, stat->s.page_rule.pseudo));
		g_print ("{\n  ");
		for (i = 0; i < stat->s.page_rule.n_decl; i++) {
			css_debug_print_declaration (stat->s.page_rule.decl[i]);
			if (i != stat->s.page_rule.n_decl - 1)
				g_print ("; ");
		}

		g_print ("\n}\n");
		break;
	case CSS_FONT_FACE_RULE:
		g_print ("@font-face {\n  ");
		for (i = 0; i < stat->s.font_face_rule.n_decl; i++) {
			css_debug_print_declaration (stat->s.font_face_rule.decl[i]);
			if (i != stat->s.font_face_rule.n_decl - 1)
				g_print ("; ");
		}
		g_print ("\n}\n");

		break;

	case CSS_RULESET:
		css_debug_print_ruleset (stat->s.ruleset);
		break;
	default:
		g_warning ("eek");
	}
}

void
css_debug_print_stylesheet (CssStylesheet *stylesheet)
{
	gint i;

	for (i = 0; i < stylesheet->n_stat; i++)
		css_debug_print_statement (stylesheet->stat[i]);
}
