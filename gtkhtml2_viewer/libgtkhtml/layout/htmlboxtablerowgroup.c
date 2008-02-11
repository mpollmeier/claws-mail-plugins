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

#include "htmlboxtable.h"
#include "htmlboxtablerowgroup.h"
#include "htmlrelayout.h"

static HtmlBoxClass *parent_class = NULL;

#if 0
static HtmlBoxTable *
get_table (HtmlBoxTableRowGroup *row)
{
	HtmlBox *parent = HTML_BOX (row)->parent;
	if (HTML_IS_BOX_TABLE (parent))
		return HTML_BOX_TABLE (parent);
	
	parent = parent->parent;
	if (HTML_IS_BOX_TABLE (parent))
		return HTML_BOX_TABLE (parent);

	return NULL;
}
#endif

static void
html_box_table_row_group_append_child (HtmlBox *self, HtmlBox *child)
{
	HtmlBoxTable *table;
	HtmlBoxTableRowGroup *group = HTML_BOX_TABLE_ROW_GROUP (self);

	if (!HTML_IS_BOX_TABLE (self->parent))
	    return;

	table = HTML_BOX_TABLE (self->parent);

	switch (HTML_BOX_GET_STYLE (child)->display) {
	case HTML_DISPLAY_TABLE_CELL:
		/* If we find a table cell without a row as a parent
		 * we have to create a new row
		 */
		{
#if 0
		HtmlBox *row = html_box_table_row_new ();
		row->style = g_new0 (HtmlStyle, 1);
		row->style->display = HTML_DISPLAY_TABLE_ROW;
		row->style->inherited->color = HTML_COLOR (g_object_ref (G_OBJECT (self->style->inherited->color)));
		row->style->inherited->font = HTML_FONT (g_object_ref (G_OBJECT (self->style->inherited->font)));
		html_box_append_child (self, row);
		html_box_append_child (row, child);
#endif
		}
		break;
	case HTML_DISPLAY_TABLE_CAPTION:

		table->caption = HTML_BOX_TABLE_CAPTION (child);
		parent_class->append_child (self, child);
		break;
	case HTML_DISPLAY_TABLE_ROW:

		switch (group->type) {
		case HTML_DISPLAY_TABLE_HEADER_GROUP:
			html_box_table_add_thead (table, HTML_BOX_TABLE_ROW (child));
			break;
		case HTML_DISPLAY_TABLE_FOOTER_GROUP:
			html_box_table_add_tfoot (table, HTML_BOX_TABLE_ROW (child));
			break;
		case HTML_DISPLAY_TABLE_ROW_GROUP:
			html_box_table_add_tbody (table, HTML_BOX_TABLE_ROW (child));
			break;
		default:
			g_warning("html_box_table_row_group_append_child: impossible!\n");
			return;
		}
		parent_class->append_child (self, child);
		break;
	default:
		parent_class->append_child (self, child);
		break;
	}
}

static void
html_box_table_row_group_class_init (HtmlBoxClass *klass)
{
	klass->append_child = html_box_table_row_group_append_child;
	parent_class = g_type_class_peek_parent (klass);
}


static void
html_box_table_row_group_init (HtmlBox *box)
{
}

GType
html_box_table_row_group_get_type (void)
{
	static GType html_type  = 0;

       if (!html_type) {
	       static GTypeInfo type_info = {
		       sizeof (HtmlBoxTableRowGroupClass),
		       NULL,
		       NULL,
                       (GClassInitFunc) html_box_table_row_group_class_init,
		       NULL,
		       NULL,
                       sizeof (HtmlBoxTableRowGroup),
		       16,
                       (GInstanceInitFunc) html_box_table_row_group_init
               };

               html_type = g_type_register_static (HTML_TYPE_BOX, "HtmlBoxTableRowGroup", &type_info, 0);
       }
       return html_type;
}

HtmlBox *
html_box_table_row_group_new (HtmlDisplayType type)
{
	HtmlBoxTableRowGroup *result;
	
	result = g_object_new (HTML_TYPE_BOX_TABLE_ROW_GROUP, NULL);
	result->type = type;
	
	return HTML_BOX (result);
}


