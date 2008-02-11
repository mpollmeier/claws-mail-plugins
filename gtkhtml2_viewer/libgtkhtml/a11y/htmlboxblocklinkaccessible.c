/*
 * Copyright 2003 Sun Microsystems Inc.
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
#include "htmlboxblocklinkaccessible.h"
#include "htmllinkaccessible.h"

static void     html_box_block_link_accessible_class_init      (HtmlBoxBlockAccessibleClass *klass);
static void     html_box_block_link_accessible_finalize        (GObject   *object);
static void     html_box_block_link_accessible_real_initialize (AtkObject *object,
                                                                gpointer  data);

static void     html_box_block_link_accessible_hypertext_interface_init (AtkHypertextIface	  *iface);

static AtkHyperlink* html_box_block_link_accessible_get_link    (AtkHypertext *hypertext,
	                                                         gint         link_index);
static gint          html_box_block_link_accessible_get_n_links (AtkHypertext *hypertext);
static gint          html_box_block_link_accessible_get_link_index  (AtkHypertext *hypertext,
                                                                    gint         char_index);

static gpointer parent_class = NULL;

/*
 * This data structure is here in case we need to maintain any data for
 * a HtmlBoxBlockLinkAccessible.
 */
struct _HtmlBoxBlockLinkAccessiblePrivate
{
	gpointer tmp;
};

static const gchar *link_hyperlink = "atk-hyperlink";

GType
html_box_block_link_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		 static const GTypeInfo tinfo = {
			sizeof (HtmlBoxBlockLinkAccessibleClass),
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_box_block_link_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlBoxBlockLinkAccessible),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		static const GInterfaceInfo atk_hypertext_info = {
       			(GInterfaceInitFunc) html_box_block_link_accessible_hypertext_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};
		type = g_type_register_static (HTML_TYPE_BOX_BLOCK_TEXT_ACCESSIBLE, "HtmlBoxBlockLinkAccessible", &tinfo, 0);
       		g_type_add_interface_static (type, ATK_TYPE_HYPERTEXT, &atk_hypertext_info);
	}

	return type;
}

AtkObject*
html_box_block_link_accessible_new (GObject *obj)
{
	GObject *object;
	AtkObject *atk_object;

	object = g_object_new (HTML_TYPE_BOX_BLOCK_LINK_ACCESSIBLE, NULL);
	atk_object = ATK_OBJECT (object);
	atk_object_initialize (atk_object, obj);
	atk_object->role = ATK_ROLE_TEXT;
	return atk_object;
}

static void
html_box_block_link_accessible_class_init (HtmlBoxBlockAccessibleClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->finalize = html_box_block_link_accessible_finalize;
	class->initialize = html_box_block_link_accessible_real_initialize;
}

static void
html_box_block_link_accessible_finalize (GObject *object)
{
	HtmlBoxBlockLinkAccessible *block;

	block = HTML_BOX_BLOCK_LINK_ACCESSIBLE (object);
	g_free (block->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
html_box_block_link_accessible_real_initialize (AtkObject *object,
                                                gpointer  data)
{
	HtmlBoxBlockLinkAccessible *block;

	ATK_OBJECT_CLASS (parent_class)->initialize (object, data);

	block = HTML_BOX_BLOCK_LINK_ACCESSIBLE (object);
	block->priv = g_new0 (HtmlBoxBlockLinkAccessiblePrivate, 1); 
}

static void
html_box_block_link_accessible_hypertext_interface_init (AtkHypertextIface *iface)
{
 	g_return_if_fail (iface != NULL);

	iface->get_link = html_box_block_link_accessible_get_link;
	iface->get_n_links = html_box_block_link_accessible_get_n_links;
	iface->get_link_index = html_box_block_link_accessible_get_link_index;
}

static gboolean
is_link (HtmlBox *box)
{
	DomNode *node;
	gboolean ret;

	ret = FALSE;
	node = box->dom_node;
	if (node->xmlnode->name) {
		if (strcasecmp ((char *)node->xmlnode->name, "a") == 0 &&
                    (xmlHasProp (node->xmlnode, (const unsigned char*)"href") != NULL)) {
			ret = TRUE;
		}
	}
	return ret;
}

static HtmlBox*
find_link (HtmlBox *root, gint *link_index, gint *char_index)
{
	HtmlBox *box;
	HtmlBox *ret_box;
	HtmlBoxText *box_text;
	gchar *text;
	gint text_len;

	box = root->children;

	while (box) {
		if (HTML_IS_BOX_TEXT (box)) {
			box_text = HTML_BOX_TEXT (box);
			text = html_box_text_get_text (box_text, &text_len);
			*char_index += g_utf8_strlen (text, text_len);
		}
		if (HTML_IS_BOX_INLINE (box)) {
			if (is_link (box)) {
				if (*link_index == 0) {
					return box;
				}
   				(*link_index)--;
				find_link (box, link_index, char_index);
			} else {
				ret_box = find_link (box, link_index, char_index);
				if (ret_box)
					return ret_box;
			}
		} else if (HTML_IS_BOX_BLOCK (box)) {
			ret_box = find_link (box, link_index, char_index);
			if (ret_box)
				return ret_box;
		}
                box = box->next;
	}
        return NULL;
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
html_box_block_link_accessible_get_link (AtkHypertext *hypertext,
	                                 gint         link_index)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBox *box;
	HtmlBox *box_link;
	gint index;
	gint lindex;
	HtmlLinkAccessible *link;
	AtkHyperlink *hyperlink;
	HtmlBox *parent;
	gpointer data;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (hypertext);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return NULL;
		
	box = HTML_BOX (g_obj);
	index = 0;
	lindex = link_index;
	box_link = find_link (box, &lindex, &index);
	if (!box_link)
		return NULL;

	hyperlink = g_object_get_data (G_OBJECT (box_link), link_hyperlink);
	if (!hyperlink) {
		hyperlink = html_link_accessible_new (ATK_OBJECT (hypertext));
		link = HTML_LINK_ACCESSIBLE (hyperlink);
		link->box = box_link;
		link->index = index;
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

static void
count_links (HtmlBox *root, gint *n_links)
{
	HtmlBox *box;

	box = root->children;

	while (box) {
		if (HTML_IS_BOX_INLINE (box)) {
			if (is_link (box)) {
   				(*n_links)++;
			} else {
				count_links (box, n_links);
			}
		} else if (HTML_IS_BOX_BLOCK (box)) {
			count_links (box, n_links);
		}
                box = box->next;
	}
        return;
}

static gboolean
get_link_index (HtmlBox *root, gint *char_index, gint *link_index)
{
	HtmlBox *box;
	HtmlBoxText *box_text;
	gchar *text;
	gint text_len;

	box = root->children;

	while (box) {
		if (HTML_IS_BOX_TEXT (box)) {
			box_text = HTML_BOX_TEXT (box);
			text = html_box_text_get_text (box_text, &text_len);
			*char_index -= g_utf8_strlen (text, text_len);
			if (*char_index < 0)
				return FALSE;
		}
		if (HTML_IS_BOX_INLINE (box)) {
			if (is_link (box)) {
   				(*link_index)++;
				get_link_index (box, char_index, link_index);
				if (*char_index < 0)
					return TRUE;	
			} else if (get_link_index (box, char_index, link_index))
				return TRUE;
			else if (*char_index < 0)
				return FALSE;	
		} else if (HTML_IS_BOX_BLOCK (box)) {
			if (get_link_index (box, char_index, link_index))
				return TRUE;
			else if (*char_index < 0)
				return FALSE;	
		}
                box = box->next;
	}
        return FALSE;
}

static gint
html_box_block_link_accessible_get_n_links (AtkHypertext *hypertext)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBox *box;
	gint n_links;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (hypertext);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return 0;
		
	box = HTML_BOX (g_obj);
	n_links = 0;
	count_links (box, &n_links);
	return n_links;
}

static gint
html_box_block_link_accessible_get_link_index (AtkHypertext *hypertext,
                                               gint         char_index)
{
	AtkGObjectAccessible *atk_gobj;
	GObject *g_obj;
	HtmlBox *box;
	gint link_index;

	atk_gobj = ATK_GOBJECT_ACCESSIBLE (hypertext);
	g_obj = atk_gobject_accessible_get_object (atk_gobj);
	if (g_obj == NULL)
		return -1;
		
	box = HTML_BOX (g_obj);
	link_index = -1;
	if (get_link_index (box, &char_index, &link_index))
		return link_index;
	else
		return -1;
}

