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

#ifndef __DOM_MOUSE_EVENT_H__
#define __DOM_MOUSE_EVENT_H__

#include <glib-object.h>
#include "dom-uievent.h"

#define DOM_TYPE_MOUSE_EVENT             (dom_mouse_event_get_type ())
#define DOM_MOUSE_EVENT(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), DOM_TYPE_MOUSE_EVENT, DomMouseEvent))
#define DOM_MOUSE_EVENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_MOUSE_EVENT, DomMouseEventClass))
#define DOM_IS_MOUSE_EVENT(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object),  DOM_TYPE_MOUSE_EVENT))
#define DOM_IS_MOUSE_EVENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_MOUSE_EVENT))
#define DOM_MOUSE_EVENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_MOUSE_EVENT, DomMouseEventClass))

struct _DomMouseEvent {
	DomUIEvent parent;

	glong clientX;
	glong clientY;
	glong screenX;
	glong screenY;

	DomBoolean ctrlKey;
	DomBoolean shiftKey;
	DomBoolean metaKey;
	DomBoolean altKey;
	gushort button;
	DomEventTarget *relatedTarget;
};

struct _DomMouseEventClass {
	DomUIEventClass parent_class;
};

GType dom_mouse_event_get_type (void);

glong dom_MouseEvent__get_clientX (DomMouseEvent *event);
glong dom_MouseEvent__get_clientY (DomMouseEvent *event);
glong dom_MouseEvent__get_screenX (DomMouseEvent *event);
glong dom_MouseEvent__get_screenY (DomMouseEvent *event);
DomBoolean dom_MouseEvent__get_ctrlKey (DomMouseEvent *event);
DomBoolean dom_MouseEvent__get_shiftKey (DomMouseEvent *event);
DomBoolean dom_MouseEvent__get_altKey (DomMouseEvent *event);
DomBoolean dom_MouseEvent__get_metaKey (DomMouseEvent *event);
gushort dom_MouseEvent__get_button (DomMouseEvent *event);
DomEventTarget *dom_MouseEvent__get_relatedTarget (DomMouseEvent *event);
void dom_MouseEvent_initMouseEvent (DomMouseEvent *event, const DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, glong detailArg, glong screenXArg, glong screenYArg, glong clientXArg, glong clientYArg, DomBoolean ctrlKeyArg, DomBoolean altKeyArg, DomBoolean shiftKeyArg, DomBoolean metaKeyArg, gushort buttonArg, DomEventTarget *relatedTargetArg);

#endif /* __DOM_MOUSE_EVENT_H__ */
