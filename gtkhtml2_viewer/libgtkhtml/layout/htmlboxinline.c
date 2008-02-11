/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000 CodeFactory AB
   Copyright (C) 2000 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000 Anders Carlsson <andersca@codefactory.se>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <glib.h>
#include "htmlboxtext.h"
#include "htmlboxinline.h"

static void
html_box_inline_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBox *box;

	box = html_box_get_before (self);
	while (box) {
		html_box_paint (box, painter, area, self->x + tx, self->y + ty);
		box = box->next;
	}

	box = html_box_get_after (self);
	while (box) {
		html_box_paint (box, painter, area, self->x + tx, self->y + ty);
		box = box->next;
	}

	box = self->children;

	while (box) {
		/*
		 * Don't paint floating, relative or absolute boxes,
		 * they get painted in html_box_root_paint ()
		 */
		if (HTML_BOX_GET_STYLE (box)->Float == HTML_FLOAT_NONE || HTML_IS_BOX_TEXT (box)) {

			html_box_paint (box, painter, area, tx, ty);
		}
		box = box->next;
	}
}

static gboolean
html_box_inline_handles_events (HtmlBox *self)
{
	/* Normal inline boxes don't handle events */
	return FALSE;
}

static gint
html_box_inline_mbp_sum (HtmlBox *box, gint width)
{
	return 0;
}

static void
html_box_inline_class_init (HtmlBoxClass *klass)
{
	klass->paint = html_box_inline_paint;
	klass->handles_events = html_box_inline_handles_events;
	klass->left_mbp_sum   = html_box_inline_mbp_sum;
	klass->right_mbp_sum  = html_box_inline_mbp_sum;
	klass->top_mbp_sum    = html_box_inline_mbp_sum;
	klass->bottom_mbp_sum = html_box_inline_mbp_sum;
}

static void
html_box_inline_init (HtmlBox *box)
{
}

GType
html_box_inline_get_type (void)
{
       static GType html_type = 0;

       if (!html_type) {
               static GTypeInfo type_info = {
                       sizeof (HtmlBoxInlineClass),
		       NULL,
		       NULL,
                       (GClassInitFunc) html_box_inline_class_init,
		       NULL,
		       NULL,
		       sizeof (HtmlBoxInline),
		       16,
                       (GInstanceInitFunc) html_box_inline_init
               };

               html_type = g_type_register_static (HTML_TYPE_BOX, "HtmlBoxInline", &type_info, 0);
       }
       
       return html_type;
}

HtmlBox *
html_box_inline_new (void)
{
	return g_object_new (HTML_TYPE_BOX_INLINE, NULL);
}








