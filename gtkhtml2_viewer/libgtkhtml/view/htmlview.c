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

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "document/htmlfocusiterator.h"
#include "document/htmldocument.h"
#include "dom/core/dom-node.h"
#include "dom/core/dom-element.h"
#include "dom/views/dom-abstractview.h"
#include "dom/traversal/dom-traversal-utils.h"
#include "layout/html/htmlboxembedded.h"
#include "layout/htmlboxroot.h"
#include "layout/htmlboxfactory.h"
#include "layout/htmlbox.h"
#include "layout/htmlboxinline.h"
#include "layout/htmlboxtext.h"
#include "layout/htmlstyle.h"
#include "graphics/htmlgdkpainter.h"
#include "util/htmlmarshal.h"
#include "htmlevent.h"
#include "htmlselection.h"
#include "htmlview.h"
#include "config.h"

#ifdef ENABLE_ACCESSIBILITY
#include "layout/htmlboxtable.h"
#include "a11y/htmlviewaccessiblefactory.h"
#include "a11y/htmlaccessiblefactory.h"
#include "a11y/htmlboxblockaccessible.h"
#include "a11y/htmlboxembeddedaccessible.h"
#include "a11y/htmlboxaccessible.h"
#include "a11y/htmlboxtableaccessible.h"
#endif

enum {
	MOVE_CURSOR,
	REQUEST_OBJECT,
	ON_URL,
	ACTIVATE,
	MOVE_FOCUS_OUT,
	TOGGLE_CURSOR,
	LAST_SIGNAL
};

static guint view_signals [LAST_SIGNAL] = { 0 };

#define RELAYOUT_TIMEOUT_INTERVAL 1000

static GtkLayoutClass *parent_class = NULL;

static gboolean html_view_remove_layout_box (HtmlView *view, DomNode *node);
static void html_view_try_jump (HtmlView *view);
static void html_view_set_saved_focus (HtmlView *view);
static DomElement* html_view_get_and_unset_saved_focus (HtmlView *view);
static void html_view_focus_element (HtmlView *view);

#ifdef ENABLE_ACCESSIBILITY
static AtkObject *  html_view_get_accessible (GtkWidget *widget);

HTML_ACCESSIBLE_FACTORY (HTML_TYPE_BOX_BLOCK_ACCESSIBLE, html_box_block_accessible)
HTML_ACCESSIBLE_FACTORY (HTML_TYPE_BOX_EMBEDDED_ACCESSIBLE, html_box_embedded_accessible)
HTML_ACCESSIBLE_FACTORY (HTML_TYPE_BOX_ACCESSIBLE, html_box_accessible)
HTML_ACCESSIBLE_FACTORY (HTML_TYPE_BOX_TABLE_ACCESSIBLE, html_box_table_accessible)
#endif

static GQuark quark_moving_focus_out = 0;
static GQuark quark_cursor_position = 0;
static GQuark quark_cursor_end_of_line = 0;
static GQuark quark_selection_bound = 0;
static GQuark quark_layout = 0;
static GQuark quark_cursor_visible = 0;
static GQuark quark_blink_timeout = 0;
static GQuark quark_button = 0;
static GQuark quark_virtual_cursor_x = 0;
static GQuark quark_virtual_cursor_y = 0;
static gboolean cursor_showing = FALSE;

HtmlBoxText* _html_view_get_cursor_box_text (HtmlView *view, gint *offset);

static HtmlBox*
find_last_child (HtmlBox *box)
{
	HtmlBox *child;

	child = box->children;
	while (child) {
		while (child->next) {
			child = child->next;
		}
		if (child->children) {
			child = child->children;
		} else {
			break;
		}
	}
	return child;
}

static HtmlBox*
find_previous_box (HtmlBox *box)
{
	HtmlBox *prev;
	HtmlBox *tmp;

	prev = box;

	if (prev->prev) {
		tmp = find_last_child (prev->prev);
		if (tmp) {
			prev = tmp;
		} else {
			prev = prev->prev;
		}
	} else {
		prev = prev->parent;
		while (prev)  {
			if (prev->prev) {
				tmp = find_last_child (prev->prev);
				if (tmp) {
					prev = tmp;
				} else {
					prev = prev->prev;
				}
				break;
			} else {
				prev = prev->parent;
			}
		}
	}
	return prev;
}

static HtmlBoxText*
find_previous_box_text (HtmlBox *box)
{
	HtmlBox *prev;
	HtmlBoxText *text;

	prev = box;

	while (prev) {
		prev = find_previous_box (prev);
		if (HTML_IS_BOX_TEXT (prev)) {
			text = HTML_BOX_TEXT (prev);
			if (html_box_text_get_len (text)) {
				return text;
			}
		}
	}
	return NULL;
}

static HtmlBox*
find_next_box (HtmlBox *box)
{
	HtmlBox *next;

	next = box;

	if (next->children) {
		next = next->children;
	} else  if (next->next) {
		next = next->next;
	} else {
		next = next->parent;
		while (next)  {
			if (next->next) {
				next = next->next;
				break;
			} else {
				next = next->parent;
			}
		}
	}
	return next;
}

static HtmlBoxText*
find_next_box_text (HtmlBox *box)
{
	HtmlBox *next;
	HtmlBoxText *text;

	next = box;

	while (next) {
		next = find_next_box (next);
		if (HTML_IS_BOX_TEXT (next)) {
			text = HTML_BOX_TEXT (next);
			if (html_box_text_get_len (text)) {
				return text;
			}
		}
	}
	return NULL;
}

static HtmlBoxText*
html_view_get_box_text_for_offset (HtmlView *view, gint *offset, gboolean end_of_line)
{
	HtmlBox *box;
	HtmlBoxText *last_text;
	HtmlBoxText *text;
	gchar *char_text;
	gint len;

	box = view->root;

	last_text = NULL;
	while (box) {
		text = find_next_box_text (box);
		if (text) {
			char_text = html_box_text_get_text (text, &len);
			len = g_utf8_strlen (char_text, len);
			if (len > 0) {
				if (len > *offset) {
					return text;
				} else if (end_of_line && *offset == len) {
					return text;	
				}
				*offset -= len;
				if (*offset == 0) {
					last_text = text;
				}
				box = HTML_BOX (text);
			}
		} else {
			if (last_text) {
				*offset = len + 1;
			}
			return last_text;
		}
	}
	return NULL;
}

static gboolean
find_offset (HtmlBox *box, HtmlBoxText *box_text, gint *offset)
{
	HtmlBox *child;
	HtmlBoxText *text;
	gchar *char_text;
	gint len;
	gboolean ret;

	if (HTML_IS_BOX_TEXT (box)) {
		text = HTML_BOX_TEXT (box);
		if (box_text == text)
			return TRUE;
		char_text = html_box_text_get_text (text, &len);
		len = g_utf8_strlen (char_text, len);
		*offset += len;
	}
	child = box->children;	
	while (child) {
		ret = find_offset (child, box_text, offset);
		if (ret)
			return ret;
		child = child->next;
	}
	return FALSE;
}

static gboolean
html_view_get_offset_for_box_text (HtmlView *view, HtmlBoxText *text, gint *offset)
{
	HtmlBox *root;
	gint temp_offset = 0;
	gboolean ret = FALSE;

	root = view->root;

	if (root) {
		ret = find_offset (root, text, &temp_offset);
		if (ret && offset)
			*offset = temp_offset;
	}
	return ret;
	
}

static gint
html_view_get_virtual_cursor_x (HtmlView *view)
{
	gint val;

	if (quark_virtual_cursor_x == 0)
		return -1;
	val = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (view),
						   quark_virtual_cursor_x));
	return val;
}

static void
html_view_set_virtual_cursor_x (HtmlView *view, gint x)
{
	if (quark_virtual_cursor_x == 0)
		quark_virtual_cursor_x = g_quark_from_static_string ("html-view-virtual-cursor-x");

	g_object_set_qdata (G_OBJECT (view), 
			    quark_virtual_cursor_x,
			    GINT_TO_POINTER (x));
}

static gint
html_view_get_virtual_cursor_y (HtmlView *view)
{
	gint val;

	if (quark_virtual_cursor_y == 0)
		return -1;
	val = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (view),
						   quark_virtual_cursor_y));
	return val;
}

static void
html_view_set_virtual_cursor_y (HtmlView *view, gint y)
{
	if (quark_virtual_cursor_y == 0)
		quark_virtual_cursor_y = g_quark_from_static_string ("html-view-virtual-cursor-y");

	g_object_set_qdata (G_OBJECT (view), 
			    quark_virtual_cursor_y,
			    GINT_TO_POINTER (y));
}

static gint
html_view_get_cursor_end_of_line (HtmlView *view)
{
	gint val;

	if (quark_cursor_end_of_line == 0)
		return 0;
	val = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (view),
						   quark_cursor_end_of_line));
	return val;
}

static void
html_view_set_cursor_end_of_line (HtmlView *view, gint val)
{
	if (quark_cursor_end_of_line == 0)
		quark_cursor_end_of_line = g_quark_from_static_string ("html-view-cursor-end-of-line");

	g_object_set_qdata (G_OBJECT (view), 
			    quark_cursor_end_of_line,
			    GINT_TO_POINTER (val));
}

static gint
html_view_get_cursor_position (HtmlView *view)
{
	gint val;
	gint val1;
	HtmlBoxText *text;

	if (view->sel_list) {
		if (view->sel_backwards) {
			if (HTML_IS_BOX_TEXT (view->sel_start))
				text = HTML_BOX_TEXT (view->sel_start);
			else
				text = HTML_BOX_TEXT (view->sel_list->data);

			val1 = view->sel_start_index;
		} else {
			if (HTML_IS_BOX_TEXT (view->sel_end))
				text = HTML_BOX_TEXT (view->sel_end);
			else
				text = HTML_BOX_TEXT (g_slist_last (view->sel_list)->data);

			val1 = view->sel_end_index;
		}
		if (html_view_get_offset_for_box_text (view, text, &val)) {
			gchar *str;

			str = html_box_text_get_text (text, NULL);
			val += g_utf8_pointer_to_offset (str, str + val1);
			return val;
		} else {
			g_warning ("No offset for cursor position");
		}
	}
	if (quark_cursor_position == 0)
		return 0;
	val = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (view),
						   quark_cursor_position));
	return val;
}

static void
html_view_update_focus_element (HtmlView *view, HtmlBox *new_cursor_box, gint offset)
{ 
	HtmlBoxText *text;

	if (new_cursor_box == NULL) {
		gint tmp_offset;

		tmp_offset = offset;
		text = html_view_get_box_text_for_offset (view, &tmp_offset, html_view_get_cursor_end_of_line (view) != 0);
		new_cursor_box = HTML_BOX (text);
	}
	if (DOM_IS_ELEMENT (new_cursor_box->parent->dom_node)) {
		DomElement *element;

		element = DOM_ELEMENT (new_cursor_box->parent->dom_node);
		if (dom_element_is_focusable (element)) {
			if (element != view->document->focus_element) {
               			html_document_update_focus_element (view->document, element);
               			html_view_focus_element (view);
			}
		} else if (view->document->focus_element) {
			html_document_update_focus_element (view->document, NULL);
			html_view_focus_element (view);
		}
	}
}

static void
html_view_notify_cursor_position (HtmlView *view)
{
	HtmlBoxText *box_text;
#ifdef ENABLE_ACCESSIBILITY
	AtkObject *obj;
	gint offset;
#endif /* ENABLE_ACCESSIBILITY */

	box_text = _html_view_get_cursor_box_text (view, NULL);	
	if (box_text) {
		html_view_update_focus_element (view, HTML_BOX (box_text), 0);
#ifdef ENABLE_ACCESSIBILITY
		obj = atk_gobject_accessible_for_object (G_OBJECT (box_text));
		if (!ATK_IS_NO_OP_OBJECT (obj)) {
			/* Accessibility is enabled */
			g_return_if_fail (ATK_IS_TEXT (obj));
			offset = atk_text_get_caret_offset (ATK_TEXT (obj));
			g_signal_emit_by_name (obj, "text-caret-moved", offset);
		}
#endif /* ENABLE_ACCESSIBILITY */
	}
	return;
}
static void
html_view_set_cursor_position (HtmlView *view, gint cursor_position)
{
	if (quark_cursor_position == 0)
		quark_cursor_position = g_quark_from_static_string ("html-view-cursor-position");

	html_view_set_virtual_cursor_x (view, -1);
	html_view_set_virtual_cursor_y (view, -1);
	g_object_set_qdata (G_OBJECT (view), 
			    quark_cursor_position,
			    GINT_TO_POINTER (cursor_position));
        html_view_notify_cursor_position (view);
}

static gint
html_view_get_selection_bound (HtmlView *view)
{
	gint val;
	gint val1;
	HtmlBoxText *text;

	if (view->sel_list) {
		if (view->sel_backwards) {
			if (HTML_IS_BOX_TEXT (view->sel_end))
				text = HTML_BOX_TEXT (view->sel_end);
			else
				text = HTML_BOX_TEXT (g_slist_last (view->sel_list)->data);

			val1 = view->sel_end_index;
		} else {
			if (HTML_IS_BOX_TEXT (view->sel_start))
				text = HTML_BOX_TEXT (view->sel_start);
			else
				text = HTML_BOX_TEXT (view->sel_list->data);

			val1 = view->sel_start_index;
		}
		if (html_view_get_offset_for_box_text (view, text, &val)) {
			gchar *str;

			str = html_box_text_get_text (text, NULL);
			val += g_utf8_pointer_to_offset (str, str + val1);
			return val;
		} else {
			g_warning ("No offset for selection bound");
		}
	}
	if (quark_selection_bound == 0)
		return 0;

	val = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (view),
						   quark_selection_bound));
	return val;
}

static void
html_view_set_selection_bound (HtmlView *view, gint selection_bound)
{
	if (quark_selection_bound == 0)
		quark_selection_bound = g_quark_from_static_string ("html-view-selection-bound");

	g_object_set_qdata (G_OBJECT (view), 
			    quark_selection_bound,
			    GINT_TO_POINTER (selection_bound));
}

static gint
html_view_get_cursor_visible (HtmlView *view)
{
	gint val;

	if (quark_cursor_visible == 0)
		return 0;
	val = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (view),
						   quark_cursor_visible));
	return val;
}

static void
html_view_set_cursor_visible (HtmlView *view, gint cursor_visible)
{
	if (quark_cursor_visible == 0)
		quark_cursor_visible = g_quark_from_static_string ("html-view-cursor-visible");

	g_object_set_qdata (G_OBJECT (view), 
			    quark_cursor_visible,
			    GINT_TO_POINTER (cursor_visible));
}

static guint
html_view_get_blink_timeout (HtmlView *view)
{
	guint val;

	if (quark_blink_timeout == 0)
		return 0;
	val = GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (view),
						    quark_blink_timeout));
	return val;
}

static void
html_view_set_blink_timeout (HtmlView *view, guint blink_timeout)
{
	if (quark_blink_timeout == 0)
		quark_blink_timeout = g_quark_from_static_string ("html-view-blink-timeout");

	g_object_set_qdata (G_OBJECT (view), 
			    quark_blink_timeout,
			    GUINT_TO_POINTER (blink_timeout));
}

static guint
html_view_get_button (HtmlView *view)
{
	guint val;

	if (quark_button == 0)
		return 0;
	val = GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (view),
						    quark_button));
	return val;
}

static void
html_view_set_button (HtmlView *view, guint button)
{
	if (quark_button == 0)
		quark_button = g_quark_from_static_string ("html-view-buttont");

	g_object_set_qdata (G_OBJECT (view), 
			    quark_button,
			    GUINT_TO_POINTER (button));
}

static PangoLayout*
html_view_get_layout (HtmlView *view)
{
	gpointer val;

	val = g_object_get_qdata (G_OBJECT (view),
			          quark_layout);
	return (PangoLayout*)val;
}

static const char*
html_view_get_layout_text (HtmlView *view)
{
	PangoLayout *layout;
	const char *ret;

	layout = html_view_get_layout (view);
	if (layout)
		ret = pango_layout_get_text (layout);
	else
		ret = NULL;
	return ret;
}

static void
html_view_set_layout (HtmlView *view, const gchar *text)
{
	PangoLayout *layout;

	if (quark_layout == 0)
		quark_layout = g_quark_from_static_string ("html-view-layout");

	layout = html_view_get_layout (view);
	if (!layout) {
		layout = gtk_widget_create_pango_layout (GTK_WIDGET (view), NULL);

		g_object_set_qdata (G_OBJECT (view), 
				    quark_layout,
				    layout);
	}
	if (text)
		pango_layout_set_text (layout, text, -1);
}

void
add_text (HtmlBox *box, GString *str)
{
	HtmlBox *child;
	HtmlBoxText *text;
	int text_len;
	gchar *text_chars;

	if (HTML_IS_BOX_TEXT (box)) {
		text = HTML_BOX_TEXT (box);
		text_chars = html_box_text_get_text (text, &text_len);
		if (text_chars) {
			g_string_append_len (str, text_chars, text_len);
		}
	}
	child = box->children;
	while (child) {
		add_text (child, str);
		child = child->next;
	}
	return;
}

static void
html_view_setup_layout (HtmlView *view)
{
	HtmlBox *root;
	GString *str;
	int len;

	if (html_view_get_layout_text (view))
		return;

	str = g_string_new ("");
	root = view->root;

	if (root)
		add_text (root, str);
	len = str->len;
	if (len > 0)
		str->str[len] = '\0';

	html_view_set_layout (view, str->str);
	g_string_free (str, TRUE);
}

static void
html_view_toggle_cursor (HtmlView *view)
{
	cursor_showing = !cursor_showing;
	if (cursor_showing) {
		html_view_notify_cursor_position (view);
	}
	gtk_widget_queue_draw (GTK_WIDGET (view));
}

/*
 * This is a copy of the function _gtk_draw_insertion_cursor
 * from gtkstyle.c
 */
static void
html_view_draw_insertion_cursor (GtkWidget        *widget,
                                 GdkDrawable      *drawable,
                                 GdkGC            *gc,
                                 GdkRectangle     *location,
                                 GtkTextDirection  direction,
                                 gboolean          draw_arrow)
{
	gint stem_width;
	gint arrow_width;
	gint x, y;
	gint i;
	gfloat cursor_aspect_ratio;
	gint offset;

	g_return_if_fail (direction != GTK_TEXT_DIR_NONE);

	gtk_widget_style_get (widget, "cursor-aspect-ratio", &cursor_aspect_ratio, NULL);
	stem_width = location->height * cursor_aspect_ratio + 1;
	arrow_width = stem_width + 1;

	/* put (stem_width % 2) on the proper side of the cursor */
	if (direction == GTK_TEXT_DIR_LTR)
		offset = stem_width / 2;
	else
		offset = stem_width - stem_width / 2;

	/* Reset line style */
	gdk_gc_set_line_attributes (gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
	for (i = 0; i < stem_width; i++)
		gdk_draw_line (drawable, gc,
                               location->x + i - offset,
                               location->y,
                               location->x + i - offset,
                               location->y + location->height - 1);

	if (draw_arrow) {
		if (direction == GTK_TEXT_DIR_RTL) {
			x = location->x - offset - 1;
			y = location->y + location->height - arrow_width * 2 - arrow_width + 1;

			for (i = 0; i < arrow_width; i++) {
				gdk_draw_line (drawable, gc,
					       x, y + i + 1,
					       x, y + 2 * arrow_width - i - 1);
				x --;
			}
		} else if (direction == GTK_TEXT_DIR_LTR) {
			x = location->x + stem_width - offset;
			y = location->y + location->height - arrow_width * 2 - arrow_width + 1;

			for (i = 0; i < arrow_width; i++) {
				gdk_draw_line (drawable, gc,
					       x, y + i + 1,
					       x, y + 2 * arrow_width - i - 1);
				x++;
			}
		}
	}
}


static void
html_view_get_box_text_location (HtmlView *view, HtmlBoxText *text, gint *offset,  GdkRectangle *pos)
{
	gint x, y;
	gchar *str;
	gchar *new_str;
	HtmlBox *box;

	str = html_box_text_get_text (text, NULL);
	new_str = g_utf8_offset_to_pointer (str, *offset);
	html_box_text_get_character_extents (text, (gint) (new_str - str), pos);
	box = HTML_BOX (text);
	x = html_box_get_absolute_x (box);
	y = html_box_get_absolute_y (box);
	pos->x += x - box->x;
	pos->y += y - box->y;
	pos->width = 0;

	return;
}

HtmlBoxText*
_html_view_get_cursor_box_text (HtmlView *view, gint *offset)
{
	HtmlBoxText *text;
	gint cursor_position;
	gint end_of_line;

        cursor_position = html_view_get_cursor_position (view);

	end_of_line = html_view_get_cursor_end_of_line (view);
	text = html_view_get_box_text_for_offset (view, &cursor_position, end_of_line);
	if (offset) {
		*offset = cursor_position;
	}
		
	if (text == NULL) {
		return NULL;
	}

	return text;
}

static HtmlBox*
html_view_get_cursor_location (HtmlView *view, GdkRectangle *pos)
{
	gint cursor_position;
	HtmlBoxText *box_text;

        box_text = _html_view_get_cursor_box_text (view, &cursor_position);
	if (box_text) {
		html_view_get_box_text_location (view, box_text, &cursor_position, pos);
		return HTML_BOX (box_text);
	} else {
		return NULL;
	}
}

static void 
html_view_draw_cursor (HtmlView *view)
{
	GdkRectangle rect;
	HtmlGdkPainter *gdk_painter;
	gint direction;
	GtkTextDirection text_direction;
	HtmlBox *box;

	if (!cursor_showing)
		return;

	box = html_view_get_cursor_location (view, &rect);
	if (box == NULL)
		return;

	direction = html_box_get_bidi_level (box);
	text_direction = (direction == HTML_DIRECTION_RTL) ? GTK_TEXT_DIR_RTL
							   : GTK_TEXT_DIR_LTR;
	gdk_painter = HTML_GDK_PAINTER (view->painter); 
	html_view_draw_insertion_cursor (GTK_WIDGET (view),
					 gdk_painter->window,
					 gdk_painter->gc,
					 &rect,
					 text_direction,
					 FALSE);
}

/* We display the cursor when
 *
 *  - the selection is empty, AND
 *  - the widget has focus
 */

#define CURSOR_ON_MULTIPLIER 0.66
#define CURSOR_OFF_MULTIPLIER 0.34
#define CURSOR_PEND_MULTIPLIER 1.0

static gboolean
cursor_blinks (HtmlView *view)
{
	GtkSettings *settings = gtk_widget_get_settings (GTK_WIDGET (view));
	gboolean blink;

	if (GTK_WIDGET_HAS_FOCUS (view) &&
	    cursor_showing &&
	    html_view_get_selection_bound (view) == html_view_get_cursor_position (view)) {
		g_object_get (settings, "gtk-cursor-blink", &blink, NULL);
		return blink;
	} else
		return FALSE;
}

static gint
get_cursor_time (HtmlView *view)
{
	GtkSettings *settings = gtk_widget_get_settings (GTK_WIDGET (view));
	gint time;

	g_object_get (settings, "gtk-cursor-blink-time", &time, NULL);

	return time;
}

static void
show_cursor (HtmlView *view)
{
	if (!html_view_get_cursor_visible (view)) {
		html_view_set_cursor_visible (view, 1);

		if (GTK_WIDGET_HAS_FOCUS (view) && 
		    (html_view_get_selection_bound (view) == html_view_get_cursor_position (view)))
			gtk_widget_queue_draw (GTK_WIDGET (view));
	}
}

static void
hide_cursor (HtmlView *view)
{
	if (html_view_get_cursor_visible (view)) {
		html_view_set_cursor_visible (view, 0);

		if (GTK_WIDGET_HAS_FOCUS (view) && 
		    html_view_get_selection_bound (view) == html_view_get_cursor_position (view))
			gtk_widget_queue_draw (GTK_WIDGET (view));
	}
}

/*
 * Blink!
 */
static gint
blink_cb (gpointer data)
{
	HtmlView *view;
	guint blink_timeout;

	GDK_THREADS_ENTER ();

	view = HTML_VIEW (data);

	if (!GTK_WIDGET_HAS_FOCUS (view)) {
		g_warning ("HtmlView - did not receive focus-out-event. If you\n"
			   "connect a handler to this signal, it must return\n"
			   "FALSE so the entry gets the event as well");
	}

	g_assert (GTK_WIDGET_HAS_FOCUS (view));
	g_assert (html_view_get_selection_bound (view) == html_view_get_cursor_position (view));

	if (html_view_get_cursor_visible (view)) {
		hide_cursor (view);
		blink_timeout = g_timeout_add (get_cursor_time (view) * CURSOR_OFF_MULTIPLIER,
					       blink_cb,
					       view);
	} else {
		show_cursor (view);
		blink_timeout = g_timeout_add (get_cursor_time (view) * CURSOR_ON_MULTIPLIER,
					       blink_cb,
					       view);
	}
	html_view_set_blink_timeout (view, blink_timeout);

	GDK_THREADS_LEAVE ();

	/* Remove ourselves */
	return FALSE;
}

static void
html_view_check_cursor_blink (HtmlView *view)
{
	guint blink_timeout;

	if (cursor_blinks (view)) {
		if (!html_view_get_blink_timeout (view)) {
			blink_timeout = g_timeout_add (get_cursor_time (view) * CURSOR_ON_MULTIPLIER,
               	                                blink_cb,
               	                                view);
			html_view_set_blink_timeout (view, blink_timeout);
			show_cursor (view);
		}
	} else {
		blink_timeout = html_view_get_blink_timeout (view);
		if (blink_timeout) {
			g_source_remove (blink_timeout);
			html_view_set_blink_timeout (view, 0);
		}
		html_view_set_cursor_visible (view, 0);
	}
}

static void
html_view_pend_cursor_blink (HtmlView *view)
{
	guint blink_timeout;

	if (cursor_blinks (view)) {
		blink_timeout = html_view_get_blink_timeout (view);
		if (blink_timeout)
			g_source_remove (blink_timeout);

		blink_timeout = g_timeout_add (get_cursor_time (view) * CURSOR_PEND_MULTIPLIER,
					       blink_cb,
					       view);
		html_view_set_blink_timeout (view, blink_timeout);
		html_view_set_cursor_visible (view, 0);
		show_cursor (view);
 	}
}

static gboolean
is_box_in_paragraph (HtmlBox *box)
{
	while (box && !HTML_IS_BOX_BLOCK (box)) {
		box = box->parent;
	}
	if (!box) {
		return FALSE;
	}
	if (box->dom_node && strcmp ((char*)box->dom_node->xmlnode->name, "p") == 0) {
		return TRUE;
	}
	return FALSE;
}

static gboolean
is_offset_in_paragraph (HtmlView *view, gint offset)
{
	HtmlBoxText *text;
	HtmlBox *box;
	gint tmp_offset;

	tmp_offset = offset;
	text = html_view_get_box_text_for_offset (view, &tmp_offset, TRUE);
	if (!text)
		return FALSE;
	box = HTML_BOX (text);
	return is_box_in_paragraph (box);
}

static gboolean
is_at_line_boundary (HtmlView *view, gint offset)
{
	HtmlBoxText *next;
	HtmlBoxText *prev;
	HtmlBox *next_box;
	HtmlBox *prev_box;
	gint tmp_offset;

	tmp_offset = offset;
	next = html_view_get_box_text_for_offset (view, &tmp_offset, FALSE);
	if (!next || tmp_offset > 0)
		return FALSE;
	next_box = HTML_BOX (next);
	prev = find_previous_box_text (next_box);
	if (!prev)
		return FALSE;

	prev_box = HTML_BOX (prev);
	if (html_box_get_absolute_y (next_box) == html_box_get_absolute_y (prev_box))
		return FALSE;
	else
		return TRUE;
}

static gint
html_view_move_visually (HtmlView *view, gint start, gint count)
{
	PangoLayout *layout;
	const gchar* text;
	gint index;
	gint offset = 0;
	gboolean forward;
	gint end_of_line;

	html_view_setup_layout (view);
	layout = html_view_get_layout (view);

	text = pango_layout_get_text (layout);
 
	index = g_utf8_offset_to_pointer (text, start) - text;

	forward = count >= 0;
	end_of_line = html_view_get_cursor_end_of_line (view);
	if (forward) {
		if (end_of_line == 1 &&
		    is_at_line_boundary (view, start)) {
			count--;
			offset = start;
		}
	} else {
		if (end_of_line == 0 &&
		    is_at_line_boundary (view, start) &&
		    !is_offset_in_paragraph (view, start)) {
			count++;
			offset = start;
		}
	}

	if (count != 0) {
		while (count != 0) {
			int new_index, new_trailing;
			gboolean strong;

			strong = TRUE;

			if (count > 0) {
				pango_layout_move_cursor_visually (layout, strong, index, 0, 1, &new_index, &new_trailing);
				count--;
			} else {
				pango_layout_move_cursor_visually (layout, strong, index, 0, -1, &new_index, &new_trailing);
				count++;
			}

			if (new_index < 0 || new_index == G_MAXINT)
				break;

			index = new_index;

			while (new_trailing--)
				index = g_utf8_next_char (text + new_index) - text;
		}
		offset = g_utf8_pointer_to_offset (text, text + index);
	}
	if (offset == start) {
		if (forward)
			html_view_set_cursor_end_of_line (view, 0);
		else {
			html_view_set_cursor_end_of_line (view, 1);
		}
	} else {
		if (forward) {
			if (is_at_line_boundary (view, offset) &&
			    is_offset_in_paragraph (view, offset)) {
				html_view_set_cursor_end_of_line (view, 0);
			} else {
				html_view_set_cursor_end_of_line (view, 1);
			}
		} else {
			html_view_set_cursor_end_of_line (view, 0);
		}
	}
 
	return offset;
}

static gint
html_view_move_backward_word (HtmlView *view, gint start)
{
	PangoLayout *layout;
	gint new_pos = start;
	PangoLogAttr *log_attrs;
	gint n_attrs;
	HtmlBoxText *text;
	gint box_offset;

	html_view_setup_layout (view);
	layout = html_view_get_layout (view);

	pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

	new_pos--;
	box_offset = new_pos;
	text = html_view_get_box_text_for_offset (view, &box_offset, FALSE);
	box_offset = new_pos - box_offset;
	/* Find the previous word beginning or start of current box*/
	while (new_pos > 0 && (!log_attrs[new_pos].is_word_start &&
				new_pos	> box_offset))
		new_pos--;

	g_free (log_attrs);
	html_view_set_cursor_end_of_line (view, 0);
	return new_pos;
}

static gint
html_view_move_forward_word (HtmlView *view, gint start)
{
	PangoLayout *layout;
	gint new_pos = start;
	PangoLogAttr *log_attrs;
	gint n_attrs;
	HtmlBoxText *text;
	gint box_offset;
	const char *char_text;
	gint len;
	
	html_view_setup_layout (view);
	layout = html_view_get_layout (view);
	len = g_utf8_strlen (pango_layout_get_text (layout), -1);
	if (new_pos >= len)
		return new_pos;

	pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);

	/* Find the next word end or end of current box*/
	new_pos++;
	box_offset = new_pos;
	text = html_view_get_box_text_for_offset (view, &box_offset, FALSE);
	char_text = html_box_text_get_text (text, &len);
	len = g_utf8_strlen (char_text, len);
	len += new_pos - box_offset;
	while (new_pos < n_attrs && (!log_attrs[new_pos].is_word_end &&
				     new_pos < len))
		new_pos++;

	g_free (log_attrs);
	html_view_set_cursor_end_of_line (view, 1);
	return new_pos;
}

static void
html_view_get_virtual_cursor_pos (HtmlView *view, gint *x, gint *y)
{
	gint virtual_x, virtual_y;
	GdkRectangle pos;

	virtual_x = html_view_get_virtual_cursor_x (view);
	virtual_y = html_view_get_virtual_cursor_y (view);

	if ((virtual_x == -1) ||
	    (virtual_y == -1)) {
        	html_view_get_cursor_location (view, &pos);
	}

	if (x) {   
		if (virtual_x != -1) {
			*x = virtual_x;
		} else {
			*x = pos.x;
		}
	}
			
	if (y) {   
		if (virtual_y != -1) {
			*y = virtual_y;
		} else {
			*y = pos.y + pos.height/2;
		}
	}
}

static void
html_view_set_virtual_cursor_pos (HtmlView *view, gint x, gint y)
{
	GdkRectangle pos;

	if (x == -1 || y == -1) {
		html_view_get_cursor_location (view, &pos);
	}

	html_view_set_virtual_cursor_x (view, (x == -1) ? pos.x : x);
	html_view_set_virtual_cursor_y (view, (y == -1) ? pos.y  + pos.height / 2 : y);
}

static gboolean
is_moving_focus_out (HtmlView *view)
{
	gint val; 

	if (quark_moving_focus_out == 0)
		return FALSE;

	val = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (view),
						   quark_moving_focus_out));
	return (val != 0); 
}

static void
set_moving_focus_out (HtmlView *view, gboolean ret)
{
	gint val;

	if (quark_moving_focus_out == 0)
		quark_moving_focus_out = g_quark_from_static_string ("html-view-moving-focus-out");
	val = ret ? 1 : 0;
	g_object_set_qdata (G_OBJECT (view), 
			    quark_moving_focus_out,
			    GINT_TO_POINTER (val));
}

static gboolean
set_adjustment_clamped (GtkAdjustment *adj, gdouble val)
{
  /* We don't really want to clamp to upper; we want to clamp to
     upper - page_size which is the highest value the scrollbar
     will let us reach. */
  if (val > (adj->upper - adj->page_size))
    val = adj->upper - adj->page_size;

  if (val < adj->lower)
    val = adj->lower;

  if (val != adj->value)
    {
      gtk_adjustment_set_value (adj, val);
      return TRUE;
    }
  else
    return FALSE;
}

static HtmlBoxText*
find_box_text_for_x_pos (HtmlView *view, HtmlBoxText* text, gboolean forward, gint x_pos)
{
	HtmlBoxText *new_text;
	HtmlBox *new_box;
	HtmlBoxText *prev_text;
	HtmlBox *prev_box;
	gint y;
	gint x;
	gint new_x;
	gint new_y;

	prev_box = HTML_BOX (text);
	x = html_box_get_absolute_x (prev_box);
	if (forward) {
		if (x + prev_box->width > x_pos) {
			return text;
		}
	} else {
		if (x <= x_pos) {
			return text;
		}
	}
	y = html_box_get_absolute_y (prev_box);
	prev_text = text;
	while (TRUE) {
		if (forward) {
			new_text = find_next_box_text (prev_box);
			if (!new_text)
				return prev_text;
			new_box = HTML_BOX (new_text);
			new_y = html_box_get_absolute_y (new_box);
			if (new_y > y) {
				return prev_text;
			}
			new_x = html_box_get_absolute_x (new_box);
			if (new_x + new_box->width > x_pos)
				return new_text;
			prev_text = new_text;
			prev_box = new_box;
		} else {
			new_text = find_previous_box_text (prev_box);
			if (!new_text)
				return prev_text;
			new_box = HTML_BOX (new_text);
			new_y = html_box_get_absolute_y (new_box);
			if (new_y < y) {
				return prev_text;
			}
			new_x = html_box_get_absolute_x (new_box);
			if (new_x <= x_pos)
				return new_text;
			prev_text = new_text;
			prev_box = new_box;
		}
	}
}

static gint 
set_offset_for_box_text (HtmlView *view, HtmlBoxText *text, gint x_pos)
{
	gint x;
	HtmlBox *box;
	gint tmp_offset;
	gint offset;
	char *char_text;

	box = HTML_BOX (text);
	x = html_box_get_absolute_x (box);

	if (x + box->width <= x_pos) {
		gint len;

		char_text = html_box_text_get_text (text, &len);
		tmp_offset = g_utf8_strlen (char_text, len);
		if (is_box_in_paragraph (box)) {
			tmp_offset--;
			html_view_set_cursor_end_of_line (view, 0);
		} else {
			html_view_set_cursor_end_of_line (view, 1);
		}
	} else {
		if (x_pos > x)
			x = x_pos - x;
		else 
			x = 0;
		tmp_offset = html_box_text_get_index (text, x);
		char_text = html_box_text_get_text (text, NULL);
		tmp_offset = g_utf8_pointer_to_offset (char_text, char_text + tmp_offset);
		html_view_set_cursor_end_of_line (view, 0);
	}
	html_view_get_offset_for_box_text (view, text, &offset);
	offset += tmp_offset;
	return offset;
} 

static void
move_cursor (HtmlView *view, HtmlBox *new_cursor_box, gint offset, gboolean extend_selection)
{
	gint selection_bound;
	gint cursor_position;

	cursor_position = html_view_get_cursor_position (view);
	selection_bound = html_view_get_selection_bound (view);
	if (extend_selection && cursor_position != offset) {
		HtmlBoxText *start_text;
		HtmlBoxText *end_text;
		HtmlBox *start_box;
		HtmlBox *end_box;
		gchar *start_str;
		gchar *end_str;
		gint start_offset;
		gint end_offset;
		gint extent;
		gboolean forward;

		extent = offset - selection_bound;
		forward = extent >= 0;
		if (forward) {
			start_offset = selection_bound;
			end_offset = offset;
		} else {
			start_offset = offset;
			end_offset = selection_bound;
			extent = -extent;
		}
		start_text = html_view_get_box_text_for_offset (view, &start_offset, FALSE);
		end_text = html_view_get_box_text_for_offset (view, &end_offset, TRUE);
		html_view_set_cursor_position (view, offset);
		start_box = HTML_BOX (start_text);
		end_box = HTML_BOX (end_text);
		html_selection_extend (view, start_box, start_offset, extent);

		view->sel_start = start_box;
		start_str = html_box_text_get_text (start_text, NULL);
		view->sel_start_index = 
			g_utf8_offset_to_pointer (start_str, start_offset) 
				- start_str;
		view->sel_end = end_box;
		end_str = html_box_text_get_text (end_text, NULL);
		view->sel_end_index = 
			g_utf8_offset_to_pointer (end_str, end_offset) 
				- end_str;
		view->sel_backwards = !forward;	
	} else {
		html_view_set_cursor_position (view, offset);
		html_view_set_selection_bound (view, offset);
	}
}

/*
 * Move cursor to x_pos in current or previous line.
 */
static HtmlBox*
html_view_move_cursor_by_line (HtmlView *view, gint count, gint x_pos, gint *offset)
{
	HtmlBoxText *text;
	HtmlBox *box;
	gint tmp_offset;
	gint y, height;
	gint old_y, old_height;
	GtkAdjustment *adj;
	gint end_of_line;

	tmp_offset = *offset;
	end_of_line = html_view_get_cursor_end_of_line (view);
	text = html_view_get_box_text_for_offset (view, &tmp_offset, end_of_line != 0);
	box = HTML_BOX (text);
	old_y = html_box_get_absolute_y (box);
	old_height = box->height;
	adj = GTK_LAYOUT (view)->vadjustment;
	if (count > 0) {
		while (count > 0) {
			text = find_next_box_text (box);
			if (!text) {
				return NULL;
			}
			box = HTML_BOX (text);
			y = html_box_get_absolute_y (box);
			if (old_y + old_height <= y) {
				text = find_box_text_for_x_pos (view, text, TRUE, x_pos);
				break;
			}
		}
		if (!(adj->value + adj->page_size > y))
			set_adjustment_clamped (adj, y - adj->page_size + box->height);
	} else if (count < 0) {
		while (count < 0) {
			text = find_previous_box_text (box);
			if (!text) {
				return NULL;
			}
			box = HTML_BOX (text);
			y = html_box_get_absolute_y (box);
			height = box->height;
			if (y + height <= old_y ) {
				text = find_box_text_for_x_pos (view, text, FALSE, x_pos);
				break;
			}
		}
		if (!(adj->value <= y))
			set_adjustment_clamped (adj, y);
	}
	*offset = set_offset_for_box_text (view, text, x_pos);
	box = HTML_BOX (text);
	return box;
}

static HtmlBox*
get_end_text_offset (HtmlView *view, gint *pos)
{
	HtmlBox *box;
	HtmlBoxText *text;
	HtmlBoxText *temp;
	gint offset;
	gint temp_offset;
	gchar *char_text;
	gint len;

	box = find_last_child (view->root);
	if (box) {
		text = NULL;
		if (HTML_IS_BOX_TEXT (box)) {
			text = HTML_BOX_TEXT (box);
			if (html_box_text_get_len (text) == 0) {
				text = NULL;
			}
		} 
		if (text == NULL) {
			text = find_previous_box_text (box);
		}
		if (text) {
			char_text = html_box_text_get_text (text, &len);
			len = g_utf8_strlen (char_text, len);
			html_view_get_offset_for_box_text (view, text, &offset);
			temp_offset = offset;
			temp = html_view_get_box_text_for_offset (view, &temp_offset, FALSE);
			offset += len;
			*pos = offset;
			return HTML_BOX (text);
		}
	}
	return NULL;
}

static HtmlBox*
html_view_move_cursor_to_end (HtmlView *view, gint *pos)
{
	HtmlBox *box;
	GtkAdjustment *adj;

	box = get_end_text_offset (view, pos);
	adj = GTK_LAYOUT (view)->vadjustment;
	if (adj->value < adj->upper - adj->page_size) {
		set_adjustment_clamped (adj, adj->upper - adj->page_size);
	}
	return box;
}

static HtmlBox*
html_view_move_cursor_to_start (HtmlView *view, gint *pos)
{
	HtmlBoxText *text;
	GtkAdjustment *adj;

	*pos = 0;
	text = html_view_get_box_text_for_offset (view, pos, FALSE);
	adj = GTK_LAYOUT (view)->vadjustment;
	if (adj->value) {
		set_adjustment_clamped (adj, 0);
	}
	return HTML_BOX (text);
}

static void
html_view_scroll_pages (HtmlView *view, gint count, gboolean extend_selection)
{
	HtmlBox *box;
	HtmlBoxText *text;
	GtkAdjustment *adj;
	gint y;
	gdouble new_value;
	gdouble old_value;
	gint cursor_x_pos;
	gint cursor_y_pos;
	gint offset;

	offset = html_view_get_cursor_position (view);

	adj = GTK_LAYOUT (view)->vadjustment;
	if (count > 0 && adj->value >= (adj->upper - adj->page_size - 1e-12)) {
		/* already at bottom, make sure we are at the end */
		box = get_end_text_offset (view, &offset);
		move_cursor (view, NULL, offset, extend_selection);
		return; 
	} else if (count < 0 && adj->value <= (adj->lower + 1e-12)) {
		/* already at top, make sure we are at ofset 0 */
		offset = 0;
		move_cursor (view, NULL, offset, extend_selection);
		return;
	}
	html_view_get_virtual_cursor_pos (view, &cursor_x_pos, &cursor_y_pos);
	new_value = adj->value;
	old_value = adj->value;

	new_value += count * adj->page_increment;
	set_adjustment_clamped (adj, new_value);
	cursor_y_pos += adj->value - old_value;
	
	text = html_view_get_box_text_for_offset (view, &offset, FALSE);
	box = HTML_BOX (text);
	y = html_box_get_absolute_y (box);
	while (count > 0) {
		text = find_next_box_text (box);
		if (!text) {
			return;
		}
		box = HTML_BOX (text);
		y = html_box_get_absolute_y (box);
		if (y >= adj->value) {
			count = 0;
		}
	}
	while (count < 0) {
		HtmlBoxText *previous_text;

		previous_text = text;
		text = find_previous_box_text (box);
		if (!text) {
			count = 0;
			text = previous_text;
			box = HTML_BOX (text);
		}
		box = HTML_BOX (text);
		y = html_box_get_absolute_y (box);
		if (y < adj->value) {
			count = 0;
			text = previous_text;
			box = HTML_BOX (text);
		}
	}
	text = find_box_text_for_x_pos (view, text, TRUE, cursor_x_pos);
	offset = set_offset_for_box_text (view, text, cursor_x_pos);
	move_cursor (view, HTML_BOX (text), offset, extend_selection);
	html_view_set_virtual_cursor_pos (view, cursor_x_pos, cursor_y_pos);
	return;
}

void
html_view_scroll_to_node (HtmlView *view, DomNode *node, HtmlViewScrollToType type)
{
	HtmlBox *box;
	GtkAdjustment *adj = GTK_LAYOUT (view)->vadjustment;
	gint y;
	
	box = html_view_find_layout_box (view, node, FALSE);

	if (box == NULL) {
		return;
	}

	if (HTML_IS_BOX_INLINE (box) && box->children) {
		box = box->children;
	}

	y = html_box_get_absolute_y (box);

	if (!(adj->value < y && adj->value + adj->page_size > y)) {
		switch (type) {
		case HTML_VIEW_SCROLL_TO_TOP:
			set_adjustment_clamped (adj, y);
			break;
		case HTML_VIEW_SCROLL_TO_BOTTOM:
			/* FIXME: Handle box height better here */
			set_adjustment_clamped (adj, y - adj->page_size +  box->height);
			break;
		default:
			break;
		}
	}


#if 0
	g_print ("%d %d %d\n", (gint)adj->value, y, (gint)(adj->value + adj->page_size));
	
	if (adj->value < y || adj->value + adj->page_size > y) 
#endif
}

static void
html_view_set_adjustments (HtmlView *view)
{
	GtkLayout *layout;
	GtkAdjustment *vertical, *horizontal;

	if (view->root) {
		layout = GTK_LAYOUT (view);
		vertical = layout->vadjustment;
		horizontal = layout->hadjustment;
		
		vertical->lower = 0;
		vertical->upper = view->root->height; 
		vertical->page_size = (gfloat)GTK_WIDGET (view)->allocation.height;
		vertical->step_increment = GTK_WIDGET (view)->allocation.height / 10.0;
		vertical->page_increment = GTK_WIDGET (view)->allocation.height * 0.9;

		horizontal->lower = 0.0;
		horizontal->upper = view->root->width;
		horizontal->page_size = GTK_WIDGET (view)->allocation.width;
		horizontal->step_increment = GTK_WIDGET (view)->allocation.width / 10.0;
		horizontal->page_increment = GTK_WIDGET (view)->allocation.width * 0.9;

		gtk_layout_set_size (layout, horizontal->upper, vertical->upper);
		
		gtk_adjustment_changed (vertical);
		gtk_adjustment_changed (horizontal);
	}
}

static void
html_view_relayout (HtmlView *view)
{
	/* Tell the root layout box to relayout itself */
	if (view->painter && view->root) {
		HtmlRelayout *relayout;

		view->root->x = 0;
		view->root->y = 0;
		
		HTML_BOX_ROOT (view->root)->min_width = GTK_WIDGET (view)->allocation.width;
		HTML_BOX_ROOT (view->root)->min_height = GTK_WIDGET (view)->allocation.height;

		relayout = html_relayout_new ();
		relayout->type = HTML_RELAYOUT_INCREMENTAL;
		
		relayout->root = view->root;
		relayout->painter = view->painter;

		relayout->magnification = view->magnification;
		relayout->magnification_modified = view->magnification_modified;
		
		html_box_relayout (view->root, relayout);

		relayout->magnification_modified = FALSE;

		html_relayout_destroy (relayout);

		/* Set adjustments */
		html_view_set_adjustments (view);

		gtk_widget_queue_draw (GTK_WIDGET (view));

		if (view->jump_to_anchor)
			html_view_try_jump (view);
#ifdef ENABLE_ACCESSIBILITY
		{
		AtkObject *child;
		/*
		 * If a new HTML page has been displayed and accessibility 
		 * is enabled we wish to notify that relayout has been called.
		 * The accessibility code will be able to figure out if a new 
		 * root has been added to the HtmlView.
		 *
		 * The following code achieves that purpose. It would be
		 * better to define a new signal on HtmlView but that would
		 * break binary compatibility.
		 */ 
		child  = atk_object_ref_accessible_child (gtk_widget_get_accessible (GTK_WIDGET (view)), 0); 
		if (child)
			g_object_unref (child);
		}
#endif
	}
	if (view->relayout_timeout_id != 0) {
		g_source_remove (view->relayout_timeout_id);
		view->relayout_timeout_id = 0;
	}
	if (view->relayout_idle_id != 0) {
		g_source_remove (view->relayout_idle_id);
		view->relayout_idle_id = 0;
	}

	if (GTK_WIDGET_HAS_FOCUS (view)) {
		if (view->document->focus_element == NULL &&
		    view->document->dom_document) {
			html_document_update_focus_element (view->document, html_focus_iterator_next_element (view->document->dom_document, NULL));
			html_view_focus_element (view);
		}
	}
}

static gint
relayout_timeout_callback (gpointer data)
{
	HtmlView *view = HTML_VIEW (data);

        html_view_relayout (view);
	
	view->relayout_timeout_id = 0;

	if (view->relayout_idle_id != 0) {
		g_source_remove (view->relayout_idle_id);
		view->relayout_idle_id = 0;
	}
        return FALSE;

}

static gint
relayout_idle_callback (gpointer data)
{
        HtmlView *view = HTML_VIEW (data);

        html_view_relayout (view);

	view->relayout_idle_id = 0;

	if (view->relayout_timeout_id != 0) {
		g_source_remove (view->relayout_timeout_id);
		view->relayout_timeout_id = 0;
	}
        return FALSE;
}

static void
html_view_relayout_when_idle (HtmlView *view)
{
	if (view->relayout_idle_id == 0)
		view->relayout_idle_id = g_idle_add (relayout_idle_callback, view);
}

static void
html_view_relayout_after_timeout (HtmlView *view)
{
	if (view->relayout_timeout_id == 0)
		view->relayout_timeout_id = g_timeout_add (RELAYOUT_TIMEOUT_INTERVAL, relayout_timeout_callback, view);
}

static void
html_view_layout_tree_free (HtmlView *view, HtmlBox *root)
{
	HtmlBox *box;

	box = root;

	while (box) {
		HtmlBox *tmp_box;
		
		if (box->children)
			html_view_layout_tree_free (view, box->children);

		tmp_box = box;

		/* Remove the box from the table */
		html_view_remove_layout_box (view, tmp_box->dom_node);

		/* Check if we're trying to remove the root box */
		if (tmp_box == view->root)
			view->root = NULL;

		box = box->next;

		while (HTML_IS_BOX_TEXT (box) && !HTML_BOX_TEXT (box)->master)
			box = box->next;
		
		html_box_remove (tmp_box);
		g_object_unref (tmp_box);
	}
}


static void
html_view_paint (HtmlView *view, GdkRectangle *area)
{
	/* Tell the root layout box to repaint itself */
	if (view->painter && view->root) {
		/* Check that the document has not been deleted */
		if (view->root->dom_node) {
			html_box_paint (view->root, view->painter, area, 0, 0);
			if (GTK_WIDGET_HAS_FOCUS (view) &&
			    (html_view_get_selection_bound (view) == html_view_get_cursor_position (view)) &&
			    html_view_get_cursor_visible (view))
				html_view_draw_cursor (view);
		}
	}
}

static void
html_view_add_layout_box (HtmlView *view, DomNode *node, HtmlBox *box)
{
	g_hash_table_insert (view->node_table, node, box);
}

static gboolean
html_view_remove_layout_box (HtmlView *view, DomNode *node)
{
	return g_hash_table_remove (view->node_table, node);
}

static void
html_view_insert_node (HtmlView *view, DomNode *node)
{
	HtmlBox *parent_box, *new_box;
	DomNode *n;
	
	if ((n = dom_Node__get_parentNode (node)))
		parent_box = html_view_find_layout_box (view, n, TRUE);
	else
		parent_box = NULL;

	/* Don't create a box if any of the parents have display: none */
	while (n) {
		if (n->style && n->style->display == HTML_DISPLAY_NONE)
			return;
		n = dom_Node__get_parentNode (n);
	}

	g_assert (node->style != NULL);

	new_box = html_box_factory_new_box (view, node, FALSE);
	
	if (new_box) {

		new_box->dom_node = node;
		g_object_add_weak_pointer (G_OBJECT (node), (gpointer *) &(new_box->dom_node));

		html_box_handle_html_properties (new_box, node->xmlnode);
		
		if (parent_box == NULL) {
			if (!HTML_IS_BOX_ROOT (new_box))
				parent_box = view->root;
		}
		if (parent_box == NULL) {
			PangoLayout *layout;

			html_view_layout_tree_free (view, view->root);
			if (view->document && view->document->focus_element) {
				g_warning ("Focus element set when inserting toplevel node");
				view->document->focus_element = NULL;
			}
			view->root = new_box;

			layout = html_view_get_layout (view);
			if (layout) {
				g_object_unref (layout);	
				g_object_set_qdata (G_OBJECT (view), 
						    quark_layout,
						    NULL);
			}
			html_view_set_cursor_position (view, 0);
			html_view_set_selection_bound (view, 0);
		 } else {
			html_box_append_child (parent_box, new_box);

			/* Mark the boxes that need to be relayouted */
			html_box_set_unrelayouted_up (new_box);		
		}
		
		html_view_add_layout_box (view, node, new_box);
	}

}

static void
html_view_build_tree (HtmlView *view, DomNode *root)
{
	DomNode *node;
	
	node = root;

	for (node = root; node; node = dom_Node__get_nextSibling (node)) {
		html_view_insert_node (view, node);
		
		if (dom_Node_hasChildNodes (node)) 
			html_view_build_tree (view, dom_Node__get_firstChild (node));
	}
}

static void
html_view_inserted (HtmlDocument *document, DomNode *node, HtmlView *view)
{
	html_view_build_tree (view, node);

	if (document->state == HTML_DOCUMENT_STATE_PARSING) {
		html_view_relayout_after_timeout (view);
	}
	else {
		html_view_relayout_when_idle (view);
	}
}

static void
html_view_removed (HtmlDocument *document, DomNode *node, HtmlView *view)
{
	HtmlBox *box = html_view_find_layout_box (view, node, FALSE);

	/* Mark the boxes that need to be relayouted */
	if (box && box->parent)
		html_box_set_unrelayouted_up (box->parent);		

	if (box) {
		/* Clear the selection first*/
		if (view->sel_list)
			html_selection_clear (view);

		if (box->children)
			html_view_layout_tree_free (view, box->children);

		/* Remove the box from the table */
		html_view_remove_layout_box (view, box->dom_node);

		/* Check if we're trying to remove the root box */
		if (box == view->root)
			view->root = NULL;

		html_box_remove (box);
		g_object_unref (G_OBJECT (box));
	}
	
	html_view_relayout_when_idle (view);
}

static void
html_view_text_updated (HtmlDocument *document, DomNode *node, HtmlView *view)
{
	HtmlBox *box = html_view_find_layout_box (view, node, FALSE);

	if (box) {
		/* FIXME: perhaps use g_object_set here? */
		html_box_text_set_text (HTML_BOX_TEXT (box), (char *)node->xmlnode->content);
		html_box_set_unrelayouted_up (box);
	}
	else {
		g_error ("talk to the box factory here!");
	}

	html_view_relayout_when_idle (view);
}

static void
html_view_relayout_callback (HtmlDocument *document, DomNode *node, HtmlView *view)
{
	HtmlBox *box = html_view_find_layout_box (view, node, FALSE);

	/* Mark the boxes that need to be relayouted */
	if (box)
		html_box_set_unrelayouted_up (box);		

	
	if (document->state == HTML_DOCUMENT_STATE_PARSING) {
		html_view_relayout_after_timeout (view);
	}
	else {
		html_view_relayout_when_idle (view);
	}
}

static void
html_view_box_repaint_traverser (HtmlBox *box, gint *x, gint *y, gint *width, gint *height)
{
	*x = box->x;
	*y = box->y;

	/* Inline box special case */
	if (HTML_IS_BOX_INLINE (box)) {
		HtmlBox *child = box->children;
		gint x1 = G_MAXINT, y1 = G_MAXINT;
		gint x2 = *x, y2 = *y;

		while (child) {
			gint child_x, child_y, child_width, child_height;

			html_view_box_repaint_traverser (child, &child_x, &child_y, &child_width, &child_height);

			x1 = MIN (x1, *x + child_x);
			y1 = MIN (y1, *y + child_y);
			x2 = MAX (x2, *x + child_x + child_width);
			y2 = MAX (y2, *y + child_y + child_height);

			child = child->next;
		}
		*x = x1;
		*y = y1;
		*width = x2 - x1;
		*height = y2 - y1;
	}
	else {
		*width  = box->width;
		*height = box->height;
	}
}

static void
html_view_repaint_callback (HtmlDocument *document, DomNode *node, HtmlView *view)
{
	HtmlBox *box = html_view_find_layout_box (view, node, FALSE);
	gint x, y, width, height;

	g_return_if_fail (box != NULL);

	html_view_box_repaint_traverser (box, &x, &y, &width, &height);

	x += html_box_get_absolute_x (box);
	y += html_box_get_absolute_y (box);

	x -= (gint) (GTK_LAYOUT (view)->hadjustment->value);
	y -= (gint) (GTK_LAYOUT (view)->vadjustment->value);

	/* The + 3 is because we else might not paint underlines of text
	   with descent < 3 */
	gtk_widget_queue_draw_area (GTK_WIDGET (view), x, y, width, height + 3);
}

static void
html_view_style_updated (HtmlDocument *document, DomNode *node, HtmlStyleChange style_change, HtmlView *view)
{
	HtmlBox *box;
	DomNode *child_node;
	HtmlStyle *style;

	if (node == NULL)
		return;

	box = html_view_find_layout_box (view, node, FALSE);
	for (child_node = dom_Node__get_firstChild (node); child_node; child_node = dom_Node__get_nextSibling (child_node)) {
		html_view_style_updated (document, child_node, style_change, view);
	}
	if (!box)
		return;

	style = HTML_BOX_GET_STYLE (box);
	if (DOM_IS_ELEMENT (node) && 
		dom_element_is_focusable (DOM_ELEMENT (node))) {
		gint focus_width;

		gtk_widget_style_get (GTK_WIDGET (view),
				      "focus-line-width", &focus_width,
				      NULL);
		html_style_set_outline_width (style, focus_width);			
	}	

	switch (style_change) {
	case HTML_STYLE_CHANGE_RECREATE: {
		HtmlBox *new_box = NULL;

		/* Don't replace boxes where display: none has been set */
		if (!style || style->display == HTML_DISPLAY_NONE)
			break;

		new_box = html_box_factory_new_box (view, node, TRUE);
		g_assert (new_box);
	
		new_box->dom_node = node;
		g_object_add_weak_pointer (G_OBJECT (node), (gpointer *)&(new_box->dom_node));
		html_box_handle_html_properties (new_box, node->xmlnode);				
		new_box->next = box->next;
		new_box->prev = box->prev;
		new_box->parent = box->parent;
		
		if (box->parent && box->parent->children == box)
				box->parent->children = new_box;
		if (box->next) box->next->prev = new_box;
		if (box->prev) box->prev->next = new_box;

		if (box->children) {
			HtmlBox *child = box->children;
			while (child) {
				HtmlBox *temp = child->next;
				html_box_append_child (new_box, child);
				child = temp;
			}
		}
		
		if (view->root == box)
			view->root = new_box;
		
		html_view_remove_layout_box (view, node);
		g_object_unref (box);
		
		html_view_add_layout_box (view, node, new_box);
		html_view_relayout_callback (document, node, view);
		break;
	}
	case HTML_STYLE_CHANGE_REPAINT:
		html_view_repaint_callback (document, node, view);
		break;
	case HTML_STYLE_CHANGE_RELAYOUT:
		html_view_relayout_callback (document, node, view);
		break;
	case HTML_STYLE_CHANGE_NONE:
		break;
	default:
		g_error ("fix me!");
	}
}

static void
html_view_setup_document (HtmlView *view)
{
	g_signal_connect (G_OBJECT (view->document),
			  "node_inserted",
			  G_CALLBACK (html_view_inserted),
			  view);

	g_signal_connect (G_OBJECT (view->document),
			   "node_removed",
			   (GCallback) html_view_removed,
			   view);

	g_signal_connect (G_OBJECT (view->document),
			  "text_updated",
			  (GCallback) html_view_text_updated,
			  view);
	
	g_signal_connect (G_OBJECT (view->document),
			  "style_updated",
			  G_CALLBACK (html_view_style_updated),
			  view);

	g_signal_connect (G_OBJECT (view->document),
			  "relayout_node",
			  G_CALLBACK (html_view_relayout_callback),
			  view);

	g_signal_connect (G_OBJECT (view->document),
			  "repaint_node",
			  G_CALLBACK (html_view_repaint_callback),
			  view);

	if (view->document->dom_document)
		html_view_build_tree (view, dom_Node_mkref (xmlDocGetRootElement ((xmlDoc *)DOM_NODE (view->document->dom_document)->xmlnode)));
}

static void
html_view_disconnect_document (HtmlView *view, HtmlDocument *document)
{
	g_signal_handlers_disconnect_matched (G_OBJECT (view->document),
					      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
					      0, 0, NULL,
					      (gpointer)html_view_inserted, view);

	g_signal_handlers_disconnect_matched (G_OBJECT (view->document),
					      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
					      0, 0, NULL,
					      (gpointer)html_view_removed, view);

	g_signal_handlers_disconnect_matched (G_OBJECT (view->document),
					      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
					      0, 0, NULL,
					      (gpointer)html_view_text_updated, view);

	g_signal_handlers_disconnect_matched (G_OBJECT (view->document),
					      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
					      0, 0, NULL,
					      (gpointer)html_view_style_updated, view);

	g_signal_handlers_disconnect_matched (G_OBJECT (view->document),
					      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
					      0, 0, NULL,
					      (gpointer)html_view_relayout_callback, 
					      view);

	g_signal_handlers_disconnect_matched (G_OBJECT (view->document),
					      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
					      0, 0, NULL,
					      (gpointer)html_view_repaint_callback, 
					      view);
}

static void
html_view_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	HtmlView *view = HTML_VIEW (widget);

	if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
		( *GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);

	view = HTML_VIEW (widget);

	/* Relayout the view */
	html_view_relayout (view);
}

static void
html_view_realize (GtkWidget *widget)
{
	HtmlView *view;
	gfloat fsize;
	gint isize;

	view = HTML_VIEW (widget);

	/*
	 * GtkLayout uses the bg color for background but we want
	 * to use base color.
	 */	
	gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, &widget->style->base[GTK_STATE_NORMAL]);
	/*
	 * Store the font size so we can adjust size of HtmlFontSpecification
	 * if the size changes.
	 */
	fsize = pango_font_description_get_size (widget->style->font_desc) / 
			(gfloat) PANGO_SCALE;
	isize = (gint) fsize;
	g_object_set_data (G_OBJECT (widget), "html-view-font-size",
			   GINT_TO_POINTER (isize));
	
	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);
	
	gdk_window_set_events (GTK_LAYOUT (view)->bin_window,
			       gdk_window_get_events (GTK_LAYOUT (view)->bin_window) |
			       GDK_POINTER_MOTION_HINT_MASK | GDK_POINTER_MOTION_MASK |
			       GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
			       GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			       GDK_KEY_PRESS_MASK |
			       GDK_EXPOSURE_MASK);
	
	/* Create the HtmlPainter */
	view->painter = html_gdk_painter_new ();

	html_gdk_painter_set_window (HTML_GDK_PAINTER (view->painter), GTK_LAYOUT (view)->bin_window);

	/* If we've got a document assigned, relayout the view */
	if (view->document)
		html_view_relayout_when_idle (view);
}


static gboolean
html_view_enter_notify (GtkWidget *widget, GdkEventCrossing *event)
{
	return TRUE;
}

static gboolean
html_view_leave_notify (GtkWidget *widget, GdkEventCrossing *event)
{
	/* Update the hover node to be NULL */
	html_document_update_hover_node (HTML_VIEW (widget)->document, NULL);
	
	return TRUE;
}

static gboolean
html_view_expose (GtkWidget *widget, GdkEventExpose *event)
{
	HtmlView *view;

	view = HTML_VIEW (widget);

	if (!GTK_WIDGET_DRAWABLE (widget) || (event->window != GTK_LAYOUT (view)->bin_window))
		return FALSE;

	html_view_paint (view, &event->area);
	return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}


static gboolean
html_view_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	gint x, y;
	GdkModifierType mask;
	HtmlView *view;
	
	if (event->window != GTK_LAYOUT (widget)->bin_window)
		return FALSE;

	gdk_window_get_pointer (widget->window, &x, &y, &mask);

	view = HTML_VIEW (widget);

	/* Generate the on_url signal before checking for button press */
	html_event_mouse_move (view, event);

	if (html_view_get_button (view) != 1)
		return FALSE;

	html_selection_update (view, event);
	html_view_check_cursor_blink (view);

	return FALSE;
}

static gboolean
html_view_button_press (GtkWidget *widget, GdkEventButton *event)
{
	HtmlView *view;
	guint button;

	if (event->window != GTK_LAYOUT (widget)->bin_window)
		return FALSE;

	view = HTML_VIEW (widget);

	button = html_view_get_button (view);
	if (button && event->button != button)
		return FALSE;
	html_view_set_button (view, event->button);

	if (!GTK_WIDGET_HAS_FOCUS (widget))
		gtk_widget_grab_focus (widget);
	
	html_event_button_press (view, event);
	
	return FALSE;
}

static gboolean
html_view_button_release (GtkWidget *widget, GdkEventButton *event)
{
	HtmlView *view;

	if (event->window != GTK_LAYOUT (widget)->bin_window)
		return FALSE;

	view = HTML_VIEW (widget);
	if (html_view_get_button (view) != event->button)
		return FALSE;

	html_view_set_button (view, 0);

	html_event_button_release (view, event);
	
	return FALSE;
}

static gboolean
html_view_focus_in (GtkWidget *widget, GdkEventFocus *event)
{
	HtmlView *view;
	DomElement *element;

	view = HTML_VIEW (widget);

	element = html_view_get_and_unset_saved_focus (view);
	if (element) {
		html_document_update_focus_element (view->document, element);
	}
	html_view_check_cursor_blink (view);

	return GTK_WIDGET_CLASS (parent_class)->focus_in_event (widget, event);
}

static gboolean
html_view_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	HtmlView *view;

	view = HTML_VIEW (widget);

	if (view->document->focus_element) {
		html_view_set_saved_focus (view);
		if (GTK_CONTAINER (widget)->focus_child == NULL)
			html_document_update_focus_element (view->document, NULL);
	}
	html_view_check_cursor_blink (view);
	return GTK_WIDGET_CLASS (parent_class)->focus_out_event (widget, event);
}


static gboolean
html_view_key_press (GtkWidget *widget, GdkEventKey *event)
{
	gboolean ret ;
	HtmlView *view = HTML_VIEW (widget);

	ret = GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
	html_view_pend_cursor_blink (view);
	return ret;
}


static void
focus_element_destroyed (gpointer data)
{
	g_object_set_data (G_OBJECT (data), "saved-focus", NULL);
}

/* We must release all references to other objects here. But be careful as
   it can be called multiple times. */
static void
html_view_destroy (GtkObject *object)
{
	HtmlView *view = HTML_VIEW (object);

	gpointer saved_focus;

	if (view->relayout_timeout_id != 0) {
		g_source_remove (view->relayout_timeout_id);
		view->relayout_timeout_id = 0;
	}

	if (view->relayout_idle_id != 0) {
		g_source_remove (view->relayout_idle_id);
		view->relayout_idle_id = 0;
	}

	saved_focus = g_object_get_data (G_OBJECT (view), "saved-focus");
	if (saved_focus) {
		g_object_weak_unref (G_OBJECT (saved_focus),
			   	     (GWeakNotify) focus_element_destroyed,
				     view);
		g_object_set_data (G_OBJECT (view), "saved-focus", NULL);
	}

	if (view->document) {
		html_view_disconnect_document (view, view->document);
		g_object_unref (view->document);
		view->document = NULL;
	}

	if (view->node_table) {
		g_hash_table_destroy (view->node_table);
		view->node_table = NULL;
	}
	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
html_view_finalize (GObject *object)
{
	HtmlView *view = HTML_VIEW (object);
	PangoLayout *layout;

	layout = html_view_get_layout (view);
	if (layout)
		g_object_unref (layout);	

	if (view->jump_to_anchor)
		g_free (view->jump_to_anchor);
		
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static gboolean
html_view_focus (GtkWidget *widget, GtkDirectionType direction)
{
	HtmlView *view = HTML_VIEW (widget);
	DomElement *new_focus_element = NULL;

        if (view->document == NULL || view->document->dom_document == NULL) {
		return FALSE;
	}
        if (is_moving_focus_out (view)) {
                /*
                 * Widget will retain focus if there is nothing else
                 * to get focus
                 */
                set_moving_focus_out (view, FALSE);
                return FALSE;
        }

	if (direction == GTK_DIR_TAB_FORWARD) {
		new_focus_element = html_focus_iterator_next_element (view->document->dom_document, view->document->focus_element);
		/* If end is reached cycle around */
		if (new_focus_element == NULL)
			new_focus_element = html_focus_iterator_next_element (view->document->dom_document, NULL);
	}
	else if (direction == GTK_DIR_TAB_BACKWARD) {
		new_focus_element = html_focus_iterator_prev_element (view->document->dom_document, view->document->focus_element);
		/* If start is reached cycle around */
		if (new_focus_element == NULL)
			new_focus_element = html_focus_iterator_prev_element (view->document->dom_document, NULL);
	}

	/* Handle special case when we haven't any focusable elements */
	if (new_focus_element == NULL) {
		if (!GTK_WIDGET_HAS_FOCUS (widget)) {
			gtk_widget_grab_focus (widget);
			return TRUE;
		} 
	} else { 
		html_document_update_focus_element (view->document, new_focus_element);

		html_view_focus_element (view);
		return TRUE;
	}

	return FALSE;
}

static void
html_view_set_focus_child (GtkContainer *container, GtkWidget *child)
{
	HtmlView *view;

	view = HTML_VIEW (container);
	
	if (child) {
		HtmlBox *box = g_object_get_data (G_OBJECT (child), "box");

		if (view->document->focus_element != DOM_ELEMENT (box->dom_node)) {
			html_document_update_focus_element (view->document,
							    DOM_ELEMENT (box->dom_node));
			
		}
	}
	
	GTK_CONTAINER_CLASS (parent_class)->set_focus_child (container, child);
}

/* Compute the X position for an offset that corresponds to the "more important
 * cursor position for that offset. We use this when trying to guess to which
 * end of the selection we should go to when the user hits the left or
 * right arrow key.
 */
static gint
get_better_cursor_x (HtmlView *view,
                     gint      offset)
{
  	GdkKeymap *keymap;
  	GtkTextDirection keymap_direction;
  	GtkTextDirection widget_direction;
  	gboolean split_cursor;
	GtkWidget *widget;
  	PangoLayout *layout;
  	const gchar *text;
  	gint index;
  	PangoRectangle strong_pos, weak_pos;

	widget = GTK_WIDGET (view);

  	keymap = gdk_keymap_get_for_display (gtk_widget_get_display (widget));
  	keymap_direction =
		(gdk_keymap_get_direction (keymap) == PANGO_DIRECTION_LTR) ?
		GTK_TEXT_DIR_LTR : GTK_TEXT_DIR_RTL;
  	widget_direction = gtk_widget_get_direction (widget);

	html_view_setup_layout (view);
	layout = html_view_get_layout (view);

	text = pango_layout_get_text (layout);
  	index = g_utf8_offset_to_pointer (text, offset) - text;
	g_object_get (gtk_widget_get_settings (widget),
			"gtk-split-cursor", &split_cursor,
			NULL);

	pango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);
	if (split_cursor)
		return strong_pos.x / PANGO_SCALE;
	else
	return (keymap_direction == widget_direction) ? 
		strong_pos.x / PANGO_SCALE : weak_pos.x / PANGO_SCALE;
}

static void
html_view_real_move_cursor (HtmlView *html_view, GtkMovementStep step, gint count, gboolean extend_selection)
{
	GtkAdjustment *vertical, *horizontal;
	HtmlBox *new_cursor_box;
	gint new_offset;
	gint selection_bound;
	gint cursor_position;
	gint cursor_x_pos = 0;

	vertical = GTK_LAYOUT (html_view)->vadjustment;
	horizontal = GTK_LAYOUT (html_view)->hadjustment;
	
	if (!cursor_showing) {
		switch (step) {
		case GTK_MOVEMENT_VISUAL_POSITIONS:
			set_adjustment_clamped (horizontal, horizontal->value + horizontal->step_increment * count);
			break;
        	case GTK_MOVEMENT_WORDS:
			break;
		case GTK_MOVEMENT_DISPLAY_LINES:
			set_adjustment_clamped (vertical, vertical->value + vertical->step_increment * count);
			break;
		case GTK_MOVEMENT_PAGES:
			set_adjustment_clamped (vertical, vertical->value + vertical->page_increment * count);
			break;
		case GTK_MOVEMENT_BUFFER_ENDS:
			if (count == -1) {
				set_adjustment_clamped (vertical, vertical->lower);
			} else {
				set_adjustment_clamped (vertical, vertical->upper);
			}
			break;
		default:
			g_warning ("unknown step!\n");
		}
		return;
	}

	if (step == GTK_MOVEMENT_PAGES) {
		html_view_scroll_pages (html_view, count, extend_selection);
		html_view_check_cursor_blink (html_view);
		html_view_pend_cursor_blink (html_view);
		return;
	}

	cursor_position = html_view_get_cursor_position (html_view);
	selection_bound = html_view_get_selection_bound (html_view);
	new_offset = cursor_position;
	new_cursor_box = NULL;
	if (cursor_position != selection_bound && !extend_selection) {
	/*
	 * If we have a current selection and are not extending it move to the
	 * start or end of the selection as appropriate.
	 */
		switch (step) {
		case GTK_MOVEMENT_VISUAL_POSITIONS:
			{
			gint current_x = get_better_cursor_x (html_view, cursor_position);
			gint bound_x = get_better_cursor_x (html_view, selection_bound);

			if (count < 0)
				new_offset = current_x < bound_x ? cursor_position : selection_bound;
			else
				new_offset = current_x > bound_x ? cursor_position : selection_bound;

			break;
          		}
        	case GTK_MOVEMENT_WORDS:
			if (count < 0)
				new_offset = MIN (cursor_position, selection_bound);
			else
				new_offset = MAX (cursor_position, selection_bound);
			break;
		default:
			break;
          	}
		html_selection_clear (html_view);
	} else {
		switch (step) {
		case GTK_MOVEMENT_VISUAL_POSITIONS:
			new_offset = html_view_move_visually (html_view, new_offset, count);
			break;
        	case GTK_MOVEMENT_WORDS:
			while (count > 0) {
				new_offset = html_view_move_forward_word (html_view, new_offset);
				count--;
			}
			while (count < 0) {
				new_offset = html_view_move_backward_word (html_view, new_offset);
				count++;
			}
			break;
		case GTK_MOVEMENT_DISPLAY_LINES:
			html_view_get_virtual_cursor_pos (html_view, &cursor_x_pos, NULL);
			new_cursor_box = html_view_move_cursor_by_line (html_view, count, cursor_x_pos, &new_offset);	
			break;
		case GTK_MOVEMENT_BUFFER_ENDS:
			if (count < 0) {
				new_cursor_box = html_view_move_cursor_to_start (html_view, &new_offset);
			} else if (count > 0) {
				new_cursor_box = html_view_move_cursor_to_end (html_view, &new_offset);
			}
			break;
		default:
			g_warning ("unknown step!\n");
		}
	}

	move_cursor (html_view, new_cursor_box, new_offset, extend_selection);
	if (step == GTK_MOVEMENT_DISPLAY_LINES) {
		html_view_set_virtual_cursor_pos (html_view, cursor_x_pos, -1);
	}
	html_view_check_cursor_blink (html_view);
	html_view_pend_cursor_blink (html_view);
}

static void
html_view_real_activate (HtmlView *view)
{
	html_event_activate (view);
}

static void html_view_real_move_focus_out (HtmlView *view, GtkDirectionType direction_type)
{
	GtkWidget *widget = GTK_WIDGET (view);
	GtkWidget *toplevel;

	html_document_update_focus_element (view->document, NULL);

	set_moving_focus_out (view, TRUE);
	toplevel = gtk_widget_get_toplevel (widget);
	g_return_if_fail (toplevel);

	gtk_widget_child_focus (toplevel, direction_type);
	set_moving_focus_out (view, FALSE);
}

static void
html_view_add_move_binding (GtkBindingSet *binding_set, guint keyval, guint modmask, GtkMovementStep step, gint count)
{
	gtk_binding_entry_add_signal (binding_set, keyval, modmask,
				      "move_cursor", 3,
				      G_TYPE_ENUM, step,
				      G_TYPE_INT, count,
				      G_TYPE_BOOLEAN, FALSE);

	/* Selection-extending version */
	gtk_binding_entry_add_signal (binding_set, keyval, modmask | GDK_SHIFT_MASK,
				      "move_cursor", 3,
				      G_TYPE_ENUM, step,
				      G_TYPE_INT, count,
				      G_TYPE_BOOLEAN, TRUE);
}

static void
html_view_add_tab_binding (GtkBindingSet *binding_set, GdkModifierType modifiers, GtkDirectionType direction)
{
	gtk_binding_entry_add_signal (binding_set, GDK_Tab, modifiers,
				      "move_focus_out", 1,
				      GTK_TYPE_DIRECTION_TYPE, direction);
	gtk_binding_entry_add_signal (binding_set, GDK_KP_Tab, modifiers,
				      "move_focus_out", 1,
				      GTK_TYPE_DIRECTION_TYPE, direction);
}

void
html_view_set_document (HtmlView *view, HtmlDocument *document)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (HTML_IS_VIEW (view));

	if (view->document == document)
		return;

	if (document != NULL)
		g_object_ref (document);
	
	if (view->document != NULL) {
		html_view_disconnect_document (view, view->document);
		g_object_unref (view->document);
		html_view_layout_tree_free (view, view->root);
	}
	
	view->document = document;

	if (view->document != NULL) {
		html_view_setup_document (view);
	}

	gtk_widget_queue_resize (GTK_WIDGET (view));
}

HtmlBox *
html_view_find_layout_box (HtmlView *view, DomNode *node, gboolean find_parent)
{
	if (!find_parent)
		return g_hash_table_lookup (view->node_table, node);

	while (node) {
		HtmlBox *box = g_hash_table_lookup (view->node_table, node);

		if (box)
			return box;

		node = dom_Node__get_parentNode (node);
	}

	return NULL;
}

static void
html_view_try_jump (HtmlView *view)
{
	DomNode *node = html_document_find_anchor (view->document, view->jump_to_anchor);
	
	if (node != NULL) {
		
		html_view_scroll_to_node (view, node, HTML_VIEW_SCROLL_TO_TOP);
		g_free (view->jump_to_anchor);
		view->jump_to_anchor = NULL;
	}
}

/**
 * html_view_jump_to_anchor:
 * @view: 
 * @anchor: The name of the anchor to jump to
 * 
 * Scroll the @view so that the anchor with the
 * name @anchor is visible
 **/
void
html_view_jump_to_anchor (HtmlView *view, const gchar *anchor)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (HTML_IS_VIEW (view));
	g_return_if_fail (anchor != NULL);

	if (view->jump_to_anchor)
		g_free (view->jump_to_anchor);

	view->jump_to_anchor = g_strdup (anchor);
	html_view_try_jump (view);
}

gdouble
html_view_get_magnification (HtmlView *view)
{
	return view->magnification;
}

void
html_view_set_magnification (HtmlView *view, gdouble magnification)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (HTML_IS_VIEW (view));

	if (magnification < 0.05 || magnification > 20)
		return;

	if (magnification != view->magnification) {

		view->magnification = magnification;
		view->magnification_modified = TRUE;

		if (view->root != NULL) {
			html_box_set_unrelayouted_down (view->root);
			html_view_relayout (view);
		}
	}
}

#define MAG_STEP 1.1

void
html_view_zoom_in (HtmlView *view)
{
	html_view_set_magnification (view, view->magnification * MAG_STEP);
}

void
html_view_zoom_out (HtmlView *view)
{
	html_view_set_magnification (view, view->magnification * (1.0 / MAG_STEP));
}

void
html_view_zoom_reset (HtmlView *view)
{
	html_view_set_magnification (view, 1.0);
}

static void
html_view_class_init (HtmlViewClass *klass)
{
	GObjectClass *object_class = (GObjectClass *)klass;
	GtkObjectClass *gtkobject_class = (GtkObjectClass *)klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
	GtkContainerClass *container_class = (GtkContainerClass *)klass;
	GtkBindingSet *binding_set;

	binding_set = gtk_binding_set_by_class (klass);
	parent_class = g_type_class_peek (GTK_TYPE_LAYOUT);

	object_class->finalize = html_view_finalize;

	gtkobject_class->destroy = html_view_destroy;

	widget_class->focus = html_view_focus;
	
	widget_class->size_allocate = html_view_size_allocate;
	widget_class->realize = html_view_realize;
	widget_class->expose_event = html_view_expose;
	widget_class->motion_notify_event = html_view_motion_notify;
	widget_class->button_press_event = html_view_button_press;
	widget_class->button_release_event = html_view_button_release;
	widget_class->key_press_event = html_view_key_press;

	widget_class->focus_in_event = html_view_focus_in;
	widget_class->focus_out_event = html_view_focus_out;
	
	widget_class->enter_notify_event = html_view_enter_notify;
	widget_class->leave_notify_event = html_view_leave_notify;

#ifdef ENABLE_ACCESSIBILITY
	widget_class->get_accessible = html_view_get_accessible;
#endif

	container_class->set_focus_child = html_view_set_focus_child;
	
	klass->move_cursor = html_view_real_move_cursor;
	klass->activate = html_view_real_activate;
	klass->move_focus_out = html_view_real_move_focus_out;
	
	view_signals [MOVE_CURSOR] =
		g_signal_new ("move_cursor",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (HtmlViewClass, move_cursor),
			      NULL, NULL,
			      html_marshal_VOID__ENUM_INT_BOOLEAN,
			      G_TYPE_NONE, 3,
			      GTK_TYPE_MOVEMENT_STEP,
			      G_TYPE_INT,
			      G_TYPE_BOOLEAN);
	
	view_signals [REQUEST_OBJECT] =
		g_signal_new ("request_object",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (HtmlViewClass, request_object),
			      NULL, NULL,
			      html_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      G_TYPE_OBJECT);
	
	view_signals [ON_URL] =
		g_signal_new ("on_url",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HtmlViewClass, on_url),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_STRING);

	view_signals [ACTIVATE] =
		g_signal_new ("activate",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (HtmlViewClass, activate),
			      NULL, NULL,
			      html_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	view_signals [MOVE_FOCUS_OUT] =
		g_signal_new ("move_focus_out",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (HtmlViewClass, move_focus_out),
			      NULL, NULL,
			      html_marshal_VOID__ENUM,
			      G_TYPE_NONE, 1,
			      GTK_TYPE_DIRECTION_TYPE);

	view_signals [TOGGLE_CURSOR] =
		g_signal_new ("toggle_cursor",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      0, /* No default signal handler */
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	widget_class->activate_signal = view_signals[ACTIVATE];

	html_view_add_move_binding (binding_set, GDK_Page_Down, 0,
				    GTK_MOVEMENT_PAGES, 1);
	html_view_add_move_binding (binding_set, GDK_Page_Up, 0,
				    GTK_MOVEMENT_PAGES, -1);
	html_view_add_move_binding (binding_set, GDK_Home, 0,
				    GTK_MOVEMENT_BUFFER_ENDS, -1);
	html_view_add_move_binding (binding_set, GDK_End, 0,
				    GTK_MOVEMENT_BUFFER_ENDS, 1);

	html_view_add_move_binding (binding_set, GDK_Down, 0,
				    GTK_MOVEMENT_DISPLAY_LINES, 1);
	html_view_add_move_binding (binding_set, GDK_KP_Down, 0,
				    GTK_MOVEMENT_DISPLAY_LINES, 1);
	html_view_add_move_binding (binding_set, GDK_Up, 0,
				    GTK_MOVEMENT_DISPLAY_LINES, -1);
	html_view_add_move_binding (binding_set, GDK_KP_Up, 0,
				    GTK_MOVEMENT_DISPLAY_LINES, -1);

	html_view_add_move_binding (binding_set, GDK_Right, 0,
				    GTK_MOVEMENT_VISUAL_POSITIONS, 1);
	html_view_add_move_binding (binding_set, GDK_KP_Right, 0,
				    GTK_MOVEMENT_VISUAL_POSITIONS, 1);
	html_view_add_move_binding (binding_set, GDK_Left, 0,
				    GTK_MOVEMENT_VISUAL_POSITIONS, -1);
	html_view_add_move_binding (binding_set, GDK_KP_Left, 0,
				    GTK_MOVEMENT_VISUAL_POSITIONS, -1);
	html_view_add_move_binding (binding_set, GDK_Right, GDK_CONTROL_MASK,
				    GTK_MOVEMENT_WORDS, 1);
	html_view_add_move_binding (binding_set, GDK_KP_Right, GDK_CONTROL_MASK,
				    GTK_MOVEMENT_WORDS, 1);
	html_view_add_move_binding (binding_set, GDK_Left, GDK_CONTROL_MASK,
				    GTK_MOVEMENT_WORDS, -1);
	html_view_add_move_binding (binding_set, GDK_KP_Left, GDK_CONTROL_MASK,
				    GTK_MOVEMENT_WORDS, -1);
	
	html_view_add_tab_binding (binding_set, GDK_CONTROL_MASK, GTK_DIR_TAB_FORWARD);
	html_view_add_tab_binding (binding_set, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_DIR_TAB_BACKWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_F7, 0,
				      "toggle_cursor", 0);
}

static void
html_view_abstract_view_init (DomAbstractViewIface *iface)
{
	
}

static void
html_view_init (HtmlView *view)
{
	GTK_WIDGET_SET_FLAGS (view, GTK_CAN_FOCUS);
	  
	view->node_table = g_hash_table_new (g_direct_hash, g_direct_equal);
	view->document = NULL;
	view->root = NULL;
	view->relayout_idle_id = 0;
	view->relayout_timeout_id = 0;
	view->magnification = 1.0;
	g_signal_connect (view, "toggle-cursor", G_CALLBACK (html_view_toggle_cursor), NULL);
	html_view_set_layout (view, NULL);
}

GType
html_view_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info= {
			sizeof (HtmlViewClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) html_view_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (HtmlView),
			16,   /* n_preallocs */
			(GInstanceInitFunc) html_view_init,
		};

		static const GInterfaceInfo dom_abstract_view_info = {
			(GInterfaceInitFunc) html_view_abstract_view_init,
			NULL,
			NULL
		};

		type = g_type_register_static (GTK_TYPE_LAYOUT, "HtmlView", &info, 0);
		g_type_add_interface_static (type,
					     DOM_TYPE_ABSTRACT_VIEW,
					     &dom_abstract_view_info);
	}

	return type;
}

static void 
html_view_update_box_style_size (HtmlBox *root, gfloat adjust, gint focus_width, GPtrArray *done)
{
	HtmlBox *box;

	for (box = root; box; box = box->next) {
		HtmlStyle *style;

		style = HTML_BOX_GET_STYLE (box);
		if (style) {
			HtmlFontSpecification *font_spec;

			font_spec = style->inherited->font_spec;
			if (font_spec) {
				gint i;
				gboolean found = FALSE;

				for (i = 0; i < done->len; i++) {
					if (g_ptr_array_index (done, i) == font_spec) {
						found = TRUE;
						break;
					}
				}
				if (!found) {
					g_ptr_array_add (done, font_spec);
					font_spec->size = font_spec->size * adjust;
				}
			}
			if (DOM_IS_ELEMENT (root->dom_node) && 
			    dom_element_is_focusable (DOM_ELEMENT (root->dom_node))) {
				html_style_set_outline_width (style, focus_width);			
			}	
		}
		html_box_set_unrelayouted_up (box);		
		if (box->children) {
			html_view_update_box_style_size (box->children, adjust, focus_width, done);
		}
	}
}

static void
html_view_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
	if (previous_style) {
		gfloat fsize;
		gint new_isize;
		gint old_isize;
		gint focus_width;

		gtk_widget_style_get (widget,
				      "focus-line-width", &focus_width,
				      NULL);

 		if (!gdk_color_equal (&widget->style->bg[GTK_STATE_NORMAL], &widget->style->base[GTK_STATE_NORMAL])) {
			gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, &widget->style->base[GTK_STATE_NORMAL]);
			return;
 		}
		fsize = pango_font_description_get_size (widget->style->font_desc) / (gfloat) PANGO_SCALE;
		new_isize = (gint) fsize;
		old_isize = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "html-view-font-size"));
		if (old_isize && old_isize != new_isize) {
			/*
			 * The font size for the HtmlView has changed so we
			 * update the size in each HtmlFontSpecification.
			 * We traverse the document but we only want to
			 * update the HtmlFontSpecification once. We maintain
			 * an array of HtmlFontSpecification which have already
			 * been updated. As a HtmlFontSpecification may be used
			 * more than than one HtmlView we use a static array
			 * which is cleared when we determine that a different
			 * size change has occurred.
			 */
			HtmlView *view;

			view = HTML_VIEW (widget);
			g_object_set_data (G_OBJECT (widget), "html-view-font-size",
					   GINT_TO_POINTER (new_isize));
			if (view->root) {
				static gint old_size = 0;
				static gint new_size = 0;
				static GPtrArray *done;

				if (old_size != old_isize ||
				    new_size != new_isize) {
					if (old_size || new_size) {
						g_ptr_array_free (done, TRUE);
					}
					done = g_ptr_array_new ();
					old_size = old_isize;
					new_size = new_isize;
				}
				html_view_update_box_style_size (view->root, (gfloat) new_isize / (gfloat) old_isize, focus_width, done);
			}
		}
	}
}

GtkWidget *
html_view_new (void)
{
	HtmlView *view = HTML_VIEW (g_object_new (HTML_TYPE_VIEW, NULL));	

	/* This creates the adjustments needed */
 	gtk_layout_set_hadjustment (GTK_LAYOUT (view), NULL);
 	gtk_layout_set_vadjustment (GTK_LAYOUT (view), NULL);

	g_signal_connect (G_OBJECT (view),
			  "style-set",
			  G_CALLBACK (html_view_style_set),
			  NULL);

	return GTK_WIDGET (view);
}

static void
html_view_set_saved_focus (HtmlView *view)
{
	gpointer saved_focus;

	saved_focus = g_object_get_data (G_OBJECT (view), "saved-focus");
	if (saved_focus) {
		g_object_weak_unref (G_OBJECT (saved_focus),
				     (GWeakNotify) focus_element_destroyed,
				     view);
	}
	g_object_weak_ref (G_OBJECT (view->document->focus_element),
			   (GWeakNotify) focus_element_destroyed,
			   view);
	g_object_set_data (G_OBJECT (view), "saved-focus", view->document->focus_element);
}

static DomElement* 
html_view_get_and_unset_saved_focus (HtmlView *view)
{
	gpointer saved_focus;
	
	saved_focus = g_object_get_data (G_OBJECT (view), "saved-focus");
	if (saved_focus) {
		g_object_weak_unref (G_OBJECT (saved_focus),
				     (GWeakNotify) focus_element_destroyed,
				     view);
		g_object_set_data (G_OBJECT (view), "saved-focus", NULL);
		return DOM_ELEMENT (saved_focus);
	} else
		return NULL;
}

static void
html_view_focus_element (HtmlView *view)
{
	HtmlBox *box;
	gint offset;

	if (view->document->focus_element) {
		html_view_scroll_to_node (view, DOM_NODE (view->document->focus_element), HTML_VIEW_SCROLL_TO_BOTTOM);

		box = html_view_find_layout_box (view, DOM_NODE (view->document->focus_element), FALSE);
		if (box && HTML_IS_BOX_EMBEDDED (box)) { 
			gtk_widget_child_focus (HTML_BOX_EMBEDDED (box)->widget, GTK_DIR_TAB_FORWARD);
		}
		else {
			gtk_widget_grab_focus (GTK_WIDGET (view));
			if (cursor_showing && 
			    HTML_IS_BOX_TEXT (box->children)) {
				HtmlBoxText *text;

				text = HTML_BOX_TEXT (box->children);
				if (html_view_get_offset_for_box_text (view, text, &offset)) {
					move_cursor (view, HTML_BOX (text), offset, FALSE);
					html_view_pend_cursor_blink (view);
					html_view_check_cursor_blink (view);
				}
			}
		}
	} else {
		/* No element focused to scroll to top */
		GtkAdjustment *adj = GTK_LAYOUT (view)->vadjustment;

		if (!cursor_showing) {
			set_adjustment_clamped (adj, 0);
		}
		gtk_widget_grab_focus (GTK_WIDGET (view));
	}
	gtk_widget_queue_draw (GTK_WIDGET (view));
}

#ifdef ENABLE_ACCESSIBILITY

static AtkObject *
html_view_get_accessible (GtkWidget *widget)
{
        static gboolean first_time = TRUE;

	if (first_time) {
		AtkObjectFactory *factory;
		AtkRegistry *registry;
		GType derived_type;
		GType derived_atk_type;

		/*
		 * Figure out whether accessibility is enabled by looking at the
		 * type of the accessible object which would be created for 
		 * the parent type of HtmlView.
		 */
		derived_type = g_type_parent (HTML_TYPE_VIEW);

		registry = atk_get_default_registry ();
		factory = atk_registry_get_factory (registry,
						    derived_type);
		derived_atk_type = atk_object_factory_get_accessible_type (factory);
		if (g_type_is_a (derived_atk_type, GTK_TYPE_ACCESSIBLE)) {
			/*
			 * Specify what factory to use to create accessible
			 * objects
			 */
			HTML_ACCESSIBLE_SET_FACTORY (HTML_TYPE_VIEW, html_view_accessible);
			HTML_ACCESSIBLE_SET_FACTORY (HTML_TYPE_BOX_BLOCK, html_box_block_accessible);
			HTML_ACCESSIBLE_SET_FACTORY (HTML_TYPE_BOX_EMBEDDED, html_box_embedded_accessible);
			HTML_ACCESSIBLE_SET_FACTORY (HTML_TYPE_BOX, html_box_accessible);
			HTML_ACCESSIBLE_SET_FACTORY (HTML_TYPE_BOX_TABLE, html_box_table_accessible);
		}
		first_time = FALSE;
	}
	return GTK_WIDGET_CLASS (parent_class)->get_accessible (widget);
}

#endif /* ENABLE_ACCESSIBILITY */
