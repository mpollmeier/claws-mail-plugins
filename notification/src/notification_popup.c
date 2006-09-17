/* Notification Plugin for Sylpheed-Claws
 * Copyright (C) 2005-2006 Holger Berndt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
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

#include "pluginconfig.h"

#ifdef NOTIFICATION_POPUP

#include <gtk/gtk.h>

#include "mainwindow.h"
#include "procmsg.h"
#include "gtk/gtkutils.h"

#include "notification_popup.h"
#include "notification_prefs.h"
#include "notification_foldercheck.h"
#include "notification_pixbuf.h"

#ifdef HAVE_LIBNOTIFY
#  include <libnotify/notify.h>
#endif


typedef struct {
  gint count;
  guint timeout_id;
#ifdef HAVE_LIBNOTIFY
  NotifyNotification *notification;
  GError *error;
#else /* !HAVE_LIBNOTIFY */
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *event_box;
  GtkWidget *vbox;
  GtkWidget *label1;
  GtkWidget *label2;
#endif
} NotificationPopup;

static NotificationPopup popup = {
  0,
  0,
#ifdef HAVE_LIBNOTIFY
  NULL,
  NULL
#else /* !HAVE_LIBNOTIFY */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL  
#endif
};

G_LOCK_DEFINE_STATIC(popup);

static gboolean popup_timeout_fun(gpointer data);

#ifdef HAVE_LIBNOTIFY
static gboolean notification_libnotify_create(MsgInfo*);
static gboolean notification_libnotify_add_msg(MsgInfo*);
#else /* !HAVE_LIBNOTIFY */
static gboolean notification_popup_add_msg(MsgInfo*);
static gboolean notification_popup_create(MsgInfo*);
static gboolean notification_popup_button(GtkWidget*, GdkEventButton*,
					  gpointer);
#endif /* !HAVE_LIBNOTIFY */


void notification_popup_msg(MsgInfo *msginfo)
{
  gboolean retval;

  if(!msginfo || !notify_config.popup_show || !MSG_IS_NEW(msginfo->flags))
    return;

  if(notify_config.popup_folder_specific) {
    guint id;
    GSList *list;
    gchar *identifier;
    gboolean found = FALSE;

    if(!(msginfo->folder))
      return;

    identifier = folder_item_get_identifier(msginfo->folder);
      
    id =
      notification_register_folder_specific_list(POPUP_SPECIFIC_FOLDER_ID_STR);
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
  }

  G_LOCK(popup);
#ifdef HAVE_LIBNOTIFY
  if(popup.notification)
    retval = notification_libnotify_add_msg(msginfo);
  else
    retval = notification_libnotify_create(msginfo);
#else /* !HAVE_LIBNOTIFY */
  if(popup.window)
    retval = notification_popup_add_msg(msginfo);
  else
    retval = notification_popup_create(msginfo);
#endif /* !HAVE_LIBNOTIFY */
  /* Renew timeout only when the above call was successful */
  if(retval) {
    if(popup.timeout_id)
      g_source_remove(popup.timeout_id);
    popup.timeout_id = g_timeout_add(notify_config.popup_timeout,
				     popup_timeout_fun, NULL);
  }
  G_UNLOCK(popup);

#ifndef HAVE_LIBNOTIFY
  /* GUI update */
  while(gtk_events_pending())
    gtk_main_iteration();
#endif /* !HAVE_LIBNOTIFY */
  
}

static gboolean popup_timeout_fun(gpointer data)
{
  G_LOCK(popup);
#ifdef HAVE_LIBNOTIFY
  if(!notify_notification_close(popup.notification, &(popup.error))) {
    debug_print("Notification Plugin: Failed to close notification: %s.\n",
		popup.error->message);
  }
  else {
    g_object_unref(G_OBJECT(popup.notification));
    popup.notification = NULL;
  }
  g_clear_error(&(popup.error));
#else /* !HAVE_LIBNOTIFY */
  if(popup.window) {
    gtk_widget_destroy(popup.window);
    popup.window = NULL;
  }
#endif
  popup.timeout_id = 0;
  popup.count = 0;
  G_UNLOCK(popup);
  debug_print("Notification Plugin: Popup closed due to timeout.\n");
  return FALSE;
}

#ifdef HAVE_LIBNOTIFY
static gboolean notification_libnotify_create(MsgInfo *msginfo)
{
  GdkPixbuf *pixbuf;

  /* init libnotify if necessary */
  if(!notify_is_initted()) {
    if(!notify_init("sylpheed-claws")) {
      debug_print("Notification Plugin: Failed to initialize libnotify. "
		  "No popup will be shown.\n");
      return FALSE;
    }
  }

  popup.notification = notify_notification_new("Sylpheed-Claws", 
					       "A new message arrived.",
					       NULL, NULL);
  if(popup.notification == NULL) {
    debug_print("Notification Plugin: Failed to create a new "
		"notification.\n");
    return FALSE;
  }

  /* Icon */
  pixbuf = notification_pixbuf_get_logo_64x64();
  if(pixbuf)
    notify_notification_set_icon_from_pixbuf(popup.notification, pixbuf);
  else /* This is not fatal */
    debug_print("Notification plugin: Icon could not be loaded.\n");

  /* Never time out, close is handled manually. */
  notify_notification_set_timeout(popup.notification, NOTIFY_EXPIRES_NEVER);

  /* Category */
  notify_notification_set_category(popup.notification, "email.arrived");

  /* hhb: todo: close handler / default action */

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
  popup.count = 1;
  return TRUE;
}

static gboolean notification_libnotify_add_msg(MsgInfo *msginfo)
{
  gchar *message;
  gboolean retval;

  popup.count++;

  message = g_strdup_printf("%d new messages arrived", popup.count);
  retval = notify_notification_update(popup.notification, "Sylpheed-Claws", 
				      message, NULL);
  g_free(message);
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

#else /* !HAVE_LIBNOTIFY */
static gboolean notification_popup_add_msg(MsgInfo *msginfo)
{
  gchar *message;

  popup.count++;

  if(popup.label2)
    gtk_widget_destroy(popup.label2);

  message = g_strdup_printf("%d new messages", popup.count);
  gtk_label_set_text(GTK_LABEL(popup.label1), message);
  g_free(message);
  return TRUE;
}

static gboolean notification_popup_create(MsgInfo *msginfo)
{
  GdkColor bg;
  GdkColor fg;

  /* Window */
  popup.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(popup.window), FALSE);
  gtk_window_set_keep_above(GTK_WINDOW(popup.window), TRUE);
  gtk_window_set_accept_focus(GTK_WINDOW(popup.window), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(popup.window), TRUE);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(popup.window), TRUE);
  gtk_window_move(GTK_WINDOW(popup.window), notify_config.popup_root_x,
		  notify_config.popup_root_y);
  gtk_window_resize(GTK_WINDOW(popup.window), notify_config.popup_width, 1);
  if(notify_config.popup_sticky)
    gtk_window_stick(GTK_WINDOW(popup.window));
  /* Signals */
  gtk_widget_set_events(popup.window,
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(popup.window, "button_press_event",
		   G_CALLBACK(notification_popup_button), NULL);

  /* Event box */
  popup.event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(popup.window), popup.event_box);  

  /* Frame */
  popup.frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(popup.frame), GTK_SHADOW_ETCHED_OUT);
  gtk_container_add(GTK_CONTAINER(popup.event_box), popup.frame);

  /* Vbox with labels */
  popup.vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width(GTK_CONTAINER(popup.vbox), 5);
  popup.label1 = gtk_label_new(msginfo->from);
  gtk_box_pack_start(GTK_BOX(popup.vbox), popup.label1, FALSE, FALSE, 0);

  popup.label2 = gtk_label_new(msginfo->subject);
  gtk_box_pack_start(GTK_BOX(popup.vbox), popup.label2, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(popup.frame), popup.vbox);
  gtk_widget_set_size_request(popup.vbox, notify_config.popup_width, -1);

  /* Color */
  if(notify_config.popup_enable_colors) {
    gtkut_convert_int_to_gdk_color(notify_config.popup_color_bg,&bg);
    gtkut_convert_int_to_gdk_color(notify_config.popup_color_fg,&fg);
    gtk_widget_modify_bg(popup.event_box,GTK_STATE_NORMAL,&bg);
    gtk_widget_modify_fg(popup.label1,GTK_STATE_NORMAL,&fg);
    gtk_widget_modify_fg(popup.label2,GTK_STATE_NORMAL,&fg);
  }

  gtk_widget_show_all(popup.window);

  popup.count = 1;

  return TRUE;
}

static gboolean notification_popup_button(GtkWidget *widget,
					  GdkEventButton *event,
					  gpointer data)
{
  if(event->type == GDK_BUTTON_PRESS) {
    if(event->button == 1) {
      MainWindow *mainwin;
      /* Let mainwindow pop up */
      mainwin = mainwindow_get_mainwindow();
      if(!mainwin)
	return TRUE;
      gtk_window_present(GTK_WINDOW(mainwin->window));
    }
  }
  return TRUE;
}
#endif /* HAVE_LIBNOTIFY */

#endif /* NOTIFICATION_POPUP */
