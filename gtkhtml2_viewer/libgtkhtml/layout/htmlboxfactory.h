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

#ifndef __HTMLBOXFACTORY_H__
#define __HTMLBOXFACTORY_H__

#include <libxml/parser.h>

#include <document/htmldocument.h>
#include <layout/htmlbox.h>
#include <view/htmlview.h>

G_BEGIN_DECLS

HtmlBox *       html_box_factory_get_box     (HtmlView *view, DomNode *node, HtmlBox *parent_box);
HtmlStyleChange html_box_factory_restyle_box (HtmlView *view, HtmlBox *box, HtmlAtom pseudo);
HtmlBox *       html_box_factory_new_box     (HtmlView *view, DomNode *node, gboolean force_new);

G_END_DECLS

#endif /* __HTMLBOXFACTORY_H__ */
