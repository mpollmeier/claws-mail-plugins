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
#include <libgtkhtml/gtkhtml.h>
#include "layout/htmlboxtext.h"
#include "layout/htmlboxinline.h"
#include "layout/htmlboxtable.h"
#include "layout/htmlboxtablecell.h"
#include "htmlboxblockaccessible.h"
#include "htmlboxblocktextaccessible.h"
#include "htmlboxblocklinkaccessible.h"

static void       html_box_block_accessible_class_init      (HtmlBoxBlockAccessibleClass *klass);

GType
html_box_block_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		 static const GTypeInfo tinfo = {
			sizeof (HtmlBoxBlockAccessibleClass),
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_box_block_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlBoxBlockAccessible),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		type = g_type_register_static (HTML_TYPE_BOX_ACCESSIBLE, "HtmlBoxBlockAccessible", &tinfo, 0);
	}

	return type;
}

static gboolean
contains_link (HtmlBox *root)
{
	HtmlBox *box;
	gboolean ret;

	box = root->children;

	ret = FALSE;
	while (box) {
		if (HTML_IS_BOX_INLINE (box)) {
			DomNode *node;

			node = box->dom_node;
			if (node->xmlnode->name) {
				if (strcasecmp ((char *)node->xmlnode->name, "a") == 0 &&
				    (xmlHasProp (node->xmlnode, (const unsigned char*)"href") != NULL)) {
					ret = TRUE;
					break;
				}
			}
			ret = contains_link (box);
			if (ret)
				break;
		}
		box = box->next;
	}
	return ret;
}

static gboolean
contains_text (HtmlBox *root)
{
	HtmlBox *box;
	gboolean ret;

	if (HTML_IS_BOX_BLOCK (root)) {
		if (root->dom_node && strcmp ((char *)root->dom_node->xmlnode->name, "p") != 0) {
			return FALSE;
		}
	}
	box = root->children;

	ret = FALSE;
	while (box) {
		if (HTML_IS_BOX_TEXT (box)) {
			if (html_box_text_get_len (HTML_BOX_TEXT (box)) > 0) {
				ret = TRUE;
				break;
			}
		} else if (HTML_IS_BOX_INLINE (box)) {
			ret = contains_text (box);
			if (ret)
				break;
		}
		box = box->next;
	}
	return ret;
}

AtkObject*
html_box_block_accessible_new (GObject *obj)
{
	GObject *object;
	AtkObject *atk_object;
	HtmlBox *box;

	g_return_val_if_fail (HTML_IS_BOX_BLOCK (obj), NULL);
	box = HTML_BOX (obj);
	if (contains_text (box)) {
		if (contains_link (box))
			atk_object = html_box_block_link_accessible_new (obj);	
		else 
			atk_object = html_box_block_text_accessible_new (obj);	
	} else {
		object = g_object_new (HTML_TYPE_BOX_BLOCK_ACCESSIBLE, NULL);
		atk_object = ATK_OBJECT (object);
		atk_object_initialize (atk_object, obj);
		atk_object->role = ATK_ROLE_PANEL;
	}
	return atk_object;
}

static void
html_box_block_accessible_class_init (HtmlBoxBlockAccessibleClass *klass)
{
}
