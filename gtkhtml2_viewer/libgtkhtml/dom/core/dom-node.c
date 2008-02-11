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
#include <libxml/tree.h>

#include "dom-node.h"
#include "dom-attr.h"
#include "dom-document.h"
#include "dom-comment.h"
#include "dom-element.h"
#include "dom-text.h"
#include "dom-nodelist.h"
#include "dom-namednodemap.h"
#include "dom-documenttype.h"
#include "dom-entity.h"
#include "dom/events/dom-eventtarget.h"
#include "dom/events/dom-eventlistener.h"
#include "dom/events/dom-event-utils.h"
#include "dom/traversal/dom-nodeiterator.h"
#include "dom/html/dom-htmlelement.h"
#include "dom/html/dom-htmlanchorelement.h"
#include "dom/html/dom-htmlformelement.h"
#include "dom/html/dom-htmlinputelement.h"
#include "dom/html/dom-htmlselectelement.h"
#include "dom/html/dom-htmloptionelement.h"
#include "dom/html/dom-htmltextareaelement.h"
#include "dom/html/dom-htmlobjectelement.h"


static GObjectClass *parent_class = NULL;

typedef struct _DomEventListenerInfo DomEventListenerInfo;

struct _DomEventListenerInfo {
	DomEventListener *listener;
	DomString *type;
	DomBoolean useCapture;
};

DomNode *
dom_Node_mkref (xmlNode *node)
{
	DomNode *result;
	
	if (!node)
		return NULL;
	
	if (node->_private)
		return DOM_NODE (node->_private);

	switch (node->type) {
	case XML_ELEMENT_NODE:

		/* FIXME: Do this only in HTML-namespace / jb */

		if (strcasecmp (node->name, "input") == 0)
			result = g_object_new (DOM_TYPE_HTML_INPUT_ELEMENT, NULL);
		else if (strcasecmp (node->name, "form") == 0)
			result = g_object_new (DOM_TYPE_HTML_FORM_ELEMENT, NULL);
		else if (strcasecmp (node->name, "a") == 0)
			result = g_object_new (DOM_TYPE_HTML_ANCHOR_ELEMENT, NULL);
		else if (strcasecmp (node->name, "select") == 0)
			result = g_object_new (DOM_TYPE_HTML_SELECT_ELEMENT, NULL);
		else if (strcasecmp (node->name, "option") == 0)
			result = g_object_new (DOM_TYPE_HTML_OPTION_ELEMENT, NULL);
		else if (strcasecmp (node->name, "object") == 0)
			result = g_object_new (DOM_TYPE_HTML_OBJECT_ELEMENT, NULL);
		else if (strcasecmp (node->name, "textarea") == 0)
			result = g_object_new (DOM_TYPE_HTML_TEXT_AREA_ELEMENT, NULL);
		else
			result = g_object_new (DOM_TYPE_HTML_ELEMENT, NULL);
		
		/* result = g_object_new (DOM_TYPE_ELEMENT, NULL);*/
		break;
	case XML_ATTRIBUTE_NODE:
		result = g_object_new (DOM_TYPE_ATTR, NULL);
		break;
	case XML_HTML_DOCUMENT_NODE:
	case XML_DOCUMENT_NODE:
		result = g_object_new (DOM_TYPE_DOCUMENT, NULL);
		break;
	case XML_TEXT_NODE:
		result = g_object_new (DOM_TYPE_TEXT, NULL);
		break;
	case XML_DTD_NODE:
		result = g_object_new (DOM_TYPE_DOCUMENT_TYPE, NULL);
		break;
	case XML_ENTITY_DECL:
		result = g_object_new (DOM_TYPE_ENTITY, NULL);
		break;
	case XML_COMMENT_NODE:
		result = g_object_new (DOM_TYPE_COMMENT, NULL);
		break;
	default:
		g_warning ("Unknown node type: %d\n", node->type);
		return NULL;
	}
	
	result->xmlnode = node;
	node->_private = result;
	
	return result;
}

static gulong
dom_Node__get_childNodes_length (DomNodeList *list)
{
	xmlNode *node = list->node->xmlnode->children;
	gulong len = 0;

	while (node) {
		len++;
		node = node->next;
	}
	
	return len;
}

static DomNode *
dom_Node__get_childNodes_item (DomNodeList *list, gulong offset)
{
	xmlNode *node = list->node->xmlnode->children;
	gulong i = 0;

	for (i = 0; i < offset; i++) {
		if (!node)
			return NULL;

		node = node->next;
	}

	return dom_Node_mkref (node);
}

/**
 * dom_Node__get_attributes:
 * @node: a DomNode
 * 
 * A NamedNodeMap containing the attributes of this node (if it is an Element) or null otherwise.
 * 
 * Return value: 
 **/
DomNamedNodeMap *
dom_Node__get_attributes (DomNode *node)
{
	DomNamedNodeMap *result;

	if (node->xmlnode->type != XML_ELEMENT_NODE)
		return NULL;

	result = g_object_new (DOM_TYPE_NAMED_NODE_MAP, NULL);

	result->attr = (xmlNode *)node->xmlnode->properties;
	result->type = XML_ATTRIBUTE_NODE;
	result->readonly = FALSE;
	
	return result;
}

/**
 * dom_Node__get_childNodes:
 * @node: a DomNode
 * 
 * Returns a DomNodeList that contains all children of this node. If no children exist, this will be an empty DomNodeList
 * 
 * Return value: a DomNodeList containing all children of this node.
 **/
DomNodeList *
dom_Node__get_childNodes (DomNode *node)
{
	DomNodeList *list = g_object_new (DOM_TYPE_NODE_LIST, NULL);

	list->length = dom_Node__get_childNodes_length;
	list->item = dom_Node__get_childNodes_item;
	
	list->node = g_object_ref (node);
	
	return list;
}

/**
 * dom_Node_hasAttributes:
 * @node: a DomNode
 * @exc: return location for an exception
 * 
 * Returns whether this node has any attributes.
 * 
 * Return value: TRUE if the node has any attributes or FALSE otherwise.
 **/
DomBoolean
dom_Node_hasAttributes (DomNode *node)
{
	if (node->xmlnode->type != XML_ELEMENT_NODE)
		return FALSE;

	return (((xmlElement *)(node->xmlnode))->attributes != NULL);
}

/**
 * dom_Node_cloneNode:
 * @node: a DomNode
 * @deep: if deep is TRUE, recursively clone the subtree of the node. 
 * 
 * Creates a duplicate of this node.
 *
 * Return value: the cloned node.
 **/
DomNode *
dom_Node_cloneNode (DomNode *node, DomBoolean deep)
{
	return dom_Node_mkref (xmlCopyNode (node->xmlnode, deep));
}

/**
 * dom_Node_appendChild:
 * @node: a DomNode
 * @newChild: the DomNode to add
 * @exc: return location for an exception.
 * 
 * Adds the node newChild to the end of the list of children in this node.
 * If newChild is already in the tree, it is first removed.
 * 
 * Return value: The node added.
 **/
DomNode *
dom_Node_appendChild (DomNode *node, DomNode *newChild, DomException *exc)
{
	if (node->xmlnode->doc != newChild->xmlnode->doc) {
		DOM_SET_EXCEPTION (DOM_WRONG_DOCUMENT_ERR);

		return NULL;
	}

	if (node->xmlnode->type == XML_TEXT_NODE) {
		DOM_SET_EXCEPTION (DOM_HIERARCHY_REQUEST_ERR);

		return NULL;
	}
	
	/* Remove the child node if it has a parent */
	if (newChild->xmlnode->parent != NULL)
		dom_Node_removeChild (dom_Node_mkref (newChild->xmlnode->parent), newChild, NULL);

	newChild->xmlnode->parent = node->xmlnode;

	if (node->xmlnode->children == NULL) {
		node->xmlnode->children = newChild->xmlnode;
		node->xmlnode->last = newChild->xmlnode;
	}
	else {
		xmlNode *prev;
		
		prev = node->xmlnode->last;
		prev->next = newChild->xmlnode;
		newChild->xmlnode->prev = prev;
		node->xmlnode->last = newChild->xmlnode;
		
	}

	/* Emit mutation events */
	dom_MutationEvent_invoke_recursively (DOM_EVENT_TARGET (newChild), "DOMNodeInsertedIntoDocument", FALSE, FALSE,
					      NULL, NULL, NULL, NULL, 0, DOM_EVENT_TRAVERSER_PRE_ORDER);
	dom_MutationEvent_invoke (DOM_EVENT_TARGET (newChild), "DOMNodeInserted", TRUE, FALSE,
				  node, NULL, NULL, NULL, 0);
	
	return newChild;
}

static void
dom_Node_notifyLiveObjectsAboutRemoval (DomDocument *document, DomNode *node)
{
	GSList *iterators;
	DomNodeIterator *iterator;


	if (document == NULL || node == NULL)
		return;
	
	for (iterators = document->iterators; iterators; iterators = iterators->next) {
		iterator = iterators->data;

		dom_NodeIterator_removeNode (iterator, node);
	}
}

/**
 * dom_Node_removeChild:
 * @node: a DomNode
 * @oldChild: the DomNode to be removed,
 * @exc: return location for an exception
 * 
 * Removes the child node indicated by oldChild from the list of children and returns it.
 * 
 * Return value: the removed node
 **/
DomNode *
dom_Node_removeChild (DomNode *node, DomNode *oldChild, DomException *exc)
{
	xmlNode *prev, *next;

	if (oldChild->xmlnode->parent != node->xmlnode) {
		DOM_SET_EXCEPTION (DOM_NOT_FOUND_ERR);
		return NULL;
	}

	dom_Node_notifyLiveObjectsAboutRemoval (dom_Node__get_ownerDocument (oldChild), oldChild);
	
	/* Emit mutation events */
	dom_MutationEvent_invoke_recursively (DOM_EVENT_TARGET (oldChild), "DOMNodeRemovedFromDocument", FALSE, FALSE,
					      NULL, NULL, NULL, NULL, 0, DOM_EVENT_TRAVERSER_POST_ORDER);

	dom_MutationEvent_invoke (DOM_EVENT_TARGET (oldChild), "DOMNodeRemoved", TRUE, FALSE,
				  node, NULL, NULL, NULL, 0);
	
	next = oldChild->xmlnode->next;
	prev = oldChild->xmlnode->prev;

	if (node->xmlnode->children == oldChild->xmlnode)
		node->xmlnode->children = next;

	if (node->xmlnode->last == oldChild->xmlnode)
		node->xmlnode->last = prev;

	if (next != NULL)
		next->prev = prev;

	if (prev != NULL)
		prev->next = next;

	oldChild->xmlnode->parent = NULL;
	oldChild->xmlnode->next = NULL;

	return oldChild;
}

/**
 * dom_Node_hasChildNodes:
 * @node: a DomNode
 * 
 * Returns whether this node has any children.
 * 
 * Return value: TRUE if the node has any children or FALSE otherwise.
 **/
DomBoolean
dom_Node_hasChildNodes (DomNode *node)
{
	return (node->xmlnode->children != NULL);
}

/**
 * dom_Node__get_localName:
 * @node: a DomNode
 * 
 * Returns the local part of the qualified name of this node.
 * 
 * Return value: the local part of the qualified name or NULL if the node is not an element or an attribute node.
 **/
DomString *
dom_Node__get_localName (DomNode *node)
{
	if (node->xmlnode->type != XML_ELEMENT_NODE &&
	    node->xmlnode->type != XML_ATTRIBUTE_NODE)
		/* FIXME: What about nodes created with DOM1 interfaces here? */
		return NULL;
	else
		return g_strdup (node->xmlnode->name);
}

/**
 * dom_Node__get_namespaceURI:
 * @node: a DomNode
 * 
 * Returns the namespace URI of this node or NULL if it's unspecified.
 * 
 * Return value: the namespace URI of the node.
 **/
DomString *
dom_Node__get_namespaceURI (DomNode *node)
{
	if (node->xmlnode->ns &&
	    node->xmlnode->ns->href)
		return g_strdup (node->xmlnode->ns->href);
	else
		return NULL;
}


/**
 * dom_Node__get_nextSibling: 
 * @node: a DomNode
 * 
 * Rmeturns the next sibling of this node or NULL if none exists.
 * 
 * Return value: the next sibling of this node.
 **/
DomNode *
dom_Node__get_nextSibling (DomNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (DOM_IS_NODE (node), NULL);

	return dom_Node_mkref (node->xmlnode->next);
}

/**
 * dom_Node__get_previousSibling: 
 * @node: a DomNode
 * 
 * Returns the previous sibling of this node or NULL if none exists.
 * 
 * Return value: the previous sibling of this node.
 **/
DomNode *
dom_Node__get_previousSibling (DomNode *node)
{
	return dom_Node_mkref (node->xmlnode->prev);
}

/**
 * dom_Node__get_lastChild:
 * @node: a DomNode
 * 
 * Returns the last child of this node or NULL if none exists.
 * 
 * Return value: the last child of this node.
 **/
DomNode *
dom_Node__get_lastChild (DomNode *node)
{
	return dom_Node_mkref (node->xmlnode->last);
}

/**
 * dom_Node__get_firstChild:
 * @node: a DomNode
 * 
 * Return value: the first child of this node.
 **/
DomNode *
dom_Node__get_firstChild (DomNode *node)
{
	return dom_Node_mkref (node->xmlnode->children);
}

/**
 * dom_Node__get_parentNode: 
 * @node: a DomNode
 * 
 * Returns the parent of this node or NULL if none exists.
 * 
 * Return value: the parent node.
 **/
DomNode *
dom_Node__get_parentNode (DomNode *node)
{
	if (node->xmlnode->parent)
		return dom_Node_mkref (node->xmlnode->parent);
	else
		return NULL;
}

/**
 * dom_Node__get_nodeType: 
 * @node: a DomNode
 * 
 * Returns the type of a dom node.
 * 
 * Return value: the type of the node.
 **/
gushort
dom_Node__get_nodeType (DomNode *node)
{
	return node->xmlnode->type;
}

/**
 * dom_Node__set_nodeValue: 
 * @node: a DomNode
 * @value: the value to be set.
 * @exc: return location for an exception.
 * 
 * Sets the value of a dom node.
 **/
void
dom_Node__set_nodeValue (DomNode *node, const DomString *value, DomException *exc)
{
	DOM_NODE_GET_CLASS (node)->_set_nodeValue (node, value, exc);
}

/**
 * dom_Node__get_nodeValue:
 * @node: a DomNode
 * @exc: Return value for an exception.
 * 
 * Returns the value of a dom node depending on its type.
 * 
 * Return value: The value of the dom node, this value must be freed.
 **/
DomString *
dom_Node__get_nodeValue (DomNode *node, DomException *exc)
{
	switch (node->xmlnode->type) {
	case XML_ELEMENT_NODE:
		return NULL;
		break;
	case XML_TEXT_NODE:
		return g_strdup (node->xmlnode->content);
		break;
	default:
		g_warning ("Unknown node type %d", node->xmlnode->type);
	}

	return NULL;
}

/**
 * dom_Node__get_nodeName:
 * @node: a DomNode
 * 
 * Returns the name of a dom node depending on its type. 
 * 
 * Return value: The name of the node. This value must be freed.
 **/
DomString *
dom_Node__get_nodeName (DomNode *node)
{
	switch (node->xmlnode->type) {
	case XML_DTD_NODE:
	case XML_ELEMENT_NODE:
	case XML_ENTITY_DECL:
		return g_strdup (node->xmlnode->name);
		break;
	case XML_TEXT_NODE:
		return g_strdup ("#text");
		break;
	case XML_HTML_DOCUMENT_NODE:
	case XML_DOCUMENT_NODE:
		return g_strdup ("#document");
		break;
	default:
		g_warning ("Unknown node type: %d", node->xmlnode->type);
	}

	return NULL;
}


/**
 * dom_Node__get_ownerDocument:
 * @node: a DomNode
 * 
 * Returns the document node associated with this node. 
 * 
 * Return value: The document node associated with this node.
 **/
DomDocument *
dom_Node__get_ownerDocument (DomNode *node)
{
	if (node->xmlnode->type == XML_DOCUMENT_NODE ||
	    node->xmlnode->type == XML_DTD_NODE)
		return NULL;
	
	return DOM_DOCUMENT (dom_Node_mkref ((xmlNode *)node->xmlnode->doc));
}

static void
dom_Node_addEventListener (DomEventTarget *target, const DomString *type, DomEventListener *listener, DomBoolean useCapture)
{
	GList *handler_list;
	DomEventListenerInfo *info;
	
	handler_list = g_object_get_data (G_OBJECT (target), "listener_list");

	/* Check if the event listener already exists */
	while (handler_list) {
		DomEventListenerInfo *info = handler_list->data;

		if (info->useCapture == useCapture &&
		    strcasecmp (info->type, type) == 0 &&
		    info->listener == listener)
			return;

		handler_list = handler_list->next;
	}

	handler_list = g_object_get_data (G_OBJECT (target), "listener_list");
	
	/* The event listener did not exist, so add it */
	info = g_new (DomEventListenerInfo, 1);
	info->type = g_strdup (type);
	info->listener = g_object_ref (G_OBJECT (listener));
	info->useCapture = useCapture;

	handler_list = g_list_append (handler_list, info);

	g_object_set_data (G_OBJECT (target), "listener_list", handler_list);
}

static void
dom_Node_removeEventListener (DomEventTarget *target, const DomString *type, DomEventListener *listener, DomBoolean useCapture)
{
	GList *handler_list;
	gboolean removed = FALSE;
	
	handler_list = g_object_get_data (G_OBJECT (target), "listener_list");
	
	/* Check if the event listener exists */
	while (handler_list) {
		DomEventListenerInfo *info = handler_list->data;

		if (info->useCapture == useCapture &&
		    strcasecmp (info->type, type) == 0 &&
		    info->listener == listener) {
			handler_list = g_list_remove (handler_list, info);
			g_free (info->type);
			g_object_unref (G_OBJECT (info->listener));
			g_free (info);
			
			removed = TRUE;
			break;
		}

		handler_list = handler_list->next;
	}

	if (removed)
		g_object_set_data (G_OBJECT (target), "listener_list", handler_list);
}

static void
dom_Node_invokeListener (DomEventTarget *target, const DomString *type, DomEvent *event, DomBoolean useCapture)
{
	GList *listener_list;

	listener_list = g_object_get_data (G_OBJECT (target), "listener_list");
	
	if (event->timeStamp == 0) {
		GTimeVal tv;

		g_get_current_time (&tv);
		event->timeStamp = (tv.tv_sec * (guint64)1000) + tv.tv_usec / 1000;
	}

	while (listener_list) {
		DomEventListenerInfo *info = listener_list->data;

		if ((strcasecmp (type, info->type)) == 0 &&
		    (useCapture == info->useCapture)) 
			dom_EventListener_handleEvent (DOM_EVENT_LISTENER (info->listener), event);
		
		listener_list = listener_list->next;
	}
}

static DomBoolean
dom_Node_dispatchEvent (DomEventTarget *target, DomEvent *event)
{
	DomNode *path_static [256];
	DomNode **path;
	DomNode *p;
	gint i, n_path; 
	DomBoolean do_capture = TRUE; /* FIXME: We don't want to always capture the events */
	path = path_static;
	
	event->target = target;
	
	p = DOM_NODE (target);
	
	/* First try to fill the static array with parent elements */
	for (i = 0; i < G_N_ELEMENTS (path_static); i++) {
		if (p == NULL)
			break;

		path[i] = p;

		p = dom_Node__get_parentNode (p);
	}

	/* If the node chain won't fit, dynamically reallocate it until it will */
	if (p != NULL) {
		gint n_path_max = i << 1;

		path = g_new (DomNode *, n_path_max);
		memcpy (path, path_static, sizeof (path_static));

		do {
			if (i == n_path_max)
				path = g_renew (DomNode *, path, (n_path_max <<= 1));

			path[i++] = p;

			p = dom_Node__get_parentNode (p);
		} while (p != NULL);
	}

	n_path = i;

	event->default_prevented = FALSE;
	event->propagation_stopped = FALSE;
	
	if (do_capture) {
		event->eventPhase = DOM_CAPTURING_PHASE;

		for (i = n_path - 1; i > 0; i--) {
			
			/* Set current target on event before invoking listeners */
			if (event->currentTarget)
				g_object_unref (event->currentTarget);
			event->currentTarget = g_object_ref (path[i]);

			dom_Node_invokeListener (DOM_EVENT_TARGET (path[i]), event->type, event, TRUE);

			if (event->propagation_stopped)
				break;
		}
	}

	if (!event->propagation_stopped) {
		event->eventPhase = DOM_AT_TARGET;

		/* Set current target on event before invoking listeners */
		if (event->currentTarget)
			g_object_unref (event->currentTarget);
		event->currentTarget = g_object_ref (path[0]);

		dom_Node_invokeListener (DOM_EVENT_TARGET (path[0]), event->type, event, TRUE);

		if (!event->propagation_stopped && event->bubbles) {
			event->eventPhase = DOM_BUBBLING_PHASE;

			for (i = 1; i < n_path; i++) {
				if (event->currentTarget)
					g_object_unref (event->currentTarget);
				event->currentTarget = g_object_ref (path[i]);

				dom_Node_invokeListener (DOM_EVENT_TARGET (path[i]), event->type, event, FALSE);

				if (event->propagation_stopped)
					break;
			}
		}
	}

	/* Unref the last target */
	if (event->currentTarget) {
		g_object_unref (event->currentTarget);
		event->currentTarget = NULL;
	}
	
	if (path != path_static)
		g_free (path);
	
	return !event->default_prevented;

}

static void
dom_node_event_target_init (DomEventTargetIface *iface)
{
	iface->addEventListener = dom_Node_addEventListener;
	iface->removeEventListener = dom_Node_removeEventListener;
	iface->dispatchEvent = dom_Node_dispatchEvent;
}

static void
dom_node_finalize (GObject *object)
{
	DomNode *dom_node = DOM_NODE (object);
	xmlNode *node = dom_node->xmlnode->children;

	/* Unref the children */
	while (node) {
		if (node->_private) {
			g_object_unref (DOM_NODE (node->_private));
		}
		
		node = node->next;
	}

	if (dom_node->style)
		html_style_unref (dom_node->style);

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);

}

static void
dom_node_class_init (DomNodeClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;
	
	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = dom_node_finalize;
}

static void
dom_node_init (DomNode *node)
{
}

GType
dom_node_get_type (void)
{
	static GType dom_node_type = 0;

	if (!dom_node_type) {
		static const GTypeInfo dom_node_info = {
			sizeof (DomNodeClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_node_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomNode),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_node_init,
		};

		static const GInterfaceInfo dom_event_target_info = {
			(GInterfaceInitFunc) dom_node_event_target_init,
			NULL,
			NULL
		};

		dom_node_type = g_type_register_static (G_TYPE_OBJECT, "DomNode", &dom_node_info, 0);
		g_type_add_interface_static (dom_node_type,
					     DOM_TYPE_EVENT_TARGET,
					     &dom_event_target_info);

	}

	return dom_node_type;
}
