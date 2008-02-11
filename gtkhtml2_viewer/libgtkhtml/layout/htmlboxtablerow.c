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

#include "layout/htmlboxtable.h"
#include "layout/htmlboxtablerow.h"
#include "layout/htmlboxtablecell.h"
#include "layout/htmlrelayout.h"
#include "layout/html/htmlboxform.h"

static HtmlBoxClass *parent_class = NULL;

gint
html_box_table_row_get_num_cols (HtmlBox *self, gint rownum)
{
	HtmlBox *box = self->children;
	gint cols = 0;
	
	while (box) {
		/*
		 * This is to allow <form> tags in the middle of <tr> and <td>
		 */
		if (HTML_IS_BOX_FORM (box))
			cols += html_box_table_row_get_num_cols (box, rownum);

		if (!HTML_IS_BOX_TABLE_CELL (box)) {
			box = box->next;
			continue;
		}
		cols += html_box_table_cell_get_colspan (HTML_BOX_TABLE_CELL (box));
		box = box->next;
	}
	return cols;
}

static gboolean
html_box_table_row_handles_events (HtmlBox *self)
{
	/* Table row boxes don't handle events */
	return FALSE;
}

gint
html_box_table_row_update_spaninfo (HtmlBoxTableRow *row, gint *spaninfo)
{
	HtmlBox *box = HTML_BOX (row)->children;
	gint cols = 0, span;
	
	while (box) {
		/*
		 * This is to allow <form> tags in the middle of <tr> and <td>
		 */
		if (HTML_IS_BOX_FORM (box)) {
			if (HTML_IS_BOX_TABLE_ROW (box))
				cols += html_box_table_row_update_spaninfo (HTML_BOX_TABLE_ROW (box), &spaninfo[cols]);
		}

		if (!HTML_IS_BOX_TABLE_CELL (box)) {
			box = box->next;
			continue;
		}
		if (spaninfo)
			while (spaninfo[cols])
				cols++;
		
		span = html_box_table_cell_get_colspan (HTML_BOX_TABLE_CELL (box));
		while (span--)
			spaninfo[cols + span] = html_box_table_cell_get_rowspan (HTML_BOX_TABLE_CELL (box));

		cols += html_box_table_cell_get_colspan (HTML_BOX_TABLE_CELL (box));
		box = box->next;
	}
	return cols;
}

gint
html_box_table_row_fill_cells_array (HtmlBox *self, HtmlBox **cells, gint *spaninfo)
{
	HtmlBox *box = self->children;
	gint col = 0;
	
	while (box) {
		/*
		 * This is to allow <form> tags in the middle of <tr> and <td>
		 */
		if (HTML_IS_BOX_FORM (box))
			col += html_box_table_row_fill_cells_array (box, &cells[col], &spaninfo[col]);

		if (!HTML_IS_BOX_TABLE_CELL (box)) {
			box = box->next;
			continue;
		}

		if (spaninfo)
			while (spaninfo[col])
			       col++;

		cells[col] = box;
		col += html_box_table_cell_get_colspan (HTML_BOX_TABLE_CELL (box));
		box = box->next;
	}
	return col;
}

static HtmlBoxTable *
get_table (HtmlBoxTableRow *row)
{
	HtmlBox *parent = HTML_BOX (row)->parent;

	if (parent == NULL)
		return NULL;
	
	if (HTML_IS_BOX_TABLE (parent))
		return HTML_BOX_TABLE (parent);
	
	parent = parent->parent;

	if (parent == NULL)
		return NULL;

	if (HTML_IS_BOX_TABLE (parent))
		return HTML_BOX_TABLE (parent);

	return NULL;
}

static void
html_box_table_row_finalize (GObject *object)
{
	HtmlBoxTableRow *row = HTML_BOX_TABLE_ROW (object);
	HtmlBoxTable *table = get_table (row);
	if (table)
		html_box_table_remove_row (table, row);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
html_box_table_row_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	HtmlBox *box = self->children;
	tx += html_box_left_mbp_sum (self, -1);
	ty += html_box_top_mbp_sum (self, -1);

	while (box) {
		html_box_paint (box, painter, area, self->x + tx, self->y + ty);
		box = box->next;
	}
}

static void
html_box_table_row_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	/* Do nothing */
}

static void
html_box_table_row_append_child (HtmlBox *self, HtmlBox *child)
{
	HtmlBoxTable *table = get_table (HTML_BOX_TABLE_ROW (self));

	HTML_BOX_CLASS (parent_class)->append_child (self, child);

	if (table)
		html_box_table_cell_added (table);
}

static void
html_box_table_row_class_init (HtmlBoxTableClass *klass)
{
	HtmlBoxClass *box_class = (HtmlBoxClass *)klass;
	GObjectClass *object_class = (GObjectClass *)klass;
	
	object_class->finalize = html_box_table_row_finalize;
		
	box_class->paint = html_box_table_row_paint;
	box_class->relayout = html_box_table_row_relayout;
	box_class->append_child = html_box_table_row_append_child;
	box_class->handles_events = html_box_table_row_handles_events;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_table_row_init (HtmlBox *box)
{
}

GType
html_box_table_row_get_type (void)
{
       static GType html_type = 0;

       if (!html_type) {
	       static GTypeInfo type_info = {
		       sizeof (HtmlBoxTableRowClass),
		       NULL,
		       NULL,
		       (GClassInitFunc) html_box_table_row_class_init,
		       NULL,
		       NULL,
                       sizeof (HtmlBoxTableRow),
		       16,
                       (GInstanceInitFunc) html_box_table_row_init
               };

               html_type = g_type_register_static (HTML_TYPE_BOX, "HtmlBoxTableRow", &type_info, 0);
       }
       return html_type;
}

HtmlBox *
html_box_table_row_new (void)
{
	return g_object_new (HTML_TYPE_BOX_TABLE_ROW, NULL);
}


