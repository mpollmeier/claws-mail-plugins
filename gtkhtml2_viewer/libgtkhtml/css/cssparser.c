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

#include <string.h>
#include "cssparser.h"
#include "cssvalue.h"
#include "util/htmlglobalatoms.h"
#include "cssdebug.h"

const gchar *css_dimensions[] = {
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

const gint css_n_dimensions = sizeof (css_dimensions) / sizeof (css_dimensions[0]);

static gint css_parser_parse_value (const gchar *buffer, gint start_pos, gint end_pos, CssValue **ret_val, const gchar *base_url);

/* FIXME: Needs more whitespace types */
static gint
css_parser_parse_whitespace (const gchar *buffer, gint start_pos, gint end_pos)
{
	gint pos = start_pos;

	while (pos < end_pos) {
		guchar c = buffer[pos];

		if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '\f')
			break;
		pos++;
	}

	return pos;
}

/* Find a particular character in the CSS buffer, skipping over quoted strings and blocks of
   code surrounded by parenthesis and braces */
static gint
css_parser_parse_to_char (const gchar *buffer, guchar ch, gint start_pos, gint end_pos)
{
	gint pos = start_pos;
	gint tmp_pos;
	gboolean in_double_quote = FALSE;
	gboolean in_single_quote = FALSE;

	while (pos < end_pos) {
		guchar c = buffer[pos];

		if (c == '\"' && !in_single_quote)
			in_double_quote = !in_double_quote;
		else if (c == '\'' && !in_double_quote)
			in_single_quote = !in_single_quote;
		else if (c == ch && !in_double_quote && !in_single_quote)
			break;
		else if (c == '{' && !in_double_quote && !in_single_quote) {
			tmp_pos = css_parser_parse_to_char (buffer, '}', pos + 1, end_pos);
			if (tmp_pos == end_pos)
				pos++;
			else
				pos = tmp_pos;
		}
		else if (c == '(' && !in_double_quote && !in_single_quote) {
			tmp_pos = css_parser_parse_to_char (buffer, ')', pos + 1, end_pos);
			if (tmp_pos == end_pos)
				pos++;
			else
				pos = tmp_pos;
		}
		pos++;
	}

	return pos;
}

static gint
css_parser_parse_escape (const gchar *p, gint start_pos, gint end_pos, gunichar *p_unicode)
{
	gint pos = start_pos;
	
	guchar c;

	if (pos + 2 > end_pos && p[pos] != '\\')
		return -1;

	c = p[pos + 1];

	if ((c >= '0' && c <= '9') ||
	    (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F')) {
		/* We have unicode */
		gint i;
		gunichar unicode;

		unicode = 0;

		for (i = 0; i < 7 && pos + 1 + i < end_pos; i++) {
			c = p[pos + 1 + i];

			if (c >= '0' && c <= '9')
				unicode = (unicode << 4) + (c - '0');
			else if (c >= 'a' && c <= 'f')
				unicode = (unicode << 4) + (10 + c - 'a');
			else if (c >= 'A' && c <= 'F')
				unicode = (unicode << 4) + (10 + c - 'A');
			else if (c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
				 c == '\f') {
				if (c == '\r' && (pos + 2 + i < end_pos) && p[pos + 2 + i] == '\n')
					i+=2;
				else
					i++;
				break;
			}
			else
				break;
		}

		*p_unicode = unicode;
		return pos + 1 + i;

	}
	else if (c >= ' ' && c <= '~') {
		/* We have a plain escaped character */
		*p_unicode = c;
		return pos + 2;
	}
	else if (c & 0x80) {
		/* We have a (needlessly) escaped nonascii character */
		g_error ("eek, we don't handle utf8 yet");
	}

	
	return -1;
}

static gchar *
css_parser_unescape (const gchar *p, gint len)
{
	gchar *unesc;
	gint unesc_pos;
	gint pos;
	gchar *result;
	gchar c;
	gunichar unicode;
	
	unesc = g_malloc (len);
	unesc_pos = 0;

	for (pos = 0; pos < len;) {
		c = ((guchar *)p)[pos];
		
		if (c == '\\') {
			pos = css_parser_parse_escape (p, pos, len, &unicode);
			unesc_pos += g_unichar_to_utf8 (unicode, unesc + unesc_pos);
		}
		else {
			/* FIXME: Case conversion /ac */
			unesc [unesc_pos++] = c;
			pos++;
		}
	}
	
	result = g_strndup (unesc, unesc_pos);
	
	g_free (unesc);
	
	return result;

}

/* FIXME: Needs to support UTF8 */
static gint
css_parser_parse_string (const gchar *str, gint start_pos, gint end_pos, gchar **ret_val)
{
	guchar c;
	gchar *buffer;
	gint buffer_size, buffer_size_max;
	gchar quote_char, nonquote_char;
	gint pos = start_pos;
	gboolean has_escape = FALSE;
	

	buffer_size = 0;
	buffer_size_max = 8;
	buffer = g_malloc (buffer_size_max);

	quote_char = str[pos++];
	nonquote_char = (quote_char == '"') ? '\'': '"';

	while (pos < end_pos) {
		c = str[pos++];

		if (c == quote_char)
			break;
		else if (c == '\\') {
			gunichar unicode;
	  
			pos = css_parser_parse_escape (str, pos -1, end_pos, &unicode);
			if (pos < 0)
				return -1;
		
			has_escape = TRUE;
		}
		else if ((c >= '(' && c <= '~') ||
			 c == ' ' || c == '!' || (c >= '#' && c <= '&') || c == '\t' ||
			 c == nonquote_char) {
			if (!has_escape) {
				if (buffer_size == buffer_size_max)
					buffer = g_realloc (buffer, (buffer_size_max <<= 1));
				buffer[buffer_size++] = c;
			}
		}
		else if (c > 0x80) {
			if (!has_escape) {
				if (buffer_size == buffer_size_max)
					buffer = g_realloc (buffer, (buffer_size_max <<= 1));
				buffer[buffer_size++] = c;
			}
		}
		else {
			g_free (buffer);
			return -1;
		}
	}

	if (ret_val) {
		if (has_escape)
			*ret_val = css_parser_unescape (str + start_pos, pos - start_pos);
		else 
			*ret_val = g_strndup (buffer, buffer_size);
	}

	g_free (buffer);

	return pos;

}


/* FIXME: Needs to support UTF8 */
static gint
css_parser_parse_ident (const gchar *buffer, gint start_pos, gint end_pos, HtmlAtom *atom)
{
	gint pos = start_pos;
	guchar c = buffer[pos];
	gboolean has_escape = FALSE;
	gchar *temp_str;

	if (c == '-')
		c = buffer[++pos];
	
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		pos++;
	else if (c == '\\') {
		gunichar unicode;
	  
		pos = css_parser_parse_escape (buffer, pos, end_pos, &unicode);
		if (pos < 0)
			return -1;
		
		has_escape = TRUE;
		
	}
	else
		return -1;

	while (pos < end_pos) {
		c = buffer[pos];
		
		if ((c >= 'A' && c <= 'Z') ||
		    (c >= 'a' && c <= 'z') ||
		    (c >= '0' && c <= '9') ||
		    c == '-') {
			pos++;
		}
		else if (c == '\\') {
			gunichar unicode;
			
			pos = css_parser_parse_escape (buffer, pos, end_pos, &unicode);
			
			if (pos < 0)
				break;
			
			has_escape = TRUE;
		}

		else
			break;
	}
	
	/* FIXME: Optimize away one strndup here */
	if (has_escape) 
		temp_str = css_parser_unescape (buffer + start_pos, pos - start_pos);
	else 
		temp_str = g_strndup (buffer + start_pos,
				      pos - start_pos);

	if (atom)
		*atom = html_atom_list_get_atom (html_atom_list, temp_str);

	g_free (temp_str);
	
	return pos;
}

/* FIXME: Needs to support escaping and UTF8 */
static gint
css_parser_parse_name (const gchar *buffer, gint start_pos, gint end_pos, HtmlAtom *atom)
{
	gint pos = start_pos;
	guchar c = buffer[pos];

	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
	    (c >= '0' && c <= '9'));
	else
		return -1;

	while (pos < end_pos) {
		c = buffer[pos];
		
		if ((c >= 'A' && c <= 'Z') ||
		    (c >= 'a' && c <= 'z') ||
		    (c >= '0' && c <= '9') ||
		    c == '-') {
			pos++;
		}
		else
			break;
	}

	if (pos == start_pos + 1 && buffer[start_pos] == '-')
		return -1;
	
	if (atom)
		*atom = html_atom_list_get_atom_length (html_atom_list, buffer + start_pos, pos - start_pos);
	
	return pos;
}


static gint
css_parser_scan_number (const gchar *buffer, gint start_pos, gint end_pos)
{
	guchar c;
	gint pos = start_pos;

	if ((buffer[pos] < '0' || buffer[pos] > '9') &&
	    buffer[pos] != '.' && buffer[pos] != '+' && buffer[pos] != '-')
		return -1;

	if (pos < end_pos && (buffer[pos] == '+' || buffer[pos] == '-'))
		pos++;
	
	for (; pos < end_pos; pos++)
		if ((c = buffer[pos]) < '0' || c > '9')
			break;

	if (pos < end_pos && buffer[pos] == '.') {
		pos++;

		if (pos < end_pos && ((c = buffer[pos++]) < '0' || c > '9'))
			return -1;

		for (; pos < end_pos; pos++)
			if ((c = buffer[pos]) < '0' || c > '9')
				break;
	}

	return pos;
}

static gdouble
css_parser_parse_number (const gchar *buffer, gint start_pos, gint end_pos)
{
	guchar c;
	gdouble d, deci;
	gint pos = start_pos;
	gdouble sign = 1.0;
	
	d = 0;

	if (pos < end_pos && ((c = buffer[pos]) == '+' || c == '-')) {
		pos++;
		sign = (c == '-' ? -1.0 : 1.0);
	}
	
	for (; pos < end_pos; pos ++) {
		if ((c = buffer[pos]) >= '0' && c <= '9')
			d = d * 10.0 + (c - '0');
		else
			break;
	}
	
	if (pos < end_pos && buffer[pos] == '.') {
		pos++;
		deci = 1.0;
		for (; pos < end_pos; pos++) {
			if ((c = buffer[pos]) >= '0' && c <= '9') {
				deci *= 0.1;
				d += deci * (c - '0');
			}
			else
				break;
		}
	}

	return d * sign;
}

static gint
css_parser_parse_term (const gchar *buffer, gint start_pos, gint end_pos, CssValue **ret_val, const gchar *base_url)
{
	gint pos;
	HtmlAtom atom;

	if (ret_val)
		*ret_val = NULL;
	
	/* First, check if we have a color */
	if (buffer[start_pos] == '#') {
		
		pos = css_parser_parse_name (buffer, start_pos + 1, end_pos, &atom);
		
		if (pos != -1) {
			if (ret_val) {
				gchar *str = g_strndup (buffer + start_pos, pos - start_pos);
				
				*ret_val = css_value_string_new (str);
				g_free (str);
				
			}
			return pos;
		}
	}
	
	pos = css_parser_parse_ident (buffer, start_pos, end_pos, &atom);

	if (pos != -1) {
		/* Check if we have a function */
		if (buffer[pos] == '(') {
			gint func_end = css_parser_parse_to_char (buffer, ')', pos + 1, end_pos);
			gint save_pos = pos;
			CssValue *val;

			if (func_end == end_pos) {
				return -1;
			}
			
			pos = css_parser_parse_value (buffer, save_pos + 1, func_end, &val, base_url);

			if (pos == -1) {
				/*
				 * If it is not a value just store as string
				 */
				gchar *str;

				str = g_strndup (buffer + save_pos + 1, func_end - save_pos - 1);
				val = css_value_string_new (str);
				g_free (str);
				pos = func_end;
			}
			if (pos != -1) {
				if (ret_val) {
					*ret_val = css_value_function_new (atom, val);
					
					if ((*ret_val)->v.function->name == HTML_ATOM_URL &&
					    (*ret_val)->v.function->args) {
						gchar *str = css_value_to_string ((*ret_val)->v.function->args);
						
						if (base_url && str) {
							regex_t reg;
							
							regcomp (&reg, "^[a-zA-Z][a-zA-Z0-9.+-]*:", REG_NOSUB);
							if (((*ret_val)->v.function->args->v.s[0] != '/') && (regexec (&reg, (*ret_val)->v.function->args->v.s, 0, NULL, 0))) {
								gchar *temp_base, *eptr;
								
								temp_base = g_strdup(base_url);
								eptr = strrchr(temp_base, '?');
								
								if (eptr != NULL)
									*eptr = '\0';
									
								eptr = strrchr(temp_base, '/');
								if (eptr != NULL) {
									gchar *temp_arg = g_strdup((*ret_val)->v.function->args->v.s);
								
									*eptr = '\0';
									
									g_free((*ret_val)->v.function->args->v.s);
									(*ret_val)->v.function->args->v.s = g_strconcat(temp_base, "/",  temp_arg, NULL);
									g_free(temp_arg);
								}
								
								g_free(temp_base);
							}
							regfree (&reg);
						}
						
						g_free (str);
					}
				}
				else {
					css_value_unref (val);
				}
			}

			/* This is due to the ) */
			pos++; 
		}
		else {
			if (ret_val)
				*ret_val = css_value_ident_new (atom);
		}

		return pos;
	}
	
	
	pos = css_parser_scan_number (buffer, start_pos, end_pos);
	
	if (pos != -1) {
		gdouble d = css_parser_parse_number (buffer, start_pos, end_pos);
		
		/* Check for percentages or units */
		if (buffer[pos] == '%') {
			if (ret_val)
				*ret_val = css_value_dimension_new (d, CSS_PERCENTAGE);
			
			return pos + 1;
		}
		/* Check for dimensions */
		else {
			gint i;
			for (i = CSS_NUMBER; i < css_n_dimensions; i++) {
				const gchar *dim = css_dimensions[i];
				gint len = strlen (dim);
				if ((pos + len <= end_pos) &&
				    strncasecmp (dim, buffer + pos, len) == 0) {
					guchar c = buffer[pos + len];
					if (!((c >= 'a' && c <= 'z') ||
					      (c >= 'A' && c <= 'Z'))) {
						if (ret_val)
							*ret_val = css_value_dimension_new (d, i);
						return pos + len;
					}
				}
			}
		}

/*		g_error ("t1"); */
		return -1;
	}

	/* Check if we have a string */
	if (buffer[start_pos] == '\'' ||
	    buffer[start_pos] == '"') {
		gchar *str;
		pos = css_parser_parse_string (buffer, start_pos, end_pos, &str);
		
		if (pos != -1) {
			if (ret_val)
				*ret_val = css_value_string_new (str);
			g_free (str);
			return pos;
		}
	}

	return -1;
}

static gint
css_parser_parse_value (const gchar *buffer, gint start_pos, gint end_pos, CssValue **ret_val, const gchar *base_url)
{
	gint pos = start_pos;
	gint n = 0;
	CssValue *list = NULL;
	CssValue *term = NULL;

	pos = css_parser_parse_whitespace (buffer, pos, end_pos);

	if (pos == end_pos)
		return -1;
	
	while (pos < end_pos) {
		static gchar list_sep;

		/* If it's the second time we get here, we need to create a value list */
		if (n == 1) {
			list = css_value_list_new ();

			/* Add the term parsed the last time */
			css_value_list_append (list, term, list_sep);
		}

		pos = css_parser_parse_term (buffer, pos, end_pos, &term, base_url);
		
		if (pos == -1) {
			if (list)
				css_value_unref (list);
			return -1;
		}
		
		n++;

		pos = css_parser_parse_whitespace (buffer, pos, end_pos);

		if (pos == end_pos) {
			if (n == 1) 
				*ret_val = term;
			else {
				/* Add the final value */
				css_value_list_append (list, term, 0);
				*ret_val = list;
			}
			return pos;
		}
		
		if (buffer[pos] == ',' || buffer[pos] == '/') {
			list_sep = buffer[pos];
			pos++;
		}
		else {
			/* Try and parse the term to see if it's valid */
			if (css_parser_parse_term (buffer, pos, end_pos, NULL, base_url) == -1) {
				if (term)
					css_value_unref (term);
				if (list)
					css_value_unref (list);
				return -1;
			}
			list_sep = ' ';

		}			

		/* Add the term if we know we have a list*/
		if (n > 1)
			css_value_list_append (list, term, list_sep);
		
		pos = css_parser_parse_whitespace (buffer, pos, end_pos);

		if (pos == end_pos) {
			/* Unfinished list : work-around */
			if (n == 1)
				*ret_val = term;
			else {
				css_value_list_append (list, term, 0);
				*ret_val = list;
			}
			return pos;
		}
	}

	return pos;
}


static gint
css_parser_parse_attr_selector (const gchar *buffer, gint start_pos, gint end_pos, CssTail *tail)
{
	HtmlAtom attr;
	gchar *str;
	gint pos = start_pos;

	pos = css_parser_parse_ident (buffer, pos, end_pos, &attr);
	pos = css_parser_parse_whitespace (buffer, pos, end_pos);

	if (tail) {
		tail->t.attr_sel.att = attr;
		tail->type = CSS_TAIL_ATTR_SEL;
	}
	
	if (pos == end_pos) {
		if (tail)
			tail->t.attr_sel.match = CSS_MATCH_EMPTY;
	}
	else {
		if (buffer[pos] == '=') {
			if (tail)
				tail->t.attr_sel.match = CSS_MATCH_EQ;
			pos++;
		}
		else if (buffer[pos] == '~' &&
			 buffer[pos + 1] == '=') {
			pos += 2;
			if (tail)
				tail->t.attr_sel.match = CSS_MATCH_INCLUDES;
		}
		else if (buffer[pos] == '|' &&
			 buffer[pos + 1] == '=') {
			pos += 2;
			if (tail)
				tail->t.attr_sel.match = CSS_MATCH_DASHMATCH;
		}
		else {
			return -1;
		}

		pos = css_parser_parse_whitespace (buffer, pos, end_pos);
		
		if (buffer[pos] == '"' ||
		    buffer[pos] == '\'') {
			pos = css_parser_parse_string (buffer, pos, end_pos, &str);
			if (tail) {
				tail->t.attr_sel.val.type = CSS_ATTR_VAL_STRING;
				tail->t.attr_sel.val.a.str = str;
			}
			else if (str)
				g_free (str);
		}
		else {
			pos = css_parser_parse_ident (buffer, pos, end_pos, &attr);

			pos = css_parser_parse_whitespace (buffer, pos, end_pos);

			if (pos != end_pos)
				return -1;

			if (tail) {
				tail->t.attr_sel.val.type = CSS_ATTR_VAL_IDENT;
				tail->t.attr_sel.val.a.id = attr;
			}
		}
		
/*	g_print ("Attr selector: %s\n", g_strndup (buffer + start_pos, end_pos - start_pos)); */
		
	}

	return pos;
}

static gint
css_parser_parse_simple_selector (const gchar *buffer, gint start_pos, gint end_pos, CssSimpleSelector **ret_val)
{
	gint pos;
	CssSimpleSelector *result;
	gint c;
	HtmlAtom element_name;
	gint n_tail = 0;
	gint n_tail_max = 1;
	CssTail *tail;

	pos = css_parser_parse_ident (buffer, start_pos, end_pos, &element_name);
	c = buffer[start_pos];
		
	if (pos == -1 && c != '*' &&
	    c != '#' && c != '.' && c != ':')
		return -1;

	if (pos == -1)
		pos = start_pos;

	result = g_new (CssSimpleSelector, 1);
	tail = g_new (CssTail, n_tail_max);
	tail->type = -1;
	tail->t.attr_sel.val.type = -1;
	
	if (c == '*') {
		result->is_star = TRUE;
		pos++;
	}
	else if (c == '#' || c == '.' || c == ':') {
		result->is_star = TRUE;
	}
	else {
		result->is_star = FALSE;
		result->element_name = element_name;
	}
		
	while (pos < end_pos) {
		c = buffer[pos];
		if (c == '#') {
			HtmlAtom id;
			pos = css_parser_parse_ident (buffer, pos + 1, end_pos, &id);

			if (pos == -1) {
				g_error ("1. return -1");
				return -1;
			}
			
			if (n_tail == n_tail_max)
				tail = g_realloc (tail, sizeof (CssTail) *
						  (n_tail_max <<= 1));
			tail [n_tail].type = CSS_TAIL_ID_SEL;
			tail [n_tail].t.id_sel.id = id;
			n_tail++;
			
		}
		else if (c == '.') {
			HtmlAtom id;

			pos = css_parser_parse_ident (buffer, pos + 1, end_pos, &id);
			
			if (pos == -1) {
				gint i;
				for (i = 0; i < n_tail; i++)
					css_tail_destroy (&tail[i]);
				g_free (tail);
				g_free (result);
				
				return -1;
			}
			
			if (n_tail == n_tail_max)
				tail = g_realloc (tail, sizeof (CssTail) *
						  (n_tail_max <<= 1));
			tail [n_tail].type = CSS_TAIL_CLASS_SEL;
			tail [n_tail].t.class_sel.css_class = id;
			n_tail++;

/*			g_print ("class sel!\n"); */
		}
		else if (c == '[') {
			gint tmp_pos;

			pos++;
			pos = css_parser_parse_whitespace (buffer, pos, end_pos);

			tmp_pos = css_parser_parse_to_char (buffer, ']', pos, end_pos);

			if (css_parser_parse_attr_selector (buffer, pos, tmp_pos, NULL) != -1) {
				if (n_tail == n_tail_max)
					tail = g_realloc (tail, sizeof (CssTail) *
							  (n_tail_max <<= 1));
				css_parser_parse_attr_selector (buffer, pos, tmp_pos, &tail[n_tail]);
				n_tail++;
				pos = tmp_pos + 1;
			}
			else {
				gint i;
				for (i = 0; i < n_tail; i++)
					css_tail_destroy (&tail[i]);
				g_free (tail);
				g_free (result);
				
				return -1;
			}
			
		}
		else if (c == ':') {
			/* FIXME: Handle functions */
			
			HtmlAtom id;

			/* FIXME: Should be parse_name */
			pos = css_parser_parse_ident (buffer, pos + 1, end_pos, &id);
			
			if (pos == -1) {
				gint i;
				for (i = 0; i < n_tail; i++)
					css_tail_destroy (&tail[i]);
				g_free (tail);
				g_free (result);
				
				return -1;
			}

			if (n_tail == n_tail_max)
				tail = g_realloc (tail, sizeof (CssTail) *
						  (n_tail_max <<= 1));
			tail [n_tail].type = CSS_TAIL_PSEUDO_SEL;
			tail [n_tail].t.pseudo_sel.name = id;
			n_tail++;
		}
		else
			break;
	}

	result->n_tail = n_tail;
	result->tail = tail;
	
	if (ret_val)
		*ret_val = result;
	else 
		css_simple_selector_destroy (result);
	
	return pos;
/*	g_print ("pos == %d\n", pos); */
	
}

static CssSelector *
css_parser_parse_selector (const gchar *buffer, gint start_pos, gint end_pos)
{
	gint pos = start_pos;
	CssSelector *result;
	gint n_simple;
	gint n_simple_max;
	CssSimpleSelector **simple;
	CssCombinator *comb;
	CssSimpleSelector *ss;
	
/*	g_print ("parsing single selector: \"%s\"\n", g_strndup (buffer + start_pos, end_pos - start_pos)); */

	n_simple_max = 1;
	simple = g_new (CssSimpleSelector *, n_simple_max);
	comb = g_new (CssCombinator, n_simple_max);
	n_simple = 0;
	
	while (pos < end_pos) {
		pos = css_parser_parse_simple_selector (buffer, pos, end_pos, &ss);

		if (pos == -1) {
			gint i;
			for (i = 0; i < n_simple; i++)
				css_simple_selector_destroy (simple[i]);
			g_free (simple);
			g_free (comb);
			
			return NULL;
		}
		
		if (n_simple == n_simple_max) {
			n_simple_max <<= 1;
			simple = g_realloc (simple, sizeof (CssSimpleSelector *) *
					    n_simple_max);
			comb  = g_realloc (comb, sizeof (CssCombinator) *
					   n_simple_max);
		}

		simple [n_simple++] = ss;
		
		pos = css_parser_parse_whitespace (buffer, pos, end_pos);

		if (pos == end_pos)
			break;
		
		if (buffer[pos] == '+') {
			pos++;
			comb [n_simple - 1] = CSS_COMBINATOR_PLUS;
		}
		else if (buffer[pos] == '>') {
			pos++;
			comb [n_simple - 1] = CSS_COMBINATOR_GT;
		}
		else if (buffer[pos] == '~') {
			pos++;
			comb [n_simple - 1] = CSS_COMBINATOR_TILDE;
		}
		else if (css_parser_parse_simple_selector (buffer, pos, end_pos, NULL) != -1) {
			comb [n_simple -1] = CSS_COMBINATOR_EMPTY;
		}
		else {
			gint i;
			for (i = 0; i < n_simple; i++)
				css_simple_selector_destroy (simple[i]);
			g_free (simple);
			g_free (comb);
			
			return NULL;
		}

		pos = css_parser_parse_whitespace (buffer, pos, end_pos);
	}

	result = g_new (CssSelector, 1);
	result->n_simple = n_simple;
	result->simple = simple;
	result->comb = comb;

	css_selector_calc_specificity (result);
	
	return result;
}

static CssSelector **
css_parser_parse_selectors (const gchar *buffer, gint start_pos, gint end_pos, gint *num_sel)
{
	gint pos = start_pos;
	gint cur_pos = start_pos;
	CssSelector **sel;
	gint n_sel_max = 1;
	gint n_sel = 0;
	CssSelector *selector;

	sel = g_new (CssSelector *, n_sel_max);

	
/*	g_print ("Parsing selectors, \"%s\"\n", g_strndup (buffer + start_pos, end_pos - start_pos)); */

	while (pos < end_pos) {
		pos = css_parser_parse_to_char (buffer, ',', pos, end_pos);

		selector = css_parser_parse_selector (buffer, cur_pos, pos);

		if (selector) {
			if (n_sel == n_sel_max)
				sel = g_realloc (sel, sizeof (CssSelector *) *
						 (n_sel_max <<= 1));
			sel[n_sel++] = selector;
		}
		
		pos = css_parser_parse_whitespace (buffer, ++pos, end_pos);
		
		cur_pos = pos;
	}

	if (num_sel)
		*num_sel = n_sel;


	if (n_sel == 0) {
		g_free (sel);
		return NULL;
	}

	return sel;

}

static gint
css_parser_parse_declaration (const gchar *buffer, gint start_pos, gint end_pos, CssDeclaration **ret_val, const gchar *base_url)
{
	CssValue *value;
	CssDeclaration *result;
	HtmlAtom prop;
	gboolean important = FALSE;
	gint prio_pos;
	gint pos = css_parser_parse_to_char (buffer, ':', start_pos, end_pos);

	if (ret_val)
		*ret_val = NULL;
	
	if (css_parser_parse_ident (buffer, start_pos, pos, &prop) == -1) {
/*		return -1; */
		return end_pos;
	}
	
	pos++;
	
	prio_pos = css_parser_parse_to_char (buffer, '!', pos, end_pos);
	
	if (prio_pos != end_pos) {
		gint pos2;
		HtmlAtom imp_atom;

		/* Check if we have an !important symbol */
		pos2 = css_parser_parse_whitespace (buffer, prio_pos + 1, end_pos);
		if (css_parser_parse_ident (buffer, pos2, end_pos + 1, &imp_atom) != -1 &&
		    imp_atom == HTML_ATOM_IMPORTANT) {
			important = TRUE;
		}
	}

	pos = css_parser_parse_whitespace (buffer, pos, prio_pos);

	pos = css_parser_parse_value (buffer, pos, prio_pos, &value, base_url);
	
	if (pos == -1) {
		return end_pos;
	}

	
	if (ret_val) {

		result = g_new (CssDeclaration, 1);
		result->property = prop;
		result->important = important;
		result->expr = value;

		*ret_val = result;
	} else {
		css_value_unref (value);
	}

	return end_pos;
}

static CssDeclaration **
css_parser_parse_declarations (const gchar *buffer, gint start_pos, gint end_pos, gint *num_decl, const gchar *base_url)
{
	gint pos = start_pos;
	gint cur_pos = start_pos;
	CssDeclaration **decl;
	gint n_decl = 0;
	gint n_decl_max = 4;
	
/*	g_print ("Parsing declarations, \"%s\"\n", g_strndup (buffer + start_pos, end_pos - start_pos)); */
	
	decl = g_new (CssDeclaration *, n_decl_max);
	while (pos < end_pos) {
		CssDeclaration *declaration;
		
		pos = css_parser_parse_to_char (buffer, ';', pos, end_pos);

		pos = css_parser_parse_declaration (buffer, cur_pos, pos, &declaration, base_url);

		if (declaration) {
			if (n_decl == n_decl_max)
				decl = g_realloc (decl, sizeof (CssDeclaration *) *
						  (n_decl_max <<= 1));
			decl [n_decl++] = declaration;
		}
		
		pos = css_parser_parse_whitespace (buffer, ++pos, end_pos);
		
		cur_pos = pos;
	}

	if (num_decl)
		*num_decl = n_decl;

	return decl;
}

static gint
css_parser_parse_ruleset (const gchar *buffer, gint start_pos, gint end_pos, CssRuleset **ret_val, const gchar *base_url)
{
	gint cur_pos;
	gint pos;

	CssRuleset *ruleset;
	
	gint n_decl;
	CssDeclaration **decl;

	gint n_sel;
	CssSelector **sel;

	if (ret_val)
		*ret_val = NULL;
	
	pos = css_parser_parse_to_char (buffer, '{', start_pos, end_pos);
	
	if (pos == end_pos)
		return -1;

/*	g_print ("ok, looks like we have some sort of validity here! :)\n"); */

	start_pos = css_parser_parse_whitespace (buffer, start_pos, pos);
	sel = css_parser_parse_selectors (buffer, start_pos, pos, &n_sel);

	cur_pos = pos + 1;
	pos = css_parser_parse_to_char (buffer, '}', cur_pos, end_pos);

	if (cur_pos == end_pos || sel == NULL) {
		gint i;

		for (i = 0; i < n_sel; i++)
			css_selector_destroy (sel[i]);
		g_free (sel);
		return pos + 1;
	}
	if (pos == end_pos) {
		gint i;

		for (i = 0; i < n_sel; i++)
			css_selector_destroy (sel[i]);
		g_free (sel);

		return pos + 1;
	}
	cur_pos = css_parser_parse_whitespace (buffer, cur_pos, end_pos);

	decl = css_parser_parse_declarations (buffer, cur_pos, pos, &n_decl, base_url);

	pos++;
	
	ruleset = g_new (CssRuleset, 1);
	ruleset->n_decl = n_decl;
	ruleset->decl = decl;
	ruleset->n_sel = n_sel;
	ruleset->sel = sel;

/*	g_print ("Setting n_sel to %d\n", n_sel); */
	
	if (ret_val) {
		*ret_val = ruleset;
	} else {
		css_ruleset_destroy (ruleset);
	}

	return pos;
}

static gchar *
css_parser_prepare_stylesheet (const gchar *str, gint len)
{
	gchar *result;
	gint buffer_size = 0, buffer_size_max = 8;
	gint pos = 0;
	
	result = g_malloc (buffer_size_max);

	while (pos < len-1) {
		if (str[pos] == '/' &&
		    pos + 1 <= len &&
		    str[pos + 1] == '*') {
			while (pos + 1 < len &&
				(str[pos] != '*' ||
				 str[pos + 1] != '/'))
				pos++;

			pos += 2;
		}
		if (buffer_size == buffer_size_max - 1)
			result = g_realloc (result, (buffer_size_max <<= 1));
		
		result[buffer_size++] = str[pos++];
		
	}

	result[buffer_size] = '\0';

	return result;
}

static gint
css_parser_parse_atkeyword (const gchar *buffer, gint start_pos, gint end_pos, CssStatement **ret_val, const gchar *base_url)
{
	gint pos = start_pos;
	gint tmp_pos, cur_pos;
	CssStatement *result;
	CssValue *val_list;

	gint n_braces = 0;
	HtmlAtom keyword, name = -1, pseudo = -1;
	
	gint n_decl = 0;
	CssDeclaration **decl;

	gint n_rs = 0;
	gint n_rs_max = 4;
	CssRuleset **rs;
	
	pos = css_parser_parse_ident (buffer, pos, end_pos, &keyword);

	switch (keyword) {
	case HTML_ATOM_MEDIA:
		cur_pos = css_parser_parse_to_char (buffer, '{', pos, end_pos);

		val_list = css_value_list_new ();
		
		tmp_pos = pos;
		while (tmp_pos < cur_pos) {
			CssValue *val;
			HtmlAtom name;

			tmp_pos = css_parser_parse_whitespace (buffer, tmp_pos, cur_pos);
			tmp_pos = css_parser_parse_ident (buffer, tmp_pos, cur_pos, &name);
			val = css_value_ident_new (name);
			css_value_list_append (val_list, val, ',');
			
			tmp_pos = css_parser_parse_whitespace (buffer, tmp_pos, cur_pos);

			if (tmp_pos == cur_pos)
				break;
			
			if (buffer[tmp_pos] == ',') {
				tmp_pos++;
			}
			else {
				
			}
		}

		cur_pos++;
		tmp_pos = css_parser_parse_to_char (buffer, '}', cur_pos, end_pos);
		tmp_pos++;

		
		rs = g_new0 (CssRuleset *, n_rs_max);

/*		g_print ("wheee: \"%s\"\n", g_strndup (buffer + cur_pos, tmp_pos - cur_pos )); */
		pos = cur_pos;
		while (pos < tmp_pos) {
			CssRuleset *ruleset;
			
			pos = css_parser_parse_ruleset (buffer, pos, tmp_pos, &ruleset, base_url);

			if (n_rs == n_rs_max) 
				rs = g_realloc (rs, sizeof (CssRuleset *) *
						(n_rs_max <<= 1));
			rs[n_rs++] = ruleset;
			     
		}

/*		g_print ("Here is rs: %p\n", rs); */
		
		pos = css_parser_parse_whitespace (buffer, tmp_pos + 1, end_pos);
		
		result = g_new0 (CssStatement, 1);
		result->type = CSS_MEDIA_RULE;
		result->s.media_rule.rs = rs;
		result->s.media_rule.n_rs = n_rs;
		result->s.media_rule.media_list = val_list;

/*		g_print ("VALUE LIST!!: %d\n", css_value_list_get_length (val_list)); */
		
		if (ret_val)
			*ret_val = result;

/*		g_print ("Going to return: %d\n", pos); */
		
		return pos + 1;
		
		break;
	case HTML_ATOM_PAGE:
		pos = css_parser_parse_whitespace (buffer, pos, end_pos);
		cur_pos = css_parser_parse_to_char (buffer, '{', pos, end_pos);
		tmp_pos = pos;

		while (tmp_pos < cur_pos) {

			tmp_pos = css_parser_parse_whitespace (buffer, tmp_pos, cur_pos);

			/* First, look for an ident */
			if (css_parser_parse_ident (buffer, tmp_pos, cur_pos, NULL) != -1) {
				tmp_pos = css_parser_parse_ident (buffer, tmp_pos, cur_pos, &name);
/*				g_print ("yes, we found the ident\n"); */
			}
			else if (buffer[tmp_pos] == ':') {
				if ((tmp_pos = css_parser_parse_ident (buffer, tmp_pos + 1, cur_pos, &pseudo)) == -1)
					return -1;
			}
		}

		pos = cur_pos + 1;
		cur_pos = css_parser_parse_to_char (buffer, '}', pos, end_pos);

		pos = css_parser_parse_whitespace (buffer, pos, cur_pos);
		
		decl = css_parser_parse_declarations (buffer, pos, cur_pos, &n_decl, base_url);

		g_print ("N_decl is: %d\n", n_decl);
		
		result = g_new0 (CssStatement, 1);
		result->type = CSS_PAGE_RULE;
		result->s.page_rule.name = name;
		result->s.page_rule.pseudo = pseudo;
		result->s.page_rule.n_decl = n_decl;
		result->s.page_rule.decl = decl;

		*ret_val = result;

		return cur_pos + 1;
	case HTML_ATOM_FONT_FACE:
		pos = css_parser_parse_to_char (buffer, '{', pos, end_pos);
		pos = css_parser_parse_whitespace (buffer, pos + 1, end_pos);
		cur_pos = css_parser_parse_to_char (buffer, '}', pos, end_pos);

		decl = css_parser_parse_declarations (buffer, pos, cur_pos, &n_decl, base_url);
		result = g_new0 (CssStatement, 1);
		result->type = CSS_FONT_FACE_RULE;

		result->s.font_face_rule.n_decl = n_decl;
		result->s.font_face_rule.decl = decl;

		*ret_val = result;
		
		return cur_pos + 1;
		
		break;
	case HTML_ATOM_IMPORT: {
		gchar *import_url;
		const gchar *s_url = NULL, *e_url = NULL;

		cur_pos = css_parser_parse_to_char (buffer, ';', pos, end_pos);

		if (strchr (buffer + pos, '(')) {
			s_url = strchr (buffer + pos, '(');
			e_url = strchr (s_url, ')');
		} else if (strchr (buffer + pos, '\"')) {
			s_url = strchr (buffer + pos, '\"');
			e_url = strchr (s_url + 1, '\"');
		} else if (strchr (buffer + pos, '\'')) {
			s_url = strchr (buffer + pos, '\'');
			e_url = strchr (s_url + 1, '\'');
		} 

		if (!s_url || !e_url || ((e_url - s_url) < 1)) {
			g_warning ("Invalid @import line");
			*ret_val = NULL;
			return cur_pos + 1;
		}
		
		s_url++;
		e_url--;
		
		if ((*s_url == '\"') && (*e_url == '\"')) {
			s_url++;
			e_url--;
		} else if ((*s_url == '\'') && (*e_url == '\'')) {
			s_url++;
			e_url--;
		}

		if (s_url > e_url) {
			g_warning ("Invalid @import line");
			*ret_val = NULL;
			return cur_pos + 1;
		}
				
		import_url = g_strndup (s_url, strlen (s_url) - strlen (e_url) + 1);
		
		result = g_new0 (CssStatement, 1);
		result->type = CSS_IMPORT_RULE;

		result->s.import_rule.url = css_value_string_new (import_url);

		*ret_val = result;

		return cur_pos + 1;

		break;
	}
	default:
		g_warning ("Unhandled keyword %d - %s", keyword, buffer);
		/* Unknown keyword detected, skip to next block */
		while (pos < end_pos) {
			/* Handle a dangling semi-colon */
			if (buffer[pos] == ';' && buffer[pos + 1] != ';' && n_braces == 0)
				break;
			else if (buffer[pos] == '{')
				n_braces++;
			else if (buffer[pos] == '}') {
				n_braces--;
				if (n_braces == 0)
					break;
			}
			
			pos++;
		}
		*ret_val = NULL;
	}

	return pos;
}

CssRuleset *
css_parser_parse_style_attr (const gchar *buffer, gint len, const gchar *base_url)
{
	CssRuleset *result;
	
	CssDeclaration **decl;
	gint n_decl;
	
	/* FIXME: Are comments allowed here? */

	decl = css_parser_parse_declarations (buffer, 0, len, &n_decl, base_url);

	if (!decl)
		return NULL;
	
	result = g_new (CssRuleset, 1);
	result->n_decl = n_decl;
	result->decl = decl;
	result->n_sel = 0;
	result->sel = NULL;
	
	return result;
}

CssStylesheet *
css_parser_parse_stylesheet (const gchar *str, gint len, const gchar *base_url)
{
	CssStylesheet *result;
	GSList *stat = NULL;
	gchar *buffer;
	gint pos = 0;
	gint end_pos;

	buffer = css_parser_prepare_stylesheet (str, len);
	len = strlen (buffer);

	pos = css_parser_parse_whitespace (buffer, pos, len);
	end_pos = len;
	while (pos < len) {
		if (buffer[pos] == '@' && pos + 1 < len &&
		    ((buffer[pos + 1] >= 'a' && buffer[pos + 1] <= 'z') ||
		     (buffer[pos + 1] >= 'A' && buffer[pos + 1] <= 'Z'))) {
			CssStatement *atstatement = NULL;
			pos = css_parser_parse_atkeyword (buffer, pos + 1, len, &atstatement, base_url);
			if (atstatement) {
				stat = g_slist_append (stat, atstatement);
			}
		}
		else {
			CssRuleset *ruleset;
			pos = css_parser_parse_ruleset (buffer, pos, end_pos, &ruleset, base_url);

			if (ruleset) {
				CssStatement *rulestatement;
				rulestatement = g_new0 (CssStatement, 1);
				rulestatement->type = CSS_RULESET;
				rulestatement->s.ruleset = ruleset;

				stat = g_slist_append (stat, rulestatement);
			}

			if (pos == -1)
				break;
		}

		pos = css_parser_parse_whitespace (buffer, pos, end_pos);
	}

	g_free (buffer);
	
	result = g_new0 (CssStylesheet, 1);
	result->stat = stat;

	return result;
}
