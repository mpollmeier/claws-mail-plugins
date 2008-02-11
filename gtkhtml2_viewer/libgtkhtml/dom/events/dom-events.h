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

#ifndef __DOM_EVENTS_H__
#define __DOM_EVENTS_H__

#define DOM_DOCUMENT_EVENT(x) ((DomDocumentEvent *)x)
#define DOM_MUTATION_EVENT(x) ((DomMutationEvent *)x)
#define DOM_UI_EVENT(x) ((DomUIEvent *)x)
#define DOM_MOUSE_EVENT(x) ((DomMouseEvent *)x)

#define DOM_EVENT(x) ((DomEvent *)x)

typedef struct _DomDocumentEvent DomDocumentEvent;
typedef struct _DomEvent DomEvent;
typedef struct _DomEventListener DomEventListener;
typedef struct _DomUIEvent DomUIEvent;
typedef struct _DomMouseEvent DomMouseEvent;
typedef struct _DomMutationEvent DomMutationEvent;
typedef struct DomNode DomEventTarget;

typedef struct _DomDocumentEventVtab DomDocumentEventVtab;
typedef struct _DomEventVtab DomEventVtab;
typedef struct _DomEventListenerVtab DomEventListenerVtab;
typedef struct _DomEventTargetVtab DomEventTargetVtab;
typedef struct _DomUIEventVtab DomUIEventVtab;
typedef struct _DomMouseEventVtab DomMouseEventVtab;
typedef struct _DomMutationEventVtab DomMutationEventVtab;

#include "dom/core/dom-core.h"
#include "dom/dom-exception.h"
#include "dom/dom-string.h"
#include "dom/views/dom-views.h"

#define DOM_CAPTURING_PHASE 1
#define DOM_AT_TARGET 2
#define DOM_BUBBLING_PHASE 3

#define DOM_MODIFICATION 1
#define DOM_ADDITION 2
#define DOM_REMOVAL 3

struct _DomDocumentEvent {
  const DomDocumentEventVtab *vtab;
};

struct _DomEvent {
  const DomEventVtab *vtab;
};

struct _DomEventListener {
  const DomEventListenerVtab *vtab;
};

struct _DomUIEvent {
  const DomUIEventVtab *vtab;
};

struct _DomMouseEvent {
  const DomMouseEventVtab *vtab;
};

struct _DomMutationEvent {
  const DomMutationEventVtab *vtab;
};

struct _DomDocumentEventVtab {
  void (*ref) (DomDocumentEvent *self, DomException *exc);
  void (*unref) (DomDocumentEvent *self, DomException *exc);
  void * (*queryInterface) (DomDocumentEvent *self, const gchar *interface, DomException *exc);
  DomEvent *(*createEvent) (DomDocumentEvent *self, DomString *eventType, DomException *exc);
};

struct _DomEventVtab {
  void (*ref) (DomEvent *self, DomException *exc);
  void (*unref) (DomEvent *self, DomException *exc);
  void * (*queryInterface) (DomEvent *self, const gchar *interface, DomException *exc);
  DomString *(*get_type) (DomEvent *self, DomException *exc);
  DomEventTarget *(*get_target) (DomEvent *self, DomException *exc);
  DomEventTarget *(*get_currentTarget) (DomEvent *self, DomException *exc);
  unsigned short (*get_eventPhase) (DomEvent *self, DomException *exc);
  DomBoolean (*get_bubbles) (DomEvent *self, DomException *exc);
  DomBoolean (*get_cancelable) (DomEvent *self, DomException *exc);
  DomTimeStamp *(*get_timeStamp) (DomEvent *self, DomException *exc);
  void (*stopPropagation) (DomEvent *self, DomException *exc);
  void (*preventDefault) (DomEvent *self, DomException *exc);
  void (*initEvent) (DomEvent *self, DomString *eventTypeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomException *exc);
};

struct _DomEventListenerVtab {
  void (*ref) (DomEventListener *self, DomException *exc);
  void (*unref) (DomEventListener *self, DomException *exc);
  void * (*queryInterface) (DomEventListener *self, const gchar *interface, DomException *exc);
  void (*handleEvent) (DomEventListener *self, DomEvent *evt, DomException *exc);
};

struct _DomUIEventVtab {
  DomEventVtab super;
  DomAbstractView *(*get_view) (DomUIEvent *self, DomException *exc);
  long (*get_detail) (DomUIEvent *self, DomException *exc);
  void (*initUIEvent) (DomUIEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, long detailArg, DomException *exc);
};

struct _DomMouseEventVtab {
  DomUIEventVtab super;
  long (*get_screenX) (DomMouseEvent *self, DomException *exc);
  long (*get_screenY) (DomMouseEvent *self, DomException *exc);
  long (*get_clientX) (DomMouseEvent *self, DomException *exc);
  long (*get_clientY) (DomMouseEvent *self, DomException *exc);
  DomBoolean (*get_ctrlKey) (DomMouseEvent *self, DomException *exc);
  DomBoolean (*get_shiftKey) (DomMouseEvent *self, DomException *exc);
  DomBoolean (*get_altKey) (DomMouseEvent *self, DomException *exc);
  DomBoolean (*get_metaKey) (DomMouseEvent *self, DomException *exc);
  unsigned short (*get_button) (DomMouseEvent *self, DomException *exc);
  DomEventTarget *(*get_relatedTarget) (DomMouseEvent *self, DomException *exc);
  void (*initMouseEvent) (DomMouseEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, long detailArg, long screenXArg, long screenYArg, long clientXArg, long clientYArg, DomBoolean ctrlKeyArg, DomBoolean altKeyArg, DomBoolean shiftKeyArg, DomBoolean metaKeyArg, unsigned short buttonArg, DomEventTarget *relatedTargetArg, DomException *exc);
};

struct _DomMutationEventVtab {
  DomEventVtab super;
  DomNode *(*get_relatedNode) (DomMutationEvent *self, DomException *exc);
  DomString *(*get_prevValue) (DomMutationEvent *self, DomException *exc);
  DomString *(*get_newValue) (DomMutationEvent *self, DomException *exc);
  DomString *(*get_attrName) (DomMutationEvent *self, DomException *exc);
  unsigned short (*get_attrChange) (DomMutationEvent *self, DomException *exc);
  void (*initMutationEvent) (DomMutationEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomNode *relatedNodeArg, DomString *prevValueArg, DomString *newValueArg, DomString *attrNameArg, unsigned short attrChangeArg, DomException *exc);
};

void dom_DocumentEvent__ref (DomDocumentEvent *self, DomException *exc);
void dom_DocumentEvent__unref (DomDocumentEvent *self, DomException *exc);
void * dom_DocumentEvent__queryInterface (DomDocumentEvent *self, const gchar *interface, DomException *exc);
DomEvent *dom_DocumentEvent__createEvent (DomDocumentEvent *self, DomString *eventType, DomException *exc);

void dom_Event__ref (DomEvent *self, DomException *exc);
void dom_Event__unref (DomEvent *self, DomException *exc);
void * dom_Event__queryInterface (DomEvent *self, const gchar *interface, DomException *exc);
DomString *dom_Event__get_type (DomEvent *self, DomException *exc);
DomEventTarget *dom_Event__get_target (DomEvent *self, DomException *exc);
DomEventTarget *dom_Event__get_currentTarget (DomEvent *self, DomException *exc);
unsigned short dom_Event__get_eventPhase (DomEvent *self, DomException *exc);
DomBoolean dom_Event__get_bubbles (DomEvent *self, DomException *exc);
DomBoolean dom_Event__get_cancelable (DomEvent *self, DomException *exc);
DomTimeStamp *dom_Event__get_timeStamp (DomEvent *self, DomException *exc);
void dom_Event__stopPropagation (DomEvent *self, DomException *exc);
void dom_Event__preventDefault (DomEvent *self, DomException *exc);
void dom_Event__initEvent (DomEvent *self, DomString *eventTypeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomException *exc);

void dom_EventListener__ref (DomEventListener *self, DomException *exc);
void dom_EventListener__unref (DomEventListener *self, DomException *exc);
void * dom_EventListener__queryInterface (DomEventListener *self, const gchar *interface, DomException *exc);
void dom_EventListener__handleEvent (DomEventListener *self, DomEvent *evt, DomException *exc);

void dom_EventTarget__ref (DomEventTarget *self, DomException *exc);
void dom_EventTarget__unref (DomEventTarget *self, DomException *exc);
void * dom_EventTarget__queryInterface (DomEventTarget *self, const gchar *interface, DomException *exc);
void dom_EventTarget__addEventListener (DomEventTarget *self, DomString *type, DomEventListener *listener, DomBoolean useCapture, DomException *exc);
void dom_EventTarget__removeEventListener (DomEventTarget *self, DomString *type, DomEventListener *listener, DomBoolean useCapture, DomException *exc);
DomBoolean dom_EventTarget__dispatchEvent (DomEventTarget *self, DomEvent *evt, DomException *exc);

DomAbstractView *dom_UIEvent__get_view (DomUIEvent *self, DomException *exc);
long dom_UIEvent__get_detail (DomUIEvent *self, DomException *exc);
void dom_UIEvent__initUIEvent (DomUIEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, long detailArg, DomException *exc);
void dom_UIEvent__ref (DomUIEvent *self, DomException *exc);
void dom_UIEvent__unref (DomUIEvent *self, DomException *exc);
void * dom_UIEvent__queryInterface (DomUIEvent *self, const gchar *interface, DomException *exc);
DomString *dom_UIEvent__get_type (DomUIEvent *self, DomException *exc);
DomEventTarget *dom_UIEvent__get_target (DomUIEvent *self, DomException *exc);
DomEventTarget *dom_UIEvent__get_currentTarget (DomUIEvent *self, DomException *exc);
unsigned short dom_UIEvent__get_eventPhase (DomUIEvent *self, DomException *exc);
DomBoolean dom_UIEvent__get_bubbles (DomUIEvent *self, DomException *exc);
DomBoolean dom_UIEvent__get_cancelable (DomUIEvent *self, DomException *exc);
DomTimeStamp *dom_UIEvent__get_timeStamp (DomUIEvent *self, DomException *exc);
void dom_UIEvent__stopPropagation (DomUIEvent *self, DomException *exc);
void dom_UIEvent__preventDefault (DomUIEvent *self, DomException *exc);
void dom_UIEvent__initEvent (DomUIEvent *self, DomString *eventTypeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomException *exc);

long dom_MouseEvent__get_screenX (DomMouseEvent *self, DomException *exc);
long dom_MouseEvent__get_screenY (DomMouseEvent *self, DomException *exc);
long dom_MouseEvent__get_clientX (DomMouseEvent *self, DomException *exc);
long dom_MouseEvent__get_clientY (DomMouseEvent *self, DomException *exc);
DomBoolean dom_MouseEvent__get_ctrlKey (DomMouseEvent *self, DomException *exc);
DomBoolean dom_MouseEvent__get_shiftKey (DomMouseEvent *self, DomException *exc);
DomBoolean dom_MouseEvent__get_altKey (DomMouseEvent *self, DomException *exc);
DomBoolean dom_MouseEvent__get_metaKey (DomMouseEvent *self, DomException *exc);
unsigned short dom_MouseEvent__get_button (DomMouseEvent *self, DomException *exc);
DomEventTarget *dom_MouseEvent__get_relatedTarget (DomMouseEvent *self, DomException *exc);
void dom_MouseEvent__initMouseEvent (DomMouseEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, long detailArg, long screenXArg, long screenYArg, long clientXArg, long clientYArg, DomBoolean ctrlKeyArg, DomBoolean altKeyArg, DomBoolean shiftKeyArg, DomBoolean metaKeyArg, unsigned short buttonArg, DomEventTarget *relatedTargetArg, DomException *exc);
DomAbstractView *dom_MouseEvent__get_view (DomMouseEvent *self, DomException *exc);
long dom_MouseEvent__get_detail (DomMouseEvent *self, DomException *exc);
void dom_MouseEvent__initUIEvent (DomMouseEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, long detailArg, DomException *exc);
void dom_MouseEvent__ref (DomMouseEvent *self, DomException *exc);
void dom_MouseEvent__unref (DomMouseEvent *self, DomException *exc);
void * dom_MouseEvent__queryInterface (DomMouseEvent *self, const gchar *interface, DomException *exc);
DomString *dom_MouseEvent__get_type (DomMouseEvent *self, DomException *exc);
DomEventTarget *dom_MouseEvent__get_target (DomMouseEvent *self, DomException *exc);
DomEventTarget *dom_MouseEvent__get_currentTarget (DomMouseEvent *self, DomException *exc);
unsigned short dom_MouseEvent__get_eventPhase (DomMouseEvent *self, DomException *exc);
DomBoolean dom_MouseEvent__get_bubbles (DomMouseEvent *self, DomException *exc);
DomBoolean dom_MouseEvent__get_cancelable (DomMouseEvent *self, DomException *exc);
DomTimeStamp *dom_MouseEvent__get_timeStamp (DomMouseEvent *self, DomException *exc);
void dom_MouseEvent__stopPropagation (DomMouseEvent *self, DomException *exc);
void dom_MouseEvent__preventDefault (DomMouseEvent *self, DomException *exc);
void dom_MouseEvent__initEvent (DomMouseEvent *self, DomString *eventTypeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomException *exc);

DomNode *dom_MutationEvent__get_relatedNode (DomMutationEvent *self, DomException *exc);
DomString *dom_MutationEvent__get_prevValue (DomMutationEvent *self, DomException *exc);
DomString *dom_MutationEvent__get_newValue (DomMutationEvent *self, DomException *exc);
DomString *dom_MutationEvent__get_attrName (DomMutationEvent *self, DomException *exc);
unsigned short dom_MutationEvent__get_attrChange (DomMutationEvent *self, DomException *exc);
void dom_MutationEvent__initMutationEvent (DomMutationEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomNode *relatedNodeArg, DomString *prevValueArg, DomString *newValueArg, DomString *attrNameArg, unsigned short attrChangeArg, DomException *exc);
void dom_MutationEvent__ref (DomMutationEvent *self, DomException *exc);
void dom_MutationEvent__unref (DomMutationEvent *self, DomException *exc);
void * dom_MutationEvent__queryInterface (DomMutationEvent *self, const gchar *interface, DomException *exc);
DomString *dom_MutationEvent__get_type (DomMutationEvent *self, DomException *exc);
DomEventTarget *dom_MutationEvent__get_target (DomMutationEvent *self, DomException *exc);
DomEventTarget *dom_MutationEvent__get_currentTarget (DomMutationEvent *self, DomException *exc);
unsigned short dom_MutationEvent__get_eventPhase (DomMutationEvent *self, DomException *exc);
DomBoolean dom_MutationEvent__get_bubbles (DomMutationEvent *self, DomException *exc);
DomBoolean dom_MutationEvent__get_cancelable (DomMutationEvent *self, DomException *exc);
DomTimeStamp *dom_MutationEvent__get_timeStamp (DomMutationEvent *self, DomException *exc);
void dom_MutationEvent__stopPropagation (DomMutationEvent *self, DomException *exc);
void dom_MutationEvent__preventDefault (DomMutationEvent *self, DomException *exc);
void dom_MutationEvent__initEvent (DomMutationEvent *self, DomString *eventTypeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomException *exc);

DomEvent *dom_DocumentEvent__createEvent (DomDocumentEvent *self, DomString *eventType, DomException *exc);

#endif /* __DOM_EVENTS_H__ */
