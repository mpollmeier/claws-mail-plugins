/* SynCE plugin -- SynCE Support for Claws Mail
 *
 * Copyright (C) 2005-2007 Holger Berndt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "main.h"
#include "mainwindow.h"
#include "menu.h"

#include "synce_comp.h"

static void synce_sync(GtkAction *action, gpointer data);


static GtkActionEntry mainwindow_tools_synce[] = {{
	"Tools/SynCE",
	NULL, N_("SynCE"), NULL, NULL, G_CALLBACK(synce_sync)
}};

static gint main_menu_id = 0;

void synce_gtk_init(void)
{
  GtkItemFactory *ifactory;
  MainWindow *mainwin;

  mainwin =  mainwindow_get_mainwindow();

	gtk_action_group_add_actions(mainwin->action_group, mainwindow_tools_synce,
			1, (gpointer)mainwin);
	MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menu/Tools", "SynCE", 
			  "Tools/SynCE", GTK_UI_MANAGER_MENUITEM,
			  main_menu_id)
}

void synce_gtk_done(void)
{
  MainWindow *mainwin;

  mainwin = mainwindow_get_mainwindow();

  if (mainwin == NULL || claws_is_exiting())
	  return;
 
	MENUITEM_REMUI_MANAGER(mainwin->ui_manager,mainwin->action_group, "Tools/SynCE", main_menu_id);
	main_menu_id = 0;
}

static void synce_sync(GtkAction *action, gpointer data)
{
  synce_comp();
}
