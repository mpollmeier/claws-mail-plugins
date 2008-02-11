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

#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "htmlboxroot.h"
#include "htmlboxinline.h"
#include "htmlboxtable.h"
#include "htmlboxtablecell.h"
#include "htmlrelayout.h"

static HtmlBoxClass *parent_class = NULL;

/* These are uggly but nessesary hacks / jb */
static gint
html_box_table_cell_left_mbp_sum (HtmlBox *box, gint width)
{
	HtmlBoxTable *table = HTML_BOX_TABLE_CELL (box)->table;
	if (table) 
		return table->cell_padding + parent_class->left_mbp_sum (box, width);
	else
		return parent_class->left_mbp_sum (box, width);
}

static gint
html_box_table_cell_right_mbp_sum (HtmlBox *box, gint width)
{
	HtmlBoxTable *table = HTML_BOX_TABLE_CELL (box)->table;
	if (table) 
		return table->cell_padding + parent_class->right_mbp_sum (box, width);
	else
		return parent_class->right_mbp_sum (box, width);
}

static gint
html_box_table_cell_top_mbp_sum (HtmlBox *box, gint width)
{
	HtmlBoxTable *table = HTML_BOX_TABLE_CELL (box)->table;
	if (table) 
		return table->cell_padding + parent_class->top_mbp_sum (box, width);
	else
		return parent_class->top_mbp_sum (box, width);
}

static gint
html_box_table_cell_bottom_mbp_sum (HtmlBox *box, gint width)
{
	HtmlBoxTable *table = HTML_BOX_TABLE_CELL (box)->table;
	if (table) 
		return table->cell_padding + parent_class->bottom_mbp_sum (box, width);
	else
		return parent_class->bottom_mbp_sum (box, width);
}

gint
html_box_table_cell_get_colspan (HtmlBoxTableCell *cell)
{
	return cell->colspan;
}

gint
html_box_table_cell_get_rowspan (HtmlBoxTableCell *cell)
{
	return cell->rowspan;
}

gint
html_box_table_cell_get_min_width (HtmlBoxTableCell *cell, HtmlRelayout *relayout)
{
	gboolean get_min_width = relayout->get_min_width;
	gboolean get_max_width = relayout->get_max_width;

	relayout->get_min_width = TRUE;
	relayout->get_max_width = FALSE;

	html_box_relayout (HTML_BOX (cell), relayout);

	relayout->get_min_width = get_min_width;
	relayout->get_max_width = get_max_width;

	return HTML_BOX (cell)->width;
}

gint
html_box_table_cell_get_max_width (HtmlBoxTableCell *cell, HtmlRelayout *relayout)
{
	gboolean get_min_width = relayout->get_min_width;
	gboolean get_max_width = relayout->get_max_width;

	relayout->get_min_width = FALSE;
	relayout->get_max_width = TRUE;

	html_box_relayout (HTML_BOX (cell), relayout);

	relayout->get_min_width = get_min_width;
	relayout->get_max_width = get_max_width;

	return HTML_BOX (cell)->width;
}

void
html_box_table_cell_relayout_width (HtmlBoxTableCell *cell, HtmlRelayout *relayout, gint width)
{
	cell->width = width;
	cell->height = 0;

	html_box_relayout (HTML_BOX (cell), relayout);

	HTML_BOX (cell)->width = width;
}

static void
apply_offset (HtmlBox *self, gint offset)
{
	HtmlBox *box = self->children;

	while (box) {
		if (HTML_IS_BOX_INLINE(box))
			apply_offset (box, offset);
		else
			box->y += offset;

		box = box->next;
	}
}

void
html_box_table_cell_do_valign (HtmlBoxTableCell *cell, gint height)
{
	HtmlBox *self = HTML_BOX (cell);
	gint diff = height - self->height;

	switch (HTML_BOX_GET_STYLE (self)->vertical_align) {
	case HTML_VERTICAL_ALIGN_TOP:
		diff = 0;
		break;
	case HTML_VERTICAL_ALIGN_BOTTOM:
		/* diff = diff */
		break;
	case HTML_VERTICAL_ALIGN_BASELINE:
	case HTML_VERTICAL_ALIGN_MIDDLE:
	default:
		diff /= 2;
		break;
	}

	apply_offset (self, diff);

	self->height = height;
}

static void
html_box_table_cell_update_geometry (HtmlBox *self, HtmlRelayout *relayout, HtmlLineBox *line, gint *y, gint *boxwidth, gint *boxheight) 
{
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);

	if (line->width > *boxwidth) {

		*boxwidth   = line->width;
		block->containing_width = line->width;
		self->width = *boxwidth + html_box_horizontal_mbp_sum (self);
		block->force_relayout = TRUE;
	}
	*y += line->height;
	
	if (*y > *boxheight) {
		*boxheight = *y;
		self->height = *boxheight + html_box_vertical_mbp_sum (self);
	}
}

static void
html_box_table_cell_get_boundaries (HtmlBox *self, HtmlRelayout *relayout, gint *boxwidth, gint *boxheight)
{
	HtmlBoxTableCell *cell = HTML_BOX_TABLE_CELL (self);
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);

	HTML_BOX_BLOCK (cell)->force_relayout = TRUE;

	if (relayout->get_min_width || relayout->get_max_width) {
		*boxwidth = *boxheight = 0;
		block->containing_width = 0;

		self->width = *boxwidth + html_box_horizontal_mbp_sum (self);
		self->height = *boxheight + html_box_vertical_mbp_sum (self);
		return;
	}

	*boxwidth  = cell->width - html_box_horizontal_mbp_sum (self);
	*boxheight = cell->height - html_box_vertical_mbp_sum (self);

	if (*boxwidth < 0)
		*boxwidth = 0;
	if (*boxheight < 0)
		*boxheight = 0;

	block->containing_width = *boxwidth;

	self->width  = *boxwidth  + html_box_horizontal_mbp_sum (self);
	self->height = *boxheight + html_box_vertical_mbp_sum (self);

	html_box_check_min_max_width_height (self, boxwidth, boxheight);
}

static void
check_floats (HtmlBox *self, GSList *float_list)
{
	HtmlBox *Float;

	while (float_list) {
		Float = (HtmlBox *)float_list->data;

		if (Float->is_relayouted && html_box_is_parent (Float, self)) {
			gint float_x_width = html_box_get_absolute_x (Float) + Float->width;
			gint float_y_height = html_box_get_absolute_y (Float) + Float->height;
			gint xdiff, ydiff;
			xdiff = float_x_width - html_box_get_absolute_x (self);
			ydiff = float_y_height - html_box_get_absolute_y (self);

			if (xdiff > self->width)
				self->width = xdiff;

			if (ydiff > self->height)
				self->height = ydiff;
		}
		float_list = float_list->next;
	}
}

static void
html_box_table_cell_find_table (HtmlBoxTableCell *cell)
{
	if (cell->table == NULL) {
		HtmlBox *box = ((HtmlBox *)cell)->parent;

		while (box && !HTML_IS_BOX_TABLE (box))
			box = box->parent;

		cell->table = HTML_BOX_TABLE (box);
	}
}
static void
html_box_table_cell_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	html_box_table_cell_find_table (HTML_BOX_TABLE_CELL (self));
	parent_class->relayout (self, relayout);
	check_floats (self, html_box_root_get_float_left_list  (HTML_BOX_ROOT (relayout->root)));
	check_floats (self, html_box_root_get_float_right_list (HTML_BOX_ROOT (relayout->root)));
	html_box_root_mark_floats_unrelayouted (HTML_BOX_ROOT (relayout->root), self);
}

static void
html_box_table_cell_handle_html_properties (HtmlBox *self, xmlNode *n)
{
	HtmlBoxTableCell *cell = HTML_BOX_TABLE_CELL (self);
	gchar *str;

	if ((str = xmlGetProp (n, "colspan"))) {

		cell->colspan = atoi (str);
		/* Try to ignore stupid values */
		if (cell->colspan < 1 || cell->colspan > 10000)
			cell->colspan = 1;

		xmlFree (str);
	}
	if ((str = xmlGetProp (n, "rowspan"))) {

		cell->rowspan = atoi (str);
		/* Try to ignore stupid values */
		if (cell->rowspan < 1 || cell->rowspan > 10000)
			cell->rowspan = 1;

		xmlFree (str);
	}
}

static void
html_box_table_cell_class_init (HtmlBoxTableCellClass *klass)
{
	HtmlBoxClass *box_class = (HtmlBoxClass *) klass;
	HtmlBoxBlockClass *block_class = (HtmlBoxBlockClass *) klass;

	box_class->relayout = html_box_table_cell_relayout;
	box_class->handle_html_properties = html_box_table_cell_handle_html_properties;
	box_class->left_mbp_sum   = html_box_table_cell_left_mbp_sum;
	box_class->right_mbp_sum  = html_box_table_cell_right_mbp_sum;
	box_class->top_mbp_sum    = html_box_table_cell_top_mbp_sum;
	box_class->bottom_mbp_sum = html_box_table_cell_bottom_mbp_sum;

	block_class->get_boundaries = html_box_table_cell_get_boundaries;
	block_class->update_geometry = html_box_table_cell_update_geometry;

       	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_table_cell_init (HtmlBox *box)
{
	HtmlBoxTableCell *cell = HTML_BOX_TABLE_CELL (box);

	cell->rowspan = cell->colspan = 1;
}

GType
html_box_table_cell_get_type (void)
{
       static GType html_type = 0;

       if (!html_type) {
               static GTypeInfo type_info = {
		       sizeof (HtmlBoxTableCellClass),
		       NULL,
		       NULL,
                       (GClassInitFunc) html_box_table_cell_class_init,
		       NULL,
		       NULL,
                       sizeof (HtmlBoxTableCell),
		       16,
                       (GInstanceInitFunc) html_box_table_cell_init
               };

               html_type = g_type_register_static (HTML_TYPE_BOX_BLOCK, "HtmlBoxTableCell", &type_info, 0);
       }
       
       return html_type;
}

HtmlBox *
html_box_table_cell_new (void)
{
	return HTML_BOX (g_type_create_instance (HTML_TYPE_BOX_TABLE_CELL));
}


