/*
 * perl_plugin -- Perl Support for Sylpheed-Claws
 *
 * Copyright (C) 2004-2005   Holger Berndt
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

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "common/utils.h"
#include "mainwindow.h"
#include "prefs_common.h"

#include "perl_plugin.h"
#include "perl_gtk.h"

static void perl_filter_edit(gpointer, guint, GtkWidget*);


static GtkItemFactoryEntry mainwindow_tools_perl_edit = {
	N_("/Tools/Edit perl filter rules (ext)"),
	NULL,
	perl_filter_edit,
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


void perl_gtk_init(void)
{
  GtkItemFactory *ifactory;
  MainWindow *mainwin;

  mainwin =  mainwindow_get_mainwindow();

  ifactory = gtk_item_factory_from_widget(mainwin->menubar);
  gtk_item_factory_create_item(ifactory, &mainwindow_tools_sep, mainwin, 1);
  gtk_item_factory_create_item(ifactory, &mainwindow_tools_perl_edit, mainwin, 1);
}

void perl_gtk_done(void)
{
  GtkItemFactory *ifactory;
  MainWindow *mainwin;
  GtkWidget *widget;
  GtkWidget *sep;

  mainwin = mainwindow_get_mainwindow();

  if(mainwin && !sylpheed_is_exiting()) {
    ifactory = gtk_item_factory_from_widget(mainwin->menubar);
    sep = gtk_item_factory_get_widget(ifactory, mainwindow_tools_sep.path);
    gtk_widget_destroy(sep);
    gtk_item_factory_delete_item(ifactory, mainwindow_tools_sep.path);
    widget = gtk_item_factory_get_widget(ifactory,
					 mainwindow_tools_perl_edit.path);
    gtk_widget_destroy(widget);
    gtk_item_factory_delete_item(ifactory, mainwindow_tools_perl_edit.path);
  }
}

static void perl_filter_edit(gpointer callback_data, guint callback_action,
                        GtkWidget *widget)
{
  gchar *perlfilter;
  gchar *pp;
  gchar buf[1024];
  gchar **cmdline;

  perlfilter = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, PERLFILTER, NULL);
  if (prefs_common.ext_editor_cmd &&
      (pp = strchr(prefs_common.ext_editor_cmd, '%')) &&
      *(pp + 1) == 's' && !strchr(pp + 2, '%')) {
    g_snprintf(buf, sizeof(buf), prefs_common.ext_editor_cmd, perlfilter);
  }
  else {
    if (prefs_common.ext_editor_cmd)
      g_warning("Perl Plugin: External editor command line is invalid: `%s'",
		prefs_common.ext_editor_cmd);
    g_snprintf(buf, sizeof(buf), "emacs %s", perlfilter);
  }
  g_free(perlfilter);
  cmdline = strsplit_with_quote(buf, " ", 1024);
  execute_detached(cmdline);
  g_strfreev(cmdline);
}
