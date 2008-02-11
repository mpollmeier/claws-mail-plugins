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

#include "htmlboxroot.h"

static HtmlBoxClass *parent_class = NULL;
static void html_box_root_float_get_size (GSList *list, gint *width, gint *height);

static void
html_box_root_paint_float_list (HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty, GSList *list)
{
        while (list) {
                HtmlBox *Float = HTML_BOX (list->data);
                
                html_box_paint (Float, painter, area,
                                html_box_get_absolute_x (Float->parent) + html_box_left_mbp_sum (Float->parent, -1),
                                html_box_get_absolute_y (Float->parent) + html_box_top_mbp_sum (Float->parent, -1));

                list = g_slist_next (list);
        }
}

static void
html_box_root_paint_position_list (HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty, GSList *list)
{
        while (list) {
                HtmlBox *box = HTML_BOX (list->data);

		if (HTML_BOX_GET_STYLE (box)->position == HTML_POSITION_ABSOLUTE ||
		    HTML_BOX_GET_STYLE (box)->position == HTML_POSITION_RELATIVE)
			html_box_paint (box, painter, area, tx + html_box_get_absolute_x (box->parent) + html_box_left_mbp_sum (box->parent, -1), 
					ty + html_box_get_absolute_y (box->parent) + html_box_top_mbp_sum (box->parent, -1));

                list = g_slist_next (list);
        }
}

void
html_box_root_paint_fixed_list (HtmlPainter *painter, HtmlBox *root, gint tx, gint ty, GSList *list)
{
	GdkRectangle area;

        while (list) {
                HtmlBox *box = HTML_BOX (list->data);
		if (HTML_BOX_GET_STYLE (box)->position == HTML_POSITION_FIXED) {
			/* redraw the window on the position where we were last time */
			/* Create an area that covers both the old and the new position */
			area.x = MIN (box->x, tx);
			area.y = MIN (box->y, ty);
			area.width  = box->width + ABS (box->x - tx);
			area.height = box->height + ABS (box->y - ty);

			html_box_paint (root, painter, &area, 0, 0);
			box->x = 0;
			box->y = 0;
			/* end */
			/* paint the fixed box at the new position */
			html_box_apply_positioned_offset (box, &tx, &ty);
			html_box_paint (root, painter, &area, 0, 0);
			html_box_paint (box, painter, &area, tx, ty);

			/* <hack> <!--save the last position so that we can redraw the background there the next time--> */
			box->x = tx;
			box->y = ty;
			/* </hack> */

		}
                list = g_slist_next (list);
        }
}

static void
html_box_root_get_boundaries (HtmlBox *self, HtmlRelayout *relayout, gint *boxwidth, gint *boxheight)
{
	HtmlBoxRoot *root = HTML_BOX_ROOT (self);
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);
	gint new_width, new_height;

	new_width  = root->min_width - html_box_horizontal_mbp_sum (self);
	new_height = root->min_height - html_box_vertical_mbp_sum (self);

	if (new_width != *boxwidth) {
		/*
		 * If HtmlBoxRoot does not have a HtmlBoxBlock as child, i.e.
		 * there was no body tag do not update the boxwidth.
		 */
                if (*boxwidth == 0 ||
                    !self->children ||
                    HTML_IS_BOX_BLOCK (self->children)) {
			*boxwidth = new_width;
			HTML_BOX_BLOCK(root)->force_relayout = TRUE;
		}
	}

	if (new_height != *boxheight)
		*boxheight = new_height;

	block->containing_width = *boxwidth;
	self->width = root->min_width;
	self->height = root->min_height;
}

static void
html_box_root_paint (HtmlBox *self, HtmlPainter *painter, GdkRectangle *area, gint tx, gint ty)
{
	parent_class->paint (self, painter, area, tx, ty);

	html_box_root_paint_float_list (painter, area, tx, ty, html_box_root_get_float_left_list  (HTML_BOX_ROOT (self)));
	html_box_root_paint_float_list (painter, area, tx, ty, html_box_root_get_float_right_list (HTML_BOX_ROOT (self)));
	html_box_root_paint_position_list (painter, area, tx, ty, html_box_root_get_positioned_list (HTML_BOX_ROOT (self)));
}

static void
html_box_root_relayout (HtmlBox *self, HtmlRelayout *relayout)
{
	HtmlBoxRoot *root = HTML_BOX_ROOT (self);
	HtmlBoxBlock *block = HTML_BOX_BLOCK (self);

	parent_class->relayout (self, relayout);
	/* Make sure all the floatboxes fit inside this root box */
	html_box_root_float_get_size (html_box_root_get_float_left_list  (root), &self->width, &self->height);
	html_box_root_float_get_size (html_box_root_get_float_right_list (root), &self->width, &self->height);

	self->width = MAX (self->width, block->full_width);
}

static void
html_box_root_finalize (GObject *object)
{
	HtmlBoxRoot *root = HTML_BOX_ROOT (object);

	html_box_root_clear_float_left_list  (root);
	html_box_root_clear_float_right_list (root);
	html_box_root_clear_positioned_list  (root);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
html_box_root_class_init (HtmlBoxRootClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	HtmlBoxClass *box_class = (HtmlBoxClass *) klass;
	HtmlBoxBlockClass *block_class = (HtmlBoxBlockClass *) klass;

	object_class->finalize = html_box_root_finalize;
	
	box_class->paint      = html_box_root_paint;
	box_class->relayout   = html_box_root_relayout;

	block_class->get_boundaries = html_box_root_get_boundaries;

	parent_class = g_type_class_peek_parent (klass);
}

static void
html_box_root_init (HtmlBoxRoot *box)
{
}

GType
html_box_root_get_type (void)
{
	static GType html_type = 0;

	if (!html_type) {
		static GTypeInfo type_info = {
			sizeof (HtmlBoxRootClass),		  
			NULL,
			NULL,
			(GClassInitFunc) html_box_root_class_init,
			NULL,
			NULL,
			sizeof (HtmlBoxRoot),
			16,
			(GInstanceInitFunc) html_box_root_init
		};

		html_type = g_type_register_static (HTML_TYPE_BOX_BLOCK, "HtmlBoxRoot", &type_info, 0);
	}

	return html_type;
}

HtmlBox *
html_box_root_new (void)
{
	return g_object_new (HTML_TYPE_BOX_ROOT, NULL);
}

static gint
float_left_sort (gconstpointer a, gconstpointer b)
{
        HtmlBox *boxa = (HtmlBox *)a;
        HtmlBox *boxb = (HtmlBox *)b;
        gint ax, bx, ay, by;
                
        ax = html_box_get_absolute_x (boxa);
        bx = html_box_get_absolute_x (boxb);
        ay = html_box_get_absolute_y (boxa);
        by = html_box_get_absolute_y (boxb);

        if (ay > by)
                return 1;
        else if (ay == by) {
                if (ax > bx)
                        return 1;
                else
                        return -1;
        }
        else
                return -1;
}

static gint
float_right_sort (gconstpointer a, gconstpointer b)
{
        HtmlBox *boxa = (HtmlBox *)a;
        HtmlBox *boxb = (HtmlBox *)b;
        gint ax, bx, ay, by;
                
        ax = html_box_get_absolute_x (boxa);
        bx = html_box_get_absolute_x (boxb);
        ay = html_box_get_absolute_y (boxa);
        by = html_box_get_absolute_y (boxb);

        if (ay > by)
                return 1;
        else if (ay == by) {
                if (ax > bx)
                        return -1;
                else
                        return 1;
        }
        else
                return -1;
}

void
html_box_root_add_float (HtmlBoxRoot *root, HtmlBox *box)
{

	switch (HTML_BOX_GET_STYLE (box)->Float) {
	case HTML_FLOAT_LEFT:
	case HTML_FLOAT_CENTER:
		/* If it is already in the list don't add it again */
		if (g_slist_find (root->float_left_list, box))
			return;
		
		root->float_left_list = g_slist_insert_sorted (root->float_left_list, box, float_left_sort);
		break;
	case HTML_FLOAT_RIGHT:
		/* If it is already in the list don't add it again */
		if (g_slist_find (root->float_right_list, box))
			return;
		
		root->float_right_list = g_slist_insert_sorted (root->float_right_list, box, float_right_sort);
		break;
	default:
		g_warning("html_box_root_add_float: impossible\n");
		return;
	}
}

void
html_box_root_clear_float_left_list (HtmlBoxRoot *root)
{
	if (root->float_left_list) {

		g_slist_free (root->float_left_list);
		root->float_left_list = NULL;
	}
}

void
html_box_root_clear_float_right_list (HtmlBoxRoot *root)
{
	if (root->float_right_list) {

		g_slist_free (root->float_right_list);
		root->float_right_list = NULL;
	}
}

void
html_box_root_clear_positioned_list (HtmlBoxRoot *root)
{
	if (root->positioned_list) {
		g_slist_free (root->positioned_list);
		root->positioned_list = NULL;
	}
}

GSList *
html_box_root_get_float_left_list (HtmlBoxRoot *root)
{
	return root->float_left_list;
}

GSList *
html_box_root_get_float_right_list (HtmlBoxRoot *root)
{
	return root->float_right_list;
}

GSList *
html_box_root_get_positioned_list (HtmlBoxRoot *root)
{
	return root->positioned_list;
}

/*
 * This function returns the width and height of the area containing
 * all the floats, this function can be used by the root block to make
 * it larhge enough to cover all the floats
 */
static void
html_box_root_float_get_size (GSList *list, gint *width, gint *height)
{
	while (list) {
		HtmlBox *Float = (HtmlBox *)list->data;

		if (html_box_get_absolute_x (Float) + Float->width > *width)
			*width = html_box_get_absolute_x (Float) + Float->width;

		if (html_box_get_absolute_y (Float) + Float->height > *height)
			*height = html_box_get_absolute_y (Float) + Float->height;

		list = g_slist_next (list);
	}
}

static void
mark_floats_unrelayouted (GSList *list, HtmlBox *box)
{
	while (list) {
		
		HtmlBox *tmp, *Float;
		tmp = Float = HTML_BOX (list->data);

		while (tmp->parent) {
			if (tmp->parent == box) {
				Float->is_relayouted = FALSE;
				break;
			}
			tmp = tmp->parent;
		}
		list = list->next;
	}
}

static void
mark_floats_relayouted (GSList *list, HtmlBox *box)
{
	while (list) {
		
		HtmlBox *tmp, *Float;
		tmp = Float = HTML_BOX (list->data);

		while (tmp->parent) {
			if (tmp->parent == box) {
				Float->is_relayouted = TRUE;
				break;
			}
			tmp = tmp->parent;
		}
		list = list->next;
	}
}

void
html_box_root_mark_floats_unrelayouted (HtmlBoxRoot *root, HtmlBox *box) 
{
	mark_floats_unrelayouted (html_box_root_get_float_left_list  (root), box);
	mark_floats_unrelayouted (html_box_root_get_float_right_list (root), box);
}

void
html_box_root_mark_floats_relayouted (HtmlBoxRoot *root, HtmlBox *box) 
{
	mark_floats_relayouted (html_box_root_get_float_left_list  (root), box);
	mark_floats_relayouted (html_box_root_get_float_right_list (root), box);
}

void
html_box_root_add_positioned (HtmlBoxRoot *root, HtmlBox *box)
{
	/* If it is already in the list don't add it again */
	if (g_slist_find (root->positioned_list, box))
		return;
	
	root->positioned_list = g_slist_prepend (root->positioned_list, box);
}





