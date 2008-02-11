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

#include <string.h>
#include <libgtkhtml/gtkhtml.h>
#include "layout/htmlboxtext.h"
#include "layout/htmlboxinline.h"
#include "htmlboxtextlinkaccessible.h"
#include "htmllinkaccessible.h"

static void     html_box_text_link_accessible_class_init      (HtmlBoxTextAccessibleClass *klass);
static void     html_box_text_link_accessible_finalize        (GObject   *object);
static void     html_box_text_link_accessible_real_initialize (AtkObject *object,
                                                               gpointer  data);

static void     html_box_text_link_accessible_hypertext_interface_init (AtkHypertextIface	  *iface);

static AtkHyperlink* html_box_text_link_accessible_get_link    (AtkHypertext *hypertext,
                                                         	gint         link_index);
static gint          html_box_text_link_accessible_get_n_links (AtkHypertext *hypertext);
static gint          html_box_text_link_accessible_get_link_index  (AtkHypertext *hypertext,
                                                                    gint         char_index);

static gpointer parent_class = NULL;

/*
 * This data structure is here in case we need to maintain any data for
 * a HtmlBoxTextLinkAccessible.
 */
struct _HtmlBoxTextLinkAccessiblePrivate
{
	gpointer tmp;
};

static const gchar *link_hyperlink = "atk-hyperlink";

GType
html_box_text_link_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		 static const GTypeInfo tinfo = {
			sizeof (HtmlBoxTextLinkAccessibleClass),
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_box_text_link_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlBoxTextLinkAccessible),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		static const GInterfaceInfo atk_hypertext_info = {
       			(GInterfaceInitFunc) html_box_text_link_accessible_hypertext_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type = g_type_register_static (HTML_TYPE_BOX_TEXT_ACCESSIBLE, "HtmlBoxTextLinkAccessible", &tinfo, 0);
       		g_type_add_interface_static (type, ATK_TYPE_HYPERTEXT, &atk_hypertext_info);
	}

	return type;
}

AtkObject*
html_box_text_link_accessible_new (GObject *obj)
{
	GObject *object;
	AtkObject *atk_object;

	object = g_object_new (HTML_TYPE_BOX_TEXT_LINK_ACCESSIBLE, NULL);
	atk_object = ATK_OBJECT (object);
	atk_object_initialize (atk_object, obj);
	atk_object->role = ATK_ROLE_TEXT;
	return atk_object;
}

static void
html_box_text_link_accessible_class_init (HtmlBoxTextAccessibleClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->finalize = html_box_text_link_accessible_finalize;
	class->initialize = html_box_text_link_accessible_real_initialize;
}

static void
html_box_text_link_accessible_finalize (GObject *object)
{
	HtmlBoxTextLinkAccessible *text;

	text = HTML_BOX_TEXT_LINK_ACCESSIBLE (object);
	g_free (text->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
html_box_text_link_accessible_real_initialize (AtkObject *object,
                                                gpointer  data)
{
	HtmlBoxTextLinkAccessible *text;

	ATK_OBJECT_CLASS (parent_class)->initialize (object, data);

	text = HTML_BOX_TEXT_LINK_ACCESSIBLE (object);
	text->priv = g_new0 (HtmlBoxTextLinkAccessiblePrivate, 1); 
}

static void
html_box_text_link_accessible_hypertext_interface_init (AtkHypertextIface *iface)
{
 	g_return_if_fail (iface != NULL);

	iface->get_link = html_box_text_link_accessible_get_link;
	iface->get_n_links = html_box_text_link_accessible_get_n_links;
	iface->get_link_index = html_box_text_link_accessible_get_link_index;
}

static void
box_link_destroyed (gpointer data)
{
	HtmlLinkAccessible *link;

	link = HTML_LINK_ACCESSIBLE (data);
	link->box = NULL;
	g_object_unref (link);
}

static AtkHyperlink*
html_box_text_link_accessible_get_link (AtkHypertext *hypertext,
	                                gint         link_index)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBox *box;
	HtmlBox *box_link;
	HtmlLinkAccessible *link;
	AtkHyperlink *hyperlink;
	HtmlBox *parent;
	gpointer data;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (hypertext);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return NULL;

	if (link_index != 0)
		return NULL;
		
	box = HTML_BOX (g_obj);
	box_link = box->parent;
	if (!box_link)
		return NULL;

	hyperlink = g_object_get_data (G_OBJECT (box_link), link_hyperlink);
	if (!hyperlink) {
		hyperlink = html_link_accessible_new (ATK_OBJECT (hypertext));
		link = HTML_LINK_ACCESSIBLE (hyperlink);
		link->box = box_link;
		link->index = 0;
		g_object_weak_ref (G_OBJECT (box_link),
				   (GWeakNotify) box_link_destroyed,
				   hyperlink);
		g_object_set_data (G_OBJECT (box_link), link_hyperlink, hyperlink);
		/* Store HtmlView on box_link */
		parent = box_link->parent;
		data = g_object_get_data (G_OBJECT (parent), "view");
		if (data)
			g_object_set_data (G_OBJECT (box_link), "view", data);

	}
	return hyperlink;
}

static gint
html_box_text_link_accessible_get_n_links (AtkHypertext *hypertext)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (hypertext);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return 0;
		
	return 1;
}

static gint
html_box_text_link_accessible_get_link_index (AtkHypertext *hypertext,
                                              gint         char_index)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (hypertext);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return -1;
		
	return 0;
}
