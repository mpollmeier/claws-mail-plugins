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

#include "dom-styleevent.h"

gushort
dom_StyleEvent__get_styleChange (DomStyleEvent *event)
{
	return event->styleChange;
}

void
dom_StyleEvent_initStyleEvent (DomStyleEvent *event, const DomString *typeArg, DomBoolean canBubbleArg, DomBoolean cancelableArg, gushort styleChangeArg)
{
	dom_Event_initEvent (DOM_EVENT (event), typeArg, canBubbleArg, cancelableArg);

	event->styleChange = styleChangeArg;
}

GType
dom_style_event_get_type (void)
{
	static GType dom_style_event_type = 0;

	if (!dom_style_event_type) {
		static const GTypeInfo dom_style_event_info = {
			sizeof (DomStyleEventClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			NULL,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomStyleEvent),
			16,   /* n_preallocs */
			NULL,
		};

		dom_style_event_type = g_type_register_static (DOM_TYPE_EVENT, "DomStyleEvent", &dom_style_event_info, 0);
	}

	return dom_style_event_type;
}

