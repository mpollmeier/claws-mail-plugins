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

#include "dom/core/dom-document.h"
#include "dom/core/dom-element.h"
#include "dom/core/dom-node.h"

#include "htmlfocusiterator.h"

static gint
find_maximum_tabindex (DomElement *start_element)
{
	gint max_tabindex = 0;
	DomNode *node = DOM_NODE (start_element);

	while (1) {
		if (dom_Node_hasChildNodes (node))
			node = dom_Node__get_firstChild (node);
		else if (dom_Node__get_nextSibling (node))
			node = dom_Node__get_nextSibling (node);
		else {
			while (node != NULL && dom_Node__get_nextSibling (node) == NULL)
				node = dom_Node__get_parentNode (node);

			if (node == NULL)
				return max_tabindex;

			node = dom_Node__get_nextSibling (node);
		}

		if (DOM_IS_ELEMENT (node) && dom_element_is_focusable (DOM_ELEMENT (node)) &&
		    DOM_ELEMENT (node)->tabindex > max_tabindex)
			max_tabindex = DOM_ELEMENT (node)->tabindex;
	}

	return max_tabindex;
}

static DomElement *
find_prev_focusable_element (DomElement *start_element, gint tabindex)
{
	DomNode *node = DOM_NODE (start_element);

	while (1) {
		if (dom_Node__get_lastChild (node))
			node = dom_Node__get_lastChild (node);
		else if (dom_Node__get_previousSibling (node)) 
			node = dom_Node__get_previousSibling (node);
		else {
			while (node != NULL && dom_Node__get_previousSibling (node) == NULL)
				node = dom_Node__get_parentNode (node);

			if (node == NULL)
				return NULL;

			node = dom_Node__get_previousSibling (node);
		}

		if (DOM_IS_ELEMENT (node) && dom_element_is_focusable (DOM_ELEMENT (node)) &&
		    DOM_ELEMENT (node)->tabindex == tabindex)
			return DOM_ELEMENT (node);
	}

	return NULL;
}

static DomElement *
find_next_focusable_element (DomElement *start_element, gint tabindex)
{
	DomNode *result = DOM_NODE (start_element);

	while (1) {
		if (dom_Node_hasChildNodes (result))
			result = dom_Node__get_firstChild (result);
		else if (dom_Node__get_nextSibling (result))
			result = dom_Node__get_nextSibling (result);
		else {
			while (result != NULL && dom_Node__get_nextSibling (result) == NULL)
				result = dom_Node__get_parentNode (result);

			if (result == NULL)
				return NULL;

			result = dom_Node__get_nextSibling (result);
		}

 		if (DOM_IS_ELEMENT (result) && dom_element_is_focusable (DOM_ELEMENT (result)) &&
		    DOM_ELEMENT (result)->tabindex == tabindex)
			return DOM_ELEMENT (result);
	}

	return NULL;
}

static DomElement *
find_last_element (DomElement *element)
{
	DomNode *node = DOM_NODE (element);
	DomElement *last_element = element;
	
	while (1) {
		while (dom_Node__get_nextSibling (node))
			node = dom_Node__get_nextSibling (node);

		if (DOM_IS_ELEMENT (node))
			last_element = DOM_ELEMENT (node);

		if (dom_Node_hasChildNodes (node))
			node = dom_Node__get_firstChild (node);
		else {
			return last_element;
		}
	}

}

DomElement *
html_focus_iterator_prev_element (DomDocument *document, DomElement *element)
{
	DomElement *last_element, *focus_element;
	gint tabindex, max_tabindex;

	last_element = find_last_element (DOM_ELEMENT (dom_Document__get_documentElement (document)));
	max_tabindex = find_maximum_tabindex (dom_Document__get_documentElement (document));

	if (element) {
		tabindex = element->tabindex;
	}
	else {
		tabindex = 0;
		element = last_element;

		/* If the last element is focusable, return it */
		if (dom_element_is_focusable (element) &&
		    element->tabindex == 0)
			return element;
	}

	if ((focus_element = find_prev_focusable_element (element, tabindex)))
		return focus_element;

	if (tabindex == 0) {
		element = last_element;
		tabindex = max_tabindex;
	}
	
	while (tabindex > 0 && tabindex <= max_tabindex) {
		if ((focus_element = find_prev_focusable_element (element, tabindex)))
			return focus_element;

		tabindex--;
		element = last_element;
	}
	

	return NULL;
}

DomElement *
html_focus_iterator_next_element (DomDocument *document, DomElement *element)
{
	gint max_tabindex;
	gint tabindex;
	DomElement *focus_element;
	
	if (element) {
		tabindex = element->tabindex;
	}
	else {
		tabindex = 1;
		element = dom_Document__get_documentElement (document);

		if (element == NULL)
			return NULL;
		
		/* If the first element is focusable, return it */
		if (dom_element_is_focusable (element) &&
		    element->tabindex == 0)
			return element;

	}

	max_tabindex = find_maximum_tabindex (dom_Document__get_documentElement (document));

	while (tabindex > 0 && tabindex <= max_tabindex) {
		if ((focus_element = find_next_focusable_element (element, tabindex)))
			return focus_element;

		tabindex++;
		element = dom_Document__get_documentElement (document);
	}

	if ((focus_element = find_next_focusable_element (element, 0))) {
		return focus_element;
	}

	return NULL;
}


