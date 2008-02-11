/*
 * Copyright 2001 Sun Microsystems Inc.
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

#include <libgtkhtml/gtkhtml.h>
#include "htmlboxembeddedaccessible.h"

static void       html_box_embedded_accessible_class_init      (HtmlBoxEmbeddedAccessibleClass *klass);
static gint       html_box_embedded_accessible_get_n_children  (AtkObject             *obj);
static AtkObject* html_box_embedded_accessible_ref_child       (AtkObject             *obj,
                                                                gint                  i);

GType
html_box_embedded_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo tinfo = {
			sizeof (HtmlBoxEmbeddedAccessibleClass),
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_box_embedded_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlBoxEmbeddedAccessible),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		type = g_type_register_static (HTML_TYPE_BOX_ACCESSIBLE, "HtmlBoxEmbeddedAccessible", &tinfo, 0);
	}

	return type;
}

AtkObject*
html_box_embedded_accessible_new (GObject *obj)
{
	gpointer object;
	AtkObject *atk_object;

	g_return_val_if_fail (HTML_IS_BOX_EMBEDDED (obj), NULL);
	object = g_object_new (HTML_TYPE_BOX_EMBEDDED_ACCESSIBLE, NULL);
	atk_object = ATK_OBJECT (object);
	atk_object_initialize (atk_object, obj);
	atk_object->role = ATK_ROLE_PANEL;
	return atk_object;
}

static void
html_box_embedded_accessible_class_init (HtmlBoxEmbeddedAccessibleClass *klass)
{
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

	class->get_n_children = html_box_embedded_accessible_get_n_children;
	class->ref_child = html_box_embedded_accessible_ref_child;
}

static gint
html_box_embedded_accessible_get_n_children (AtkObject *obj)
{
	AtkGObjectAccessible *atk_gobject;
	HtmlBoxEmbedded *box_embedded;
	GObject *g_obj;

	g_return_val_if_fail (HTML_IS_BOX_EMBEDDED_ACCESSIBLE (obj), 0);
	atk_gobject = ATK_GOBJECT_ACCESSIBLE (obj); 
	g_obj = atk_gobject_accessible_get_object (atk_gobject);
	if (g_obj == NULL)
		/* State is defunct */
		return 0;

	g_return_val_if_fail (HTML_IS_BOX_EMBEDDED (g_obj), 0);

	box_embedded = HTML_BOX_EMBEDDED (g_obj);
	g_return_val_if_fail (box_embedded->widget, 0);
	return 1;
}

static AtkObject*
html_box_embedded_accessible_ref_child (AtkObject *obj, gint i)
{
	AtkGObjectAccessible *atk_gobject;
	HtmlBoxEmbedded *box_embedded;
	GObject *g_obj;
	AtkObject *atk_child;

	if (i != 0)
		return NULL;

	g_return_val_if_fail (HTML_IS_BOX_EMBEDDED_ACCESSIBLE (obj), NULL);

	atk_gobject = ATK_GOBJECT_ACCESSIBLE (obj); 
	g_obj = atk_gobject_accessible_get_object (atk_gobject);
	if (g_obj == NULL)
		/* State is defunct */
		return NULL;

	g_return_val_if_fail (HTML_IS_BOX_EMBEDDED (g_obj), NULL);

	box_embedded = HTML_BOX_EMBEDDED (g_obj);
	g_return_val_if_fail (box_embedded->widget, NULL);

	atk_child = gtk_widget_get_accessible (box_embedded->widget);
	g_object_ref (atk_child);
	atk_child->accessible_parent = g_object_ref (obj);
	return atk_child;
}
