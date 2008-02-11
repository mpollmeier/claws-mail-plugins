/* 
 * Copyright 2004 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <strings.h>

#include <libgtkhtml/gtkhtml.h>
#include "layout/htmlboxtext.h"
#include "layout/htmlboxtable.h"
#include "layout/htmlboxinline.h"
#include "htmlboxtextaccessible.h"
#include "htmlboxtextlinkaccessible.h"
#include <libgail-util/gail-util.h>

static void     html_box_text_accessible_class_init      (HtmlBoxAccessibleClass *klass);
static void     html_box_text_accessible_finalize        (GObject   *object);
static void     html_box_text_accessible_real_initialize (AtkObject *object,
                                                          gpointer  data);
static gint     html_box_text_accessible_get_n_children  (AtkObject *obj);
static AtkObject*   html_box_text_accessible_ref_child   (AtkObject *obj,
                                                          gint      i);
static AtkRelationSet* html_box_text_accessible_ref_relation_set
                                                         (AtkObject *obj);

static void     html_box_text_accessible_text_interface_init (AtkTextIface	*iface);
static gchar*   html_box_text_accessible_get_text                  (AtkText             *text,
                                                                    gint                start_offset,
                                                                    gint                end_offset);
static gchar*   html_box_text_accessible_get_text_after_offset     (AtkText             *text,
                                                                    gint                offset,
                                                                    AtkTextBoundary     boundary_type,
                                                                    gint                *start_offset,
                                                                    gint                *end_offset);
static gchar*   html_box_text_accessible_get_text_at_offset        (AtkText             *text,
                                                                    gint                offset,
                                                                    AtkTextBoundary     boundary_type,
                                                                    gint                *start_offset,
                                                                    gint                *end_offset);
static gchar*    html_box_text_accessible_get_text_before_offset   (AtkText             *text,
                                                                    gint                offset,
                                                                    AtkTextBoundary     boundary_type,
                                                                    gint                *start_offset,
                                                                    gint                *end_offset);
static gunichar  html_box_text_accessible_get_character_at_offset  (AtkText            *text,
                                                                    gint               offset);
static gint     html_box_text_accessible_get_character_count       (AtkText             *text);
static gint     html_box_text_accessible_get_caret_offset          (AtkText            *text);
static gboolean html_box_text_accessible_set_caret_offset          (AtkText            *text,
                                                                    gint               offset);
static gint     html_box_text_accessible_get_offset_at_point       (AtkText            *text,
                                                                    gint               x,
                                                                    gint               y,
                                                                    AtkCoordType       coords);
static void     html_box_text_accessible_get_character_extents     (AtkText           *text,
                                                                    gint              offset,
                                                                    gint              *x,
                                                                    gint              *y,
                                                                    gint              *width,
                                                                    gint              *height,
                                                                    AtkCoordType      coords);
static AtkAttributeSet* 
                html_box_text_accessible_get_run_attributes        (AtkText           *text,
                                                                    gint              offset,
                                                                    gint              *start_offset,
                                                                    gint              *end_offset);
static AtkAttributeSet* 
                html_box_text_accessible_get_default_attributes    (AtkText          *text);
static gint     html_box_text_accessible_get_n_selections          (AtkText           *text);
static gchar*   html_box_text_accessible_get_selection             (AtkText           *text,
                                                                    gint              selection_num,
                                                                    gint              *start_pos,
                                                                    gint              *end_pos);
static gboolean html_box_text_accessible_add_selection             (AtkText           *text,
                                                                    gint              start_pos,
                                                                    gint              end_pos);
static gboolean html_box_text_accessible_remove_selection          (AtkText           *text,
                                                                    gint              selection_num);
static gboolean html_box_text_accessible_set_selection             (AtkText           *text,
                                                                    gint              selection_num,
                                                                    gint              start_pos,
                                                                    gint              end_pos);
static gchar*   get_text_near_offset                               (AtkText           *text,
                                                                    GailOffsetType    function,
                                                                    AtkTextBoundary   boundary_type,
                                                                    gint              offset,
                                                                    gint              *start_offset,
                                                                    gint              *end_offset);

extern HtmlBoxText* _html_view_get_cursor_box_text (HtmlView *view, gint *offset);

static gpointer parent_class = NULL;

struct _HtmlBoxTextAccessiblePrivate
{
	GailTextUtil *textutil;
	gint          caret_offset;
};

GType
html_box_text_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		 static const GTypeInfo tinfo = {
			sizeof (HtmlBoxTextAccessibleClass),
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_box_text_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlBoxTextAccessible),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		static const GInterfaceInfo atk_text_info = {
			(GInterfaceInitFunc) html_box_text_accessible_text_interface_init,
          		(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (HTML_TYPE_BOX_ACCESSIBLE, "HtmlBoxTextAccessible", &tinfo, 0);
  		g_type_add_interface_static (type, ATK_TYPE_TEXT, &atk_text_info);
	}

	return type;
}

static gboolean
is_link (HtmlBox *box)
{
	DomNode *node;
	gboolean ret;

	ret = FALSE;
	if HTML_IS_BOX_INLINE (box->parent) {
		node = box->parent->dom_node;
		if (strcasecmp ((char *)node->xmlnode->name, "a") == 0 &&
		    xmlHasProp (node->xmlnode, (const unsigned char *)"href") != NULL) {
			ret = TRUE;
		}
	}
	return ret;
}

AtkObject*
html_box_text_accessible_new (GObject *obj)
{
	GObject *object;
	AtkObject *atk_object;
	HtmlBox *box;

	box = HTML_BOX (obj);
	if (is_link (box)) {
		atk_object = html_box_text_link_accessible_new (obj);
	} else {
		object = g_object_new (HTML_TYPE_BOX_TEXT_ACCESSIBLE, NULL);
		atk_object = ATK_OBJECT (object);
		atk_object_initialize (atk_object, obj);
		atk_object->role = ATK_ROLE_TEXT;
	}
	return atk_object;
}

static void
html_box_text_accessible_class_init (HtmlBoxAccessibleClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->finalize = html_box_text_accessible_finalize;
	class->initialize = html_box_text_accessible_real_initialize;
	class->get_n_children = html_box_text_accessible_get_n_children;
	class->ref_child = html_box_text_accessible_ref_child;
	class->ref_relation_set = html_box_text_accessible_ref_relation_set;
}

static void
html_box_text_accessible_finalize (GObject *object)
{
	HtmlBoxTextAccessible *box_text = HTML_BOX_TEXT_ACCESSIBLE (object);

	g_object_unref (box_text->priv->textutil);
	g_free (box_text->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
append_text (HtmlBox *root,
             GString *content)
{
	gint len;
	gchar *text;

	if (HTML_IS_BOX_TEXT (root)) {
		text = html_box_text_get_text (HTML_BOX_TEXT (root), &len);
		if (text) {
			g_string_append_len (content, text, len);
		}
	}
}

static void
html_box_text_accessible_real_initialize (AtkObject *object,
                                          gpointer  data)
{
	HtmlBoxTextAccessible *box_text;
	HtmlBox *box;
	GtkTextBuffer *text_buffer;
	GString *content;

	ATK_OBJECT_CLASS (parent_class)->initialize (object, data);

	box_text = HTML_BOX_TEXT_ACCESSIBLE (object);
	box_text->priv = g_new0 (HtmlBoxTextAccessiblePrivate, 1); 
	text_buffer = gtk_text_buffer_new (NULL);
	content = g_string_new (NULL);
	box = HTML_BOX (data);
	append_text (box, content);
	if (content->len) {
		gtk_text_buffer_set_text (text_buffer, 
                                          content->str, content->len);
	}
	g_string_free (content, TRUE);
	box_text->priv->textutil = gail_text_util_new ();
	gail_text_util_buffer_setup (box_text->priv->textutil, text_buffer);
	g_object_unref (text_buffer);
}

static gint
html_box_text_accessible_get_n_children (AtkObject *obj)
{
        g_return_val_if_fail (HTML_IS_BOX_ACCESSIBLE (obj), 0);
        return 0;
}

static AtkObject*
html_box_text_accessible_ref_child (AtkObject *obj,
                                    gint      i)
{
        g_return_val_if_fail (HTML_IS_BOX_ACCESSIBLE (obj), NULL);
        return NULL;
}

static AtkObject *
ref_last_child (AtkObject *obj)
{
	AtkObject *child;
	gint n_children;

	n_children = atk_object_get_n_accessible_children (obj);
	if (n_children > 0) {
		child = atk_object_ref_accessible_child (obj, n_children - 1);

		while (child) {
			n_children = atk_object_get_n_accessible_children (child);
			if (n_children > 0) {
				g_object_unref (child);
				child = atk_object_ref_accessible_child (child, n_children -1);
			} else {
				break;
			}
		}
		return child;
	} else {
		return NULL;
	}
}

static AtkObject*
ref_previous_object (AtkObject *obj)
{
	AtkObject *parent;
	AtkObject *prev;
	AtkObject *tmp;
	gint index;
	gint n_children;

	index = atk_object_get_index_in_parent (obj);
	parent= atk_object_get_parent (obj);
	if (!HTML_IS_BOX_ACCESSIBLE (parent))
		return NULL;

	if (index > 0) {
		n_children = atk_object_get_n_accessible_children (obj);
		prev = atk_object_ref_accessible_child (parent, index - 1);
		tmp = ref_last_child (prev);
		if (tmp) {
			g_object_unref (prev);
			prev = tmp;
		}
	} else {
		prev = parent;
		while (prev) {
			index = atk_object_get_index_in_parent (prev);
			parent= atk_object_get_parent (prev);
			if (!HTML_IS_BOX_ACCESSIBLE (parent))
				return NULL;

			if (index > 0) {
				n_children = atk_object_get_n_accessible_children (obj);
				prev = atk_object_ref_accessible_child (parent, index - 1);
				tmp = ref_last_child (prev);
				if (tmp) {
					g_object_unref (prev);
					prev = tmp;
				}
				break;
			} else {
				prev = parent;
			}
		}
	}	
	return prev;
}

static AtkObject*
ref_next_object (AtkObject *obj)
{
	AtkObject *parent;
	AtkObject *next;
	gint index;
	gint n_children;

	n_children = atk_object_get_n_accessible_children (obj);
	if (n_children) {
		return atk_object_ref_accessible_child (obj, 0);
	}
	parent = atk_object_get_parent (obj);
	if (!HTML_IS_BOX_ACCESSIBLE (parent))
		return NULL;

	index = atk_object_get_index_in_parent (obj);
	n_children = atk_object_get_n_accessible_children (parent);
	if (index < n_children - 1) {
		return atk_object_ref_accessible_child (parent, index + 1);
	} else {
		next = parent;
		while (next) {
			parent = atk_object_get_parent (next);
			if (!HTML_IS_BOX_ACCESSIBLE (parent))
				return NULL;

			index = atk_object_get_index_in_parent (next);
			n_children = atk_object_get_n_accessible_children (parent);
			if (index < n_children - 1) {
				return atk_object_ref_accessible_child (parent, index + 1);
			} else {
				next = parent;
			}
		}
		return NULL;
	}	
}

static AtkRelationSet*
html_box_text_accessible_ref_relation_set (AtkObject *obj)
{
	AtkRelationSet *relation_set;
	AtkObject *atk_obj;
	AtkRelation *relation;
	AtkObject *accessible_array[1];

	relation_set = ATK_OBJECT_CLASS (parent_class)->ref_relation_set (obj);
	if (!atk_relation_set_contains (relation_set, ATK_RELATION_FLOWS_TO)) {
		atk_obj = ref_next_object (obj);
		while (atk_obj) {
			if (ATK_IS_TEXT (atk_obj))
				break;
			g_object_unref (atk_obj);
			atk_obj = ref_next_object (atk_obj);
		}
		if (atk_obj) {
			g_object_unref (atk_obj);
			accessible_array [0] = atk_obj;
			relation = atk_relation_new (accessible_array, 1, ATK_RELATION_FLOWS_TO);
			atk_relation_set_add (relation_set, relation);
			g_object_unref (relation);
		}
	}
	if (!atk_relation_set_contains (relation_set, ATK_RELATION_FLOWS_FROM)) {
		atk_obj = ref_previous_object (obj);
		while (atk_obj) {
			if (ATK_IS_TEXT (atk_obj))
				break;
			g_object_unref (atk_obj);
			atk_obj = ref_previous_object (atk_obj);
		}
		if (atk_obj) {
			g_object_unref (atk_obj);
			accessible_array [0] = atk_obj;
			relation = atk_relation_new (accessible_array, 1, ATK_RELATION_FLOWS_FROM);
			atk_relation_set_add (relation_set, relation);
			g_object_unref (relation);
		}

	}
	return relation_set;
}

static void
html_box_text_accessible_text_interface_init (AtkTextIface *iface)
{
 	g_return_if_fail (iface != NULL);

	iface->get_text                = html_box_text_accessible_get_text;
	iface->get_text_after_offset   = html_box_text_accessible_get_text_after_offset;
	iface->get_text_at_offset      = html_box_text_accessible_get_text_at_offset;
	iface->get_text_before_offset  = html_box_text_accessible_get_text_before_offset;
	iface->get_character_at_offset = html_box_text_accessible_get_character_at_offset;
	iface->get_character_count     = html_box_text_accessible_get_character_count;
	iface->get_caret_offset        = html_box_text_accessible_get_caret_offset;
	iface->set_caret_offset        = html_box_text_accessible_set_caret_offset;
	iface->get_offset_at_point     = html_box_text_accessible_get_offset_at_point;
	iface->get_character_extents   = html_box_text_accessible_get_character_extents;
	iface->get_n_selections        = html_box_text_accessible_get_n_selections;
	iface->get_selection           = html_box_text_accessible_get_selection;
	iface->add_selection           = html_box_text_accessible_add_selection;
	iface->remove_selection        = html_box_text_accessible_remove_selection;
	iface->set_selection           = html_box_text_accessible_set_selection;
	iface->get_run_attributes      = html_box_text_accessible_get_run_attributes;
	iface->get_default_attributes  = html_box_text_accessible_get_default_attributes;
}

static gchar*
html_box_text_accessible_get_text (AtkText *text,
                                   gint    start_offset,
                                   gint    end_offset)
{
	HtmlBoxTextAccessible *box_text;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;	

	g_return_val_if_fail (HTML_BOX_TEXT_ACCESSIBLE (text), NULL);
	box_text = HTML_BOX_TEXT_ACCESSIBLE (text);
	g_return_val_if_fail (box_text->priv->textutil, NULL);
	buffer = box_text->priv->textutil->buffer;
	gtk_text_buffer_get_iter_at_offset (buffer, &start, start_offset);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, end_offset);
	return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gchar*
html_box_text_accessible_get_text_after_offset (AtkText         *text,
                                                gint            offset,
                                                AtkTextBoundary boundary_type,
                                                gint            *start_offset,
                                                gint            *end_offset)
{
	return get_text_near_offset (text, GAIL_AFTER_OFFSET,
				     boundary_type, offset, 
				     start_offset, end_offset);
}

static gchar*
html_box_text_accessible_get_text_at_offset (AtkText         *text,
                                             gint            offset,
                                             AtkTextBoundary boundary_type,
                                             gint            *start_offset,
                                             gint            *end_offset)
{
	return get_text_near_offset (text, GAIL_AT_OFFSET,
				     boundary_type, offset, 
				     start_offset, end_offset);
}

static gchar*
html_box_text_accessible_get_text_before_offset (AtkText         *text,
                                                 gint            offset,
                                                 AtkTextBoundary boundary_type,
                                                 gint            *start_offset,
                                                 gint            *end_offset)
{
	return get_text_near_offset (text, GAIL_BEFORE_OFFSET,
				     boundary_type, offset, 
				     start_offset, end_offset);
}

static gunichar
html_box_text_accessible_get_character_at_offset (AtkText *text,
                                                  gint    offset)
{
	HtmlBoxTextAccessible *box_text;
	GtkTextIter start, end;
	GtkTextBuffer *buffer;
	gchar *string;
	gchar *index;
	gunichar unichar;

	g_return_val_if_fail (HTML_BOX_TEXT_ACCESSIBLE (text), '\0');
	box_text = HTML_BOX_TEXT_ACCESSIBLE (text);
	g_return_val_if_fail (box_text->priv->textutil, '\0');
	buffer = box_text->priv->textutil->buffer;
	if (offset >= gtk_text_buffer_get_char_count (buffer))
		return '\0';

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	string = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	index = g_utf8_offset_to_pointer (string, offset);

	unichar = g_utf8_get_char (index);
	g_free (string);
	return unichar;
}

static gint
html_box_text_accessible_get_character_count (AtkText *text)
{
	HtmlBoxTextAccessible *box_text;
	GtkTextBuffer *buffer;

	g_return_val_if_fail (HTML_BOX_TEXT_ACCESSIBLE (text), 0);
	box_text = HTML_BOX_TEXT_ACCESSIBLE (text);
	g_return_val_if_fail (box_text->priv->textutil, 0);
	buffer = box_text->priv->textutil->buffer;
	return gtk_text_buffer_get_char_count (buffer);
}

static gint
html_box_text_accessible_get_caret_offset (AtkText *text)
{
	HtmlBoxTextAccessible *box_text_a11y;
	HtmlBoxText *box_text;
	HtmlBoxText *cursor_box_text;
	GtkWidget *widget;
	HtmlView *view;
        GObject *g_obj;
	gint offset;

	g_return_val_if_fail (HTML_BOX_TEXT_ACCESSIBLE (text), 0);
	box_text_a11y = HTML_BOX_TEXT_ACCESSIBLE (text);
        g_obj = atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (text));
        if (g_obj == NULL)
                return 0;

	box_text = HTML_BOX_TEXT (g_obj);
	widget = html_box_accessible_get_view_widget (HTML_BOX (box_text));
	view = HTML_VIEW (widget);
	cursor_box_text = _html_view_get_cursor_box_text (view, &offset);
	if (cursor_box_text == box_text) {
		box_text_a11y->priv->caret_offset = offset;
	}
	return box_text_a11y->priv->caret_offset;
}

static gboolean
html_box_text_accessible_set_caret_offset (AtkText *text,
                                           gint    offset)
{
	g_return_val_if_fail (HTML_BOX_TEXT_ACCESSIBLE (text), FALSE);
	g_warning ("set_caret_offset not implemented");
	return FALSE;
}

static gboolean
find_box_text_for_position (HtmlBox     *root,
                            gint        *x,
                            gint        *y,
			    HtmlBoxText **text,
			    gint        *offset)
{
	HtmlBox *box;
	gint real_x, real_y;

	if (!root)
		return FALSE;

	if (HTML_IS_BOX_TEXT (root)) {
		*text = HTML_BOX_TEXT (root);
		real_x = html_box_get_absolute_x (root) - root->x;
		real_y = html_box_get_absolute_y (root) - root->y;
		if (root->width > 0 &&
		    root->x + root->width > *x &&
		    root->height > 0 &&
		    root->y + root->height > *y) {
		/* Allow for point being before HtmlBoxText */
			if (*x < root->x)
				*x = root->x;
			if (*y < root->y)
				*y = root->y;
			
			*x = *x - root->x;
			*y = *y - root->y;
			return TRUE; 
		} else {
			gchar *text_chars;
			gint text_len;

			text_chars = html_box_text_get_text (*text, &text_len);
			*offset += g_utf8_strlen (text_chars, text_len);
		}
		
	}
	box = root->children;
	while (box) {
		real_x = *x;
		real_y = *y;
		if (find_box_text_for_position (box, &real_x, &real_y, text, offset)) {
			*x = real_x;
			*y = real_y;
			return TRUE;
		}
		box = box->next;
	}
	return FALSE;
}
static gint
html_box_text_accessible_get_offset_at_point (AtkText      *text,
                                              gint         x,
                                              gint         y,
                                              AtkCoordType coords)
{
	gint real_x, real_y, real_width, real_height;
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBoxText *box_text;
	HtmlBox *box;
	gint x_offset;
	gint y_offset;
	gboolean found;
	gint offset = 0;
	gint index;
	gchar *text_chars;

	atk_component_get_extents (ATK_COMPONENT (text), &real_x, &real_y,
				   &real_width, &real_height, coords);
	if (y < real_y || y >= real_y + real_height)
		return -1;
	if (x < real_x || x >= real_x + real_width)
		return -1;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (text);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return -1;

	box = HTML_BOX (g_obj);
	box_text = NULL;
	x_offset = x - real_x;
	y_offset = y - real_y;
	found = find_box_text_for_position (box, &x_offset, &y_offset, &box_text, &offset);
	g_return_val_if_fail (box_text, -1);
	box = HTML_BOX (box_text);
	if (!found) {
		/* Assume that point is after last HtmlBoxText */
		return offset;
	}
	if (x_offset > box->width)
		x_offset = box->width;

	if (box->prev == NULL) {
		while (HTML_IS_BOX_INLINE (box->parent)) {
			x_offset -= html_box_left_border_width (box->parent);
			box = box->parent;
		}
	}
	index = html_box_text_get_index (box_text, x_offset);
	text_chars = html_box_text_get_text (box_text, NULL);
	offset += g_utf8_strlen (text_chars, index);
	return offset;
}

static HtmlBoxText*
find_box_text_for_offset (HtmlBox *root,
                          gint    *offset)
{
	HtmlBox *box;
	HtmlBoxText *text;
	gint len;

	if (!root)
		return NULL;

	if (HTML_IS_BOX_TEXT (root)) {
		gchar *text_chars;
		gint text_len;

		text = HTML_BOX_TEXT (root);
		text_chars = html_box_text_get_text (text, &text_len);
		len = g_utf8_strlen (text_chars, text_len);
		if (*offset < len)
			return text;
		else
			*offset -= len;
	}
	box = root->children;
	while (box) {
		text = find_box_text_for_offset (box, offset);	
		if (text)
			return text;
		box = box->next;
	}
	return NULL;
}

static void
html_box_text_accessible_get_character_extents (AtkText      *text,
                                                gint         offset,
                                                gint         *x,
                                                gint         *y,
                                                gint         *width,
                                                gint         *height,
                                                AtkCoordType coords)
{
	gint real_x, real_y;
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBoxText *box_text;
	HtmlBox *box;
	HtmlBox *top_box;
	gchar *text_chars;
	GdkRectangle rect;
	gint real_offset;

	atk_component_get_position (ATK_COMPONENT (text), &real_x, &real_y,
				    coords);

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (text);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return;

	top_box = HTML_BOX (g_obj);
	real_offset = offset;

	box_text = find_box_text_for_offset (top_box, &real_offset);
	if (!box_text)
		return;

	text_chars = html_box_text_get_text (box_text, NULL);
	real_offset = g_utf8_offset_to_pointer (text_chars, real_offset) - text_chars;
	html_box_text_get_character_extents (box_text, real_offset, &rect);
	box = HTML_BOX (box_text);
	
        *x = real_x + rect.x;
        *y = real_y + rect.y;
        *width = rect.width;
	*height = rect.height;
}

static AtkAttributeSet*
html_box_text_accessible_get_run_attributes (AtkText *text,
                                             gint    offset,
                                             gint    *start_offset,
                                             gint    *end_offset)
{
	return NULL;
}

static AtkAttributeSet*
html_box_text_accessible_get_default_attributes (AtkText *text)
{
	AtkGObjectAccessible *atk_gobj;
	AtkAttributeSet *attrib_set = NULL;
	GObject *g_obj;
	GtkWidget *view;
	HtmlBox *box;
	HtmlFontSpecification *font_spec;
	PangoAttrFontDesc *pango_font_desc;
	PangoFontDescription *font;
	PangoFontMask mask;
	PangoAttrList *attrs;
	PangoAttrIterator *iter;
	PangoAttrInt *pango_int;
	HtmlColor *color;
	HtmlStyle *style;
	HtmlTextAlignType text_align;
	gint len;
	gint int_value;
	gchar *value;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (text);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return NULL;

	box = HTML_BOX (g_obj);
	
	view = html_box_accessible_get_view_widget (box);
	style = HTML_BOX_GET_STYLE (box);
	font_spec = style->inherited->font_spec;
	attrs = pango_attr_list_new ();
	len = 0;
	html_font_specification_get_all_attributes (font_spec, attrs, 0, len,
						    HTML_VIEW (view)->magnification);
	iter = pango_attr_list_get_iterator (attrs);

        int_value = html_box_get_bidi_level (box);
	if (int_value > 1)
		int_value = 1;
	/*
         * int_value + 1 is to allow for skip "none" value for
	 * ATK_TEXT_ATTR_DIRECTION
	 */
	value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_DIRECTION, int_value + 1));
	attrib_set = gail_misc_add_attribute (attrib_set,
                                              ATK_TEXT_ATTR_DIRECTION,
                                              value);
	/*
	 * Currently unable to get language; see bug 297 in 
	 * bugzilla.codefactory.se
	 */
	if ((pango_font_desc  = (PangoAttrFontDesc*) pango_attr_iterator_get (iter, PANGO_ATTR_FONT_DESC)) != NULL) {
		font = pango_font_desc->desc;
		mask = pango_font_description_get_set_fields (font);
		if (mask & PANGO_FONT_MASK_STYLE) {
			value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_STYLE, pango_font_description_get_style (font)));
			attrib_set = gail_misc_add_attribute (attrib_set,
							      ATK_TEXT_ATTR_STYLE,
							      value);
		}
		if (mask & PANGO_FONT_MASK_VARIANT) {
			value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_VARIANT, pango_font_description_get_variant (font)));
			attrib_set = gail_misc_add_attribute (attrib_set,
							      ATK_TEXT_ATTR_VARIANT,
							      value);
		}
		if (mask & PANGO_FONT_MASK_STRETCH) {
			value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_STRETCH, pango_font_description_get_variant (font)));
			attrib_set = gail_misc_add_attribute (attrib_set,
							      ATK_TEXT_ATTR_STRETCH,
							      value);
		}
		if (mask & PANGO_FONT_MASK_FAMILY) {
			value = g_strdup (pango_font_description_get_family (font));
			attrib_set = gail_misc_add_attribute (attrib_set,
							      ATK_TEXT_ATTR_FAMILY_NAME,
							      value);
		}
		if (mask & PANGO_FONT_MASK_WEIGHT) {
			value = g_strdup_printf ("%i", pango_font_description_get_weight (font));
			attrib_set = gail_misc_add_attribute (attrib_set,
							      ATK_TEXT_ATTR_WEIGHT,
							      value);
		}
		if (mask & PANGO_FONT_MASK_SIZE) {
			value = g_strdup_printf ("%i", pango_font_description_get_size (font) / PANGO_SCALE);
			attrib_set = gail_misc_add_attribute (attrib_set,
							      ATK_TEXT_ATTR_SIZE,
							      value);
		}
	}

	text_align = style->inherited->text_align;
	if (text_align == HTML_TEXT_ALIGN_RIGHT)
		int_value = 1;
	else if (text_align == HTML_TEXT_ALIGN_CENTER)
		int_value = 2;
	else if (text_align == HTML_TEXT_ALIGN_JUSTIFY)
		int_value = 3;
	else
		int_value = 0;
	value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_JUSTIFICATION, int_value));
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_JUSTIFICATION,
					      value);

	/* Guess wrap word */
	value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_WRAP_MODE, 2));
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_WRAP_MODE,
					      value);

	color = &style->background->color;
	value = g_strdup_printf ("%u,%u,%u",
				 color->red, color->green, color->blue);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_BG_COLOR,
					      value);

	color = style->inherited->color;
	if (color) {
		value = g_strdup_printf ("%u,%u,%u",
					 color->red, color->green, color->blue);
		attrib_set = gail_misc_add_attribute (attrib_set,
						      ATK_TEXT_ATTR_FG_COLOR,
						      value);
	}

	value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_FG_STIPPLE, 0));
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_FG_STIPPLE,
					      value);
	value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_BG_STIPPLE, 0));
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_BG_STIPPLE,
					      value);
	if ((pango_int  = (PangoAttrInt*) pango_attr_iterator_get (iter, PANGO_ATTR_UNDERLINE)) != NULL) 
		int_value = pango_int->value;
	else
		int_value = 0;
	value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_UNDERLINE, int_value));
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_UNDERLINE,
					      value);
	if ((pango_int  = (PangoAttrInt*) pango_attr_iterator_get (iter, PANGO_ATTR_STRIKETHROUGH)) != NULL)
		int_value = pango_int->value;
	else
		int_value = 0;
	value = g_strdup (atk_text_attribute_get_value (ATK_TEXT_ATTR_STRIKETHROUGH, int_value));
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_STRIKETHROUGH,
					      value);
	value = g_strdup_printf ("%i", 0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_RISE,
					      value);
	value = g_strdup_printf ("%g", 1.0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_SCALE,
					      value);
	value = g_strdup_printf ("%i", 0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_BG_FULL_HEIGHT,
					      value);
	value = g_strdup_printf ("%i", 0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_PIXELS_INSIDE_WRAP,
					      value);
	value = g_strdup_printf ("%i", 0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_PIXELS_BELOW_LINES,
					      value);
	value = g_strdup_printf ("%i", 0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_PIXELS_ABOVE_LINES,
					      value);
	value = g_strdup_printf (atk_text_attribute_get_value (ATK_TEXT_ATTR_EDITABLE, 0));
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_EDITABLE,
					      value);
	if (style->visibility == HTML_VISIBILITY_VISIBLE)
		int_value = 0;
	else
		int_value = 1;
	value = g_strdup_printf (atk_text_attribute_get_value (ATK_TEXT_ATTR_INVISIBLE, int_value));
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_INVISIBLE,
					      value);
	value = g_strdup_printf ("%i", 0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_INDENT,
					      value);
	value = g_strdup_printf ("%i", 0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_RIGHT_MARGIN,
					      value);
	value = g_strdup_printf ("%i", 0);
	attrib_set = gail_misc_add_attribute (attrib_set,
					      ATK_TEXT_ATTR_LEFT_MARGIN,
					      value);

	pango_attr_iterator_destroy (iter);
	pango_attr_list_unref (attrs);
	return attrib_set;
}

static gboolean
find_selection (HtmlBox *root, HtmlBoxText **text, gint *offset)
{
	HtmlBox *box;

	if (!root)
		return FALSE;

	if (HTML_IS_BOX_TEXT (root)) {
		*text = HTML_BOX_TEXT (root);
		if ((*text)->selection != HTML_BOX_TEXT_SELECTION_NONE) {
			return TRUE;
		} else {
			if (offset) {
				gchar *text_chars;
				gint text_len;

				text_chars = html_box_text_get_text (*text, &text_len);
				*offset += g_utf8_strlen (text_chars, text_len);
			}
		}
	}
	box = root->children;
	while (box) {
		if (find_selection (box, text, offset))
			return TRUE;
		box = box->next;
	}
	return FALSE;
}

static gint
html_box_text_accessible_get_n_selections (AtkText *text)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBox *box;
	HtmlBoxText *box_text;
        gint n_selections;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (text);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return 0;

	box = HTML_BOX (g_obj);
	if (find_selection (box, &box_text, NULL))
		n_selections = 1;
	else
		n_selections = 0;

	return n_selections;
}

static HtmlBoxText*
find_next_text (HtmlBox *root, HtmlBox *last)
{
	HtmlBox *box;
	HtmlBoxText *text;

	if (last == NULL)
		box = root->children;
	else
		box = last->next;

	while (box) {
		if (HTML_IS_BOX_TEXT (box))
			return HTML_BOX_TEXT (box);
		if (box->children) {
			text = find_next_text (box, NULL);
			if (text)
				return text;
		}
		box = box->next;
	}
	box = last->parent;
	if (box != root)
		return find_next_text (root, box);
	return NULL;
}

static gchar*
html_box_text_accessible_get_selection (AtkText *text,
                                              gint    selection_num,
                                              gint    *start_pos,
                                              gint    *end_pos)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBox *box;
	HtmlBoxText *box_text;
	gint start_index;
	gchar *text_chars;
	gint sel_start_index;
	gint sel_end_index;

        if (selection_num)
		return NULL;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (text);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return NULL;

	box = HTML_BOX (g_obj);
	start_index = 0;
	if (find_selection (box, &box_text, &start_index)) {
		text_chars = html_box_text_get_text (box_text, NULL);
		sel_start_index = g_utf8_strlen (text_chars, box_text->sel_start_index);
		*start_pos = start_index + sel_start_index;
		*end_pos = *start_pos;
		while (box_text) {
			if (box_text->selection == HTML_BOX_TEXT_SELECTION_NONE)
				break;
			text_chars = html_box_text_get_text (box_text, NULL);
			if (box_text->selection == HTML_BOX_TEXT_SELECTION_FULL) {
				sel_start_index = 0;
				sel_end_index = g_utf8_strlen (text_chars, -1);
			} else {
				sel_start_index = g_utf8_strlen (text_chars, box_text->sel_start_index);
				sel_end_index = g_utf8_strlen (text_chars, box_text->sel_end_index);
			}
			*end_pos += sel_end_index - sel_start_index;
			box_text = find_next_text (box, HTML_BOX (box_text));
		}
		return atk_text_get_text (text, *start_pos, *end_pos);
	} else
		return NULL;
}

static gboolean
html_box_text_accessible_add_selection (AtkText *text,
                                        gint    start_pos,
                                        gint    end_pos)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBoxText *start_text, *end_text, *box_text;
	HtmlBox *box;
	GtkWidget *view;
	gchar *text_chars;
	gint start_offset, end_offset;

	if (start_pos < 0 || 
	    end_pos < 0 || 
	    start_pos == end_pos)
		return FALSE;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (text);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return FALSE;

	box = HTML_BOX (g_obj);
	if (start_pos < end_pos) {
		start_offset = start_pos;
		end_offset = end_pos;
	 } else { 
		start_offset = end_pos;
		end_offset = start_pos;
	}

	start_text = find_box_text_for_offset (box, &start_offset);
	end_text = find_box_text_for_offset (box, &end_offset);
	if (!start_text)
		return FALSE;

	box_text = start_text;
	while (box_text) {
		if (box_text == end_text) {
			text_chars = html_box_text_get_text (box_text, NULL);
			start_offset = g_utf8_offset_to_pointer (text_chars, start_offset) - text_chars;
			end_offset = g_utf8_offset_to_pointer (text_chars, end_offset) - text_chars;
			html_box_text_set_selection (box_text, 
						     HTML_BOX_TEXT_SELECTION_BOTH,
						     start_offset, end_offset);
		} else if (box_text == start_text) {
			text_chars = html_box_text_get_text (box_text, NULL);
			start_offset = g_utf8_offset_to_pointer (text_chars, start_offset) - text_chars;
			html_box_text_set_selection (box_text, 
						     HTML_BOX_TEXT_SELECTION_START,
						     start_offset,
						     html_box_text_get_len (box_text));
		} else 
			html_box_text_set_selection (box_text, 
						     HTML_BOX_TEXT_SELECTION_FULL,
						     0,
						     html_box_text_get_len (box_text));
		box_text = find_next_text (box, HTML_BOX (box_text));
		start_offset = 0;
	}
	view = html_box_accessible_get_view_widget (box);
	gtk_widget_queue_draw (view);

	return TRUE;
}

static gboolean
html_box_text_accessible_remove_selection (AtkText *text,
                                           gint    selection_num)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBoxText *box_text;
	HtmlBox *box;
	GtkWidget *view;

	if (selection_num)
		return FALSE;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (text);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return FALSE;

	box = HTML_BOX (g_obj);
	if (find_selection (box, &box_text, NULL)) {
		while (box_text) {
			if (box_text->selection == HTML_BOX_TEXT_SELECTION_NONE)
				break;
			html_box_text_set_selection (box_text, HTML_BOX_TEXT_SELECTION_NONE, -1, -1);
			box_text = find_next_text (box, HTML_BOX (box_text));
		}
		view = html_box_accessible_get_view_widget (box);
		gtk_widget_queue_draw (view);
		return TRUE;
	}
	return FALSE;
}

static gboolean
html_box_text_accessible_set_selection (AtkText *text,
                                        gint    selection_num,
                                        gint    start_pos,
                                        gint    end_pos)
{
	if (selection_num)
		return FALSE;

	return html_box_text_accessible_add_selection (text, start_pos, end_pos);
}

static gchar*
get_text_near_offset (AtkText          *text,
                      GailOffsetType   function,
                      AtkTextBoundary  boundary_type,
                      gint             offset,
                      gint             *start_offset,
                      gint             *end_offset)
{
	return gail_text_util_get_text (HTML_BOX_TEXT_ACCESSIBLE (text)->priv->textutil, NULL,
					function, boundary_type, offset, 
					start_offset, end_offset);
}
