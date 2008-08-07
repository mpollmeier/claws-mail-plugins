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

#include <stdlib.h>
#include "dom-htmlselectelement.h"
#include "dom-htmloptionelement.h"
#include "util/rfc1738.h"

static GObjectClass *parent_class = NULL;

DomHTMLFormElement *
dom_HTMLSelectElement__get_form (DomHTMLSelectElement *element)
{
	DomNode *form = dom_Node__get_parentNode (DOM_NODE (element));
	
	while (form && !DOM_IS_HTML_FORM_ELEMENT (form))
		form = dom_Node__get_parentNode (form);
	
	return (DomHTMLFormElement *)form;
}

DomString *
dom_HTMLSelectElement__get_type (DomHTMLSelectElement *element)
{
	if (dom_HTMLSelectElement__get_multiple (element))
		return g_strdup ("select-multiple");
	else
		return g_strdup ("select-one");
}

DomString *
dom_HTMLSelectElement__get_name (DomHTMLSelectElement *element)
{
	return dom_Element_getAttribute (DOM_ELEMENT (element), "name");
}

DomString *
dom_HTMLSelectElement__get_value (DomHTMLSelectElement *element)
{
	return g_strdup (element->str_value);
}

glong
dom_HTMLSelectElement__get_size (DomHTMLSelectElement *element)
{
	gchar *str;
	glong value = 1;

	if ((str = dom_Element_getAttribute (DOM_ELEMENT (element), "size"))) {
		g_strchug (str);
		value = atoi (str);
		xmlFree (str);
	}
	return value;
}

DomBoolean
dom_HTMLSelectElement__get_multiple (DomHTMLSelectElement *element)
{
	return dom_Element_hasAttribute (DOM_ELEMENT (element), "multiple");
}

void
dom_HTMLSelectElement__set_name (DomHTMLSelectElement *element, const DomString *name)
{
	dom_Element_setAttribute (DOM_ELEMENT (element), "name", name);
}

void
dom_HTMLSelectElement__set_value (DomHTMLSelectElement *element, const DomString *value)
{
	if (element->str_value)
		g_free (element->str_value);
	
	if (value)
		element->str_value = g_strdup (value);
	else
		element->str_value = g_strdup ("");
}

void
dom_HTMLSelectElement__set_size (DomHTMLSelectElement *element, glong size)
{
	gchar *str = g_strdup_printf ("%ld", size);
	dom_Element_setAttribute (DOM_ELEMENT (element), "size", str);
	g_free (str);
}

void
dom_HTMLSelectElement_add (DomHTMLSelectElement *select, DomHTMLElement *element, DomHTMLElement *before, DomException *exception)
{
	GtkTreeIter iter;
	gint position = -1;

	*exception = DOM_NO_EXCEPTION;

	if (before) {

		position = g_slist_index (select->options, before);

		if (position == -1) {

			*exception = DOM_NOT_FOUND_ERR;
			return;
		}
		select->options = g_slist_insert (select->options, element, position);
	}
	else {
		select->options = g_slist_append (select->options, element);
	}
	if (position != -1)
		gtk_list_store_insert (select->list_store, &iter, position);
	else
		gtk_list_store_append (select->list_store, &iter);
}

void
dom_HTMLSelectElement_remove (DomHTMLSelectElement *select, glong index)
{
	DomHTMLElement *element = (DomHTMLElement *)g_slist_nth (select->options, index);

	if (element)
		select->options = g_slist_remove (select->options, element);
}

GtkTreeModel *
dom_html_select_element_get_tree_model (DomHTMLSelectElement *select)
{
	return GTK_TREE_MODEL (select->list_store);
}

void
dom_html_select_element_update_option_data (DomHTMLSelectElement *select, DomHTMLOptionElement *element)
{
	gint position = g_slist_index (select->options, element);

	if (position >= 0) {
		gchar *label = NULL, *value = NULL;
		DomNode *labelnode;

		if ((labelnode = dom_Node__get_firstChild (DOM_NODE (element)))) {

			DomException exc;

			if ((label = dom_Node__get_nodeValue (DOM_NODE (labelnode), &exc))) {

				GtkTreeIter  iter;

				value = dom_HTMLOptionElement__get_value (DOM_HTML_OPTION_ELEMENT (element));

				if (value == NULL) {
					value = g_strdup (label);
				}
				 gtk_tree_model_get_iter_first (GTK_TREE_MODEL (select->list_store), &iter);

				while (position--) 
					gtk_tree_model_iter_next (GTK_TREE_MODEL (select->list_store), &iter);

				gtk_list_store_set (select->list_store, &iter, 0, label, 1, value, 2, element, -1);

				g_free (label);

				g_free (value);
			}
		}
	}
}

DomString *
dom_html_select_element_encode (DomHTMLSelectElement *select)
{
	GString *encoding = g_string_new ("");
	gchar *ptr;
	gchar *name, *value;;

	name = dom_HTMLSelectElement__get_name (select);
	value = dom_HTMLSelectElement__get_value (select);
			
	if (name && value) {
		ptr = rfc1738_encode_string (name);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);
			
		encoding = g_string_append_c (encoding, '=');
			
		ptr = rfc1738_encode_string (value);
		encoding = g_string_append (encoding, ptr);
		g_free (ptr);
	}
	g_free (value);
	g_free (name);

	ptr = encoding->str;
	g_string_free(encoding, FALSE);

	return ptr;
}

static void
finalize (GObject *object)
{
	DomHTMLSelectElement *select = (DomHTMLSelectElement *)object;

	g_object_unref (G_OBJECT (select->list_store));

	parent_class->finalize (object);
}


static void
dom_html_select_element_class_init (GObjectClass *klass)
{
	klass->finalize = finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
dom_html_select_element_init (DomHTMLSelectElement *select)
{
	select->list_store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_OBJECT);
}

GType
dom_html_select_element_get_type (void)
{
	static GType dom_html_select_element_type = 0;

	if (!dom_html_select_element_type) {
		static const GTypeInfo dom_html_select_element_info = {
			sizeof (DomHTMLSelectElementClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_html_select_element_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomHTMLSelectElement),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_html_select_element_init,
		};

		dom_html_select_element_type = g_type_register_static (DOM_TYPE_HTML_ELEMENT, 
								     "DomHTMLSelectElement", &dom_html_select_element_info, 0);
	}
	return dom_html_select_element_type;
}
