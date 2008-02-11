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

#include "dom-nodeiterator.h"

DomNode *
dom_NodeIterator__get_root (DomNodeIterator *iterator)
{
	return iterator->root;
}

gulong
dom_NodeIterator__get_whatToShow (DomNodeIterator *iterator)
{
	return iterator->whatToShow;
}

DomNodeFilter *
dom_NodeIterator__get_nodeFilter (DomNodeIterator *iterator)
{
	return iterator->filter;
}

gboolean
dom_NodeIterator__get_expandEntityReferences (DomNodeIterator *iterator)
{
	return iterator->expandEntityReferences;
}

static DomNode *
dom_next_node_helper (DomNode *node)
{
	if (dom_Node_hasChildNodes (node)) {
		return dom_Node__get_firstChild (node);
	}
	
	if (dom_Node__get_nextSibling (node))
		return dom_Node__get_nextSibling (node);
	
	while (node != NULL && dom_Node__get_nextSibling (node) == NULL) {
	  node = dom_Node__get_parentNode (node);
	}

	if (node != NULL) {
		return dom_Node__get_nextSibling (node);
	}
	else {
		return NULL;
	}
}

static gboolean
accept_node (DomNodeIterator *iterator, DomNode *node)
{
	if (((1 << (node->xmlnode->type - 1)) & iterator->whatToShow) != 0) {
		if (iterator->filter)
			return (dom_NodeFilter_acceptNode (iterator->filter, node) == DOM_NODE_FILTER_ACCEPT);
		else
			return TRUE;
	}

	return FALSE;
}

DomNode *
dom_NodeIterator_nextNode (DomNodeIterator *iterator, DomException *exc)
{
	DomNode *next_node, *tmp_node = NULL;

	if (iterator->detached == TRUE) {
		DOM_SET_EXCEPTION(DOM_INVALID_STATE_ERR);

		return NULL;
	}

	iterator->forward_direction = TRUE;
	
	if (iterator->reference_node == NULL) {
		next_node = iterator->root;
	}
	else {
		tmp_node = iterator->reference_node;
		next_node = dom_next_node_helper (iterator->reference_node);
	}

	iterator->forward_direction = FALSE;
	
	while (next_node) {
		if (accept_node (iterator, next_node)) {
			iterator->reference_node = next_node;
			return iterator->reference_node;
		}

		tmp_node = next_node;
		next_node = dom_next_node_helper (next_node);
	}

	iterator->reference_node = tmp_node;
	
	return NULL;
}

static DomNode *
dom_prev_node_helper (DomNodeIterator *iterator, DomNode *node)
{
	DomNode *prev_node;

	if (node == iterator->root) {
		return NULL;
	}

	prev_node = dom_Node__get_previousSibling (node);

	if (prev_node == NULL) {
		prev_node = dom_Node__get_parentNode (node);

		return prev_node;
	}

	while (prev_node && dom_Node_hasChildNodes (prev_node)) {
		prev_node = dom_Node__get_lastChild (prev_node);
	}

	return prev_node;
}

DomNode *
dom_NodeIterator_previousNode (DomNodeIterator *iterator, DomException *exc)
{
	DomNode *prev_node, *tmp_node = NULL;
	
	if (iterator->detached == TRUE) {
		DOM_SET_EXCEPTION(DOM_INVALID_STATE_ERR);

		return NULL;
	}

	iterator->forward_direction = FALSE;
	
	if (iterator->reference_node == NULL) {
		prev_node = iterator->root;
	}
	else {
		tmp_node = iterator->reference_node;
		prev_node = dom_prev_node_helper (iterator, iterator->reference_node);
	}
	
	while (prev_node != NULL) {
		if (accept_node (iterator, prev_node)) {
			iterator->reference_node = prev_node;
			return iterator->reference_node;
		}

		tmp_node = prev_node;
		prev_node = dom_prev_node_helper (iterator, prev_node);
	}

	iterator->reference_node = tmp_node;
	return NULL;
}

void
dom_NodeIterator_detach (DomNodeIterator *iterator)
{
	iterator->detached = TRUE;

	iterator->document->iterators =
		g_slist_remove (iterator->document->iterators, iterator);
}

void
dom_NodeIterator_removeNode (DomNodeIterator *iterator, DomNode *node)
{
	if (node == NULL)
		return;

	if (iterator->forward_direction == TRUE) {
		iterator->reference_node = dom_prev_node_helper (iterator, iterator->reference_node);
	}
	else {
		DomNode *next_node;

		next_node = dom_next_node_helper (node);

		if (next_node != NULL) {
			iterator->reference_node = next_node;
		}
		else {
			iterator->reference_node = dom_prev_node_helper (iterator, node);
			iterator->forward_direction = TRUE;
		}
	}
}

static void
dom_node_iterator_class_init (DomNodeIteratorClass *klass)
{
}

static void
dom_node_iterator_init (DomNodeIterator *iterator)
{
	iterator->forward_direction = TRUE;
}


GType
dom_node_iterator_get_type (void)
{
	static GType dom_node_iterator_type = 0;

	if (!dom_node_iterator_type) {
		static const GTypeInfo dom_node_iterator_info = {
			sizeof (DomNodeIteratorClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_node_iterator_class_init,
			NULL, /* class_finalize */
			NULL, /* class_iterator */
			sizeof (DomNodeIterator),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_node_iterator_init,
		};

		dom_node_iterator_type = g_type_register_static (G_TYPE_OBJECT, "DomNodeIterator", &dom_node_iterator_info, 0);
	}

	return dom_node_iterator_type;
}
