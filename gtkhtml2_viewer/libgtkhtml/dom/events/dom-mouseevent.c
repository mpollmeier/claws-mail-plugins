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

#include "dom-mouseevent.h"

static DomUIEventClass *parent_class = NULL;

/**
 * dom_MouseEvent__get_clientX:
 * @event: a DomMouseEvent
 * 
 * Returns the horizontal coordinate at which the event occurred relative to the DOM implementation's client area.
 * 
 * Return value: The horizontal coordinate at which the event occurred relative to the DOM implementation's client area.
 **/
glong
dom_MouseEvent__get_clientX (DomMouseEvent *event)
{
	return event->clientX;
}

/**
 * dom_MouseEvent__get_clientY:
 * @event: a DomMouseEvent
 * 
 * Returns the vertical coordinate at which the event occurred relative to the DOM implementation's client area.
 * 
 * Return value: The vertical coordinate at which the event occurred relative to the DOM implementation's client area.
 **/
glong
dom_MouseEvent__get_clientY (DomMouseEvent *event)
{
	return event->clientY;
}

/**
 * dom_MouseEvent__get_screenX:
 * @event: a DomMouseEvent.
 * 
 * Returns the horizontal coordinate at which the event occurred relative to the origin of the screen coordinate system.
 * 
 * Return value: The horizontal coordinate at which the event occurred relative to the origin of the screen coordinate system.
 **/
glong
dom_MouseEvent__get_screenX (DomMouseEvent *event)
{
	return event->screenX;
}


/**
 * dom_MouseEvent__get_screenY:
 * @event: a DomMouseEvent.
 * 
 * Returns the vertical coordinate at which the event occurred relative to the origin of the screen coordinate system.
 * 
 * Return value: The vertical coordinate at which the event occurred relative to the origin of the screen coordinate system.
 **/
glong
dom_MouseEvent__get_screenY (DomMouseEvent *event)
{
	return event->screenY;
}

/**
 * dom_MouseEvent__get_ctrlKey:
 * @event: a DomMouseEvent
 * 
 * Used to indicate whether the 'ctrl' key was depressed during the firing of the event.
 * 
 * Return value: Returns TRUE if the 'ctrl' key was depressed during the firing of the event, FALSE otherwise.
 **/
DomBoolean
dom_MouseEvent__get_ctrlKey (DomMouseEvent *event)
{
	return event->ctrlKey;
}

/**
 * dom_MouseEvent__get_shiftKey:
 * @event: a DomMouseEvent
 * 
 * Used to indicate whether the 'shift' key was depressed during the firing of the event.
 * 
 * Return value: Returns TRUE if the 'shift' key was depressed during the firing of the event, FALSE otherwise.
 **/
DomBoolean
dom_MouseEvent__get_shiftKey (DomMouseEvent *event)
{
	return event->shiftKey;
}

/**
 * dom_MouseEvent__get_altKey:
 * @event: a DomMouseEvent
 * 
 * Used to indicate whether the 'alt' key was depressed during the firing of the event.
 * 
 * Return value: Returns TRUE if the 'alt' key was depressed during the firing of the event, FALSE otherwise.
 **/
DomBoolean
dom_MouseEvent__get_altKey (DomMouseEvent *event)
{
	return event->altKey;
}

/**
 * dom_MouseEvent__get_metaKey:
 * @event: a DomMouseEvent
 * 
 * Used to indicate whether the 'meta' key was depressed during the firing of the event.
 * 
 * Return value: Returns TRUE if the 'meta' key was depressed during the firing of the event, FALSE otherwise.
 **/
DomBoolean
dom_MouseEvent__get_metaKey (DomMouseEvent *event)
{
	return event->metaKey;
}

/**
 * dom_MouseEvent__get_button:
 * @event: a DomMouseEvent
 * 
 * During mouse events caused by the depression or release of a mouse button, button is used to indicate which mouse button changed state.
 * 
 * Return value: The mouse button that caused the event
 **/
gushort
dom_MouseEvent__get_button (DomMouseEvent *event)
{
	return event->button;
}

/**
 * dom_MouseEvent__get_relatedTarget:
 * @event: a DomMouseEvent
 * 
 * Used to identify a secondary EventTarget related to a UI event.
 * 
 * Return value: A secondary EventTarget related to a UI event.
 **/
DomEventTarget *
dom_MouseEvent__get_relatedTarget (DomMouseEvent *event)
{
	return event->relatedTarget;
}

/**
 * dom_MouseEvent_initMouseEvent:
 * @event: a DomMouseEvent
 * @typeArg: Specifies the event type.
 * @canBubbleArg: Specifies whether or not the event can bubble
 * @cancelableArg: Specifies whether or not the event's default action can be prevented.
 * @viewArg: Specifies the event's DomAbstractView.
 * @detailArg: Specifies the mouse event's mouse click count.
 * @screenXArg: Specifies the event's screen x coordinate.
 * @screenYArg: Specifies the event's screen y coordinate.
 * @clientXArg: Specifies the event's client x coordinate.
 * @clientYArg: Specifies the event's client y coordinate.
 * @ctrlKeyArg: Specifies whether the control key was depressed during the event.
 * @altKeyArg: Specifies whether the alt key was depressed during the even.t
 * @shiftKeyArg: Specifies whether the shift key was depressed during the event. 
 * @metaKeyArg: Specifies whether the meta key was depressed during the event.
 * @buttonArg: Specifies the event's mouse button.
 * @relatedTargetArg: Specifies the event's related DomEventTarget.
 * 
 * Initializes the event. This method may be called multiple times, but the final invocation takes precedence.
 **/
void
dom_MouseEvent_initMouseEvent (DomMouseEvent *event, const DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, glong detailArg, glong screenXArg, glong screenYArg, glong clientXArg, glong clientYArg, DomBoolean ctrlKeyArg, DomBoolean altKeyArg, DomBoolean shiftKeyArg, DomBoolean metaKeyArg, gushort buttonArg, DomEventTarget *relatedTargetArg)
{
	dom_UIEvent_initUIEvent (DOM_UI_EVENT (event), typeArg, canBubbleArg, cancelableArg, viewArg, detailArg);

	event->screenX = screenXArg;
	event->screenY = screenYArg;
	event->clientX = clientXArg;
	event->clientY = clientYArg;
	event->ctrlKey = ctrlKeyArg;
	event->altKey = altKeyArg;
	event->shiftKey = shiftKeyArg;
	event->metaKey = metaKeyArg;
	event->button = buttonArg;
	if (event->relatedTarget)
		g_object_unref (event->relatedTarget);

	if (relatedTargetArg)
		event->relatedTarget = g_object_ref (relatedTargetArg);
	
}

static void
dom_mouse_event_finalize (GObject *object)
{
	DomMouseEvent *event = DOM_MOUSE_EVENT (object);

	if (event->relatedTarget)
		g_object_unref (event->relatedTarget);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dom_mouse_event_class_init (DomMouseEventClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;

	object_class->finalize = dom_mouse_event_finalize;

	parent_class = g_type_class_peek_parent (klass);

}

static void
dom_mouse_event_init (DomMouseEvent *event)
{
}

GType
dom_mouse_event_get_type (void)
{
	static GType dom_mouse_event_type = 0;

	if (!dom_mouse_event_type) {
		static const GTypeInfo dom_mouse_event_info = {
			sizeof (DomMouseEventClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_mouse_event_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomMouseEvent),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_mouse_event_init,
		};

		dom_mouse_event_type = g_type_register_static (DOM_TYPE_UI_EVENT, "DomMouseEvent", &dom_mouse_event_info, 0);
	}

	return dom_mouse_event_type;
}
