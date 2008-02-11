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
#include "htmlbox.h"
#include "htmlrelayout.h"
#include "htmlboxroot.h"

/*
 * FIXME: All these left_margin, max_width functions aren't complete untill we have a box->is_relayout flag that is working. / jb
 */


HtmlRelayout *
html_relayout_new (void)
{
	HtmlRelayout *relayout;

	relayout = g_new0 (HtmlRelayout, 1);

	return relayout;
}

void
html_relayout_destroy (HtmlRelayout *relayout)
{
	g_return_if_fail (relayout != NULL);

	g_free (relayout);
}

/*
 * This function should return the next y offset (box->y) where the
 * block should try to fit the box. It should also return -1 if there
 * aren't any more places to try */
static gint
html_relayout_next_float_offset_real (HtmlRelayout *relayout, HtmlBox *box, gint y, gint width, gint height, GSList *list)
{
	gint best = G_MAXINT;
	gint boxx, boxy;

	if (list == NULL)
		return -1;

	boxx = html_box_get_absolute_x (box) + html_box_left_mbp_sum (box, -1);
	boxy = html_box_get_absolute_y (box) + html_box_top_mbp_sum (box, -1);

	while (list) {
		HtmlBox *Float;
		gint Floatx, Floaty;

		Float = (HtmlBox *)list->data;

		/* If we have found our self in this list, then stop. Because a floating box doesn't
		   intercept the layout of boxes before itself */
		if (Float->is_relayouted == FALSE) {
			list = list->next;

			continue;
		}
		Floatx = html_box_get_absolute_x (Float);
		Floaty = html_box_get_absolute_y (Float);

		if ((boxy + y + height) > Floaty && (Floaty + Float->height) > (boxy + y) &&
		    (boxx + width) > Floatx && boxx < Floatx + Float->width && 
		    Floaty + Float->height < best)
			best = Floaty + Float->height;

		list = g_slist_next (list);
	}

	if (best == G_MAXINT)
		return -1;
	else
		return best - boxy;
}

/*
 * This function should return the next y offset (box->y) where the
 * block should try to fit the box. It should also return -1 if there
 * aren't any more places to try */
gint
html_relayout_next_float_offset (HtmlRelayout *relayout, HtmlBox *box, gint y, gint width, gint height)
{
	gint left, right;

	left  = html_relayout_next_float_offset_real (relayout, box, y, width, height, html_box_root_get_float_left_list  (HTML_BOX_ROOT (relayout->root)));
	right = html_relayout_next_float_offset_real (relayout, box, y, width, height, html_box_root_get_float_right_list (HTML_BOX_ROOT (relayout->root)));

	if (left == -1 && right == -1)
		return -1;
	else {
		if (left == -1)
			left = G_MAXINT;
		if (right == -1)
			right = G_MAXINT;
		return MIN (left, right);
	}
}

static gboolean
float_in_float (HtmlBox *box, HtmlBox *parent)
{
	box = box->parent;

	while (box) {

		if (box == parent)
			return FALSE;
		if (HTML_BOX_GET_STYLE (box)->Float != HTML_FLOAT_NONE)
			return TRUE;

		box = box->parent;
	}
	return FALSE;
}

gint 
html_relayout_get_left_margin (HtmlRelayout *relayout, HtmlBox *self, gint width, gint height, gint y)
{
	return html_relayout_get_left_margin_ignore (relayout, self, width, height, y, NULL);
}

gint 
html_relayout_get_left_margin_ignore (HtmlRelayout *relayout, HtmlBox *self, gint width, gint height, gint y, HtmlBox *ignore)
{
	GSList *list = html_box_root_get_float_left_list (HTML_BOX_ROOT (relayout->root));
	HtmlBox *Float;
	gint best, selfxwidth, x, boxx;
	gint Floatxwidth, Floatx, Floaty;

	if (list == NULL)
		return 0;

	best       = html_box_get_absolute_x (self);
	boxx       = best + html_box_left_mbp_sum (self, -1);
	selfxwidth = best + width;
#if 0
	selfxwidth = best + self->width - html_box_right_mbp_sum (self, -1);
#endif
	best      += html_box_left_mbp_sum (self, -1);
	x          = best;
	y          = html_box_get_absolute_y (self) + html_box_top_mbp_sum  (self, -1) + y;

	while (list) {
		Float = list->data;
		
		/* If we have found our self in this list, then stop. Because a floating box doesn't
		   intercept the layout of boxes before itself */
		if (Float->is_relayouted == FALSE) {
			list = list->next;

			continue;
		}
		if (Float == ignore)
			break;

		Floatx = html_box_get_absolute_x (Float);
		Floaty = html_box_get_absolute_y (Float);
		Floatxwidth = Floatx + Float->width;

		if ((y + height) > Floaty && (Floaty + Float->height) > y &&
		    selfxwidth > Floatx && boxx < Floatx + Float->width &&
		    best < Floatxwidth && !float_in_float (Float, self))
			best = Floatxwidth;

		list = list->next;
	}

	return (best - x) > 0 ? best - x : 0;
}

gint 
html_relayout_get_max_width (HtmlRelayout *relayout, HtmlBox *box, gint width, gint height, gint y)
{
	return html_relayout_get_max_width_ignore (relayout, box, width, height, y, NULL);
}
/*
 * This function returns the maximum width the box can have depending 
 * on the "flow:right" block boxes. If there aren't any "float:right"
 * boxes this function will return -1.
 */
gint 
html_relayout_get_max_width_ignore (HtmlRelayout *relayout, HtmlBox *box, gint width, gint height, gint y, HtmlBox *ignore)
{
	GSList *list = html_box_root_get_float_right_list (HTML_BOX_ROOT (relayout->root));
	HtmlBox *Float;
	gint best = G_MAXINT;
	gint boxx, boxy, Floatxwidth, Floatx, Floaty;
	gint boxy_box_height, x;

	if (list == NULL)
		return -1;

	boxx = x = html_box_get_absolute_x (box) + html_box_left_mbp_sum (box, -1);
	boxy = html_box_get_absolute_y (box) + html_box_top_mbp_sum (box, -1) + y;

	boxy_box_height = boxy + height;

	while (list) {
		Float = list->data;

		/* If we have found our self in this list, then stop. Because a floating box doesn't
		   intercept the layout of boxes before itself */
		if (Float->is_relayouted == FALSE) {
			list = list->next;

			continue;
		}
		if (Float == ignore)
			break;

		Floatx = html_box_get_absolute_x (Float);
		Floaty = html_box_get_absolute_y (Float);
		Floatxwidth = Floatx + Float->width;

		if (Floaty < boxy_box_height &&
		    boxy   < Floaty + Float->height &&
		    Floatx < boxx + width &&
		    Floatxwidth > boxx &&
		    Floatx <= best && !float_in_float (Float, box))
			best = Floatx;

		list = list->next;
	}
	if (best == G_MAXINT)
		return -1;

	return best - x >= 0 ? best - x : 0;
}

/* A helper function to position "float:left" boxes */
static gboolean 
html_relayout_will_fit_left (HtmlBox *parent, HtmlRelayout *relayout, HtmlBox *box, gint width, gint y)
{
	gint left_margin, max, real_max;

	left_margin = html_relayout_get_left_margin_ignore (relayout, parent, width, box->height, y, box);
	max = real_max = html_relayout_get_max_width_ignore (relayout, parent, width, box->height, y, box);
	if (max == -1)
		max = parent->width - html_box_horizontal_mbp_sum (parent);

	if (box->x < left_margin)
		return FALSE;
	
	if (max - left_margin < box->width && (real_max != -1 || left_margin != 0))
		return FALSE;
	
	if (max - left_margin >= box->width && (box->x + box->width > max))
		return FALSE;
	
	return TRUE;
}

/* A helper function to position "float:right" boxes */
static gboolean 
html_relayout_will_fit_right (HtmlBox *parent, HtmlRelayout *relayout, HtmlBox *box, gint width, gint y)
{
	gint max, real_max, left_margin;

	left_margin = html_relayout_get_left_margin (relayout, parent, width, box->height, y);
	max = real_max = html_relayout_get_max_width_ignore (relayout, parent, width, box->height, y, box);
	if (max == -1)
		max = parent->width - html_box_horizontal_mbp_sum (parent);

	if (real_max != -1 && box->x + box->width > max)
		return FALSE;

	if (left_margin > box->x && (left_margin > 0 || real_max != -1))
		return FALSE;

	return TRUE;
}

/* A helper function to position "float:left" boxes */
void
html_relayout_make_fit_left (HtmlBox *parent, HtmlRelayout *relayout, HtmlBox *box, gint width, gint y)
{
	while (html_relayout_will_fit_left (parent, relayout, box, width, y) == FALSE) {

		gint next_y;

		next_y = html_relayout_next_float_offset (relayout, parent, y, width, box->height);

		if (next_y != -1)
			y = next_y;
		else
			break;

		box->x = html_relayout_get_left_margin_ignore (relayout, parent, width, box->height, y, box);
	}
	box->y = y;
}


/* A helper function to position "float:right" boxes */
void
html_relayout_make_fit_right (HtmlBox *parent, HtmlRelayout *relayout, HtmlBox *box, gint width, gint y)
{
	while (html_relayout_will_fit_right (parent, relayout, box, width, y) == FALSE) {
		gint max_width, max_width2, next_y;
		
		next_y = html_relayout_next_float_offset (relayout, parent, y, width, box->height);
		
		if (next_y != -1)
			y = next_y;
		else
			break;
		
		max_width2 = max_width = html_relayout_get_max_width_ignore (relayout, parent, width, box->height, y, box);
		if (max_width == -1)
			max_width = parent->width - html_box_horizontal_mbp_sum (parent);

		box->x = max_width - box->width;
#if 0
		if (box->x < 0 && max_width2 == -1)
			box->x = 0;
#endif
	}
	box->y = y;
}













