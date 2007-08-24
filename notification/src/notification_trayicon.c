/* Notification plugin for Claws-Mail
 * Copyright (C) 2005-2007 Holger Berndt and the Claws Mail Team.
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

/* This module is of course inspired by the trayicon plugin which is
 * shipped with Claws-Mail, copyrighted by the Claws-Mail team. */

#include "pluginconfig.h"

#ifdef NOTIFICATION_TRAYICON

#include "notification_trayicon.h"
#include "notification_prefs.h"
#include "notification_core.h"
#include "notification_pixbuf.h"

#include "gettext.h"

#include "main.h"
#include "mainwindow.h"
#include "prefs_common.h"
#include "alertpanel.h"
#include "menu.h"
#include "gtk/manage_window.h"


static GdkPixbuf* notification_trayicon_create(void);

static void notification_trayicon_on_activate(GtkStatusIcon*, gpointer);
static void notification_trayicon_on_popup_menu(GtkStatusIcon*,guint,
						guint,gpointer);
static gboolean notification_trayicon_on_size_changed(GtkStatusIcon*,
						      gint, gpointer);

static void trayicon_get_all_cb(gpointer, guint, GtkWidget*);
static void trayicon_compose_cb(gpointer, guint, GtkWidget*);
static void trayicon_compose_acc_cb(GtkMenuItem*, gpointer);
static void trayicon_addressbook_cb(gpointer, guint, GtkWidget*);
static void trayicon_exit_cb(gpointer, guint, GtkWidget*);
static void trayicon_toggle_offline_cb(gpointer, guint, GtkWidget*);


static GtkStatusIcon *trayicon;
static gboolean updating_menu = FALSE;
static GtkWidget *traymenu_popup;
static GtkItemFactory *traymenu_factory;

static GtkItemFactoryEntry trayicon_popup_menu_entries[] = {
  {N_("/_Get Mail"),	      NULL,trayicon_get_all_cb,	      0, NULL},
  {"/---",		      NULL,NULL,		      0, "<Separator>"},
  {N_("/_Email"),	      NULL,trayicon_compose_cb,	      0, NULL},
  {N_("/_Email from account"),NULL,NULL,		      0, "<Branch>"},
  {"/---",		      NULL,NULL,		      0, "<Separator>"},
  {N_("/Open A_ddressbook"),  NULL,trayicon_addressbook_cb,   0, NULL},
  {"/---",		      NULL,NULL,		      0, "<Separator>"},
  {N_("/_Work Offline"),      NULL,trayicon_toggle_offline_cb,0, "<CheckItem>"},
  {"/---",		      NULL,NULL,		      0, "<Separator>"},
  {N_("/E_xit Claws Mail"),   NULL,trayicon_exit_cb,	      0, NULL}
};


void notification_trayicon_destroy(void)
{
  if(trayicon) {
    g_object_unref(trayicon);
    trayicon = NULL;
  }
}

void notification_update_trayicon(guint new_msgs, guint unread_msgs,
				  guint unreadmarked_msgs, guint marked_msgs,
				  guint total_msgs)
{
  gchar *buf;
  static GdkPixbuf *old_icon = NULL;
  GdkPixbuf *new_icon;
  gint offset;

  if(!notify_config.trayicon_enabled)
    return;

  if(!trayicon) {
    old_icon = notification_trayicon_create();
    if(!trayicon) {
      debug_print("Notification plugin: Could not create trayicon\n");
      return;
    }
  }

  /* Tooltip */
  buf = g_strdup_printf(_("New %d, Unread: %d, Total: %d"),
			new_msgs, unread_msgs, total_msgs);
  gtk_status_icon_set_tooltip(trayicon, buf);
  g_free(buf);

  /* Pixmap */
  (prefs_common.work_offline) ? (offset = 1) : (offset = 0);

  if((new_msgs > 0) && (unreadmarked_msgs > 0))
    new_icon =
      notification_pixbuf_get(NOTIFICATION_TRAYICON_NEWMARKEDMAIL+offset);
  else if(new_msgs > 0)
    new_icon =
      notification_pixbuf_get(NOTIFICATION_TRAYICON_NEWMAIL+offset);
  else if(unreadmarked_msgs > 0)
    new_icon =
      notification_pixbuf_get(NOTIFICATION_TRAYICON_UNREADMARKEDMAIL+offset);
  else if(unread_msgs > 0)
    new_icon =
      notification_pixbuf_get(NOTIFICATION_TRAYICON_UNREADMAIL+offset);
  else
    new_icon =
      notification_pixbuf_get(NOTIFICATION_TRAYICON_NOMAIL+offset);

  if(new_icon != old_icon) {
    gtk_status_icon_set_from_pixbuf(trayicon, new_icon);
    old_icon = new_icon;
  }
}

gboolean notification_trayicon_main_window_close(gpointer source, gpointer data)
{
  if(source) {
    gboolean *close_allowed = (gboolean*)source;

    if(notify_config.trayicon_close_to_tray) {
      MainWindow *mainwin = mainwindow_get_mainwindow();

      *close_allowed = FALSE;
      if(mainwin && GTK_WIDGET_VISIBLE(GTK_WIDGET(mainwin->window)))
	main_window_hide(mainwin);
    }
  }
  return FALSE;
}

gboolean notification_trayicon_main_window_got_iconified(gpointer source,
							 gpointer data)
{
  MainWindow *mainwin = mainwindow_get_mainwindow();

  if(notify_config.trayicon_hide_when_iconified &&
     mainwin && GTK_WIDGET_VISIBLE(GTK_WIDGET(mainwin->window))
     && !gtk_window_get_skip_taskbar_hint(GTK_WINDOW(mainwin->window))) {
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mainwin->window), TRUE);
  }
  return FALSE;
}

gboolean notification_trayicon_account_list_changed(gpointer source,
						    gpointer data)
{
  GList *cur_ac, *cur_item = NULL;
  GtkWidget *menu;
  GtkWidget *menuitem;
  PrefsAccount *ac_prefs;
  
  GList *account_list = account_get_list();

  menu = gtk_item_factory_get_widget(traymenu_factory,
				     "/Email from account");

  /* destroy all previous menu item */
  cur_item = GTK_MENU_SHELL(menu)->children;
  while(cur_item != NULL) {
    GList *next = cur_item->next;
    gtk_widget_destroy(GTK_WIDGET(cur_item->data));
    cur_item = next;
  }

  for(cur_ac = account_list; cur_ac != NULL; cur_ac = cur_ac->next) {
    ac_prefs = (PrefsAccount *)cur_ac->data;
    
    menuitem = gtk_menu_item_new_with_label
      (ac_prefs->account_name ? ac_prefs->account_name
       : _("Untitled"));
    gtk_widget_show(menuitem);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    g_signal_connect(G_OBJECT(menuitem), "activate",
		     G_CALLBACK(trayicon_compose_acc_cb),
		     ac_prefs);
  }
  return FALSE;
}

static GdkPixbuf* notification_trayicon_create(void)
{
  GdkPixbuf *trayicon_nomail;
  gint n_entries = 0;

  trayicon_nomail = notification_pixbuf_get(NOTIFICATION_TRAYICON_NOMAIL);

  notification_trayicon_destroy();

  trayicon = gtk_status_icon_new_from_pixbuf(trayicon_nomail);

  g_signal_connect(G_OBJECT(trayicon), "activate", 
		   G_CALLBACK(notification_trayicon_on_activate), NULL);
  g_signal_connect(G_OBJECT(trayicon), "popup-menu",
		   G_CALLBACK(notification_trayicon_on_popup_menu), NULL);
  g_signal_connect(G_OBJECT(trayicon), "size-changed",
		   G_CALLBACK(notification_trayicon_on_size_changed), NULL);

  /* Popup-Menu */
  n_entries = sizeof(trayicon_popup_menu_entries) /
    sizeof(trayicon_popup_menu_entries[0]);
  traymenu_popup = menu_create_items(trayicon_popup_menu_entries,
				     n_entries, "<TrayiconMenu>", &traymenu_factory,
				     NULL);

  return trayicon_nomail;
}

/* Hide/show main window */
static void notification_trayicon_on_activate(GtkStatusIcon *status_icon,
					      gpointer user_data)
{
  MainWindow *mainwin;
  if((mainwin = mainwindow_get_mainwindow()) == NULL)
    return;

  if(GTK_WIDGET_VISIBLE(GTK_WIDGET(mainwin->window))) {
    if((gdk_window_get_state(GTK_WIDGET(mainwin->window)->window)&GDK_WINDOW_STATE_ICONIFIED)
       || mainwindow_is_obscured()) {
      gtk_window_deiconify(GTK_WINDOW(mainwin->window));
      gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mainwin->window), FALSE);
      main_window_show(mainwin);
      gtk_window_present(GTK_WINDOW(mainwin->window));
    }
    else {
      main_window_hide(mainwin);
    }
  }
  else {
    gtk_window_deiconify(GTK_WINDOW(mainwin->window));
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mainwin->window), FALSE);
    main_window_show(mainwin);
    gtk_window_present(GTK_WINDOW(mainwin->window));
  }
}

static void notification_trayicon_on_popup_menu(GtkStatusIcon *status_icon,
						guint button, guint activate_time,
						gpointer user_data)
{
  MainWindow *mainwin = mainwindow_get_mainwindow();

  if(!mainwin)
    return;

  /* tell callbacks to skip any event */
  updating_menu = TRUE;
  /* initialize checkitem according to current offline state */
  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item
			 (traymenu_factory, "/Work Offline")),
     prefs_common.work_offline);
  gtk_widget_set_sensitive(GTK_WIDGET(gtk_item_factory_get_item(traymenu_factory,
								"/Get Mail")),
			   mainwin->lock_count == 0);
  updating_menu = FALSE;

  gtk_menu_popup(GTK_MENU(traymenu_popup), NULL, NULL, NULL, NULL,
		 button, activate_time);
}

static gboolean notification_trayicon_on_size_changed(GtkStatusIcon *icon,
						      gint size,
						      gpointer user_data)
{
  notification_update_msg_counts(NULL);
  return FALSE;  
}

/* popup menu callbacks */
static void trayicon_get_all_cb( gpointer data, guint action, GtkWidget *widget )
{
  MainWindow *mainwin = mainwindow_get_mainwindow();
  inc_all_account_mail_cb(mainwin, 0, NULL);
}

static void trayicon_compose_cb( gpointer data, guint action, GtkWidget *widget )
{
  MainWindow *mainwin = mainwindow_get_mainwindow();
  compose_mail_cb(mainwin, 0, NULL);
}

static void trayicon_compose_acc_cb( GtkMenuItem *menuitem, gpointer data )
{
  compose_new((PrefsAccount *)data, NULL, NULL);
}

static void trayicon_addressbook_cb( gpointer data, guint action, GtkWidget *widget )
{
  addressbook_open(NULL);
}

static void trayicon_toggle_offline_cb( gpointer data, guint action, GtkWidget *widget )
{
  /* toggle offline mode if menu checkitem has been clicked */
  if(!updating_menu) {
    MainWindow *mainwin = mainwindow_get_mainwindow();
    main_window_toggle_work_offline(mainwin, !prefs_common.work_offline, TRUE);
  }
}

static void app_exit_cb(MainWindow *mainwin, guint action, GtkWidget *widget)
{
  if(prefs_common.confirm_on_exit) {
    if(alertpanel(_("Exit"), _("Exit Claws Mail?"),
		  GTK_STOCK_CANCEL, GTK_STOCK_OK,
		  NULL) != G_ALERTALTERNATE) {
      return;
    }
    manage_window_focus_in(mainwin->window, NULL, NULL);
  }
  
  app_will_exit(NULL, mainwin);
}

static void trayicon_exit_cb( gpointer data, guint action, GtkWidget *widget )
{
  MainWindow *mainwin = mainwindow_get_mainwindow();
  
  if(mainwin->lock_count == 0) {
    app_exit_cb(mainwin, 0, NULL);
  }
}

#endif /* NOTIFICATION_TRAYICON */
