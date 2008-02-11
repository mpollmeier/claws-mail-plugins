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

#include <libxml/entities.h>
#include <libxml/debugXML.h>
#include "dom-entity.h"

DomString *
dom_Entity__get_publicId (DomEntity *entity)
{
#ifdef LIBXML_DEBUG_ENABLED
	xmlDebugDumpOneNode (stdout, DOM_NODE (entity)->xmlnode, 0);
#endif

	return NULL;
}

static void
dom_entity_class_init (DomEntityClass *klass)
{
}

static void
dom_entity_init (DomEntity *doc)
{
}

GType
dom_entity_get_type (void)
{
	static GType dom_entity_type = 0;

	if (!dom_entity_type) {
		static const GTypeInfo dom_entity_info = {
			sizeof (DomEntityClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_entity_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomEntity),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_entity_init,
		};

		dom_entity_type = g_type_register_static (DOM_TYPE_NODE, "DomEntity", &dom_entity_info, 0);
	}

	return dom_entity_type;
}
