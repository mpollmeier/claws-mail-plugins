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

#include <gtk/gtk.h>
#include "htmlviewaccessiblefactory.h"
#include "htmlviewaccessible.h"

static void html_view_accessible_factory_class_init (
                                HtmlViewAccessibleFactoryClass        *klass);

static AtkObject * html_view_accessible_factory_create_accessible (GObject *obj);

static GType html_view_accessible_factory_get_accessible_type (void);

GType
html_view_accessible_factory_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo tinfo = {
			sizeof (HtmlViewAccessibleFactoryClass),
 			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_view_accessible_factory_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlViewAccessibleFactory),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};
		type = g_type_register_static (ATK_TYPE_OBJECT_FACTORY, "HtmlViewAccessibleFactory" , &tinfo, 0);
	}
	return type;
}

static void 
html_view_accessible_factory_class_init (HtmlViewAccessibleFactoryClass *klass)
{
	AtkObjectFactoryClass *class = ATK_OBJECT_FACTORY_CLASS (klass);

	class->create_accessible = html_view_accessible_factory_create_accessible;
	class->get_accessible_type = html_view_accessible_factory_get_accessible_type;
}

static AtkObject* 
html_view_accessible_factory_create_accessible (GObject *obj)
{
	GtkWidget     *widget;

	g_return_val_if_fail (GTK_IS_WIDGET (obj), NULL);

	widget = GTK_WIDGET (obj);

	return  html_view_accessible_new (widget);
}

static GType
html_view_accessible_factory_get_accessible_type (void)
{
	return HTML_TYPE_VIEW_ACCESSIBLE;
}
