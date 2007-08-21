/* Notification plugin for Claws-Mail
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

#include "pluginconfig.h"

#ifdef NOTIFICATION_POPUP

#include <gtk/gtk.h>

#include "mainwindow.h"
#include "procmsg.h"
#include "folder.h"
#include "codeconv.h"
#include "gtk/gtkutils.h"

#include "gettext.h"

#include "notification_popup.h"
#include "notification_prefs.h"
#include "notification_foldercheck.h"
#include "notification_pixbuf.h"

#ifdef HAVE_LIBNOTIFY
#  include <libnotify/notify.h>
#endif

typedef enum {
  F_TYPE_MAIL=0,
  F_TYPE_NEWS,
  F_TYPE_CALENDAR,
  F_TYPE_RSS,
  F_TYPE_LAST
} NotificationFolderType;

typedef struct {
  gint count;
  guint timeout_id;
  gchar *msg_path;
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

G_LOCK_DEFINE_STATIC(popup);

#ifdef HAVE_LIBNOTIFY
static NotificationPopup popup[F_TYPE_LAST];
#else
static NotificationPopup popup;
#endif

static gboolean popup_timeout_fun(gpointer data);

#ifdef HAVE_LIBNOTIFY
static gboolean notification_libnotify_create(MsgInfo*,
					      NotificationFolderType);
static gboolean notification_libnotify_add_msg(MsgInfo*,
					       NotificationFolderType);
static void default_action_cb(NotifyNotification*, const char*,void*);
static gchar* notification_libnotify_sanitize_str(gchar*);
static void notification_libnotify_free_func(gpointer);

#else /* !HAVE_LIBNOTIFY */
static gboolean notification_popup_add_msg(MsgInfo*);
static gboolean notification_popup_create(MsgInfo*);
static gboolean notification_popup_button(GtkWidget*, GdkEventButton*,
					  gpointer);
#endif /* !HAVE_LIBNOTIFY */


void notification_popup_msg(MsgInfo *msginfo)
{
  gboolean retval;
  FolderType ftype;
  NotificationPopup *ppopup;
#if HAVE_LIBNOTIFY
  gchar *uistr;
#endif
  NotificationFolderType nftype;

  nftype = F_TYPE_MAIL;

  if(!msginfo || !notify_config.popup_show)
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

  ftype = msginfo->folder->folder->klass->type;

  G_LOCK(popup);
#ifdef HAVE_LIBNOTIFY
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
      G_UNLOCK(popup);
      return;
    }
    else if(!strcmp(uistr, "vCalendar"))
      nftype = F_TYPE_CALENDAR;
    else if(!strcmp(uistr, "RSSyl"))
      nftype = F_TYPE_RSS;
    else {
      debug_print("Notification Plugin: Unknown folder type %d\n",ftype);
      G_UNLOCK(popup);
      return;
    } 
    break;
  default:
    debug_print("Notification Plugin: Unknown folder type %d\n",ftype);
    G_UNLOCK(popup);
    return;
  }

  ppopup = &(popup[nftype]);
  retval = notification_libnotify_add_msg(msginfo, nftype);
#else /* !HAVE_LIBNOTIFY */
  ppopup = &popup;
  retval = notification_popup_add_msg(msginfo);
#endif /* !HAVE_LIBNOTIFY */
  /* Renew timeout only when the above call was successful */
  if(retval) {
    if(ppopup->timeout_id)
      g_source_remove(ppopup->timeout_id);
    ppopup->timeout_id = g_timeout_add(notify_config.popup_timeout,
				       popup_timeout_fun,
				       GINT_TO_POINTER(nftype));
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
  NotificationPopup *ppopup;
  NotificationFolderType nftype;

  nftype = GPOINTER_TO_INT(data);

  G_LOCK(popup);
#ifdef HAVE_LIBNOTIFY
  ppopup = &(popup[nftype]);
  if(!notify_notification_close(ppopup->notification, &(ppopup->error))) {
    debug_print("Notification Plugin: Failed to close notification: %s.\n",
		ppopup->error->message);
    /* do I need to g_object_unref()? */
    ppopup->notification = NULL;    
  }
  else {
    g_object_unref(G_OBJECT(ppopup->notification));
    ppopup->notification = NULL;
  }
  g_clear_error(&(ppopup->error));
#else /* !HAVE_LIBNOTIFY */
  ppopup = &popup;
  if(ppopup->window) {
    gtk_widget_destroy(ppopup->window);
    ppopup->window = NULL;
  }
#endif
  ppopup->timeout_id = 0;
  if(ppopup->msg_path) {
    g_free(ppopup->msg_path);
    ppopup->msg_path = NULL;
  }
  ppopup->count = 0;
  G_UNLOCK(popup);
  debug_print("Notification Plugin: Popup closed due to timeout.\n");
  return FALSE;
}

#ifdef HAVE_LIBNOTIFY
static void default_action_cb(NotifyNotification *notification,
			      const char *action,
			      void *user_data)
{
  if(strcmp("default", action))
    return;

  MainWindow *mainwin;
  mainwin = mainwindow_get_mainwindow();
  if(mainwin) {
    NotificationFolderType nftype;

    /* Let mainwindow pop up */
    gtk_window_deiconify(GTK_WINDOW(mainwin->window));
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mainwin->window), FALSE);
    main_window_show(mainwin);
    gtk_window_present(GTK_WINDOW(mainwin->window));
    /* If there is only one new mail message, jump to this message */
    nftype = (NotificationFolderType)GPOINTER_TO_INT(user_data);
    if(nftype == F_TYPE_MAIL) {
      if(popup[F_TYPE_MAIL].count == 1) {
	gchar *select_str;
	G_LOCK(popup);
	select_str = g_strdup(popup[F_TYPE_MAIL].msg_path);
	G_UNLOCK(popup);
	debug_print("Select message %s\n", select_str);
	mainwindow_jump_to(select_str, FALSE);
	g_free(select_str);
      }
    }
  }
}

static gboolean notification_libnotify_create(MsgInfo *msginfo,
					      NotificationFolderType nftype)
{
  GdkPixbuf *pixbuf;
  NotificationPopup *ppopup;
  gchar *summary = NULL;
  gchar *text = NULL;
  gchar *utf8_str = NULL;
  gchar *subj = NULL;
  gchar *from = NULL;

  g_return_val_if_fail(msginfo, FALSE);

  ppopup = &(popup[nftype]);

  /* init libnotify if necessary */
  if(!notify_is_initted()) {
    if(!notify_init("claws-mail")) {
      debug_print("Notification Plugin: Failed to initialize libnotify. "
		  "No popup will be shown.\n");
      return FALSE;
    }
  }

  switch(nftype) {
  case F_TYPE_MAIL:
    summary = _("New Mail message");
    from    = notification_libnotify_sanitize_str(msginfo->from ? 
                                                  msginfo->from : _("(No From)"));
    subj    = notification_libnotify_sanitize_str(msginfo->subject ?
                                                  msginfo->subject : _("(No Subject)"));
    text    = g_strconcat(from,"\n\n",subj,NULL);
    /* Make sure text is valid UTF8 */
    if(!g_utf8_validate(text, -1, NULL)) {
      debug_print("Notification plugin: String is not valid utf8, trying to fix it...\n");
      /* fix it */
      utf8_str = conv_codeset_strdup(text,
				     conv_get_locale_charset_str_no_utf8(),
				     CS_INTERNAL);
      /* check if the fix worked */
      if(utf8_str == NULL || !g_utf8_validate(utf8_str, -1, NULL)) {
	debug_print("Notification plugin: String is still not valid utf8, sanitizing...\n");
	utf8_str = g_malloc(strlen(text)*2+1);
	conv_localetodisp(utf8_str, strlen(text)*2+1, text);
      }
      g_free(text);
    }
    else {
      debug_print("Notification plugin: String is valid utf8\n");
      utf8_str = text;
    }
    if(from) g_free(from);
    if(subj) g_free(subj);
    break;
  case F_TYPE_NEWS:
    summary = _("New News message");
    utf8_str    = g_strdup(_("A new mesesage arrived"));
    break;
  case F_TYPE_CALENDAR:
    summary = _("New Calendar message");
    utf8_str    = g_strdup(_("A new calendar message arrived"));
    break;
  case F_TYPE_RSS:
    summary = _("New RSS feed article");
    utf8_str = g_strdup(_("A new article in a RSS feed arrived"));
    break;
  default:
    summary = _("New unknown message");
    utf8_str = g_strdup(_("Unknown message type arrived"));
    break;
  }

  ppopup->notification = notify_notification_new(summary, utf8_str, NULL, NULL);
  g_free(utf8_str);
  if(ppopup->notification == NULL) {
    debug_print("Notification Plugin: Failed to create a new "
		"notification.\n");
    return FALSE;
  }

  /* Default action */
  notify_notification_add_action(ppopup->notification,
				 "default", "Present main window",
				 (NotifyActionCallback)default_action_cb,
				 GINT_TO_POINTER(nftype),
				 notification_libnotify_free_func);

  /* Icon */
  pixbuf = notification_pixbuf_get_logo_64x64();
  if(pixbuf)
    notify_notification_set_icon_from_pixbuf(ppopup->notification, pixbuf);
  else /* This is not fatal */
    debug_print("Notification plugin: Icon could not be loaded.\n");

  /* Never time out, close is handled manually. */
  notify_notification_set_timeout(ppopup->notification, NOTIFY_EXPIRES_NEVER);

  /* Category */
  notify_notification_set_category(ppopup->notification, "email.arrived");

  /* Show the popup */
  if(!notify_notification_show(ppopup->notification, &(ppopup->error))) {
    debug_print("Notification Plugin: Failed to send notification: %s\n",
		ppopup->error->message);
    g_clear_error(&(ppopup->error));
    g_object_unref(G_OBJECT(ppopup->notification));
    ppopup->notification = NULL;
    return FALSE;
  }

  debug_print("Notification Plugin: Popup created with libnotify.\n");
  ppopup->count = 1;

  /* Store path to message */
  if(nftype == F_TYPE_MAIL) {
    if(msginfo->folder && msginfo->folder) {
      gchar *ident;
      ident = folder_item_get_identifier(msginfo->folder);
      ppopup->msg_path = g_strdup_printf("%s%s%u", ident,G_DIR_SEPARATOR_S,
					 msginfo->msgnum);
      g_free(ident);
    }
    else
      ppopup->msg_path = NULL;
  }

  return TRUE;
}

static gboolean notification_libnotify_add_msg(MsgInfo *msginfo,
					       NotificationFolderType nftype)
{
  gchar *summary;
  gchar *text;
  gboolean retval;
  NotificationPopup *ppopup;

  ppopup = &(popup[nftype]);

  if(!ppopup->notification)
    return notification_libnotify_create(msginfo,nftype);

  ppopup->count++;

  if(ppopup->msg_path) {
    g_free(ppopup->msg_path);
    ppopup->msg_path = NULL;
  }

  switch(nftype) {
  case F_TYPE_MAIL:
    summary = _("Mail message");
    text = g_strdup_printf(ngettext("%d new message arrived",
				    "%d new messages arrived",
				    ppopup->count), ppopup->count);
    break;
  case F_TYPE_NEWS:
    summary = _("News message");
    text = g_strdup_printf( ngettext("%d new message arrived",
                                     "%d new messages arrived",
				     ppopup->count), ppopup->count);
    break;
  case F_TYPE_CALENDAR:
    summary = _("Calendar message");
    text = g_strdup_printf( ngettext("%d new calendar message arrived",
                                     "%d new calendar messages arrived",
				     ppopup->count), ppopup->count);
    break;
  case F_TYPE_RSS:
    summary = _("RSS news feed");
    text = g_strdup_printf( ngettext("%d new article in a RSS feed arrived",
                                     "%d new articles in a RSS feed arrived",
				     ppopup->count), ppopup->count);
    break;
  default:
    /* Should not happen */
    debug_print("Notification Plugin: Unknown folder type ignored\n");
    return FALSE;
  }

  retval = notify_notification_update(ppopup->notification, summary, 
				      text, NULL);
  g_free(text);
  if(!retval) {
    debug_print("Notification Plugin: Failed to update notification.\n");
    return FALSE;
  }

  /* Show the popup */
  if(!notify_notification_show(ppopup->notification, &(ppopup->error))) {
    debug_print("Notification Plugin: Failed to send updated notification: "
		"%s\n",	ppopup->error->message);
    g_clear_error(&(ppopup->error));
    return FALSE;
  }

  debug_print("Notification Plugin: Popup successfully modified "
	      "with libnotify.\n");
  return TRUE;
}

#define STR_MAX_LEN 511
/* Returns a newly allocated string which needs to be freed */
static gchar* notification_libnotify_sanitize_str(gchar *in)
{
  gint i_out;
  gchar tmp_str[STR_MAX_LEN+1];

  if(in == NULL) return NULL;

  i_out = 0;
  while(*in) {
    if(*in == '<') {
      if(i_out+3 >= STR_MAX_LEN) break;
      memcpy(&(tmp_str[i_out]),"&lt;",4);
      in++; i_out += 4;
    }
    else if(*in == '>') {
      if(i_out+3 >= STR_MAX_LEN) break;
      memcpy(&(tmp_str[i_out]),"&gt;",4);
      in++; i_out += 4;
    }
    else if(*in == '&') {
      if(i_out+4 >= STR_MAX_LEN) break;
      memcpy(&(tmp_str[i_out]),"&amp;",5);
      in++; i_out += 5;
    }
    else {
      if(i_out >= STR_MAX_LEN) break;
      tmp_str[i_out++] = *in++;
    }
  }
  tmp_str[i_out] = '\0';
  return strdup(tmp_str);
}

void notification_libnotify_free_func(gpointer data)
{
  if(popup[F_TYPE_MAIL].msg_path) {
    g_free(popup[F_TYPE_MAIL].msg_path);
    popup[F_TYPE_MAIL].msg_path = NULL;
  }
  debug_print("Freed notification data\n");
}

#else /* !HAVE_LIBNOTIFY */
static gboolean notification_popup_add_msg(MsgInfo *msginfo)
{
  gchar *message;
  NotificationPopup *ppopup;

  ppopup = &popup;

  if(!ppopup->window)
    return notification_popup_create(msginfo);

  ppopup->count++;

  if(ppopup->msg_path) {
    g_free(ppopup->msg_path);
    ppopup->msg_path = NULL;
  }

  if(ppopup->label2)
    gtk_widget_destroy(ppopup->label2);

  message = g_strdup_printf(ngettext("%d new message",
				     "%d new messages",
				     ppopup->count), ppopup->count);
  gtk_label_set_text(GTK_LABEL(ppopup->label1), message);
  g_free(message);
  return TRUE;
}

static gboolean notification_popup_create(MsgInfo *msginfo)
{
  GdkColor bg;
  GdkColor fg;
  NotificationPopup *ppopup;

  g_return_val_if_fail(msginfo, FALSE);

  ppopup = &popup;
  
  /* Window */
  ppopup->window = gtkut_window_new(GTK_WINDOW_TOPLEVEL, "notification_popup");
  gtk_window_set_decorated(GTK_WINDOW(ppopup->window), FALSE);
  gtk_window_set_keep_above(GTK_WINDOW(ppopup->window), TRUE);
  gtk_window_set_accept_focus(GTK_WINDOW(ppopup->window), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(ppopup->window), TRUE);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(ppopup->window), TRUE);
  gtk_window_move(GTK_WINDOW(ppopup->window), notify_config.popup_root_x,
		  notify_config.popup_root_y);
  gtk_window_resize(GTK_WINDOW(ppopup->window), notify_config.popup_width, 1);
  if(notify_config.popup_sticky)
    gtk_window_stick(GTK_WINDOW(ppopup->window));
  /* Signals */
  gtk_widget_set_events(ppopup->window,
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(ppopup->window, "button_press_event",
		   G_CALLBACK(notification_popup_button), NULL);

  /* Event box */
  ppopup->event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(ppopup->window), ppopup->event_box);  

  /* Frame */
  ppopup->frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(ppopup->frame), GTK_SHADOW_ETCHED_OUT);
  gtk_container_add(GTK_CONTAINER(ppopup->event_box), ppopup->frame);

  /* Vbox with labels */
  ppopup->vbox = gtk_vbox_new(FALSE, 2);
  gtk_container_set_border_width(GTK_CONTAINER(ppopup->vbox), 5);
  ppopup->label1 = gtk_label_new(msginfo->from ?
                                 msginfo->from : _("(No From)"));
  gtk_box_pack_start(GTK_BOX(ppopup->vbox), ppopup->label1, FALSE, FALSE, 0);

  ppopup->label2 = gtk_label_new(msginfo->subject ?
                                 msginfo->subject : _("(No Subject)"));
  gtk_box_pack_start(GTK_BOX(ppopup->vbox), ppopup->label2, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(ppopup->frame), ppopup->vbox);
  gtk_widget_set_size_request(ppopup->vbox, notify_config.popup_width, -1);

  /* Color */
  if(notify_config.popup_enable_colors) {
    gtkut_convert_int_to_gdk_color(notify_config.popup_color_bg,&bg);
    gtkut_convert_int_to_gdk_color(notify_config.popup_color_fg,&fg);
    gtk_widget_modify_bg(ppopup->event_box,GTK_STATE_NORMAL,&bg);
    gtk_widget_modify_fg(ppopup->label1,GTK_STATE_NORMAL,&fg);
    gtk_widget_modify_fg(ppopup->label2,GTK_STATE_NORMAL,&fg);
  }

  gtk_widget_show_all(ppopup->window);

  ppopup->count = 1;

  if(msginfo->folder && msginfo->folder->name) {
      gchar *ident;
      ident = folder_item_get_identifier(msginfo->folder);
      ppopup->msg_path = g_strdup_printf("%s%s%u", ident,G_DIR_SEPARATOR_S,
					 msginfo->msgnum);
      g_free(ident);
  }

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
      gtk_window_deiconify(GTK_WINDOW(mainwin->window));
      gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mainwin->window), FALSE);
      main_window_show(mainwin);
      gtk_window_present(GTK_WINDOW(mainwin->window));
      /* If there is only one new mail message, jump to this message */
      if(popup.count == 1) {
	gchar *select_str;
	G_LOCK(popup);
	select_str = g_strdup(popup.msg_path);
	G_UNLOCK(popup);
	debug_print("Select message %s\n", select_str);
	mainwindow_jump_to(select_str, FALSE);
	g_free(select_str);
      }
    }
  }
  return TRUE;
}
#endif /* !HAVE_LIBNOTIFY */

#endif /* NOTIFICATION_POPUP */
