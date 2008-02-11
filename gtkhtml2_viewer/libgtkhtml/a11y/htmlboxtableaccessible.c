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

#include <gtk/gtk.h>
#include "htmlboxtableaccessible.h"
#include "htmlboxaccessible.h"
#include "htmlboxblockaccessible.h"
#include "a11y/htmlviewaccessible.h"
#include <libgtkhtml/layout/htmlboxtable.h>
#include <libgtkhtml/layout/htmlboxtablecell.h>

static void         html_box_table_accessible_class_init               (HtmlBoxTableAccessibleClass  *klass);
static void         html_box_table_accessible_finalize                 (GObject          *object);
static void	    html_box_table_accessible_real_initialize          (AtkObject        *object,
                                               			        gpointer         data);

static gint         html_box_table_accessible_get_n_children           (AtkObject        *obj);
static AtkObject*   html_box_table_accessible_ref_child                (AtkObject        *obj,
                                                                        gint             i);

static AtkObject*   find_cell (HtmlBoxTableAccessible *table,
                               gint                   index);
                                                                      
static gpointer parent_class = NULL;

struct _HtmlBoxTableAccessiblePrivate
{
	GList *extra_cells;
};

typedef struct _HtmlBoxTableAccessibleCellData HtmlBoxTableAccessibleCellData;

struct _HtmlBoxTableAccessibleCellData
{
	gint index;
	AtkObject *cell;
};

GType
html_box_table_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo tinfo = {
			sizeof (HtmlBoxTableAccessibleClass),
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) html_box_table_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			sizeof (HtmlBoxTableAccessible),
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};

		type = g_type_register_static (HTML_TYPE_BOX_ACCESSIBLE, "HtmlBoxTableAccessible", &tinfo, 0);
	}

	return type;
}

static void
html_box_table_accessible_class_init (HtmlBoxTableAccessibleClass *klass)
{
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	gobject_class->finalize = html_box_table_accessible_finalize;

	class->get_n_children = html_box_table_accessible_get_n_children;
	class->ref_child = html_box_table_accessible_ref_child;
	class->initialize = html_box_table_accessible_real_initialize;
}

static void
html_box_table_accessible_real_initialize (AtkObject *object,
                                           gpointer  data)
{
 	HtmlBoxTableAccessible *table;

	ATK_OBJECT_CLASS (parent_class)->initialize (object, data);

	table = HTML_BOX_TABLE_ACCESSIBLE (object);
	table->priv = g_new0 (HtmlBoxTableAccessiblePrivate, 1);
}

static void
html_box_table_accessible_finalize (GObject *object)
{
	HtmlBoxTableAccessible *table;
	GList *l;
	HtmlBoxTableAccessibleCellData *cell_data;

	table = HTML_BOX_TABLE_ACCESSIBLE (object);
	if (table->priv) {
		if (table->priv->extra_cells) {
			for (l = table->priv->extra_cells; l; l = l->next) {
				cell_data = (HtmlBoxTableAccessibleCellData *) (l->data);
				g_object_unref (cell_data->cell);
				g_free (l->data);
			}
			g_list_free (table->priv->extra_cells);
		}
		g_free (table->priv);
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

AtkObject*
html_box_table_accessible_new (GObject *obj)
{
	GObject *object;
	AtkObject *atk_object;

	g_return_val_if_fail (HTML_IS_BOX_TABLE (obj), NULL);
	object = g_object_new (HTML_TYPE_BOX_TABLE_ACCESSIBLE, NULL);
	atk_object = ATK_OBJECT (object);
	atk_object_initialize (atk_object, obj);
	atk_object->role = ATK_ROLE_TABLE;
	return atk_object;
}

static gint
html_box_table_accessible_get_n_children (AtkObject *obj)
{
	AtkGObjectAccessible *atk_gobject;
	HtmlBoxTable *box_table;
	HtmlBox *box;
	GObject *g_obj;
	GSList *last;
        gint n_rows, n_cells;

	g_return_val_if_fail (HTML_IS_BOX_TABLE_ACCESSIBLE (obj), 0);
	atk_gobject = ATK_GOBJECT_ACCESSIBLE (obj);
	g_obj = atk_gobject_accessible_get_object (atk_gobject);
	if (g_obj == NULL)
		return 0;

	g_return_val_if_fail (HTML_IS_BOX_TABLE (g_obj), 0);

	box_table = HTML_BOX_TABLE (g_obj);

	n_rows = g_slist_length (box_table->body_list);
	n_cells =  (n_rows -1) * box_table->cols;
	last = g_slist_last (box_table->body_list);
	box = HTML_BOX (last->data);
	if (box->children) {
		box = box->children;

		while (box) {
			n_cells++;
			box = box->next;
		}
	}
	return n_cells;
}

static AtkObject*
html_box_table_accessible_ref_child (AtkObject *obj,
                                     gint      i)
{
	AtkGObjectAccessible *atk_gobject;
	HtmlBoxTable *box_table;
	HtmlBoxTableAccessibleCellData *cell_data;
	HtmlBox *cell;
	GObject *g_obj;
	AtkObject *atk_child = NULL;
 	gint index;
        gint n_rows;

	g_return_val_if_fail (HTML_IS_BOX_TABLE_ACCESSIBLE (obj), NULL);
	atk_gobject = ATK_GOBJECT_ACCESSIBLE (obj);
	g_obj = atk_gobject_accessible_get_object (atk_gobject);
	if (g_obj == NULL)
		return NULL;

	g_return_val_if_fail (HTML_IS_BOX_TABLE (g_obj), NULL);

	box_table = HTML_BOX_TABLE (g_obj);

	n_rows = g_slist_length (box_table->body_list);
	if (i < 0 || i >= n_rows * box_table->cols)
		return NULL;

	index = g_slist_length (box_table->header_list) * box_table->cols + i;
	cell = box_table->cells[index];
	if (!cell) {
		atk_child = find_cell (HTML_BOX_TABLE_ACCESSIBLE (obj), index);
		if (!atk_child) {
			cell_data  = g_new (HtmlBoxTableAccessibleCellData, 1);
			cell = html_box_table_cell_new ();
			atk_child = atk_gobject_accessible_for_object (G_OBJECT (cell));
			cell_data->cell = g_object_ref (atk_child);
			atk_child->accessible_parent = g_object_ref (obj);
			g_object_unref (cell);
			g_assert (HTML_BOX_ACCESSIBLE (atk_child));
			HTML_BOX_ACCESSIBLE (atk_child)->index = i;	
		}
	} else {
		atk_child = atk_gobject_accessible_for_object (G_OBJECT (cell));
	}
	g_object_ref (atk_child);
	return atk_child;
}

static AtkObject*
find_cell (HtmlBoxTableAccessible *table,
           gint                   index)
{
	GList *list, *l;
	HtmlBoxTableAccessibleCellData *cell_data;

	list = table->priv->extra_cells;
	for (l = list; l; l = l->next) {
		cell_data = (HtmlBoxTableAccessibleCellData *) (l->data);
		if (cell_data->index == index)
			return cell_data->cell;
	}

	return NULL;
}

