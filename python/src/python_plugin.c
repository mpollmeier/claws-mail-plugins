/* Python plugin for Claws-Mail
 * Copyright (C) 2009 Holger Berndt
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
#include "pluginconfig.h"

#include <Python.h>

#include "common/hooks.h"
#include "common/plugin.h"
#include "common/version.h"
#include "common/utils.h"
#include "gtk/menu.h"
#include "main.h"
#include "mainwindow.h"

#include "python-shell.h"
#include "python-hooks.h"
#include "clawsmailmodule.h"

#include "gettext.h"

#define PYTHON_SCRIPTS_DIR "python-scripts"


static GSList *menu_id_list = NULL;
static GSList *python_scripts_id_list = NULL;
static GSList *python_scripts_action_names = NULL;

static GtkWidget *python_console = NULL;


static gboolean python_console_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  MainWindow *mainwin;
  GtkToggleAction *action;

  mainwin =  mainwindow_get_mainwindow();
  action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(mainwin->action_group, "Tools/ShowPythonConsole"));
  gtk_toggle_action_set_active(action, FALSE);
  return TRUE;
}

static void setup_python_console(void)
{
  GtkWidget *vbox;
  GtkWidget *console;

  python_console = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request(python_console, 600, 400);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(python_console), vbox);

  console = parasite_python_shell_new();
  gtk_box_pack_start(GTK_BOX(vbox), console, TRUE, TRUE, 0);

  g_signal_connect(python_console, "delete-event", G_CALLBACK(python_console_delete_event), NULL);

  gtk_widget_show_all(python_console);

  parasite_python_shell_focus(PARASITE_PYTHON_SHELL(console));
}

static void show_hide_python_console(GtkToggleAction *action, gpointer callback_data)
{
  if(gtk_toggle_action_get_active(action)) {
    if(!python_console)
      setup_python_console();
    gtk_widget_show(python_console);
  }
  else {
    gtk_widget_hide(python_console);
  }
}

static void remove_python_scripts_menus(void)
{
  GSList *walk;
  MainWindow *mainwin;

  mainwin =  mainwindow_get_mainwindow();

  /* ui */
  for(walk = python_scripts_id_list; walk; walk = walk->next)
      gtk_ui_manager_remove_ui(mainwin->ui_manager, GPOINTER_TO_UINT(walk->data));
  g_slist_free(python_scripts_id_list);
  python_scripts_id_list = NULL;

  /* actions */
  for(walk = python_scripts_action_names; walk; walk = walk->next) {
    GtkAction *action;
    action = gtk_action_group_get_action(mainwin->action_group, walk->data);
    if(action)
      gtk_action_group_remove_action(mainwin->action_group, action);
    g_free(walk->data);
  }
  g_slist_free(python_scripts_action_names);
  python_scripts_action_names = NULL;
}

static void python_script_callback(GtkAction *action, gpointer data)
{
  char *filename;
  FILE *fp;

  filename = g_strrstr(data, "/");
  if(!filename || *(filename+1) == '\0') {
    g_print("Error: Could not extract filename\n");
    return;
  }

  filename++;
  filename = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, PYTHON_SCRIPTS_DIR, G_DIR_SEPARATOR_S, filename, NULL);

  fp = fopen(filename, "r");
  if(!fp) {
    g_print("Error: Could not open file '%s'\n", filename);
    return;
  }
  PyRun_SimpleFile(fp, filename);
  fclose(fp);

  g_free(filename);
}

static void refresh_python_scripts_menu(GtkAction *action, gpointer data)
{
  char *scripts_dir;
  GDir *dir;
  GError *error = NULL;
  const char *filename;
  gint num_entries, ii;
  GSList *filenames = NULL;
  GSList *walk;
  GtkActionEntry *entries;
  MainWindow *mainwin;

  remove_python_scripts_menus();

  scripts_dir = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, PYTHON_SCRIPTS_DIR, NULL);
  debug_print("Refreshing: %s\n", scripts_dir);

  dir = g_dir_open(scripts_dir, 0, &error);
  g_free(scripts_dir);

  if(!dir) {
    g_print("Error opening directory: %s\n", error->message);
    return;
  }

  /* get filenames */
  num_entries = 0;
  while((filename = g_dir_read_name(dir)) != NULL) {
    char *fn;

    fn = g_strdup(filename);
    filenames = g_slist_prepend(filenames, fn);
    num_entries++;
  }
  g_dir_close(dir);

  /* create menu items */
  entries = g_new0(GtkActionEntry, num_entries);
  ii = 0;
  mainwin =  mainwindow_get_mainwindow();
  for(walk = filenames; walk; walk = walk->next) {
    entries[ii].name = g_strconcat("Tools/PythonScripts/", walk->data, NULL);
    entries[ii].label = walk->data;
    entries[ii].callback = G_CALLBACK(python_script_callback);
    gtk_action_group_add_actions(mainwin->action_group, &(entries[ii]), 1, (gpointer)entries[ii].name);
    ii++;
  }
  for(ii = 0; ii < num_entries; ii++) {
    guint id;

    python_scripts_action_names = g_slist_prepend(python_scripts_action_names, (gpointer)entries[ii].name);
    MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menu/Tools/PythonScripts", entries[ii].label,
        entries[ii].name, GTK_UI_MANAGER_MENUITEM, id)
    python_scripts_id_list = g_slist_prepend(python_scripts_id_list, GUINT_TO_POINTER(id));
  }

  /* cleanup */
  g_free(entries);
  for(walk = filenames; walk; walk = walk->next)
    g_free(walk->data);
  g_slist_free(filenames);
}

static GtkToggleActionEntry mainwindow_tools_python_toggle[] = {
    {"Tools/ShowPythonConsole", NULL, N_("Show Python console..."),
        NULL, NULL, G_CALLBACK(show_hide_python_console), FALSE},
};

static GtkActionEntry mainwindow_tools_python_actions[] = {
    {"Tools/PythonScripts", NULL, N_("Python scripts") },
    {"Tools/PythonScripts/Refresh", NULL, N_("Refresh"),
        NULL, NULL, G_CALLBACK(refresh_python_scripts_menu) },
    {"Tools/PythonScripts/---", NULL, "---" },
};

void python_menu_init(void)
{
  MainWindow *mainwin;
  guint id;

  mainwin =  mainwindow_get_mainwindow();

  gtk_action_group_add_toggle_actions(mainwin->action_group, mainwindow_tools_python_toggle, 1, mainwin);
  gtk_action_group_add_actions(mainwin->action_group, mainwindow_tools_python_actions, 3, mainwin);

  MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menu/Tools", "ShowPythonConsole",
      "Tools/ShowPythonConsole", GTK_UI_MANAGER_MENUITEM, id)
  menu_id_list = g_slist_prepend(menu_id_list, GUINT_TO_POINTER(id));

  MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menu/Tools", "PythonScripts",
      "Tools/PythonScripts", GTK_UI_MANAGER_MENU, id)
  menu_id_list = g_slist_prepend(menu_id_list, GUINT_TO_POINTER(id));

  MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menu/Tools/PythonScripts", "Refresh",
      "Tools/PythonScripts/Refresh", GTK_UI_MANAGER_MENUITEM, id)
  menu_id_list = g_slist_prepend(menu_id_list, GUINT_TO_POINTER(id));

  MENUITEM_ADDUI_ID_MANAGER(mainwin->ui_manager, "/Menu/Tools/PythonScripts", "Separator1",
      "Tools/PythonScripts/---", GTK_UI_MANAGER_SEPARATOR, id)
  menu_id_list = g_slist_prepend(menu_id_list, GUINT_TO_POINTER(id));

  refresh_python_scripts_menu(NULL, NULL);
}

void python_menu_done(void)
{
  MainWindow *mainwin;

  mainwin = mainwindow_get_mainwindow();

  if(mainwin && !claws_is_exiting()) {
    GSList *walk;

    remove_python_scripts_menus();

    for(walk = menu_id_list; walk; walk = walk->next)
      gtk_ui_manager_remove_ui(mainwin->ui_manager, GPOINTER_TO_UINT(walk->data));
    MENUITEM_REMUI_MANAGER(mainwin->ui_manager, mainwin->action_group, "Tools/ShowPythonConsole", 0);
    MENUITEM_REMUI_MANAGER(mainwin->ui_manager, mainwin->action_group, "Tools/PythonScripts", 0);
    MENUITEM_REMUI_MANAGER(mainwin->ui_manager, mainwin->action_group, "Tools/PythonScripts/Refresh", 0);
    MENUITEM_REMUI_MANAGER(mainwin->ui_manager, mainwin->action_group, "Tools/PythonScripts/---", 0);
  }
}

gint plugin_init(gchar **error)
{
#ifdef G_OS_UNIX
	bindtextdomain(TEXTDOMAIN, LOCALEDIR);
#else
	bindtextdomain(TEXTDOMAIN, get_locale_dir());
#endif
  bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");

  /* Version check */
  if(!check_plugin_version(MAKE_NUMERIC_VERSION(3,7,3,13),
			   VERSION_NUMERIC, _("Python"), error))
    return -1;

  /* initialize python interpreter */
  Py_Initialize();

  /* initialize python interactive shell */
  parasite_python_init();

  /* initialize Claws Mail Python module */
  claws_mail_python_init();

  /* load menu options */
  python_menu_init();

  debug_print("Python plugin loaded\n");

  return 0;
}

gboolean plugin_done(void)
{
  python_menu_done();

  if(python_console) {
    gtk_widget_destroy(python_console);
    python_console = NULL;
  }

  /* finialize python interpreter */
  Py_Finalize();

  debug_print("Python plugin done and unloaded.\n");
  return FALSE;
}

const gchar *plugin_name(void)
{
  return _("Python");
}

const gchar *plugin_desc(void)
{
  return _("This plugin provides Python integration features.\n"
           "\nFeedback to <berndth@gmx.de> is welcome.");
}

const gchar *plugin_type(void)
{
  return "GTK2";
}

const gchar *plugin_licence(void)
{
  return "GPL3+";
}

const gchar *plugin_version(void)
{
  return PLUGINVERSION;
}

struct PluginFeature *plugin_provides(void)
{
  static struct PluginFeature features[] =
    { {PLUGIN_UTILITY, N_("Python integration")},
      {PLUGIN_NOTHING, NULL}};
  return features;
}
