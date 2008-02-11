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

#ifndef __DOM_EVENT_UTILS_H__
#define __DOM_EVENT_UTILS_H__

#include <libgtkhtml/dom/events/dom-mutationevent.h>
#include <libgtkhtml/dom/events/dom-styleevent.h>
#include <libgtkhtml/dom/events/dom-mouseevent.h>

DomEventListener *dom_event_listener_signal_new (void);

typedef enum {
	DOM_EVENT_TRAVERSER_PRE_ORDER,
	DOM_EVENT_TRAVERSER_POST_ORDER
} DomEventTraverserType;


void
dom_MutationEvent_invoke (DomEventTarget *target, const gchar *eventType, DomBoolean canBubble, DomBoolean cancelable, DomNode *relatedNode, const gchar *prevValue, const gchar *newValue, const gchar *attrName, gushort attrChange);

void
dom_Event_invoke (DomEventTarget *target, const gchar *eventType, DomBoolean canBubble, DomBoolean cancelable);

void dom_StyleEvent_invoke (DomEventTarget *target, const gchar *eventType, DomBoolean canBubble, DomBoolean cancelable, gushort styleChange);

void dom_MutationEvent_invoke_recursively (DomEventTarget *target, const gchar *eventType, DomBoolean canBubble, DomBoolean cancelable,
					   DomNode *relatedNode, const gchar *prevValue, const gchar *newValue, const gchar *attrName, gushort attrChange, DomEventTraverserType traverser_type);

gboolean dom_MouseEvent_invoke (DomEventTarget *target, const gchar *eventType, gboolean canBubble, gboolean cancelable, DomAbstractView *viewArg, glong detailArg, glong screenXArg, glong screenYArg, glong clientXArg, glong clientYArg, DomBoolean ctrlKeyArg, DomBoolean altKeyArg, DomBoolean shiftKeyArg, DomBoolean metaKeyArg, gushort buttonArg, DomEventTarget *relatedTargetArg);

DomEventListener *dom_event_listener_signal_new (void);


#endif /* __DOM_EVENT_UTILS_H__ */
