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

#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "dom-string.h"

static void
_dom_string_const_unref (DomString *str)
{
	g_free (str);
}

static void
_dom_string_own_unref (DomString *str)
{
	g_free (str->str);
	g_free (str);
}

static void
_dom_string_xml_unref (DomString *str)
{
	xmlFree (str->str);
	g_free (str);
}

void
dom_string_ref (DomString *str)
{
	str->refcount++;
}

void
dom_string_unref (DomString *str)
{
	str->refcount--;

	if (str->refcount == 0)
		str->unref (str);
}

DomString *
dom_string_dup_mkref (gchar *str)
{
	DomString *result;
	
	result = g_new (DomString, 1);
	result->refcount = 1;
	result->str = g_strdup (str);
	result->unref = _dom_string_own_unref;
	
	return result;

}

DomString *
dom_string_own_mkref (gchar *str)
{
	DomString *result;

	result = g_new (DomString, 1);
	result->refcount = 1;
	result->str = str;
	result->unref = _dom_string_own_unref;

	return result;
}

DomString *
dom_string_xml_mkref (xmlChar *str)
{
	DomString *result;

	result = g_new (DomString, 1);
	result->refcount = 1;
	result->str = str;
	result->unref = _dom_string_xml_unref;

	return result;
}

DomString *
dom_string_const_mkref (const gchar *str)
{
	DomString *result;

	result = g_new (DomString, 1);
	result->refcount = 1;
	result->str = (gchar *)str;
	result->unref = _dom_string_const_unref;
	
	return result;
}

