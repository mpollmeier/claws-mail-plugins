/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
   Copyright (C) 2001 CodeFactory AB
   Copyright (C) 2001 Jonas Borgström <jonas@codefactory.se>
   Copyright (C) 2001 Anders Carlsson <andersca@codefactory.se>
   
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

#include "view/htmlevent.h"
#include "view/htmlselection.h"
#include "layout/htmlboxtext.h"
#include "dom/core/dom-text.h"

gchar *
html_selection_get_text (HtmlView *view)
{
	GSList *list = view->sel_list;
	GString *str = g_string_new ("");
	gchar *ptr;

	if (view->sel_list == NULL)
		return NULL;
	
	while (list) {
		HtmlBoxText *text = HTML_BOX_TEXT (list->data);

		list = list->next;
		/*
		 * Some boxes may not have any text
		 */
		if (text->canon_text == NULL)
			continue;
		ptr = (gchar *)text->canon_text;
		switch (text->selection) {
		case HTML_BOX_TEXT_SELECTION_NONE:
			return NULL;
		case HTML_BOX_TEXT_SELECTION_END:
			g_string_append_len (str, ptr, text->sel_end_index);
			break;
		case HTML_BOX_TEXT_SELECTION_START:
			g_string_append_len (str, ptr + text->sel_start_index,
					     text->length - text->sel_start_index);
			break;
		case HTML_BOX_TEXT_SELECTION_FULL:
			g_string_append_len (str, ptr, text->length);
			break;
		case HTML_BOX_TEXT_SELECTION_BOTH:
			g_string_append_len (str, ptr + MIN (text->sel_start_index, text->sel_end_index),
					 MAX (text->sel_end_index, text->sel_start_index) - MIN (text->sel_start_index, text->sel_end_index));
			break;
		}
	}
	ptr = str->str;
	g_string_free (str, FALSE);
	return ptr;
}

static void
primary_get_cb (GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, gpointer data)
{
	gchar *str = html_selection_get_text (HTML_VIEW (data));
	if (str) {
		gtk_selection_data_set_text (selection_data, str, -1);
		g_free (str);
	}
}

static void
primary_clear_cb (GtkClipboard *clipboard, gpointer data)
{
	html_selection_clear (HTML_VIEW (data));
}

static void
html_selection_update_primary_selection (HtmlView *view)
{
	static const GtkTargetEntry targets[] = {
		{ "UTF8_STRING", 0, 0 },
		{ "STRING", 0, 0 },
		{ "TEXT",   0, 0 }, 
		{ "COMPOUND_TEXT", 0, 0 }
	};
  
	GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
	
	if (!gtk_clipboard_set_with_owner (clipboard, targets, G_N_ELEMENTS (targets),
					   primary_get_cb, primary_clear_cb, G_OBJECT (view)))
		primary_clear_cb (clipboard, view);

}

static void
select_text (gpointer data, gpointer user_data)
{
	HtmlBoxText *text = HTML_BOX_TEXT (data);
	/* Use this to clear the selection if the box is destroyed */
	g_object_set_data_full (G_OBJECT (text), "gtkhtml2selection", user_data, (GDestroyNotify)html_selection_clear);
	html_box_text_set_selection (text, HTML_BOX_TEXT_SELECTION_FULL, -1, -1);
}

static void
unselect_text (gpointer data, gpointer user_data)
{
	HtmlBoxText *text = HTML_BOX_TEXT (data);
	/* Remove the "clear selection on finalize" hook */
	g_object_steal_data (G_OBJECT (text), "gtkhtml2selection");
	html_box_text_set_selection (text, HTML_BOX_TEXT_SELECTION_NONE, -1, -1);
}

static void
repaint_sel (gpointer data, gpointer user_data)
{
	HtmlView *view = HTML_VIEW (user_data);
	HtmlBox  *box  = HTML_BOX (data);
	gint x, y;

	if (!box->dom_node)
	/*
	 * We may be destroying the node. In any case the underlying node
	 * has been deleted.
	 */
		return;
	
	x = html_box_get_absolute_x (box);
	y = html_box_get_absolute_y (box);

	x -= (gint) (GTK_LAYOUT (view)->hadjustment->value);
	y -= (gint) (GTK_LAYOUT (view)->vadjustment->value);

	gtk_widget_queue_draw_area (GTK_WIDGET (view), x, y, box->width, box->height);
}

void
html_selection_clear (HtmlView *view)
{
	if (view->sel_list != NULL) {

		GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
		
		if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (view))
			gtk_clipboard_clear (clipboard);

		g_slist_foreach (view->sel_list, unselect_text, view);
		g_slist_foreach (view->sel_list, repaint_sel, view);
		g_slist_free (view->sel_list);
		view->sel_list  = NULL;
	}
}

void
html_selection_start (HtmlView *view, GdkEventButton *event)
{
	HtmlBox *new_start;

	new_start = html_event_find_root_box (view->root, (gint)event->x, (gint)event->y);

	if (new_start) {

		view->sel_start = new_start;
		view->sel_end   = NULL;
		view->sel_start_ypos = event->y;

		if (HTML_IS_BOX_TEXT (new_start))
			view->sel_start_index = html_box_text_get_index (HTML_BOX_TEXT (view->sel_start), 
									 event->x - html_box_get_absolute_x (view->sel_start));
		else
			view->sel_start_index = 0;

		html_selection_update_primary_selection (view);
	
		html_selection_clear (view);

		view->sel_flag = TRUE;
	}
}

void
html_selection_end (HtmlView *view, GdkEventButton *event)
{
	view->sel_flag = FALSE;
	html_selection_update_primary_selection (view);
}

static gboolean
html_selection_create_list (HtmlView *view, HtmlBox *box, HtmlBox **start, HtmlBox **end, 
			    gint *startypos, gint *endypos, gint *numfound, gboolean *start_found)
{
	HtmlBox *child;

	if (*numfound == 0) {
		if (box == view->sel_start || box == view->sel_end) {
			
			if (view->sel_start == view->sel_end) {
				*numfound = 2;
				*start = box;
				*end = box;
				*startypos = MIN (view->sel_start_ypos, view->sel_end_ypos);
				*endypos   = MAX (view->sel_start_ypos, view->sel_end_ypos);
			}
			else {
				*numfound = 1;
				*start = box;
				if (box == view->sel_start)
					*startypos = view->sel_start_ypos;
				else
					*startypos = view->sel_end_ypos;
			}
			if (HTML_IS_BOX_TEXT (box)) {
				*start_found = TRUE;
				view->sel_backwards = box == view->sel_end;
			}
		}			
	}
	else if (*numfound == 1) {
		if (box == view->sel_start || box == view->sel_end) {
			
			*numfound = 2;
			*end = box;

			if (box == view->sel_end)
				*endypos = view->sel_end_ypos;
			else
				*endypos = view->sel_start_ypos;
			
			if (HTML_IS_BOX_TEXT (box)) {
				if (*start_found == TRUE) {
					view->sel_list = g_slist_prepend (view->sel_list, box);
					view->sel_backwards = box == view->sel_start;
					return TRUE;
				}
				else {
					*start_found = TRUE;

					view->sel_backwards = box == view->sel_end;
					
					if (box == view->sel_start)
						*endypos = view->sel_end_ypos;
					else
						*endypos = view->sel_start_ypos;

				}
			}
		}
	}
	if(*numfound == 1 && *start_found == FALSE) {
		if (HTML_IS_BOX_TEXT (box)) {
			gint pos = html_box_get_absolute_y (box);
			if (pos >= *startypos)
				*start_found = TRUE;
		}
	}
	if(*numfound == 1 && *start_found == TRUE) {
		if (HTML_IS_BOX_TEXT (box))
			view->sel_list = g_slist_prepend (view->sel_list, box);

	}
	if(*numfound == 2 && *start_found == FALSE) {
		if (HTML_IS_BOX_TEXT (box)) {
			gint pos = html_box_get_absolute_y (box);
			gint min = MIN(*startypos, *endypos);

			if (pos >= min) {
				*start_found = TRUE;
				if (*startypos > *endypos) {
					gint tmp = *startypos;
					*startypos = *endypos;
					*endypos = tmp;
					view->sel_backwards = TRUE;
				}
			}
		}
	}
	if(*numfound == 2 && *start_found == TRUE) {
		if (HTML_IS_BOX_TEXT (box)) {
			gint pos = html_box_get_absolute_y (box);

			if (pos > *endypos)
				return TRUE;
			else
				view->sel_list = g_slist_prepend (view->sel_list, box);
		}
	}
	child = box->children;
	while (child) {
		
		if (html_selection_create_list (view, child, start, end, startypos, endypos, numfound, start_found))
			return TRUE;

		child = child->next;
	}
	return FALSE;
}

static gboolean
html_selection_create_list_fast (HtmlView *view, HtmlBox *box, gboolean *start_found)
{
	HtmlBox *child;

	if (*start_found == TRUE) {
		if (box == view->sel_start || box == view->sel_end) {
			
			view->sel_list = g_slist_prepend (view->sel_list, box);
			return TRUE;
		}
	}
	if (*start_found == FALSE) {
		if (box == view->sel_start || box == view->sel_end) {
			
			view->sel_list = g_slist_prepend (view->sel_list, box);
			if (view->sel_start == view->sel_end)
				return TRUE;

			view->sel_backwards = box == view->sel_end;
			*start_found = TRUE;
		}
	}
	else if (HTML_IS_BOX_TEXT (box)) {
		view->sel_list = g_slist_prepend (view->sel_list, box);
	}

	child = box->children;
	while (child) {
		
		if (html_selection_create_list_fast (view, child, start_found))
			return TRUE;

		child = child->next;
	}
	return FALSE;
}

static void
html_selection_update_ends (HtmlView *view)
{
	if (view->sel_start == view->sel_end) {

		if (HTML_IS_BOX_TEXT (view->sel_end)) {
			html_box_text_set_selection (HTML_BOX_TEXT (view->sel_end), 
						     HTML_BOX_TEXT_SELECTION_BOTH, 
						     view->sel_start_index, view->sel_end_index);
		}
	}
	else {
		if (view->sel_backwards) {
			
			if (HTML_IS_BOX_TEXT (view->sel_start))
				html_box_text_set_selection (HTML_BOX_TEXT (view->sel_start), 
							     HTML_BOX_TEXT_SELECTION_END, -1, view->sel_start_index); 
			if (HTML_IS_BOX_TEXT (view->sel_end)) 
				html_box_text_set_selection (HTML_BOX_TEXT (view->sel_end), 
							     HTML_BOX_TEXT_SELECTION_START, view->sel_end_index, -1); 
		}
		else {
			if (HTML_IS_BOX_TEXT (view->sel_start)) 
				html_box_text_set_selection (HTML_BOX_TEXT (view->sel_start), 
							     HTML_BOX_TEXT_SELECTION_START, view->sel_start_index, -1); 
			if (HTML_IS_BOX_TEXT (view->sel_end)) 
				html_box_text_set_selection (HTML_BOX_TEXT (view->sel_end), 
							     HTML_BOX_TEXT_SELECTION_END, -1, view->sel_end_index); 
		}
	}
}

void
html_selection_update (HtmlView *view, GdkEventMotion *event)
{
	if (view->sel_flag) {

		HtmlBox *new_end = html_event_find_root_box (view->root, (gint)event->x, (gint)event->y);

		if (new_end == NULL)
			return;

		if (HTML_IS_BOX_TEXT (new_end))
			view->sel_end_index = html_box_text_get_index (HTML_BOX_TEXT (new_end), 
								       event->x - html_box_get_absolute_x (new_end));
		else
			view->sel_end_index = 0;

		if (new_end != view->sel_end) {

			view->sel_end = new_end;
			view->sel_end_ypos = event->y;

			if (view->sel_start && view->sel_end) {

				HtmlBox *start = NULL, *end = NULL;
				gint startypos, endypos;
				gint numfound = 0;
				gboolean start_found = FALSE;

				html_selection_clear (view);

				if (HTML_IS_BOX_TEXT (view->sel_start) && HTML_IS_BOX_TEXT (view->sel_end))
					html_selection_create_list_fast (view, view->root, &start_found);
				else
					html_selection_create_list (view, view->root, &start, &end, &startypos, &endypos, &numfound, &start_found);

				view->sel_list = g_slist_reverse (view->sel_list);
				
				g_slist_foreach (view->sel_list, select_text, view);
				html_selection_update_ends (view);
				g_slist_foreach (view->sel_list, repaint_sel, view);
			}
		}
		else {
			view->sel_end = new_end;
			html_selection_update_ends (view);
			repaint_sel (view->sel_end, view);
		}
	}
}

static void
set_traversal (HtmlView *view, HtmlBox *root, HtmlBox *start, int *offset, int *len, int *found_start)
{
	if (root == start)
		*found_start = TRUE;

	if (*found_start) {
		if (HTML_IS_BOX_TEXT (root)) {
			HtmlBoxText *text = HTML_BOX_TEXT (root);
			int text_len;
			gchar *char_text;
				
			view->sel_list = g_slist_prepend (view->sel_list, text);
			char_text = (gchar *)text->canon_text;
			text_len = g_utf8_pointer_to_offset (char_text,
							     char_text + text->length);
				
			if (*offset > 0) {
				if (*offset < text_len) {
					if (text_len >= *offset + *len) {
						html_box_text_set_selection (text, 
									     HTML_BOX_TEXT_SELECTION_BOTH, 
									     g_utf8_offset_to_pointer (char_text, *offset) - char_text,
									     g_utf8_offset_to_pointer (char_text, *offset + *len) - char_text);
						*len = 0;
					}
					else {
						html_box_text_set_selection (text, 
									     HTML_BOX_TEXT_SELECTION_START, 
									     g_utf8_offset_to_pointer (char_text, *offset + *len) - char_text, -1);
						*len = *len - text_len + *offset;
					}
				}
				*offset = MAX (0, *offset - text_len);
			}
			else {
				if (*len > text_len) {
					html_box_text_set_selection (HTML_BOX_TEXT (root), 
								     HTML_BOX_TEXT_SELECTION_FULL, 
								     -1, -1);
				}
				else {
					html_box_text_set_selection (text, 
								     HTML_BOX_TEXT_SELECTION_END, 
								     -1,
								     g_utf8_offset_to_pointer (char_text, *len) - char_text);
				}
				*len = MAX (0, *len - text_len);
			}
		}
	}
	if (*len > 0) {
		HtmlBox *child = root->children;
		
		while (child) {
			set_traversal (view, child, start, offset, len, found_start);
			if (*len == 0)
				return;
			child = child->next;
		}
	}
}

/**
 * html_selection_set:
 * @view:   the view to make the selection on.
 * @start:  the DomNode the @offset is relative of.
 * @offset: offset in characters from @start node to the selection start.
 * @len:    selection length in characters.
 * 
 * This function selects the text starting from @start + @offset and 
 * @len characters forward.
 **/
void
html_selection_set (HtmlView *view, 
		    DomNode *start, int offset, int len)
{
	HtmlBox *start_box;
	HtmlBox *root;
	int found_start = FALSE;

	g_return_if_fail (HTML_IS_VIEW (view));
	g_return_if_fail (DOM_IS_NODE (start));

	start_box = html_view_find_layout_box (view, DOM_NODE (start), FALSE);
	root      = view->root;

	g_return_if_fail (HTML_IS_BOX (start_box));

	html_selection_clear (view);

	set_traversal (view, root, start_box, &offset, &len, &found_start);

	view->sel_list = g_slist_reverse (view->sel_list);
	g_slist_foreach (view->sel_list, repaint_sel, view);
	html_selection_update_primary_selection (view);
}

/**
 * html_selection_extend:
 * @view:   the view in which to extend the selection.
 * @start:  the HtmlBoxText the @offset is relative of.
 * @offset: offset in characters from @start box to the selection start.
 * @len:    selection length in characters.
 * 
 * This function extends the current selection from the @start box + @offset 
 * @len characters forward.
 **/
void
html_selection_extend (HtmlView *view, 
		       HtmlBox *start, int start_offset, 
		       int len)
{
	HtmlBox *root;
	int found_start = FALSE;

	g_return_if_fail (HTML_IS_VIEW (view));
	g_return_if_fail (HTML_IS_BOX (start));

	if (!view->sel_list) {
		view->sel_start = NULL;
		view->sel_end = NULL;
		html_selection_update_primary_selection (view);
		html_selection_clear (view);
		view->sel_flag = TRUE;
	}
	root      = view->root;

	html_selection_clear (view);

	set_traversal (view, root, start, &start_offset, &len, &found_start);

	view->sel_list = g_slist_reverse (view->sel_list);
	g_slist_foreach (view->sel_list, repaint_sel, view);
	html_selection_update_primary_selection (view);
}
