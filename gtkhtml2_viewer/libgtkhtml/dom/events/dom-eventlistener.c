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

#include "dom-eventlistener.h"

/**
 * dom_EventListener__handleEvent:
 * @listener: a DomEventListener
 * @event: The event that's going to be handled.
 * 
 * This function is called whenever an event occurs of the typ for which the event listener was registered.
 **/
void
dom_EventListener_handleEvent (DomEventListener *listener, DomEvent *event)
{
	DOM_EVENT_LISTENER_GET_IFACE (listener)->handleEvent (listener, event);
}

GType
dom_event_listener_get_type (void)
{
	static GType dom_event_listener_type = 0;

	if (!dom_event_listener_type) {
		static const GTypeInfo dom_event_listener_info = {
			sizeof (DomEventListenerIface),
			NULL, /* base_init */
			NULL, /* base_finalize */
			NULL,
			NULL, /* class_finalize */
			NULL, /* class_data */
			0,
			0,   /* n_preallocs */
			NULL
		};

		dom_event_listener_type = g_type_register_static (G_TYPE_INTERFACE, "DomEventListener", &dom_event_listener_info, 0);
		g_type_interface_add_prerequisite (dom_event_listener_type, G_TYPE_OBJECT);
	}

	return dom_event_listener_type;
}
