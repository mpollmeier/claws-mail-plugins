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

#include <string.h>

#include "dom/events/dom-event-utils.h"
#include "dom-characterdata.h"

/**
 * dom_CharacterData__get_data:
 * @cdata: a CharacterData
 * @exc: return value for an exception
 * 
 * Returns the character data of the node
 * 
 * Return value: the character data, this value must be freed.
 **/
DomString *
dom_CharacterData__get_data (DomCharacterData *cdata)
{
	return g_strdup (DOM_NODE (cdata)->xmlnode->content);
}

/**
 * dom_CharacterData__set_data:
 * @cdata: a DomCharacterData
 * @data: The data to set.
 * @exc: Return location for an exception.
 * 
 * Sets the data for this CharacterData node
 **/
void
dom_CharacterData__set_data (DomCharacterData *cdata, const DomString *data, DomException *exc)
{
	gchar *oldValue= DOM_NODE (cdata)->xmlnode->content;
	
	DOM_NODE (cdata)->xmlnode->content = g_strdup (data);

	/* Emit a mutation event */
	dom_MutationEvent_invoke (DOM_EVENT_TARGET (cdata), "DOMCharacterDataModified", TRUE, FALSE,
				  NULL, oldValue, DOM_NODE (cdata)->xmlnode->content, NULL, 0);
	
	if (oldValue)
		xmlFree (oldValue);
}

/**
 * dom_CharacterData__get_length: 
 * @cdata: a CharacterData
 * 
 * Returns the length of the character data of the node
 * 
 * Return value: the length of the character data
 **/
gulong
dom_CharacterData__get_length (DomCharacterData *cdata)
{
	return g_utf8_strlen (DOM_NODE (cdata)->xmlnode->content, -1);
}

/**
 * dom_CharacterData_replaceData:
 * @cdata: a DomCharacterData
 * @offset: The character offset at which to start replacing
 * @count: The number of characters to replace
 * @arg: The DomString which with the range must be replaced.
 * @exc: return value for an exception
 * 
 * Replace the characters starting at the specified offset with the specified string.
 **/
void
dom_CharacterData_replaceData (DomCharacterData *cdata, gulong offset, gulong count, DomString *arg, DomException *exc)
{
	gint datalen = g_utf8_strlen (DOM_NODE (cdata)->xmlnode->content, -1);
	gchar *buf = DOM_NODE (cdata)->xmlnode->content;
	gchar *start_buf, *end_buf;
	
	if (offset < 0 || offset > datalen || count < 0 || count > datalen || count > g_utf8_strlen (arg, -1)) {
		DOM_SET_EXCEPTION (DOM_INDEX_SIZE_ERR);
		return;
	}
	
	start_buf = g_utf8_offset_to_pointer (buf, offset);
	end_buf = g_utf8_offset_to_pointer (buf, offset + count);
	
	memcpy (start_buf, arg, (end_buf - start_buf));
}

/**
 * dom_CharacterData_substringData: 
 * @cdata: a DomCharacterData
 * @offset: Start offset from substring to extract.
 * @count: The number of characters to extract
 * @exc: return value for an exception
 * 
 * Extracts a range of data from the node.
 * 
 * Return value: The specified substring. This value has to be freed.
 **/
DomString *
dom_CharacterData_substringData (DomCharacterData *cdata, gulong offset, gulong count, DomException *exc)
{
	gint datalength = g_utf8_strlen (DOM_NODE (cdata)->xmlnode->content, -1);
	gchar *buf = DOM_NODE (cdata)->xmlnode->content;
	gchar *end_buf;
	DomString *result;
	
	if (offset < 0 || offset > datalength || count > datalength || count < 0) {
		DOM_SET_EXCEPTION (DOM_INDEX_SIZE_ERR);
		return NULL;
	}

	buf = g_utf8_offset_to_pointer (buf, offset);
	end_buf = g_utf8_offset_to_pointer (buf, count);
	
	result = g_malloc (end_buf - buf + 1);
	memcpy (result, buf, end_buf - buf + 1);
	result [end_buf - buf] = '\0';
	
	return result;
}

/**
 * dom_CharacterData_appendData: 
 * @cdata: a DomCharacterData
 * @arg: the string to append
 * @exc: return value for an exception
 * 
 * Append the string to the end of the character data of the node.
 **/
void
dom_CharacterData_appendData (DomCharacterData *cdata, const DomString *arg, DomException *exc)
{
	gint len1, len2;
	gchar *new_str;

	len1 = strlen (DOM_NODE (cdata)->xmlnode->content);
	len2 = strlen (arg);

	new_str = g_malloc (len1 + len2 + 1);
	memcpy (new_str, DOM_NODE (cdata)->xmlnode->content, len1);
	memcpy (new_str + len1, arg, len2 + 1);
	new_str[len1+len2] = '\0';

	g_free (DOM_NODE (cdata)->xmlnode->content);
	DOM_NODE (cdata)->xmlnode->content = new_str;
}

/**
 * dom_CharacterData_deleteData:
 * @cdata: a DomCharacterData
 * @offset: The offset from which to start removing.
 * @count: The number of units to delete.
 * @exc: Return value for an exception
 * 
 * Remove a range of characters from the specified node.
 **/
void
dom_CharacterData_deleteData (DomCharacterData *cdata, gulong offset, gulong count, DomException *exc)
{
	gint buflen = g_utf8_strlen (DOM_NODE (cdata)->xmlnode->content, -1);
	gchar *buf = DOM_NODE (cdata)->xmlnode->content;
	gchar *result, *start_buf, *end_buf;
	
	if (offset < 0 || offset > buflen || count < 0 || count > buflen) {
		DOM_SET_EXCEPTION (DOM_INDEX_SIZE_ERR);
		return;
	}

	start_buf = g_utf8_offset_to_pointer (buf, offset);
	end_buf = g_utf8_offset_to_pointer (buf, offset + count);

	result = g_malloc (buflen - (end_buf - start_buf) + 1);

	memcpy (result, buf, start_buf - buf);
	memcpy (result + (start_buf - buf), buf + (end_buf - buf), buflen - (end_buf - buf));

	result[buflen - (end_buf - start_buf)] = '\0';

	g_free (buf);
	
	DOM_NODE (cdata)->xmlnode->content = result;
}

static void
dom_character_data_class_init (DomCharacterDataClass *klass)
{
}

static void
dom_character_data_init (DomCharacterData *doc)
{
}

GType
dom_character_data_get_type (void)
{
	static GType dom_character_data_type = 0;

	if (!dom_character_data_type) {
		static const GTypeInfo dom_character_data_info = {
			sizeof (DomCharacterDataClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) dom_character_data_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (DomCharacterData),
			16,   /* n_preallocs */
			(GInstanceInitFunc) dom_character_data_init,
		};

		dom_character_data_type = g_type_register_static (DOM_TYPE_NODE, "DomCharacterData", &dom_character_data_info, 0);
	}

	return dom_character_data_type;
}
