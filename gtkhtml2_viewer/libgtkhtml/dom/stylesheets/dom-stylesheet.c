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

#include "dom/core/dom-node.h"
#include "dom-stylesheet.h"

/**
 * dom_StyleSheet__get_type:
 * @sheet: a DomStyleSheet.
 * 
 * Returns the style sheet type for this style sheet.
 * 
 * Return value: The style sheet type. This value must be freed.
 **/
DomString *
dom_StyleSheet__get_type (DomStyleSheet *sheet)
{
	return dom_Element_getAttribute (DOM_ELEMENT (sheet->owner), "type");
}

/**
 * dom_StyleSheet__get_disabled:
 * @sheet: a DomStyleSheet.
 * 
 * Returns whether the style sheet is disabled or not.
 * 
 * Return value: TRUE if the style sheet is disabled, FALSE otherwise.
 **/
DomBoolean
dom_StyleSheet__get_disabled (DomStyleSheet *sheet)
{
	return sheet->sheet->disabled;
}


/**
 * dom_StyleSheet__set_disabled:
 * @sheet: a DomStyleSheet 
 * @disabled: TRUE if the stylesheet should be disabled, FALSE otherwise.
 * 
 * Sets whether the style sheet is disabled or not.
 **/
void
dom_StyleSheet__set_disabled (DomStyleSheet *sheet, DomBoolean disabled)
{
	sheet->sheet->disabled = disabled;

	/* FIXME: Emit our own style event here */
}

DomNode *
dom_StyleSheet__get_ownerNode (DomStyleSheet *sheet)
{
	return sheet->owner;
}

static void
dom_style_sheet_class_init (DomStyleSheetClass *klass)
{
}

static void
dom_style_sheet_init (DomStyleSheet *doc)
{
}

GType
dom_style_sheet_get_type (void)
{
	static GType dom_style_sheet_type = 0;

	if (!dom_style_sheet_type) {
		static const GTypeInfo dom_style_sheet_info = {
			sizeof (DomStyleSheetClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_style_sheet_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomStyleSheet),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_style_sheet_init,
		};

		dom_style_sheet_type = g_type_register_static (G_TYPE_OBJECT, "DomStyleSheet", &dom_style_sheet_info, 0);
	}

	return dom_style_sheet_type;
}
