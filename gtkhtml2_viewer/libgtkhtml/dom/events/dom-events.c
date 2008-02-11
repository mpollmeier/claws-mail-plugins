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

#include "dom-events.h"

void
dom_EventListener__ref (DomEventListener *self, DomException *exc)
{
  *exc = 0;
  self->vtab->ref (self, exc);
}

void
dom_EventListener__unref (DomEventListener *self, DomException *exc)
{
  *exc = 0;
  self->vtab->unref (self, exc);
}

void *
dom_EventListener__queryInterface (DomEventListener *self, const gchar *interface, DomException *exc)
{
  *exc = 0;
  return self->vtab->queryInterface (self, interface, exc);
}

void
dom_EventListener__handleEvent (DomEventListener *self, DomEvent *evt, DomException *exc)
{
  *exc = 0;
  self->vtab->handleEvent (self, evt, exc);
}

void
dom_EventTarget__ref (DomEventTarget *self, DomException *exc)
{
  *exc = 0;
  ((DomNode *)self)->vtab->ref ((DomNode *)self, exc);
}

void
dom_EventTarget__unref (DomEventTarget *self, DomException *exc)
{
  *exc = 0;
  ((DomNode *)self)->vtab->unref ((DomNode *)self, exc);
}

void *
dom_EventTarget__queryInterface (DomEventTarget *self, const gchar *interface, DomException *exc)
{
  *exc = 0;
  return ((DomNode *)self)->vtab->queryInterface ((DomNode *)self, interface, exc);
}

void
dom_EventTarget__addEventListener (DomEventTarget *self, DomString *type, DomEventListener *listener, DomBoolean useCapture, DomException *exc)
{
	*exc = 0;

	((DomNode *)self)->vtab->addEventListener ((DomNode *)self, type, listener, useCapture, exc);
}

void
dom_EventTarget__removeEventListener (DomEventTarget *self, DomString *type, DomEventListener *listener, DomBoolean useCapture, DomException *exc)
{
  *exc = 0;
  ((DomNode *)self)->vtab->removeEventListener ((DomNode *)self, type, listener, useCapture, exc);
}

DomBoolean
dom_EventTarget__dispatchEvent (DomEventTarget *self, DomEvent *evt, DomException *exc)
{
  *exc = 0;
  return ((DomNode *)self)->vtab->dispatchEvent ((DomNode *)self, evt, exc);
}

DomEvent *
dom_DocumentEvent__createEvent (DomDocumentEvent *self, DomString *eventType, DomException *exc)
{
	*exc = 0;

	return ((DomDocument *)self)->vtab->createEvent ((DomDocument *)self, eventType, exc);
}

void
dom_Event__ref (DomEvent *self, DomException *exc)
{
  *exc = 0;
  self->vtab->ref (self, exc);
}

void
dom_Event__unref (DomEvent *self, DomException *exc)
{
  *exc = 0;
  self->vtab->unref (self, exc);
}

void *
dom_Event__queryInterface (DomEvent *self, const gchar *interface, DomException *exc)
{
  *exc = 0;
  return self->vtab->queryInterface (self, interface, exc);
}

DomString *
dom_Event__get_type (DomEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_type (self, exc);
}

DomEventTarget *
dom_Event__get_target (DomEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_target (self, exc);
}

DomEventTarget *
dom_Event__get_currentTarget (DomEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_currentTarget (self, exc);
}

unsigned short
dom_Event__get_eventPhase (DomEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_eventPhase (self, exc);
}

DomBoolean
dom_Event__get_bubbles (DomEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_bubbles (self, exc);
}

DomBoolean
dom_Event__get_cancelable (DomEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_cancelable (self, exc);
}

DomTimeStamp *
dom_Event__get_timeStamp (DomEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_timeStamp (self, exc);
}

void
dom_Event__stopPropagation (DomEvent *self, DomException *exc)
{
  *exc = 0;
  self->vtab->stopPropagation (self, exc);
}

void
dom_Event__preventDefault (DomEvent *self, DomException *exc)
{
  *exc = 0;
  self->vtab->preventDefault (self, exc);
}

void
dom_Event__initEvent (DomEvent *self, DomString *eventTypeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomException *exc)
{
  *exc = 0;
  self->vtab->initEvent (self, eventTypeArg, canBubbleArg, cancelableArg, exc);
}

DomNode *
dom_MutationEvent__get_relatedNode (DomMutationEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_relatedNode (self, exc);
}

DomString *
dom_MutationEvent__get_prevValue (DomMutationEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_prevValue (self, exc);
}

DomString *
dom_MutationEvent__get_newValue (DomMutationEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_newValue (self, exc);
}

DomString *
dom_MutationEvent__get_attrName (DomMutationEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_attrName (self, exc);
}

unsigned short
dom_MutationEvent__get_attrChange (DomMutationEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_attrChange (self, exc);
}

void
dom_MutationEvent__initMutationEvent (DomMutationEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomNode *relatedNodeArg, DomString *prevValueArg, DomString *newValueArg, DomString *attrNameArg, unsigned short attrChangeArg, DomException *exc)
{
  *exc = 0;
  self->vtab->initMutationEvent (self, typeArg, canBubbleArg, cancelableArg, relatedNodeArg, prevValueArg, newValueArg, attrNameArg, attrChangeArg, exc);
}

DomAbstractView *
dom_UIEvent__get_view (DomUIEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_view (self, exc);
}

long
dom_UIEvent__get_detail (DomUIEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_detail (self, exc);
}

void
dom_UIEvent__initUIEvent (DomUIEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, long detailArg, DomException *exc)
{
  *exc = 0;
  self->vtab->initUIEvent (self, typeArg, canBubbleArg, cancelableArg, viewArg, detailArg, exc);
}

long
dom_MouseEvent__get_screenX (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_screenX (self, exc);
}

long
dom_MouseEvent__get_screenY (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_screenY (self, exc);
}

long
dom_MouseEvent__get_clientX (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_clientX (self, exc);
}

long
dom_MouseEvent__get_clientY (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_clientY (self, exc);
}

DomBoolean
dom_MouseEvent__get_ctrlKey (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_ctrlKey (self, exc);
}

DomBoolean
dom_MouseEvent__get_shiftKey (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_shiftKey (self, exc);
}

DomBoolean
dom_MouseEvent__get_altKey (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_altKey (self, exc);
}

DomBoolean
dom_MouseEvent__get_metaKey (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_metaKey (self, exc);
}

unsigned short
dom_MouseEvent__get_button (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_button (self, exc);
}

DomEventTarget *
dom_MouseEvent__get_relatedTarget (DomMouseEvent *self, DomException *exc)
{
  *exc = 0;
  return self->vtab->get_relatedTarget (self, exc);
}

void
dom_MouseEvent__initMouseEvent (DomMouseEvent *self, DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, DomAbstractView *viewArg, long detailArg, long screenXArg, long screenYArg, long clientXArg, long clientYArg, DomBoolean ctrlKeyArg, DomBoolean altKeyArg, DomBoolean shiftKeyArg, DomBoolean metaKeyArg, unsigned short buttonArg, DomEventTarget *relatedTargetArg, DomException *exc)
{
  *exc = 0;
  self->vtab->initMouseEvent (self, typeArg, canBubbleArg, cancelableArg, viewArg, detailArg, screenXArg, screenYArg, clientXArg, clientYArg, ctrlKeyArg, altKeyArg, shiftKeyArg, metaKeyArg, buttonArg, relatedTargetArg, exc);
}
