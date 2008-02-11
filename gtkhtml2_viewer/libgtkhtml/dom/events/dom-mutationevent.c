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

#include "dom-mutationevent.h"

static DomEventClass *parent_class = NULL;
	
/**
 * dom_MutationEvent__get_relatedNode:
 * @event: a DomMutationEvent.
 * 
 * relatedNode is used to identify a secondary node related to a mutation event.
 * 
 * Return value: The event's related node.
 **/
DomNode *
dom_MutationEvent__get_relatedNode (DomMutationEvent *event)
{
	return event->relatedNode;
}

/**
 * dom_MutationEvent__get_prevValue:
 * @event: a DomMutationEvent.
 * 
 * Returns the previous value depending on the mutation event type.
 * 
 * Return value: The previous value. This value has to be freed.
 **/
DomString *
dom_MutationEvent__get_prevValue (DomMutationEvent *event)
{
	return g_strdup (event->prevValue);
}

/**
 * dom_MutationEvent__get_newValue:
 * @event: a DomMutationEvent.
 * 
 * Returns the new value depending on the mutation event type.
 * 
 * Return value: The new value. This value has to be freed.
 **/
DomString *
dom_MutationEvent__get_newValue (DomMutationEvent *event)
{
	return g_strdup (event->newValue);
}

/**
 * dom_MutationEvent__get_attrName:
 * @event: a DomMutationEvent.
 * 
 * attrName indicates the name of the changed Attr node in a DOMAttrModified event.
 * 
 * Return value: The attribute name.
 **/
DomString *
dom_MutationEvent__get_attrName (DomMutationEvent *event)
{
	return g_strdup (event->attrName);
}

/**
 * dom_MutationEvent__get_attrChange:
 * @event: a DomMutationEvent.
 * 
 * Returns the type of change which triggered the DOMAttrModified event.
 * 
 * Return value: The type of change which triggered the DOMAttrModified event.
 **/
gushort
dom_MutationEvent__get_attrChange (DomMutationEvent *event)
{
	return event->attrChange;
}

void
dom_MutationEvent_initMutationEvent (DomMutationEvent *event, const DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomNode *relatedNodeArg, const DomString *prevValueArg, const DomString *newValueArg, const DomString *attrNameArg, gushort attrChangeArg)
{
	dom_Event_initEvent (DOM_EVENT (event), typeArg, canBubbleArg, cancelableArg);

	if (event->relatedNode)
		g_object_unref (event->relatedNode);

	if (relatedNodeArg)
		event->relatedNode = g_object_ref (relatedNodeArg);
	
	if (event->prevValue)
		g_free (event->prevValue);
	event->prevValue = g_strdup (prevValueArg);

	if (event->newValue)
		g_free (event->newValue);
	event->newValue = g_strdup (newValueArg);

	if (event->attrName)
		g_free (event->attrName);
	event->attrName = g_strdup (attrNameArg);

	event->attrChange = attrChangeArg;
}

static void
dom_mutation_event_finalize (GObject *object)
{
	DomMutationEvent *event = DOM_MUTATION_EVENT (object);
	
	if (event->relatedNode)
		g_object_unref (event->relatedNode);
	
	if (event->prevValue)
		g_free (event->prevValue);

	if (event->newValue)
		g_free (event->newValue);

	if (event->attrName)
		g_free (event->attrName);

	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dom_mutation_event_class_init (DomMutationEventClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	object_class->finalize = dom_mutation_event_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
dom_mutation_event_init (DomMutationEvent *event)
{
}

GType
dom_mutation_event_get_type (void)
{
	static GType dom_mutation_event_type = 0;

	if (!dom_mutation_event_type) {
		static const GTypeInfo dom_mutation_event_info = {
			sizeof (DomMutationEventClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_mutation_event_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomMutationEvent),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_mutation_event_init,
		};

		dom_mutation_event_type = g_type_register_static (DOM_TYPE_EVENT, "DomMutationEvent", &dom_mutation_event_info, 0);
	}

	return dom_mutation_event_type;
}

