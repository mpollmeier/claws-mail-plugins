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

#ifndef __DOM_DOCUMENTEVENT_H__
#define __DOM_DOCUMENTEVENT_H__

#include <glib-object.h>

typedef struct _DomDocumentEvent DomDocumentEvent;
typedef struct _DomDocumentEventIface DomDocumentEventIface;

#include "dom-event.h"

#define DOM_TYPE_DOCUMENT_EVENT             (dom_document_event_get_type ())
#define DOM_DOCUMENT_EVENT(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), DOM_TYPE_DOCUMENT_EVENT, DomDocumentEvent))
#define DOM_DOCUMENT_EVENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_DOCUMENT_EVENT, DomDocumentEventClass))
#define DOM_IS_DOCUMENT_EVENT(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object),  DOM_TYPE_DOCUMENT_EVENT))
#define DOM_IS_DOCUMENT_EVENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_DOCUMENT_EVENT))
#define DOM_DOCUMENT_EVENT_GET_IFACE(obj)   ((DomDocumentEventIface *)g_type_interface_peek (((GTypeInstance *)DOM_DOCUMENT_EVENT (obj))->g_class, DOM_TYPE_DOCUMENT_EVENT))


struct _DomDocumentEventIface {
	GTypeInterface g_iface;

	DomEvent * (* createEvent) (DomDocumentEvent *doc, const DomString *eventType, DomException *exc);
};

GType dom_document_event_get_type (void);
DomEvent *dom_DocumentEvent__createEvent (DomDocumentEvent *doc, const DomString *eventType, DomException *exc);

#endif /* __DOM_DOCUMENTEVENT_H__ */
