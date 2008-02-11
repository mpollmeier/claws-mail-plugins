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
#include "a11y/htmllinkaccessible.h"
#include "a11y/htmlboxaccessible.h"
#include "layout/htmlboxinline.h"
#include "layout/htmlboxtext.h"

static void         html_link_accessible_class_init              (HtmlLinkAccessibleClass  *klass);

static void         html_link_accessible_finalize                (GObject           *object);

static gchar*       html_link_accessible_get_uri                 (AtkHyperlink      *link,
                                                                  gint              i);
static AtkObject*   html_link_accessible_get_object              (AtkHyperlink      *link,
                                                                  gint              i);
static gint         html_link_accessible_get_end_index           (AtkHyperlink      *link);
static gint         html_link_accessible_get_start_index         (AtkHyperlink      *link);
static gboolean     html_link_accessible_is_valid                (AtkHyperlink      *link);
static gint         html_link_accessible_get_n_anchors           (AtkHyperlink      *link);

static gchar*       get_uri_from_box                             (HtmlBox           *box);

static void         html_link_accessible_action_interface_init   (AtkActionIface    *iface);
static gboolean     html_link_accessible_do_action               (AtkAction         *action,
                                                                  gint              i);
gboolean            idle_do_action                               (gpointer          data);
static gint         html_link_accessible_get_n_actions           (AtkAction         *action);

static G_CONST_RETURN gchar*   html_link_accessible_get_description  (AtkAction         *action,
                                                                      gint              i);
static G_CONST_RETURN gchar*   html_link_accessible_get_name         (AtkAction         *action,
                                                                      gint              i);
static gboolean     html_link_accessible_set_description         (AtkAction         *action,
                                                                  gint              i,
                                                                  const gchar       *desc);

static gpointer parent_class = NULL;

GType
html_link_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo tinfo = {
			sizeof (HtmlLinkAccessibleClass),
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_link_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlLinkAccessible),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		static const GInterfaceInfo atk_action_info = {
			(GInterfaceInitFunc) html_link_accessible_action_interface_init,
			(GInterfaceFinalizeFunc) NULL,
			NULL
		};

		type = g_type_register_static (ATK_TYPE_HYPERLINK, "HtmlLinkAccessible", &tinfo, 0);
		g_type_add_interface_static (type, ATK_TYPE_ACTION, &atk_action_info);
	}

	return type;
}

static void
html_link_accessible_class_init (HtmlLinkAccessibleClass *klass)
{
	AtkHyperlinkClass *class = ATK_HYPERLINK_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->finalize = html_link_accessible_finalize;

	class->get_uri = html_link_accessible_get_uri;
	class->get_object = html_link_accessible_get_object;
	class->get_end_index = html_link_accessible_get_end_index;
	class->get_start_index = html_link_accessible_get_start_index;
	class->is_valid = html_link_accessible_is_valid;
	class->get_n_anchors = html_link_accessible_get_n_anchors;
}

static void
html_link_accessible_finalize (GObject *object)
{
	HtmlLinkAccessible *link;

	link = HTML_LINK_ACCESSIBLE (object);
	if (link->obj)
		g_object_remove_weak_pointer (G_OBJECT (link->obj),
					      (gpointer *)&link->obj);
	g_free (link->click_description);
	if (link->action_idle_handler) {
		g_source_remove (link->action_idle_handler);
		link->action_idle_handler = 0;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

AtkHyperlink*
html_link_accessible_new (AtkObject *obj)
{
	GObject *object;
	AtkHyperlink *atk_link;
	HtmlLinkAccessible *link;

	object = g_object_new (HTML_TYPE_LINK_ACCESSIBLE, NULL);
	atk_link = ATK_HYPERLINK (object);
	link = HTML_LINK_ACCESSIBLE (object);
	link->obj = obj;
	g_object_add_weak_pointer (G_OBJECT (link->obj),
				   (gpointer *)&link->obj);
	link->click_description = NULL;
	return atk_link;
}

static gchar*
html_link_accessible_get_uri (AtkHyperlink *link,
                              gint         i)
{
	HtmlLinkAccessible *html_link;

	 if (i)
		return NULL;
	html_link = HTML_LINK_ACCESSIBLE (link);
	if (!html_link->box)
		return NULL;

	return get_uri_from_box (html_link->box);
}

static AtkObject*
html_link_accessible_get_object (AtkHyperlink *link,
                                 gint         i)
{
	HtmlLinkAccessible *html_link;

	if (i)
		return NULL;
	html_link = HTML_LINK_ACCESSIBLE (link);
	return html_link->obj;
}

static gint
html_link_accessible_get_end_index (AtkHyperlink *link)
{
	HtmlLinkAccessible *html_link;
	HtmlBoxText *box_text;
	HtmlBox *box;
	gint len;
	gint text_len;
	gchar *text_chars;

	html_link = HTML_LINK_ACCESSIBLE (link);
	if (!html_link->box)
		return 0;

	box = html_link->box;
	len = 0;
	if (HTML_BOX_INLINE (box)) {
		box = box->children;

		while (box) {
			if (HTML_IS_BOX_TEXT (box)) {
				box_text = HTML_BOX_TEXT (box);
				text_chars = html_box_text_get_text (box_text, &text_len);
				len += g_utf8_strlen (text_chars, text_len);
			}
			box = box->next;
		}
	}
	return len + html_link->index;
}

static gint
html_link_accessible_get_start_index (AtkHyperlink *link)
{
	HtmlLinkAccessible *html_link;

	html_link = HTML_LINK_ACCESSIBLE (link);
	if (!html_link->box)
		return 0;
	return html_link->index;
}

static gboolean
html_link_accessible_is_valid (AtkHyperlink *link)
{
	return TRUE;
}

static gint
html_link_accessible_get_n_anchors (AtkHyperlink *link)
{
	return 1;
}

static gchar*
get_uri_from_box (HtmlBox *box)
{
	gchar *uri = NULL;

	if (HTML_BOX_INLINE (box)) {
		DomNode *node;
		xmlChar *href_prop;

		node = box->dom_node;

		if (node->xmlnode->name) {
			if (strcasecmp (node->xmlnode->name, "a") == 0) {
			   href_prop = xmlGetProp (node->xmlnode, "href");
			   uri = g_strdup ((gchar *)href_prop);
			}
		}
	}
	return uri;
}

static void
html_link_accessible_action_interface_init (AtkActionIface *iface)
{
	g_return_if_fail (iface != NULL);

	iface->do_action = html_link_accessible_do_action;
	iface->get_n_actions = html_link_accessible_get_n_actions;
	iface->get_description = html_link_accessible_get_description;
	iface->get_name = html_link_accessible_get_name;
	iface->set_description = html_link_accessible_set_description;
}

static gboolean
html_link_accessible_do_action (AtkAction *action,
                                gint       i)
{
	HtmlLinkAccessible *link;

	if (i == 0) {
		link = HTML_LINK_ACCESSIBLE (action);
		if (link->action_idle_handler)
			return FALSE;
		else {
			link->action_idle_handler = g_idle_add (idle_do_action, link);
			return TRUE;
		}
	} else {
		return FALSE;
	}
}

gboolean
idle_do_action (gpointer data)
{
	HtmlLinkAccessible *link;
	GtkWidget *view;
	gchar *uri;

	link = HTML_LINK_ACCESSIBLE (data);
	
	if (!link->box)
		return FALSE;
	link->action_idle_handler = 0;
	view = html_box_accessible_get_view_widget (link->box);
	uri = get_uri_from_box (link->box);
	g_signal_emit_by_name (HTML_VIEW (view)->document,
			       "link_clicked",
			       uri);
	g_free (uri);
	return FALSE;
}


static gint
html_link_accessible_get_n_actions (AtkAction *action)
{
	return 1;
}

static G_CONST_RETURN gchar*
html_link_accessible_get_description (AtkAction *action,
                                      gint      i)
{
	if (i == 0) {
		HtmlLinkAccessible *link;

		link = HTML_LINK_ACCESSIBLE (action);
		return link->click_description;
	} else {
		return NULL;
	}
}

static G_CONST_RETURN gchar*
html_link_accessible_get_name (AtkAction *action,
                               gint      i)
{
	if (i == 0)
		return "link click";
	else
		return NULL;
}

static gboolean
html_link_accessible_set_description (AtkAction   *action,
                                      gint        i,
                                      const gchar *desc)
{
	if (i == 0) {
		HtmlLinkAccessible *link;

		link = HTML_LINK_ACCESSIBLE (action);
		if (link->click_description)
			g_free (link->click_description);
		link->click_description = g_strdup (desc);
		return TRUE;	
	} else {
		return FALSE;
	}
}
