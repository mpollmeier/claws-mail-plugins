#include <string.h>
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

#include "dom-eventtarget.h"

/**
 * dom_EventTarget_addEventListener:
 * @target: a DomEventTarget
 * @type: The event type for which the user is registering.
 * @listener: Which event listener that's going to listen for the event.
 * @useCapture: If TRUE, useCapture indicates that the user wishes to initiate capture.
 * 
 * This function allows the registration of event listeners on the event target.
 **/
void
dom_EventTarget_addEventListener (DomEventTarget *target, const DomString *type, DomEventListener *listener, DomBoolean useCapture)
{
	DOM_EVENT_TARGET_GET_IFACE (target)->addEventListener (target, type, listener, useCapture);
}

/**
 * dom_EventTarget_removeEventListener:
 * @target: a DomEventTarget
 * @type: Specifies the event type of the eventlistener being removed.
 * @listener: The listener parameter indicates the event listener to be removed.
 * @useCapture: Specifies whether the event listener being removed was registered as a capturing listener or not.
 * 
 * This function allows the removal of event listeners from the event target.
 **/
void
dom_EventTarget_removeEventListener (DomEventTarget *target, const DomString *type, DomEventListener *listener, DomBoolean useCapture)
{
	DOM_EVENT_TARGET_GET_IFACE (target)->removeEventListener (target, type, listener, useCapture);
}

/**
 * dom_EventTarget_dispatchEvent:
 * @target: a DomEventTarget
 * @event: Specifies the event type, behavior, and contextual information to be used in processing the event.
 * @exc: Return location for an exception.
 * 
 * This function allows the dispatch of events into the implementations event model. 
 * 
 * Return value: FALSE if any of the event listeners which handled the event called preventDefault, TRUE otherwise.
 **/
DomBoolean
dom_EventTarget_dispatchEvent (DomEventTarget *target, DomEvent *event, DomException *exc)
{
	/* First check if the event is valid */
	if (event->initialized == FALSE || event->type == NULL || strcmp (event->type, "") == 0) {
		DOM_SET_EXCEPTION (DOM_UNSPECIFIED_EVENT_TYPE_ERR);
		return FALSE;
	}

	return DOM_EVENT_TARGET_GET_IFACE (target)->dispatchEvent (target, event);
}

static void
dom_event_target_base_init (gpointer g_class)
{
}

GType
dom_event_target_get_type (void)
{
	static GType event_target_type = 0;

	if (!event_target_type) {
		static const GTypeInfo event_target_info =
		{
			sizeof (DomEventTargetIface), /* class_size */
			dom_event_target_base_init,   /* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			0,
			0,              /* n_preallocs */
			NULL
		};
		
		event_target_type = g_type_register_static (G_TYPE_INTERFACE, "DomEventTarget", &event_target_info, 0);
		g_type_interface_add_prerequisite (event_target_type, G_TYPE_OBJECT);
	}
	
	return event_target_type;
}
