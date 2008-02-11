/*
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef HTML_VIEW_ACCESSIBLE_H__
#define HTML_VIEW_ACCESSIBLE_H__

#include <libgtkhtml/gtkhtml.h>
#include <gtk/gtkaccessible.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HTML_TYPE_VIEW_ACCESSIBLE            (html_view_accessible_get_type ())
#define HTML_VIEW_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HTML_TYPE_VIEW_ACCESSIBLE, HtmlViewAccessible))
#define HTML_VIEW_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HTML_TYPE_VIEW_ACCESSIBLE, HtmlViewAccessibleClass))
#define HTML_IS_VIEW_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HTML_TYPE_VIEW_ACCESSIBLE))
#define HTML_IS_VIEW_CLASS_ACCESSIBLE(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HTML_VIEW_TYPE_ACCESSIBLE))
#define HTML_VIEW_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HTML_TYPE_VIEW_ACCESSIBLE, HtmlViewAccessibleClass))

typedef struct _HtmlViewAccessible      HtmlViewAccessible;
typedef struct _HtmlViewAccessibleClass HtmlViewAccessibleClass;

struct HtmlViewAccessible
{
	GtkAccessible parent;
};

GType html_view_accessible_get_type (void);

struct HtmlViewAccessibleClass
{
	GtkAccessibleClass parent_class;
};

AtkObject* html_view_accessible_new (GtkWidget *widget);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __HTML_VIEW_ACCESSIBLE_H__ */
