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

#include "gtk/gtkutils.h"

#include "htmlviewaccessible.h"
#include "htmlviewaccessiblefactory.h"
#include <libgtkhtml/layout/htmlboxinline.h>
#include <libgtkhtml/layout/htmlboxtext.h>

#undef DUMP_BOXES

static void       html_view_accessible_class_init          (HtmlViewAccessibleClass *klass);
static void       html_view_accessible_finalize            (GObject            *obj);
static void       html_view_accessible_initialize          (AtkObject          *obj,
                                                            gpointer           data);
static gint       html_view_accessible_get_n_children      (AtkObject          *obj);
static AtkObject* html_view_accessible_ref_child           (AtkObject          *obj,
                                                            gint               i);
static AtkStateSet* html_view_accessible_ref_state_set     (AtkObject          *obj);

static void       html_view_accessible_grab_focus_cb       (GtkWidget          *widget);
static AtkObject* html_view_accessible_get_focus_object    (GtkWidget          *widget,
                                                            gint               *link_index);

static void       focus_object_destroyed                   (gpointer           data);       
static void       root_object_destroyed                    (gpointer           data);
static void       set_root_object                          (GObject            *obj,
                                                            HtmlBox            *root);

static gpointer parent_class = NULL;

static const gchar* gail_focus_object = "gail-focus-object";
static const gchar* html_root = "html_root";

#ifdef DUMP_BOXES
static void
debug_dump_boxes (HtmlBox *root, gint indent, gboolean has_node, xmlNode *n)
{
	HtmlBox *box;
        gint i;

	if (!root)
		return;

	if (has_node) {
		if (root->dom_node != NULL && root->dom_node->xmlnode != n)
			return;
        }

	 box = root->children;


	 for (i = 0; i < indent; i++)
		g_print (" ");

	g_print ("Type %d: %s %s (%p, %p, %p) (%d %d %d %d)",
                 indent,
		 G_OBJECT_TYPE_NAME (root), G_OBJECT_TYPE_NAME (root->dom_node), root, root->dom_node, HTML_BOX_GET_STYLE (root), root->x, root->y, root->width, root->height);
	if (root->dom_node)
		g_print ("%s ", root->dom_node->xmlnode->name);
	g_print ("\n");
	if (HTML_IS_BOX_TEXT (root)) {
		HtmlBoxText *box_text;
		gint len;
		gchar *text;

		box_text = HTML_BOX_TEXT (root);
		text = html_box_text_get_text (box_text, &len);
		g_print ("Master: %p forced_newline: %d\n", box_text->master, box_text->forced_newline);
  		if (len) {
                        gchar *buffer;

			for (i = 0; i < indent; i++)
				g_print (" ");
			buffer = g_malloc (len + 4);
			buffer[0] = '|';
			strncpy (buffer + 1, text, len);
			buffer[len + 1] = '|';
			buffer[len + 2] = '\n';
			buffer[len + 3] = 0;
			g_print (buffer);
			g_free (buffer);
		}
	}

        while (box) {
	  debug_dump_boxes (box, indent + 1, has_node, n);
	  box = box->next;
	}
}
#endif

GType
html_view_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static GTypeInfo tinfo = {
			0, /* class size */
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_view_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			0, /* instance size */
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		/*
		 * Figure out the size of the class and instance 
		 * we are deriving from
		 */
		AtkObjectFactory *factory;
		GType derived_type;
		GTypeQuery query;
		GType derived_atk_type;

		derived_type = g_type_parent (HTML_TYPE_VIEW);
		factory = atk_registry_get_factory (atk_get_default_registry (), derived_type);
		derived_atk_type = atk_object_factory_get_accessible_type (factory);
		g_type_query (derived_atk_type, &query);
		tinfo.class_size = query.class_size;
		tinfo.instance_size = query.instance_size;

		type = g_type_register_static (derived_atk_type, "HtmlViewAccessible", &tinfo, 0);
	}

	return type;
}

static void
html_view_accessible_class_init (HtmlViewAccessibleClass *klass)
{
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->finalize = html_view_accessible_finalize;

	class->get_n_children = html_view_accessible_get_n_children;
	class->initialize = html_view_accessible_initialize;
	class->ref_child = html_view_accessible_ref_child;
	class->ref_state_set = html_view_accessible_ref_state_set;
}

AtkObject* 
html_view_accessible_new (GtkWidget *widget)
{
	GObject *object;
	AtkObject *accessible;

	object = g_object_new (HTML_TYPE_VIEW_ACCESSIBLE, NULL);

	accessible = ATK_OBJECT (object);
	atk_object_initialize (accessible, widget);
	accessible->role =  ATK_ROLE_HTML_CONTAINER;
	return accessible;
}

static void
html_view_accessible_initialize (AtkObject *obj,
                                 gpointer  data)
{
	GtkWidget *widget;
	HtmlView *view;

	ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

	widget = GTK_WIDGET (data);
 	view = HTML_VIEW (data);
	set_root_object (G_OBJECT (obj), view->root);
	g_signal_connect_after (widget, "grab_focus",
			        G_CALLBACK (html_view_accessible_grab_focus_cb),
                                NULL);
}

static void
html_view_accessible_finalize (GObject *obj)
{
	gpointer focus_obj;

	focus_obj = g_object_get_data (obj, gail_focus_object);
	if (focus_obj) {
		g_object_weak_unref (focus_obj, 
				     (GWeakNotify) focus_object_destroyed,
				     obj);
	}
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static gint 
html_view_accessible_get_n_children (AtkObject* obj)
{
	GtkAccessible *accessible;
	GtkWidget *widget;
	HtmlView *html_view;
	gint n_children = 0;

	g_return_val_if_fail (HTML_IS_VIEW_ACCESSIBLE (obj), 0);
	accessible = GTK_ACCESSIBLE (obj);
	widget = accessible->widget;
	if (widget == NULL)
		/* State is defunct */
		return 0;

	g_return_val_if_fail (HTML_IS_VIEW (widget), 0);

	html_view = HTML_VIEW (widget);

	if (html_view->root)
		n_children = 1;

	return n_children;
}

static AtkObject* 
html_view_accessible_ref_child (AtkObject *obj, gint i)
{
	GtkAccessible *accessible;
	GtkWidget *widget;
	HtmlView *html_view;
	HtmlBox *html_box;
	AtkObject *atk_child = NULL;

	if (i != 0)
		return NULL;

	g_return_val_if_fail (HTML_IS_VIEW_ACCESSIBLE (obj), NULL);
	accessible = GTK_ACCESSIBLE (obj);
	widget = accessible->widget;
	if (widget == NULL)
		/* State is defunct */
		return NULL;

	g_return_val_if_fail (HTML_IS_VIEW (widget), NULL);

	html_view = HTML_VIEW (widget);

	html_box = html_view->root;

	if (html_box) {
		gpointer old_root;

		atk_child = atk_gobject_accessible_for_object (G_OBJECT (html_box));
		g_object_set_data (G_OBJECT (html_box), "view", widget);
		g_object_ref (atk_child);
		/* We check whether the root node has changed */	
		old_root = g_object_get_data (G_OBJECT (obj), html_root);
		if (!old_root) {
			set_root_object (G_OBJECT (obj), html_box);
			g_signal_emit_by_name (obj, "children_changed::add", 0, NULL, NULL);
		}
	}
	return atk_child;
}

static void
html_view_accessible_grab_focus_cb (GtkWidget *widget)
{
	AtkObject *focus_object;
	AtkObject *obj;
	gint link_index = 0;

	focus_object = html_view_accessible_get_focus_object (widget, &link_index);

	obj = gtk_widget_get_accessible (widget);
	if (gtkut_widget_has_focus (widget)) {
		if (focus_object) {
			atk_focus_tracker_notify (focus_object);
			g_signal_emit_by_name (focus_object, "link-selected", link_index);
		} else {
			atk_focus_tracker_notify (obj);
		}
	}
}

static gboolean
get_link_index (HtmlBox *root, HtmlBox *link_box, gint *link_index)
{
	HtmlBox *box;
	gboolean ret;

	if (!root)
		return FALSE;

	if (HTML_IS_BOX_INLINE (root)) {
		if (root == link_box)
			return TRUE;
		*link_index++;
	}
	
	box = root->children;
	while (box) {
		ret = get_link_index (box, link_box, link_index);
		if (ret)
			return TRUE;
		box = box->next;
	}
	return FALSE;
}

static AtkObject*
html_view_accessible_get_focus_object (GtkWidget *widget, gint *link_index)
{
	HtmlView *view;
	HtmlBox *box;
	HtmlBox *focus_box;
	HtmlBox *parent;
	DomElement *focus_element;
	AtkObject *atk_obj;
	gint index;

	view = HTML_VIEW (widget);

	focus_element = view->document->focus_element;

	if (focus_element) {
		focus_box = box = html_view_find_layout_box (view, DOM_NODE (focus_element), FALSE);	
		parent = box->parent;
		while (parent) {
			if (!HTML_IS_BOX_BLOCK (parent)) {
				parent = parent->parent;
			} else {
				box = parent;
				break;
			}
		}
		g_assert (HTML_IS_BOX_BLOCK (box));
		if (box->dom_node && strcmp ((char *)box->dom_node->xmlnode->name, "p") == 0) {
			if (link_index) {
				index = 0;
				if (get_link_index (box, focus_box, &index))
					*link_index = index;
			}
		} else {
			box = focus_box->children;
			if (link_index) {
				*link_index = 0;
			}
		}
		g_object_set_data (G_OBJECT (box), "view", widget);
		atk_obj = atk_gobject_accessible_for_object (G_OBJECT (box));
	} else {
		atk_obj = NULL;
	}
	return atk_obj;
}

static AtkStateSet*
html_view_accessible_ref_state_set (AtkObject *obj)
{
	GtkWidget *widget = GTK_ACCESSIBLE (obj)->widget;
	HtmlView *view;
 	AtkStateSet *state_set; 

	state_set = ATK_OBJECT_CLASS (parent_class)->ref_state_set (obj);

	if (widget == NULL)
    		return state_set;

	view = HTML_VIEW (widget);

#ifdef DUMP_BOXES 
	debug_dump_boxes (view->root, 0, FALSE, NULL);
#endif

	if (view->document->focus_element && gtkut_widget_has_focus (widget))
		atk_state_set_remove_state (state_set, ATK_STATE_FOCUSED);

	return state_set;	
}

static void
focus_object_destroyed (gpointer data)
{
	g_object_set_data (G_OBJECT (data), gail_focus_object, NULL);
}       

static void
root_object_destroyed (gpointer data)
{
	set_root_object (G_OBJECT (data), NULL);
	g_signal_emit_by_name (data, "children_changed::remove", 0, NULL, NULL);
}

static void
set_root_object (GObject *obj,
                 HtmlBox *root) 
{
	gpointer old_root;

	old_root = g_object_get_data (obj, html_root);
	if (old_root && root) {
		g_object_weak_unref (old_root, 
				     (GWeakNotify) root_object_destroyed,
				     obj);
	}
	if (root) {
		g_object_weak_ref (G_OBJECT (root), 
				   (GWeakNotify) root_object_destroyed,
				   obj);
	}
	g_object_set_data (obj, html_root, root);
}

