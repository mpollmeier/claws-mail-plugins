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

#include "dom-event-utils.h"

enum {
	EVENT,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _DomEventListenerSignalClass DomEventListenerSignalClass;

struct _DomEventListenerSignalClass {
	GObjectClass parent_class;

	void (* event) (DomEventListener *listener, DomEvent *event);
};

static void
dom_event_listener_signal_handleEvent (DomEventListener *listener, DomEvent *event)
{
	g_signal_emit (G_OBJECT (listener), signals[EVENT], 0, event);
}

static void
dom_event_listener_signal_event_listener_init (DomEventListenerIface *iface)
{
	iface->handleEvent = dom_event_listener_signal_handleEvent;
}

static void
dom_event_listener_signal_class_init (GObjectClass *klass)
{
	signals[EVENT] = g_signal_new ("event",
				       G_OBJECT_CLASS_TYPE (klass),
				       G_SIGNAL_RUN_FIRST,
				       G_STRUCT_OFFSET (DomEventListenerSignalClass, event),
				       NULL, NULL,
				       g_cclosure_marshal_VOID__OBJECT,
				       G_TYPE_NONE, 1,
				       DOM_TYPE_EVENT);
		       
}

static GType
dom_event_listener_signal_get_type (void)
{
	static GType dom_type = 0;

	if (!dom_type) {
		static const GTypeInfo dom_info = {
			sizeof (DomEventListenerSignalClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_event_listener_signal_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (GObject),
			16,   /* n_preallocs */
			NULL,
		};

		static const GInterfaceInfo dom_event_listener_info = {
			(GInterfaceInitFunc) dom_event_listener_signal_event_listener_init,
			NULL,
			NULL
		};

		dom_type = g_type_register_static (G_TYPE_OBJECT, "DomEventListenerSignal", &dom_info, 0);
		g_type_add_interface_static (dom_type,
					     DOM_TYPE_EVENT_LISTENER,
					     &dom_event_listener_info);

	}

	
	return dom_type;

}

DomEventListener *
dom_event_listener_signal_new (void)
{
	return DOM_EVENT_LISTENER (g_object_new (dom_event_listener_signal_get_type (), NULL));
}

void
dom_MutationEvent_invoke (DomEventTarget *target, const gchar *eventType, DomBoolean canBubble, DomBoolean cancelable,
			  DomNode *relatedNode, const gchar *prevValue, const gchar *newValue, const gchar *attrName, gushort attrChange)
{
	DomMutationEvent *event = g_object_new (DOM_TYPE_MUTATION_EVENT, NULL);

	dom_MutationEvent_initMutationEvent (event, eventType, canBubble, cancelable, relatedNode, prevValue, newValue, attrName, attrChange);
	dom_EventTarget_dispatchEvent (target, DOM_EVENT (event), NULL);

	g_object_unref (event);
}


void
dom_StyleEvent_invoke (DomEventTarget *target, const gchar *eventType, DomBoolean canBubble, DomBoolean cancelable, gushort styleChange)
{
	DomStyleEvent *event = g_object_new (DOM_TYPE_STYLE_EVENT, NULL);

	dom_StyleEvent_initStyleEvent (event, eventType, canBubble, cancelable, styleChange);
	dom_EventTarget_dispatchEvent (target, DOM_EVENT (event), NULL);

	g_object_unref (event);
}

gboolean
dom_MouseEvent_invoke (DomEventTarget *target, const gchar *eventType, gboolean canBubble, gboolean cancelable, DomAbstractView *viewArg, glong detailArg, glong screenXArg, glong screenYArg, glong clientXArg, glong clientYArg, DomBoolean ctrlKeyArg, DomBoolean altKeyArg, DomBoolean shiftKeyArg, DomBoolean metaKeyArg, gushort buttonArg, DomEventTarget *relatedTargetArg)
{
	DomMouseEvent *event = g_object_new (DOM_TYPE_MOUSE_EVENT, NULL);
	gboolean ret_val;
	
	dom_MouseEvent_initMouseEvent (event, eventType, canBubble, cancelable, viewArg,
				       detailArg, screenXArg, screenYArg, clientXArg, clientYArg,
				       ctrlKeyArg, altKeyArg, shiftKeyArg, metaKeyArg, buttonArg, relatedTargetArg);

	ret_val = dom_EventTarget_dispatchEvent (target, DOM_EVENT (event), NULL);
	g_object_unref (event);
	return ret_val;
}

void
dom_Event_invoke (DomEventTarget *target, const gchar *eventType, DomBoolean canBubble, DomBoolean cancelable)
{
	DomEvent *event = g_object_new (DOM_TYPE_EVENT, NULL);

	dom_Event_initEvent (event, eventType, canBubble, cancelable);
	
	dom_EventTarget_dispatchEvent (target, event, NULL);
	
	g_object_unref (event);
}

static void
dom_Event_dispatch_traverser_post (DomNode *node, DomEvent *event)
{
	while (node) {
		if (dom_Node_hasChildNodes (node))
			dom_Event_dispatch_traverser_post (dom_Node__get_firstChild (node), event);

		/* Emit the event */
		dom_EventTarget_dispatchEvent (DOM_EVENT_TARGET (node), event, NULL);

		node = dom_Node__get_nextSibling (node);
	}
}

static void
dom_Event_dispatch_traverser_pre (DomNode *node, DomEvent *event)
{
	while (node) {
		/* Emit the event */
		dom_EventTarget_dispatchEvent (DOM_EVENT_TARGET (node), event, NULL);
		
		if (dom_Node_hasChildNodes (node))
			dom_Event_dispatch_traverser_pre (dom_Node__get_firstChild (DOM_NODE (node)), event);

		node = dom_Node__get_nextSibling (node);
	}
}

void
dom_MutationEvent_invoke_recursively (DomEventTarget *target, const gchar *eventType, DomBoolean canBubble, DomBoolean cancelable,
				      DomNode *relatedNode, const gchar *prevValue, const gchar *newValue, const gchar *attrName, gushort attrChange, DomEventTraverserType traverser_type)
{
	DomMutationEvent *event = g_object_new (DOM_TYPE_MUTATION_EVENT, NULL);
	
	dom_MutationEvent_initMutationEvent (event, eventType, canBubble, cancelable, relatedNode, prevValue, newValue, attrName, attrChange);

	switch (traverser_type) {
	case DOM_EVENT_TRAVERSER_POST_ORDER:
		/* First, emit the event recursively on the children */
		if (dom_Node_hasChildNodes (DOM_NODE (target))) 
			dom_Event_dispatch_traverser_post (dom_Node__get_firstChild (DOM_NODE (target)), DOM_EVENT (event));
		
		/* Then, emit the event on the root node */
		dom_EventTarget_dispatchEvent (target, DOM_EVENT (event), NULL);

		break;
	case DOM_EVENT_TRAVERSER_PRE_ORDER:
		/* First, emit the event on the root node */
		dom_EventTarget_dispatchEvent (target, DOM_EVENT (event), NULL);

		/* Then, emit the event recursively on the children */
		if (dom_Node_hasChildNodes (DOM_NODE (target))) 
			dom_Event_dispatch_traverser_pre (dom_Node__get_firstChild (DOM_NODE (target)), DOM_EVENT (event));
		
		break;
		
	}

	/* Unref the event */
	g_object_unref (event);
}
