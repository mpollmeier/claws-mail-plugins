/*
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include <string.h>

#include <gtk/gtk.h>
#include "htmlboxaccessible.h"
#include "htmlboxtextaccessible.h"
#include "a11y/htmlviewaccessible.h"
#include <libgtkhtml/layout/htmlbox.h>
#include <libgtkhtml/layout/htmlboxtable.h>
#include <libgtkhtml/layout/htmlboxtablecell.h>
#include <libgtkhtml/layout/htmlboxtablerowgroup.h>
#include <libgtkhtml/layout/htmlboxtext.h>
#include <libgtkhtml/layout/htmlboxinline.h>
#include <libgtkhtml/layout/htmlboxtable.h>
#include <libgtkhtml/layout/htmlboxtablecell.h>
#include <libgtkhtml/dom/core/dom-element.h>

static void         html_box_accessible_class_init               (HtmlBoxAccessibleClass  *klass);
static void         html_box_accessible_initialize               (AtkObject         *obj,
                                                                  gpointer          data);
static gint         html_box_accessible_get_n_children           (AtkObject         *obj);
static AtkObject*   html_box_accessible_ref_child                (AtkObject         *obj,
                                                                  gint              i);
static gint         html_box_accessible_get_index_in_parent      (AtkObject         *obj);
static AtkObject*   html_box_accessible_get_parent               (AtkObject         *obj);
static AtkStateSet* html_box_accessible_ref_state_set	         (AtkObject         *obj);

static void         html_box_accessible_component_interface_init (AtkComponentIface *iface);
static guint        html_box_accessible_add_focus_handler        (AtkComponent      *component,
                                                                  AtkFocusHandler   handler);
static void         html_box_accessible_get_extents              (AtkComponent      *component,
                                                                  gint              *x,
                                                                  gint              *y,
                                                                  gint              *width,
                                                                  gint              *height,
                                                                  AtkCoordType      coord_type);
static gboolean     html_box_accessible_grab_focus               (AtkComponent      *component);
static void         html_box_accessible_remove_focus_handler     (AtkComponent      *component,
                                                                  guint             handler_id);
static gboolean     is_box_showing                               (HtmlBox           *box);

static char* view_str = "view";
static AtkGObjectAccessibleClass *parent_class = NULL;

GType
html_box_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo tinfo = {
			sizeof (HtmlBoxAccessibleClass),
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_box_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlBoxAccessible),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		static const GInterfaceInfo atk_component_info = {
			(GInterfaceInitFunc) html_box_accessible_component_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (ATK_TYPE_GOBJECT_ACCESSIBLE, "HtmlBoxAccessible", &tinfo, 0);
		g_type_add_interface_static (type, ATK_TYPE_COMPONENT, &atk_component_info);
	}

	return type;
}

static void
html_box_accessible_class_init (HtmlBoxAccessibleClass *klass)
{
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	class->get_index_in_parent = html_box_accessible_get_index_in_parent;
	class->get_parent = html_box_accessible_get_parent;
	class->ref_state_set = html_box_accessible_ref_state_set;
	class->get_n_children = html_box_accessible_get_n_children;
	class->ref_child = html_box_accessible_ref_child;
	class->initialize = html_box_accessible_initialize;
}

AtkObject*
html_box_accessible_new (GObject *obj)
{
	GObject *object;
	AtkObject *atk_object;

	g_return_val_if_fail (HTML_IS_BOX (obj), NULL);
	if (HTML_IS_BOX_TEXT (obj)) {
		HtmlBoxText *text;
		HtmlBox *box;

		text = HTML_BOX_TEXT (obj);
		if (html_box_text_get_len (text) > 0) {
			box = HTML_BOX (obj);

			/*
			   The following while loop seems to crash, as the
			   box->parent can apparently be NULL
			   This is blocked out for now, to solve this crash,
			   but the selection needs to be fixed to not get lost
			   when the font size is changed
			*/
#if 0
			while (!HTML_IS_BOX_BLOCK (box)) {
			      box = box->parent;
			}
#endif
			if (!(HTML_IS_BOX_BLOCK (box) &&
			      box->dom_node &&
			      strcmp ((char*)box->dom_node->xmlnode->name, "p") == 0)) { 
				atk_object = html_box_text_accessible_new (obj);
			} else {
				atk_object = atk_gobject_accessible_for_object (G_OBJECT (box));
			}
			return atk_object;
		}
	}

	object = g_object_new (HTML_TYPE_BOX_ACCESSIBLE, NULL);
	atk_object = ATK_OBJECT (object);
	atk_object_initialize (atk_object, obj);
	atk_object->role = ATK_ROLE_PANEL;
	return atk_object;
}

static void
html_box_accessible_initialize (AtkObject *obj, gpointer data)
{
	HtmlBox *box;
	AtkObject *parent_obj;

	ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

	HTML_BOX_ACCESSIBLE (obj)->index = -1;
	box = HTML_BOX (data);

	if (box->parent) { 
		HtmlBox *parent;
		gpointer data;

		/*
		 * Set the parent of a TableCell to be the Table rather
		 * than the TableRow or the TableRowGroup
		 */
		if (HTML_IS_BOX_TABLE_CELL (box)) {
			parent = box->parent->parent;
			if (HTML_IS_BOX_TABLE_ROW_GROUP (parent))
				parent = parent->parent;
			g_assert (HTML_IS_BOX_TABLE (parent));
		}
		/*
		 * Set the parent for a Box in HtmlBoxInline to be its parent
		 * allow for more than one HtmlBoxInLine in the hierarchy.
		 * Do this only if HtmlBoxInLine has only one child.
		 */
		else if (HTML_IS_BOX_INLINE (box->parent)) {
			if (box->next == NULL) {
				parent = box->parent;

				while (HTML_IS_BOX_INLINE (parent) &&
				       parent->children->next == NULL)
					parent = parent->parent;
			} else
				parent = box->parent;
			
		 } else
			parent = box->parent;
		data = g_object_get_data (G_OBJECT (parent), view_str);
		if (data)
			g_object_set_data (G_OBJECT (box), view_str, data);
		else {
			data = g_object_get_data (G_OBJECT (box), view_str);
			if (data)
				g_object_set_data (G_OBJECT (parent), view_str, data);
		}
		parent_obj = atk_gobject_accessible_for_object (G_OBJECT (parent));
		obj->accessible_parent = g_object_ref (parent_obj);
	}
}

static gint
html_box_accessible_get_n_children (AtkObject *obj)
{
	AtkGObjectAccessible *atk_gobject;
	HtmlBox *box;
	gint n_children = 0;
	GObject *g_obj;

	g_return_val_if_fail (HTML_IS_BOX_ACCESSIBLE (obj), 0);
	atk_gobject = ATK_GOBJECT_ACCESSIBLE (obj); 
	g_obj = atk_gobject_accessible_get_object (atk_gobject);
	if (g_obj == NULL)
		return 0;

	g_return_val_if_fail (HTML_IS_BOX (g_obj), 0);
	box = HTML_BOX (g_obj);

	if (box) {
		HtmlBox *child;

		child = box->children;

		while (child) {
			n_children++;
			child = child->next;
		}
	}
	return n_children;
}

static AtkObject *
html_box_accessible_ref_child (AtkObject *obj, gint i)
{
	AtkGObjectAccessible *atk_gobject;
	GObject *g_obj;
	HtmlBox *box;
	AtkObject *atk_child = NULL;
	gint n_children = 0;

	g_return_val_if_fail (HTML_IS_BOX_ACCESSIBLE (obj), NULL);
	atk_gobject = ATK_GOBJECT_ACCESSIBLE (obj); 
	g_obj = atk_gobject_accessible_get_object (atk_gobject);
	if (g_obj == NULL)
		return NULL;

	g_return_val_if_fail (HTML_IS_BOX (g_obj), NULL);
	box = HTML_BOX (g_obj);

	if (box) {
		HtmlBox *child;

		child = box->children;

		while (child) {
			if (n_children == i) {
				while (HTML_IS_BOX_INLINE (child) && 
				       child->children &&
				       child->children->next == NULL)
					child = child->children;	
				if (!child)
					return NULL;

				atk_child = atk_gobject_accessible_for_object (G_OBJECT (child));
				g_object_ref (atk_child);
				break;
			}
			n_children++;
			child = child->next;
		}
	}
	return atk_child;
}

static gint
html_box_accessible_get_index_in_parent (AtkObject *obj)
{
	AtkObject *parent;
	AtkGObjectAccessible *atk_gobj;
	HtmlBox *box;
	HtmlBox *parent_box;
	gint n_children = 0;
	GObject *g_obj;

	g_return_val_if_fail (HTML_IS_BOX_ACCESSIBLE (obj), -1);
	if (HTML_BOX_ACCESSIBLE (obj)->index != -1)
		return HTML_BOX_ACCESSIBLE (obj)->index;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (obj);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return -1;

	g_return_val_if_fail (HTML_IS_BOX (g_obj), -1);
	box = HTML_BOX (g_obj);
	parent = atk_object_get_parent (obj);
	if (HTML_IS_VIEW_ACCESSIBLE (parent)) {
		return 0;
	}
	else if (ATK_IS_GOBJECT_ACCESSIBLE (parent)) {
		parent_box = HTML_BOX (atk_gobject_accessible_get_object (ATK_GOBJECT_ACCESSIBLE (parent)));
	}
	else {
		g_assert_not_reached ();
		return -1;
	}
	while (HTML_IS_BOX_INLINE (box->parent) && 
	       box->parent->children->next == NULL)
		box = box->parent;

	if (HTML_IS_BOX_TABLE_CELL (box)) {
		gint i, n_cells;
		HtmlBoxTable *table;

		g_return_val_if_fail (HTML_BOX_TABLE (parent_box), -1);

		table = HTML_BOX_TABLE (parent_box);
		n_cells = table->rows * table->cols;
		for (i = 0; i < n_cells; i++) {
			if (table->cells[i] == box)
				break;
		}
		g_return_val_if_fail (i < n_cells, -1);

		i -= g_slist_length (table->header_list) * table->cols;
		return i;
	} else if (parent_box) {
		HtmlBox *child;

		child = parent_box->children;

		while (child) {
			if (child == box)
				return n_children;

			n_children++;
			child = child->next;
		}
	}
	return -1;
}

static AtkObject*
html_box_accessible_get_parent (AtkObject *obj)
{
	AtkObject *parent;

	parent = ATK_OBJECT_CLASS (parent_class)->get_parent (obj);

	if (!parent) {
		AtkGObjectAccessible *atk_gobj;
		GObject *g_obj;
		HtmlBox *box;
		GtkWidget *widget;

		g_return_val_if_fail (HTML_IS_BOX_ACCESSIBLE (obj), NULL);
		atk_gobj = ATK_GOBJECT_ACCESSIBLE (obj);

		g_obj = atk_gobject_accessible_get_object (atk_gobj);
		if (g_obj != NULL) {
			widget = g_object_get_data (g_obj, view_str);
			box = HTML_BOX (g_obj);
			g_return_val_if_fail (!box->parent, NULL); 
			g_return_val_if_fail (widget, NULL); 
			parent = gtk_widget_get_accessible (widget);
			obj->accessible_parent = g_object_ref (parent);
		}
	}
	return parent;
}

static AtkStateSet*
html_box_accessible_ref_state_set (AtkObject *obj)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	AtkStateSet *state_set;

	g_return_val_if_fail (HTML_IS_BOX_ACCESSIBLE (obj), NULL);
	atk_gobj = ATK_GOBJECT_ACCESSIBLE (obj);
	state_set = ATK_OBJECT_CLASS (parent_class)->ref_state_set (obj);

	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL) {
		/* Object is defunct */
		atk_state_set_add_state (state_set, ATK_STATE_DEFUNCT);
	}
	else {
		HtmlBox *box;
		HtmlStyle *style;
  
		box = HTML_BOX (g_obj);
		style = HTML_BOX_GET_STYLE (box);

		if (style->display != HTML_DISPLAY_NONE &&
		    style->visibility == HTML_VISIBILITY_VISIBLE) {
			atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);
			if (is_box_showing (box))
				atk_state_set_add_state (state_set, ATK_STATE_SHOWING);
		}
		if (HTML_IS_BOX_INLINE (box->parent) &&
		    DOM_IS_ELEMENT (box->parent->dom_node)) {
			DomElement *element;

			element = DOM_ELEMENT (box->parent->dom_node);
			if (dom_element_is_focusable (element)) {
				GtkWidget *widget;
				HtmlView *view;

				atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);
				widget = html_box_accessible_get_view_widget (box);
				view = HTML_VIEW (widget);
				if (view->document->focus_element == element)
					atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);
			}
		}
	}
	return state_set;
} 

static void
html_box_accessible_component_interface_init (AtkComponentIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->add_focus_handler = html_box_accessible_add_focus_handler;
  iface->get_extents = html_box_accessible_get_extents;
  iface->grab_focus = html_box_accessible_grab_focus;
  iface->remove_focus_handler = html_box_accessible_remove_focus_handler;
}

static guint
html_box_accessible_add_focus_handler (AtkComponent *component, AtkFocusHandler handler)
{
	return g_signal_connect_closure (component, 
					"focus-event",
					g_cclosure_new (G_CALLBACK (handler), NULL,
							(GClosureNotify) NULL),
					FALSE);
}

static void
html_box_accessible_get_extents (AtkComponent *component, gint *x, gint *y, gint *width, gint *height, AtkCoordType coord_type)
{
	AtkGObjectAccessible *atk_gobj;
	HtmlBox *box;
	GObject *g_obj;
	AtkObject *atk_obj;
	GtkWidget *view;
	gint view_x, view_y;

	g_return_if_fail (HTML_IS_BOX_ACCESSIBLE (component));

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (component);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return;

	g_return_if_fail (HTML_IS_BOX (g_obj));
	box = HTML_BOX (g_obj);

	*x = html_box_get_absolute_x (box);
	*y = html_box_get_absolute_y (box);
	*width = box->width;
	*height = box->height;

	/*
	 * This position is relative to the HtmlView so we need to get the
	 * position of its HtmlView
	 */
	view = html_box_accessible_get_view_widget (box);
	atk_obj = gtk_widget_get_accessible (view);
	atk_component_get_extents (ATK_COMPONENT (atk_obj), &view_x, &view_y,
				   NULL, NULL, coord_type);
	*x += view_x;
	*y += view_y;

	*x -= (gint) (GTK_LAYOUT (view)->hadjustment->value);
	*y -= (gint) (GTK_LAYOUT (view)->vadjustment->value);
}

static gboolean
html_box_accessible_grab_focus (AtkComponent *component)
{
  return FALSE;
}

static void
html_box_accessible_remove_focus_handler (AtkComponent *component, guint handler_id)
{
	g_signal_handler_disconnect (ATK_OBJECT (component), handler_id);
}

GtkWidget*
html_box_accessible_get_view_widget (HtmlBox *box)
{
	GtkWidget *widget;
	HtmlBox *temp_box;
	
	temp_box = box;
	widget = g_object_get_data (G_OBJECT (temp_box), view_str);
	/*
	 * If box has not have an accessible object created then the data
	 * will not have been set so try the box's parent. This will work
	 * as some box must have had an accessible created for this function
	 * to be called.
	 */
	while (!widget) {
		temp_box = temp_box->parent;
		widget = g_object_get_data (G_OBJECT (temp_box), view_str);
	}
	return widget;
}

static gboolean
is_box_showing (HtmlBox *box)
{
	GtkWidget *view;
	gint x, y;

	view = html_box_accessible_get_view_widget (box);

	x = html_box_get_absolute_x (box);
	y = html_box_get_absolute_y (box);

	x -= (gint) (GTK_LAYOUT (view)->hadjustment->value);
	y -= (gint) (GTK_LAYOUT (view)->vadjustment->value);

	if (x >= view->allocation.width || 
	    x + box->width  < 0 ||
	    y >= view->allocation.height ||
	    y + box->height < 0)
		return FALSE;
	else
		return TRUE;
}
