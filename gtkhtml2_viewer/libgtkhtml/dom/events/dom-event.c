/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2000-2001 CodeFactory AB
 * Copyright (C) 2000-2001 Anders Carlsson <andersca@codefactory.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "dom-event.h"

static GObjectClass *parent_class;

/**
 * dom_Event__get_type: 
 * @event: a DomEvent
 * 
 * Returns the name of the event (case-insensitive).
 * 
 * Return value: The name of the event. This value has to be free.
 **/
DomString *
dom_Event__get_type (DomEvent *event)
{
	return g_strdup (event->type);
}

/**
 * dom_Event__get_target:
 * @event: a DomEvent.
 * 
 * Used to inidicate the EventTarget to which the event was currently dispatched.
 * 
 * Return value: The original target.
 **/
DomEventTarget *
dom_Event__get_target (DomEvent *event)
{
	return event->target;
}

/**
 * dom_Event__get_currentTarget:
 * @event: a DomEvent.
 * 
 * Used to indicate the EventTarget whose EventListeners are currently being processed.
 * 
 * Return value: The current EventTarget.
 **/
DomEventTarget *
dom_Event__get_currentTarget (DomEvent *event)
{
	return event->currentTarget;
}

/**
 * dom_Event__get_eventPhase:
 * @event: a DomEvent.
 * 
 * Used to indicate which phase of event flow is currently being evaluated.
 * 
 * Return value: The current event phase.
 **/
gushort
dom_Event__get_eventPhase (DomEvent *event)
{
	return event->eventPhase;
}

/**
 * dom_Event__get_timeStamp:
 * @event: a DomEvent
 * 
 * Used to specify the time (in milliseconds relativeto the epoch) at which the event was created.
 * 
 * Return value: 
 **/
DomTimeStamp
dom_Event__get_timeStamp (DomEvent *event)
{
	return event->timeStamp;
}

/**
 * dom_Event_initEvent:
 * @event: a DomEvent
 * @eventTypeArg: Specifies the event type.
 * @canBubbleArg: Specifies whether or not the event can bubble
 * @cancelableArg: Specifies whether or not the event's default action can be prevented.
 * 
 * Initializes the event. This method may be called multiple times, but the final invocation takes precedence.
 **/
void
dom_Event_initEvent (DomEvent *event, const DomString *eventTypeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg)
{
	event->initialized = TRUE;

	if (event->type)
		g_free (event->type);
	
	event->type = g_strdup (eventTypeArg);
	event->bubbles = canBubbleArg;
	event->cancelable = cancelableArg;
}

/**
 * dom_Event_stopPropagation:
 * @event: a DomEvent
 * 
 * The stopPropagation method is used prevent further propagation of an event during event flow.
 **/
void
dom_Event_stopPropagation (DomEvent *event)
{
	event->propagation_stopped = TRUE;
}

/**
 * dom_Event_preventDefault:
 * @event: 
 * 
 * If an event is cancelable, the preventDefault method is used to signify that the event
 * is to be canceled, meaning any default action normally taken by the implementation
 * as a result of the event will not occur.
 **/
void
dom_Event_preventDefault (DomEvent *event)
{
	if (event->cancelable)
		event->default_prevented = TRUE;
}

static void
dom_event_finalize (GObject *object)
{
	DomEvent *event = DOM_EVENT (object);

	if (event->type)
		g_free (event->type);

	G_OBJECT_CLASS (parent_class)->finalize (object);
	
}

static void
dom_event_class_init (DomEventClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	object_class->finalize = dom_event_finalize;
	
	parent_class = g_type_class_peek_parent (klass);
}

static void
dom_event_init (DomEvent *event)
{
	event->initialized = FALSE;
	event->type = NULL;
}

GType
dom_event_get_type (void)
{
	static GType dom_event_type = 0;

	if (!dom_event_type) {
		static const GTypeInfo dom_event_info = {
			sizeof (DomEventClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_event_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomEvent),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_event_init,
		};

		dom_event_type = g_type_register_static (G_TYPE_OBJECT, "DomEvent", &dom_event_info, 0);
	}

	return dom_event_type;
}
