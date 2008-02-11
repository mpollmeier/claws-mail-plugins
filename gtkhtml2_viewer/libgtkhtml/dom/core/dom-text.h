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

#ifndef __DOM_TEXT_H__
#define __DOM_TEXT_H__

#include <dom/core/dom-characterdata.h>

#define DOM_TYPE_TEXT             (dom_text_get_type ())
#define DOM_TEXT(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), DOM_TYPE_TEXT, DomText))
#define DOM_TEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DOM_TYPE_TEXT, DomTextClass))
#define DOM_IS_TEXT(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object),  DOM_TYPE_TEXT))
#define DOM_IS_TEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DOM_TYPE_TEXT))
#define DOM_TEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DOM_TYPE_TEXT, DomTextClass))

struct _DomText {
	DomCharacterData parent;
};

struct _DomTextClass {
	DomCharacterDataClass parent_class;
};

GType dom_text_get_type (void);

DomText *dom_Text_splitText (DomText *text, gulong offset, DomException *exc);

#endif /* __DOM_TEXT_H__ */
