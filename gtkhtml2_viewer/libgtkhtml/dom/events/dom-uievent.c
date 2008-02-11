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

#include "dom-uievent.h"

static DomEventClass *parent_class = NULL;

/**
 * dom_UIEvent__get_view:
 * @event: a DomUIEvent.
 * 
 * Returns the AbstractView from which the event was generated.
 * 
 * Return value: The abstract view from which the event was generated.
 **/
DomAbstractView *
dom_UIEvent__get_view (DomUIEvent *event)
{
	return event->view;
}

/**
 * dom_UIEvent__get_detail:
 * @event: a DomUIEvent.
 * 
 * Returns some sort of detail information about the event, depending on the event type.
 * 
 * Return value: Some sort of detail information about the event.
 **/
gulong
dom_UIEvent__get_detail (DomUIEvent *event)
{
	return event->detail;
}

/**
 * dom_UIEvent_initUIEvent:
 * @event: a DomUIEvent
 * @typeArg: Specifies the event type.
 * @canBubbleArg: Specifies whether or not the event can bubble
 * @cancelableArg: Specifies whether or not the event's default action can be prevented.
 * @viewArg: Specifies the event's DomAbstractView.
 * @detailArg: Specifies the event's detail.
 * 
 * Initializes the event. This method may be called multiple times, but the final invocation takes precedence.
 **/
void
dom_UIEvent_initUIEvent (DomUIEvent *event, const DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, glong detailArg)
{
	dom_Event_initEvent (DOM_EVENT (event), typeArg, canBubbleArg, cancelableArg);

	if (event->view)
		g_object_unref (event->view);

	event->view = g_object_ref (viewArg);
	event->detail = detailArg;
}

static void
dom_ui_event_finalize (GObject *object)
{
	DomUIEvent *event = DOM_UI_EVENT (object);
	
	if (event->view)
		g_object_unref (event->view);
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dom_ui_event_class_init (DomUIEventClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	object_class->finalize = dom_ui_event_finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
dom_ui_event_init (DomUIEvent *doc)
{
}

GType
dom_ui_event_get_type (void)
{
	static GType dom_ui_event_type = 0;

	if (!dom_ui_event_type) {
		static const GTypeInfo dom_ui_event_info = {
			sizeof (DomUIEventClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_ui_event_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomUIEvent),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_ui_event_init,
		};

		dom_ui_event_type = g_type_register_static (DOM_TYPE_EVENT, "DomUIEvent", &dom_ui_event_info, 0);
	}

	return dom_ui_event_type;
}
