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

#ifndef __DOM_MUTATION_EVENT_H__
#define __DOM_MUTATION_EVENT_H__

typedef struct _DomMutationEvent DomMutationEvent;
typedef struct _DomMutationEventClass DomMutationEventClass;

#include <glib-object.h>

#include <dom/core/dom-node.h>
#include <dom/events/dom-uievent.h>

#define DOM_TYPE_MUTATION_EVENT             (dom_mutation_event_get_type ())
#define DOM_MUTATION_EVENT(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), DOM_TYPE_MUTATION_EVENT, DomMutationEvent))
#define DOM_MUTATION_EVENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_MUTATION_EVENT, DomMutationEventClass))
#define DOM_IS_MUTATION_EVENT(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object),  DOM_TYPE_MUTATION_EVENT))
#define DOM_IS_MUTATION_EVENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_MUTATION_EVENT))
#define DOM_MUTATION_EVENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_MUTATION_EVENT, DomMutationEventClass))

struct _DomMutationEvent {
	DomEvent parent;

	DomNode *relatedNode;
	DomString *prevValue;
	DomString *newValue;
	DomString *attrName;
	gushort attrChange;
};

struct _DomMutationEventClass {
	DomEventClass parent_class;
};

enum {
	DOM_MODIFICATION = 1,
	DOM_ADDITION = 2,
	DOM_REMOVAL = 3
};

GType dom_mutation_event_get_type (void);

DomNode *dom_MutationEvent__get_relatedNode (DomMutationEvent *event);
DomString *dom_MutationEvent__get_prevValue (DomMutationEvent *event);
DomString *dom_MutationEvent__get_newValue (DomMutationEvent *event);
DomString *dom_MutationEvent__get_attrName (DomMutationEvent *event);
gushort dom_MutationEvent__get_attrChange (DomMutationEvent *event);

void dom_MutationEvent_initMutationEvent (DomMutationEvent *event, const DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomNode *relatedNodeArg, const DomString *prevValueArg, const DomString *newValueArg, const DomString *attrNameArg, gushort attrChangeArg);

#endif /* __DOM_MUTATION_EVENT_H__ */
