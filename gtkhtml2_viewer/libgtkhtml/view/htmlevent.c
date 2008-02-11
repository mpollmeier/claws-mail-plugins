/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2000-2001 CodeFactory AB
   Copyright (C) 2000-2001 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2000-2001 Anders Carlsson <andersca@codefactory.se>
   
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#include "dom/events/dom-event-utils.h"
#include "view/htmlevent.h"
#include "view/htmlselection.h"
#include "layout/htmlboxinline.h"
#include "layout/html/htmlboxform.h"
#include "layout/htmlboxtablerowgroup.h"

static DomNode *
html_event_find_parent_dom_node (HtmlBox *box)
{
	if (box && box->dom_node)
		return box->dom_node;
	
	while (box && box->dom_node == NULL)
		box = box->parent;

	return box ? box->dom_node : NULL;
}

static gboolean
html_event_xy_in_box (HtmlBox *box, gint tx, gint ty, gint x, gint y)
{
	gint ox, oy;
	gboolean rv;
	
	ox = box->x;
	oy = box->y;
	
	rv = TRUE;
	
	if ((HTML_BOX_GET_STYLE (box)->position == HTML_POSITION_RELATIVE ||  
	     HTML_BOX_GET_STYLE (box)->position == HTML_POSITION_ABSOLUTE)) { 
		gint width  = html_box_get_containing_block_width (box); 
		gint height = html_box_get_containing_block_height (box); 
		
		if (HTML_BOX_GET_STYLE (box)->surround->position.left.type != HTML_LENGTH_AUTO) 
			box->x += html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.left, width); 
		else if (HTML_BOX_GET_STYLE (box)->surround->position.right.type != HTML_LENGTH_AUTO) { 
			if (HTML_BOX_GET_STYLE (box)->display == HTML_DISPLAY_INLINE) 
				box->x -= html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.right, width); 
			else 
				box->x += width - box->width - html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.right, width); 
		} 
		if (HTML_BOX_GET_STYLE (box)->surround->position.top.type != HTML_LENGTH_AUTO) 
			box->y += html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.top, height); 
			
		else if (HTML_BOX_GET_STYLE (box)->surround->position.bottom.type != HTML_LENGTH_AUTO) { 
			if (HTML_BOX_GET_STYLE (box)->display == HTML_DISPLAY_INLINE) 
				box->y -= html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.bottom, height); 
			else 
				box->y += height - box->height - html_length_get_value (&HTML_BOX_GET_STYLE (box)->surround->position.bottom, height); 
		} 
	}

	if (x < box->x + tx ||
	    x > box->x + tx + box->width ||
	    y < box->y + ty ||
	    y > box->y + ty + box->height)
		rv = FALSE;
	
	box->x = ox;
	box->y = oy;

	return rv;
}

static void
html_event_find_box_traverser (HtmlBox *self, gint tx, gint ty, gint x, gint y, HtmlBox **smallest)
{
        HtmlBox *box;

        box = self->children;

        while (box) {
		/* These boxes always has x = 0, y = 0, w = 0 and h = 0 so we have to do
		 * a special case for these */
		if (HTML_IS_BOX_INLINE (box) || HTML_IS_BOX_TABLE_ROW_GROUP (box) || HTML_IS_BOX_FORM (box)) {

			HtmlBox *old_smallest = *smallest;
			html_event_find_box_traverser (box,
						       box->x + tx + html_box_left_mbp_sum (box, -1),
						       box->y + ty + html_box_top_mbp_sum (box, -1), x, y, smallest);

			if (old_smallest != *smallest)
				break;
		}
		else if (html_event_xy_in_box (box, tx, ty, x, y)) {
			
			*smallest = box;
			html_event_find_box_traverser (box,
						       box->x + tx + html_box_left_mbp_sum (box, -1),
						       box->y + ty + html_box_top_mbp_sum (box, -1), x, y, smallest);
			break;
		
                }

                box = box->next;
        }
}

HtmlBox *
html_event_find_root_box (HtmlBox *self, gint x, gint y)
{
	HtmlBox *box;
	gint tx = 0, ty = 0;
	
	if (html_event_xy_in_box (self, 0, 0, x, y)) {
		box = self;
		tx = html_box_left_mbp_sum (box, -1);
		ty = html_box_top_mbp_sum (box, -1);
	}
	else
		box = NULL;
	
        html_event_find_box_traverser (self, self->x + tx, self->y + ty, x, y, &box);

        return box;
}

static gboolean
emit_button_mouse_event (HtmlView *view, DomNode *node, const gchar *event_type, GdkEventButton *event)
{
	gboolean ctrl_key, alt_key, shift_key, meta_key;
	long client_x, client_y, screen_x, screen_y, detail;

	client_x = event->x;
	client_y = event->y;
	screen_x = event->x_root;
	screen_y = event->y_root;
	
	shift_key = ((event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK);
	ctrl_key = ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK);
	alt_key = ((event->state & GDK_MOD1_MASK) == GDK_MOD1_MASK);
	meta_key = FALSE;
	
	detail = view->mouse_detail;

	/* FIXME: Use the view argument here */
	return dom_MouseEvent_invoke (DOM_EVENT_TARGET (node), event_type, TRUE, TRUE, DOM_ABSTRACT_VIEW (view), 0, screen_x, screen_y, client_x, client_y,
				      ctrl_key, alt_key, shift_key, meta_key, detail, NULL);
}

static gboolean
emit_motion_mouse_event (HtmlView *view, DomNode *node, const gchar *event_type, GdkEventMotion *event)
{
	gboolean ctrl_key, alt_key, shift_key, meta_key;
	long client_x, client_y, screen_x, screen_y, detail;

	client_x = event->x;
	client_y = event->y;
	screen_x = event->x_root;
	screen_y = event->y_root;
	
	shift_key = ((event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK);
	ctrl_key = ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK);
	alt_key = ((event->state & GDK_MOD1_MASK) == GDK_MOD1_MASK);
	meta_key = FALSE;
	
	detail = view->mouse_detail;

	/* FIXME: Use the view argument here */
	return dom_MouseEvent_invoke (DOM_EVENT_TARGET (node), event_type, TRUE, TRUE, DOM_ABSTRACT_VIEW (view), 0, screen_x, screen_y, client_x, client_y,
				      ctrl_key, alt_key, shift_key, meta_key, detail, NULL);
}

void
html_event_button_press (HtmlView *view, GdkEventButton *event)
{
	DomNode *node;
	HtmlBox *box;

	if (!view->root || event->type != GDK_BUTTON_PRESS)
		return;

	html_selection_start (view, event);

	box = html_event_find_root_box (view->root, (gint)event->x, (gint)event->y);
	
	node = html_event_find_parent_dom_node (box);

	if (node != NULL &&
	    event->x - view->mouse_down_x == 0 &&
	    event->y - view->mouse_down_y == 0) {
		view->mouse_detail++;
	}
	else {
		view->mouse_detail = 0;
	}

	view->mouse_down_x = event->x;
	view->mouse_down_y = event->y;

	if (node && emit_button_mouse_event (view, node, "mousedown", event)) {
		html_document_update_active_node (view->document, node);
	}
}

static xmlChar *
get_href (DomNode *node)
{
	xmlChar *href_prop;

	/* Check if we have a link anywhere */
	while (node) {
		if (node->xmlnode->name &&
		    strcasecmp (node->xmlnode->name, "a") == 0 &&
		    (href_prop = xmlGetProp (node->xmlnode, "href")))
			return href_prop;

		node = dom_Node__get_parentNode (node);
	}
	return NULL;
}

void
html_event_button_release (HtmlView *view, GdkEventButton *event)
{
	DomNode *node;
	HtmlBox *box;
	gchar *href_prop;
	
	if (!view->root)
		return;
	
	html_selection_end (view, event);

	box = html_event_find_root_box (view->root, (gint)event->x, (gint)event->y);
	
	node = html_event_find_parent_dom_node (box);

	if (node && emit_button_mouse_event (view, node, "mouseup", event)) {
		html_document_update_active_node (view->document, NULL);
	}
	
	/* Check whether to emit a clicked event */
	if (event->x - view->mouse_down_x == 0 &&
	    event->y - view->mouse_down_y == 0) {
		if (node != NULL && emit_button_mouse_event (view, node, 
							     "click",  event)) {

			/* Check if we have a link anywhere */
			if ((href_prop = get_href (node))) {

				g_signal_emit_by_name (view->document, 
						       "link_clicked", 
						       href_prop);
				xmlFree (href_prop);
			}
		}
	}
	else {
		view->mouse_detail = 0;
	}
}

void
html_event_mouse_move (HtmlView *view, GdkEventMotion *event)
{
	DomNode *node;
	HtmlBox *box;
	
	if (!view->root)
		return;
	
	box = html_event_find_root_box (view->root, (gint)event->x, (gint)event->y);
	
	node = html_event_find_parent_dom_node (box);
	
	if (node && (node != view->document->hover_node)) {
		GdkCursor *cursor = NULL;
		xmlChar *href_prop;

		/* FIXME: What to do if these events are cancelled? */
		if (view->document->hover_node != NULL)
			emit_motion_mouse_event (view, view->document->hover_node, "mouseout", event);
		
		emit_motion_mouse_event (view, node, "mouseover", event);
		
		html_document_update_hover_node (view->document, node);

		/* Check if we have a link anywhere */
		if ((href_prop = get_href (node))) {
			
			g_signal_emit_by_name (view, "on_url", href_prop);
			xmlFree (href_prop);
			view->on_url = TRUE;
		}
		else if (view->on_url) {
			g_signal_emit_by_name (view, "on_url", NULL);
			view->on_url = FALSE;
		}

		
		/* We change cursors here, since we only have to do that in one view */
		switch (HTML_BOX_GET_STYLE (box)->inherited->cursor) {
		case HTML_CURSOR_CROSSHAIR:
			cursor = gdk_cursor_new (GDK_CROSSHAIR);
			break;
		case HTML_CURSOR_DEFAULT:
			cursor = NULL;
			break;
		case HTML_CURSOR_POINTER:
			cursor = gdk_cursor_new (GDK_HAND2);
			break;
		case HTML_CURSOR_MOVE:
			cursor = gdk_cursor_new (GDK_FLEUR);
			break;
		case HTML_CURSOR_E_RESIZE:
			cursor = gdk_cursor_new (GDK_RIGHT_SIDE);
			break;
		case HTML_CURSOR_NE_RESIZE:
			cursor = gdk_cursor_new (GDK_TOP_RIGHT_CORNER);
			break;
		case HTML_CURSOR_NW_RESIZE:
			cursor = gdk_cursor_new (GDK_TOP_LEFT_CORNER);
			break;
		case HTML_CURSOR_N_RESIZE:
			cursor = gdk_cursor_new (GDK_TOP_SIDE);
			break;
		case HTML_CURSOR_SE_RESIZE:
			cursor = gdk_cursor_new (GDK_BOTTOM_RIGHT_CORNER);
			break;
		case HTML_CURSOR_SW_RESIZE:
			cursor = gdk_cursor_new (GDK_BOTTOM_LEFT_CORNER);
			break;
		case HTML_CURSOR_S_RESIZE:
			cursor = gdk_cursor_new (GDK_BOTTOM_SIDE);
			break;
		case HTML_CURSOR_W_RESIZE:
			cursor = gdk_cursor_new (GDK_LEFT_SIDE);
			break;
		case HTML_CURSOR_TEXT:
			cursor = gdk_cursor_new (GDK_XTERM);
			break;
		case HTML_CURSOR_HELP:
			cursor = gdk_cursor_new (GDK_QUESTION_ARROW);
			break;
		case HTML_CURSOR_WAIT:
			cursor = gdk_cursor_new (GDK_WATCH);
			break;
			
		case HTML_CURSOR_AUTO:
			/* This sets the cursor depending on the type of content */
			
			if (node->xmlnode->type == XML_TEXT_NODE && !cursor)
				cursor = gdk_cursor_new (GDK_XTERM);
			else
				cursor = NULL;
			break;
		default:
			cursor = NULL;
		}
		
		/* Set the cursor */
		gdk_window_set_cursor (GTK_WIDGET (view)->window, cursor);
		
		/* And unref it */
		if (cursor)
			gdk_cursor_unref (cursor);
	}
}

void
html_event_activate (HtmlView *view)
{
	DomNode *node;
	gchar *href_prop;

	if (view->document && view->document->focus_element) {
		node = DOM_NODE (view->document->focus_element);

		if ((href_prop = get_href (node))) {

			g_signal_emit_by_name (view->document, 
					       "link_clicked", 
					       href_prop);
			xmlFree (href_prop);
		}
	}
}

gboolean
html_event_key_press (HtmlView *view, GdkEventKey *event)
{
	/* FIXME: Emit DOM3 key events here */
	
	switch (event->keyval) {
	case GDK_Down:
		return TRUE;
	default:
		break;
	}
	
	return FALSE;
}
