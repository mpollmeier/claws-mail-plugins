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
 * shipped with Claws-Mail, copyrighted by the Claws-Mail Team. */

#include "pluginconfig.h"

#ifdef NOTIFICATION_TRAYICON

#include "notification_trayicon.h"
#include "notification_prefs.h"
#include "notification_core.h"
#include "notification_pixbuf.h"
#include "notification_foldercheck.h"

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
#ifdef HAVE_LIBNOTIFY
static void trayicon_toggle_notify_cb(gpointer, guint, GtkWidget*);
#endif

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>

typedef struct {
  gint count;
  gint num_mail;
  gint num_news;
  gint num_calendar;
  gint num_rss;
  guint timeout_id;
  NotifyNotification *notification;
  GError *error;
} NotificationTrayiconPopup;

static NotificationTrayiconPopup popup;

static gboolean notification_trayicon_popup_add_msg(MsgInfo*,
						    NotificationFolderType);
static gboolean notification_trayicon_popup_create(MsgInfo*,
						   NotificationFolderType);
static gboolean popup_timeout_fun(gpointer);
static void notification_trayicon_popup_free_func(gpointer);
static void notification_trayicon_popup_default_action_cb(NotifyNotification*,
							  const char*,void*);
static gchar* notification_trayicon_popup_assemble_summary(void);
static gchar* notification_trayicon_popup_assemble_body(MsgInfo*);
static void   notification_trayicon_popup_count_msgs(NotificationFolderType);

G_LOCK_DEFINE_STATIC(trayicon_popup);

#endif

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
#ifdef HAVE_LIBNOTIFY
  {N_("/Show Trayicon Notifications"), NULL,trayicon_toggle_notify_cb, 0, "<CheckItem>"},
#endif
  {N_("/_Work Offline"),      NULL,trayicon_toggle_offline_cb,0, "<CheckItem>"},
  {"/---",		      NULL,NULL,		      0, "<Separator>"},
  {N_("/E_xit Claws Mail"),   NULL,trayicon_exit_cb,	      0, NULL}
};


void notification_trayicon_msg(MsgInfo *msginfo)
{
#ifndef HAVE_LIBNOTIFY
  return;

#else
  FolderType ftype;
  NotificationFolderType nftype;
  gchar *uistr;

  nftype = F_TYPE_MAIL;

  if(!msginfo || !notify_config.trayicon_enabled ||
     !notify_config.trayicon_popup_enabled ||
     !MSG_IS_NEW(msginfo->flags))
    return;

  if(notify_config.trayicon_popup_folder_specific) {
    guint id;
    GSList *list;
    gchar *identifier;
    gboolean found = FALSE;

    if(!(msginfo->folder))
      return;

    identifier = folder_item_get_identifier(msginfo->folder);

    id =
      notification_register_folder_specific_list
      (TRAYICON_POPUP_SPECIFIC_FOLDER_ID_STR);
    list = notification_foldercheck_get_list(id);
    for(; (list != NULL) && !found; list = g_slist_next(list)) {
      gchar *list_identifier;
      FolderItem *list_item = (FolderItem*) list->data;
      
      list_identifier = folder_item_get_identifier(list_item);
      if(!strcmp2(list_identifier, identifier))
	found = TRUE;

      g_free(list_identifier);
    }
    g_free(identifier);
    
    if(!found)
      return;
  } /* folder specific */
  
  ftype = msginfo->folder->folder->klass->type;

  G_LOCK(trayicon_popup);
  /* Check out which type to notify about */
  switch(ftype) {
  case F_MH:
  case F_MBOX:
  case F_MAILDIR:
  case F_IMAP:
    nftype = F_TYPE_MAIL;
    break;
  case F_NEWS:
    nftype = F_TYPE_NEWS;
    break;
  case F_UNKNOWN:
    if((uistr = msginfo->folder->folder->klass->uistr) == NULL) {
      G_UNLOCK(trayicon_popup);
      return;
    }
    else if(!strcmp(uistr, "vCalendar"))
      nftype = F_TYPE_CALENDAR;
    else if(!strcmp(uistr, "RSSyl"))
      nftype = F_TYPE_RSS;
    else {
      debug_print("Notification Plugin: Unknown folder type %d\n",ftype);
      G_UNLOCK(trayicon_popup);
      return;
    } 
    break;
  default:
    debug_print("Notification Plugin: Unknown folder type %d\n",ftype);
    G_UNLOCK(trayicon_popup);
    return;
  }

  
  if(notification_trayicon_popup_add_msg(msginfo, nftype)) {
    if(popup.timeout_id)
      g_source_remove(popup.timeout_id);
    popup.timeout_id = g_timeout_add(notify_config.trayicon_popup_timeout,
				     popup_timeout_fun, NULL);
  }

  G_UNLOCK(trayicon_popup);

#endif /* HAVE_LIBNOTIFY */
}

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
  if(!notify_config.trayicon_enabled)
    return FALSE;

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

  if(!notify_config.trayicon_enabled)
    return FALSE;

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

  if(!notify_config.trayicon_enabled)
    return FALSE;

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
  /* initialize checkitems according to current states */
  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item
			 (traymenu_factory, "/Work Offline")),
     prefs_common.work_offline);
  gtk_check_menu_item_set_active
    (GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item
			 (traymenu_factory, "/Show Trayicon Notifications")),
     notify_config.trayicon_popup_enabled);

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

static void trayicon_addressbook_cb(gpointer data, guint action,
				    GtkWidget *widget)
{
  addressbook_open(NULL);
}

static void trayicon_toggle_offline_cb(gpointer data, guint action,
				       GtkWidget *widget)
{
  /* toggle offline mode if menu checkitem has been clicked */
  if(!updating_menu) {
    MainWindow *mainwin = mainwindow_get_mainwindow();
    main_window_toggle_work_offline(mainwin, !prefs_common.work_offline, TRUE);
  }
}

#ifdef HAVE_LIBNOTIFY
static void trayicon_toggle_notify_cb(gpointer data, guint action,
				      GtkWidget *widget)
{
  if(!updating_menu) {
    notify_config.trayicon_popup_enabled = !notify_config.trayicon_popup_enabled;
  }
}
#endif

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

#ifdef HAVE_LIBNOTIFY
static gboolean notification_trayicon_popup_add_msg(MsgInfo *msginfo,
						    NotificationFolderType nftype)
{
  gchar *summary;
  gchar *utf8_str;
  gboolean retval;

  g_return_val_if_fail(msginfo, FALSE);

  if(!popup.notification)
    return notification_trayicon_popup_create(msginfo,nftype);

  /* Count messages */
  notification_trayicon_popup_count_msgs(nftype);

  summary  = notification_trayicon_popup_assemble_summary();
  utf8_str = notification_trayicon_popup_assemble_body(msginfo);
  retval = notify_notification_update(popup.notification, summary, 
				      utf8_str, NULL);
  g_free(summary);
  g_free(utf8_str);
  if(!retval) {
    debug_print("Notification Plugin: Failed to update notification.\n");
    return FALSE;
  }
  /* Show the popup */
  if(!notify_notification_show(popup.notification, &(popup.error))) {
    debug_print("Notification Plugin: Failed to send updated notification: "
		"%s\n",	popup.error->message);
    g_clear_error(&(popup.error));
    return FALSE;
  }
  
  debug_print("Notification Plugin: Popup successfully modified "
	      "with libnotify.\n");

  return TRUE;
}

static gboolean notification_trayicon_popup_create(MsgInfo *msginfo,
						   NotificationFolderType nftype)
{
  gchar *summary = NULL;
  gchar *utf8_str = NULL;
  GdkPixbuf *pixbuf;

  /* init libnotify if necessary */
  if(!notify_is_initted()) {
    if(!notify_init("claws-mail")) {
      debug_print("Notification Plugin: Failed to initialize libnotify. "
		  "No popups will be shown.\n");
      return FALSE;
    }
  }

  /* Count messages */
  notification_trayicon_popup_count_msgs(nftype);

  summary  = notification_trayicon_popup_assemble_summary();
  utf8_str = notification_trayicon_popup_assemble_body(msginfo);

  popup.notification = notify_notification_new(summary, utf8_str, NULL, NULL);
  notify_notification_attach_to_status_icon(popup.notification, trayicon);

  g_free(summary);
  g_free(utf8_str);

  /* Default action */
  notify_notification_add_action(popup.notification,
				 "default", "Present main window",
				 (NotifyActionCallback)
				 notification_trayicon_popup_default_action_cb,
				 NULL,
				 notification_trayicon_popup_free_func);

  if(popup.notification == NULL) {
    debug_print("Notification Plugin: Failed to create a new notification.\n");
    return FALSE;
  }

  /* Icon */
  pixbuf = notification_pixbuf_get(NOTIFICATION_CM_LOGO_64x64);
  if(pixbuf)
    notify_notification_set_icon_from_pixbuf(popup.notification, pixbuf);
  else /* This is not fatal */
    debug_print("Notification plugin: Icon could not be loaded.\n");

  /* Never time out, close is handled manually. */
  notify_notification_set_timeout(popup.notification, NOTIFY_EXPIRES_NEVER);

  /* Category */
  notify_notification_set_category(popup.notification, "email.arrived");

  /* Show the popup */
  if(!notify_notification_show(popup.notification, &(popup.error))) {
    debug_print("Notification Plugin: Failed to send notification: %s\n",
		popup.error->message);
    g_clear_error(&(popup.error));
    g_object_unref(G_OBJECT(popup.notification));
    popup.notification = NULL;
    return FALSE;
  }

  debug_print("Notification Plugin: Popup created with libnotify.\n");

  return TRUE;
}

static gboolean popup_timeout_fun(gpointer data)
{
  G_LOCK(trayicon_popup);

  if(!notify_notification_close(popup.notification, &(popup.error))) {
    debug_print("Notification Plugin: Failed to close notification: %s.\n",
		popup.error->message);
    /* do I need to g_object_unref()? */
  }
  else {
    g_object_unref(G_OBJECT(popup.notification));
    debug_print("Notification Plugin: Popup closed due to timeout.\n");
  }
  popup.notification = NULL;    
  g_clear_error(&(popup.error));

  popup.timeout_id = 0;

  popup.count = 0;
  popup.num_mail = 0;
  popup.num_news = 0;
  popup.num_calendar = 0;
  popup.num_rss = 0;

  G_UNLOCK(trayicon_popup);

  return FALSE;
}

static void notification_trayicon_popup_free_func(gpointer data)
{
  debug_print("Freed notification data\n");
}

static void notification_trayicon_popup_default_action_cb(NotifyNotification
							  *notification,
							  const char *action,
							  void *user_data)
{
  if(strcmp("default", action))
    return;

  return;
}

static void notification_trayicon_popup_count_msgs(NotificationFolderType nftype)
{
  switch(nftype) {
  case F_TYPE_MAIL:
    popup.num_mail++;
    break;
  case F_TYPE_NEWS:
    popup.num_news++;
    break;
  case F_TYPE_CALENDAR:
    popup.num_calendar++;
    break;
  case F_TYPE_RSS:
    popup.num_rss++;
    break;
  default:
    debug_print("Notification plugin: Unknown folder type\n");
    return;
  }
  popup.count++;
}

/* The returned value has to be freed by the caller */
static gchar* notification_trayicon_popup_assemble_summary(void)
{
  gchar *summary = NULL;

  if(popup.count == 1) {
    if(popup.num_mail)
      summary = g_strdup(_("New mail message"));
    else if(popup.num_news)
      summary = g_strdup(_("New news post"));
    else if(popup.num_calendar)
      summary = g_strdup(_("New calendar message"));
    else
      summary = g_strdup(_("New article in RSS feed"));
  } /* One new message */
  else {
    summary = g_strdup(_("New messages arrived"));
  } /* Many new messages */

  return summary;
}

/* The returned value has to be freed by the caller */
static gchar* notification_trayicon_popup_assemble_body(MsgInfo *msginfo)
{
  gchar *utf8_str;

  if(popup.count == 1) {
    if(popup.num_mail || popup.num_news) {
      gchar *from;
      gchar *subj;
      gchar *text;
      
      from = notification_libnotify_sanitize_str(msginfo->from ? 
						 msginfo->from :
						 _("(No From)"));
      subj = notification_libnotify_sanitize_str(msginfo->subject ?
						 msginfo->subject :
						 _("(No Subject)"));
      text = g_strconcat(from,"\n\n",subj,NULL);

      /* Make sure text is valid UTF8 */
      utf8_str = notification_validate_utf8_str(text);
      g_free(text);

      if(from) g_free(from);
      if(subj) g_free(subj);
    }
    else if(popup.num_calendar) {
      utf8_str = g_strdup(_("A new calendar message arrived"));
    }
    else {
      utf8_str = g_strdup(_("A new article in a RSS feed arrived"));
    }
  } /* One message */

  else {
    gchar *msg;
    gchar *tmp;
    gboolean str_empty = TRUE;

    utf8_str = g_strdup("");

    if(popup.num_mail) {
      msg = g_strdup_printf(_("%d new mail message arrived"),
			    popup.num_mail);
      tmp = g_strdup_printf("%s%s%s",utf8_str,str_empty?"":"\n",msg);
      g_free(msg);
      g_free(utf8_str);
      utf8_str = tmp;
      str_empty = FALSE;
    }
    if(popup.num_news) {
      msg = g_strdup_printf(_("%d new news posts arrived"),
			    popup.num_news);
      tmp = g_strdup_printf("%s%s%s",utf8_str,str_empty?"":"\n",msg);
      g_free(msg);
      g_free(utf8_str);
      utf8_str = tmp;
      str_empty = FALSE;
    }
    if(popup.num_calendar) {
      msg = g_strdup_printf(_("%d new calendar messages arrived"),
			    popup.num_calendar);
      tmp = g_strdup_printf("%s%s%s",utf8_str,str_empty?"":"\n",msg);
      g_free(msg);
      g_free(utf8_str);
      utf8_str = tmp;
      str_empty = FALSE;
    }
    if(popup.num_rss) {
      msg = g_strdup_printf(_("%d new articles in RSS feeds arrived"),
			    popup.num_rss);
      tmp = g_strdup_printf("%s%s%s",utf8_str,str_empty?"":"\n",msg);
      g_free(msg);
      g_free(utf8_str);
      utf8_str = tmp;
      str_empty = FALSE;
    }
  } /* Many messages */

  return utf8_str;
}

#endif /* HAVE_LIBNOTIFY */

#endif /* NOTIFICATION_TRAYICON */