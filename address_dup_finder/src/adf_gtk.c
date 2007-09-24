/* Address duplicate finder plugin for Claws-Mail
 * Copyright (C) 2007 Holger Berndt
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

#include "adf_gtk.h"
#include "adf_prefs.h"
#include "adf_finder.h"

#include "main.h"
#include "mainwindow.h"
#include "alertpanel.h"
#include "gtkutils.h"
#include "addritem.h"

#include "gettext.h"

static void     adf_find(gpointer, guint, GtkWidget*);
static gboolean adf_create_dialog(void);

static void append_to_email_store(gpointer,gpointer,gpointer);
static GtkWidget* create_email_view(GtkListStore*);
static GtkWidget* create_detail_view(GtkListStore*);
static void email_selection_changed(GtkTreeSelection*,gpointer);

static void cb_dialog_destroy(GtkWindow*, gpointer);

static void cb_delete_clicked(GtkButton*, gpointer);

enum {
  COL_BOOK = 0,
  COL_NAME,
  NUM_COLS
};

static GtkItemFactoryEntry mainwindow_tools_adf = {
  N_("/Tools/Find addressbook duplicates"),
  NULL,
  adf_find,
  0,
  NULL
};

static GtkListStore *detail_store;

void adf_gtk_init(void)
{
  GtkItemFactory *ifactory;
  MainWindow *mainwin;

  mainwin =  mainwindow_get_mainwindow();

  ifactory = gtk_item_factory_from_widget(mainwin->menubar);
  gtk_item_factory_create_item(ifactory, &mainwindow_tools_adf, mainwin, 1);
}

void adf_gtk_done(void)
{
  GtkItemFactory *ifactory;
  MainWindow *mainwin;
  GtkWidget *widget;

  mainwin = mainwindow_get_mainwindow();

  if (mainwin == NULL || claws_is_exiting())
	  return;
 
  ifactory = gtk_item_factory_from_widget(mainwin->menubar);
  widget = gtk_item_factory_get_widget(ifactory, mainwindow_tools_adf.path);
  gtk_widget_destroy(widget);
  gtk_item_factory_delete_item(ifactory, mainwindow_tools_adf.path);
}

static void adf_find(gpointer callback_data, guint callback_action,
		     GtkWidget *widget)
{
  if(adf_create_dialog())
    adf_finder_find();
}

static void email_selection_changed(GtkTreeSelection *selection, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *email;
  GHashTable *hash = (GHashTable*)data;

  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    GSList *hashval;
    GSList *walk;

    gtk_tree_model_get(model, &iter, 0, &email, -1);

    hashval = (GSList*) g_hash_table_lookup(hash, email);
    gtk_list_store_clear(detail_store);
    for(walk = hashval; walk; walk = walk->next) {
      AdfListEntry *entry = (AdfListEntry*)walk->data;
      if(!entry)
	continue;
      gtk_list_store_append(detail_store, &iter);
      gtk_list_store_set(detail_store, &iter,
			 COL_BOOK, entry->book,
			 COL_NAME, ADDRITEM_NAME(entry->person),
			 -1);

    }
    
    g_free(email);
  }
}

void adf_present_finder_results(GHashTable *hash)
{
  GtkListStore *email_store;
  GtkWidget *email_view;
  GtkWidget *detail_view;
  GtkWidget *dialog = NULL;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hpaned;
  GtkWidget *close;
  GtkWidget *delete;
  GtkTreeSelection *email_select;
  GtkTreeSelection *detail_select;
  static GdkGeometry geometry;

  if(g_hash_table_size(hash) == 0) {
    alertpanel_notice(_("No duplicate email addresses in the addressbook found"));
    return;
  }

  email_store = gtk_list_store_new(1, G_TYPE_STRING);
  g_hash_table_foreach(hash,append_to_email_store,email_store);
  email_view = create_email_view(email_store);
  email_select = gtk_tree_view_get_selection(GTK_TREE_VIEW(email_view));
  gtk_tree_selection_set_mode(email_select,GTK_SELECTION_SINGLE);

  g_signal_connect(email_select, "changed",
		   (GCallback)email_selection_changed, hash);

  detail_store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);
  detail_view = create_detail_view(detail_store);
  detail_select = gtk_tree_view_get_selection(GTK_TREE_VIEW(detail_view));
  gtk_tree_selection_set_mode(email_select,GTK_SELECTION_SINGLE);

  dialog = gtkut_window_new(GTK_WINDOW_TOPLEVEL, "address_dupes_finder");
  if(!geometry.min_height) {
    geometry.min_width = 600;
    geometry.min_height = 400;
  }
  gtk_window_set_geometry_hints(GTK_WINDOW(dialog), NULL, &geometry,
				GDK_HINT_MIN_SIZE);
  gtk_window_set_title(GTK_WINDOW(dialog), _("Duplicate email addresses"));


  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(dialog), vbox);

  hpaned = gtk_hpaned_new();
  gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);

  gtk_paned_add1(GTK_PANED(hpaned), email_view);
  gtk_paned_add2(GTK_PANED(hpaned), detail_view);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_box_pack_end(GTK_BOX(hbox), close, FALSE, FALSE, 0);

  /*delete = gtk_button_new_from_stock(GTK_STOCK_DELETE);*/
  /*gtk_box_pack_end(GTK_BOX(hbox), delete, FALSE, FALSE, 10);*/

  g_signal_connect(dialog, "destroy",
		   G_CALLBACK(cb_dialog_destroy), hash);
  g_signal_connect_swapped(close, "clicked",
			   G_CALLBACK(gtk_widget_destroy), dialog);
  /*g_signal_connect(delete, "clicked",
    G_CALLBACK(cb_delete_clicked), NULL);*/

  gtk_widget_show_all(dialog);
}

static void append_to_email_store(gpointer key,gpointer value,gpointer data)
{
  GtkTreeIter iter;
  GtkListStore *store = (GtkListStore*) data;

  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, 0, (gchar*) key, -1);
}

static GtkWidget* create_detail_view(GtkListStore *store)
{
  GtkWidget *view;
  GtkCellRenderer *renderer;

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  renderer = gtk_cell_renderer_text_new();

  /* col 1 */
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
					      -1,
					      _("Bookname"),
					      renderer,
					      "text", COL_BOOK,
					      NULL);
  /* col 2 */
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
					      -1,
					      _("Name"),
					      renderer,
					      "text", COL_NAME,
					      NULL);

  return view;
}

static GtkWidget* create_email_view(GtkListStore *store)
{
  GtkWidget *view;
  GtkCellRenderer *renderer;

  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view),
					      -1,
					      _("Address"),
					      renderer,
					      "text", 0,
					      NULL);
  g_object_unref(store);
  return view;
}



static gboolean adf_create_dialog(void)
{
  gboolean want_search;
  GtkWidget *vbox;
  GtkWidget *check_same_book;
  GtkWidget *check_other_book;
  AlertValue val;

  want_search = FALSE;

  vbox = gtk_vbox_new(FALSE, 0);
  check_same_book = gtk_check_button_new_with_label(_("Show duplicates in "
						      "the same book"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_same_book),
			       adf_config.include_same_book);
  gtk_box_pack_start(GTK_BOX(vbox), check_same_book, FALSE, FALSE, 0);
  gtk_widget_show(check_same_book);
  check_other_book = gtk_check_button_new_with_label(_("Show duplicates in "
						       "different books"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_other_book),
			       adf_config.include_other_books);
  gtk_box_pack_start(GTK_BOX(vbox), check_other_book, FALSE, FALSE, 0);
  gtk_widget_show(check_other_book);

  /* prevent checkboxes from being destroyed on dialog close */
  g_object_ref(check_same_book);
  g_object_ref(check_other_book);

  val = alertpanel_full(_("Address book email duplicate finder"),
			_("The plugin will now search for duplicate email "
			  "addresses in the addressbook."),
			_("Cancel"),_("Search"),NULL, FALSE, vbox, ALERT_NOTICE,
			G_ALERTALTERNATE);
  if(val == G_ALERTALTERNATE) {
    want_search = TRUE;

    /* save options */
    adf_config.include_same_book = 
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_same_book));
    adf_config.include_other_books = 
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_other_book));

  }

  g_object_unref(check_same_book);
  g_object_unref(check_other_book);
  return want_search;
}

static void cb_dialog_destroy(GtkWindow *window, gpointer data)
{
  g_hash_table_destroy((GHashTable*)data);
}

static void cb_delete_clicked(GtkButton *button, gpointer data)
{
  g_print("delete clicked\n");
}
