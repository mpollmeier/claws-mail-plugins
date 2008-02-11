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

#ifndef __DOM_UI_EVENT_H__
#define __DOM_UI_EVENT_H__

typedef struct _DomUIEvent DomUIEvent;
typedef struct _DomUIEventClass DomUIEventClass;

#include <glib-object.h>
#include <dom/events/dom-event.h>
#include <dom/views/dom-abstractview.h>

#define DOM_TYPE_UI_EVENT             (dom_ui_event_get_type ())
#define DOM_UI_EVENT(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), DOM_TYPE_UI_EVENT, DomUIEvent))
#define DOM_UI_EVENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_UI_EVENT, DomUIEventClass))
#define DOM_IS_UI_EVENT(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object),  DOM_TYPE_UI_EVENT))
#define DOM_IS_UI_EVENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_UI_EVENT))
#define DOM_UI_EVENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_UI_EVENT, DomUIEventClass))

struct _DomUIEvent {
	DomEvent parent;

	DomAbstractView *view;
	gulong detail;
};

struct _DomUIEventClass {
	DomEventClass parent_class;
};

GType dom_ui_event_get_type (void);

DomAbstractView *dom_UIEvent__get_view (DomUIEvent *event);
gulong dom_UIEvent__get_detail (DomUIEvent *event);
void dom_UIEvent_initUIEvent (DomUIEvent *event, const DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, glong detailArg);

#endif /* __DOM_UI_EVENT_H__ */
