/*
 * synce_plugin -- SynCE Support for Sylpheed-Claws
 *
 * Copyright (C) 2005   Holger Berndt
 *
 * Sylpheed is a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto and the Sylpheed-Claws Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "main.h"
#include "mainwindow.h"

#include "synce_comp.h"

static void synce_sync(gpointer callback_data,
		       guint callback_action, GtkWidget *widget);


static GtkItemFactoryEntry mainwindow_tools_synce = {
	N_("/Tools/SynCE"),
	NULL,
	synce_sync,
	0,
	NULL
};

static GtkItemFactoryEntry mainwindow_tools_sep = {
	N_("/Tools/---"),
	NULL,
	NULL,
	0,
	"<Separator>"
};

void synce_gtk_init(void)
{
  GtkItemFactory *ifactory;
  MainWindow *mainwin;

  mainwin =  mainwindow_get_mainwindow();

  ifactory = gtk_item_factory_from_widget(mainwin->menubar);
  gtk_item_factory_create_item(ifactory, &mainwindow_tools_sep, mainwin, 1);
  gtk_item_factory_create_item(ifactory, &mainwindow_tools_synce, mainwin, 1);
}

void synce_gtk_done(void)
{
  GtkItemFactory *ifactory;
  MainWindow *mainwin;
  GtkWidget *widget;
  GtkWidget *sep;

  mainwin = mainwindow_get_mainwindow();

  if (mainwin == NULL || sylpheed_is_exiting())
	  return;
 
  ifactory = gtk_item_factory_from_widget(mainwin->menubar);
  sep = gtk_item_factory_get_widget(ifactory, mainwindow_tools_sep.path);
  gtk_widget_destroy(sep);
  gtk_item_factory_delete_item(ifactory, mainwindow_tools_sep.path);
  widget = gtk_item_factory_get_widget(ifactory, mainwindow_tools_synce.path);
  gtk_widget_destroy(widget);
  gtk_item_factory_delete_item(ifactory, mainwindow_tools_synce.path);
}

static void synce_sync(gpointer callback_data, guint callback_action,
                        GtkWidget *widget)
{
  synce_comp();
}
