/* Notification Plugin for Claws Mail
 * Copyright (C) 2005 Holger Berndt
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

#ifdef NOTIFICATION_BANNER

#include <gtk/gtk.h>

#include "gtk/gtkutils.h"

#include "prefs_common.h"
#include "gettext.h"

#include "notification_core.h"
#include "notification_prefs.h"
#include "notification_banner.h"

typedef struct {
  GtkWidget *table;
} NotificationBannerEntry;

typedef struct {
  GtkWidget *window;
  GtkWidget *scrolled_win;
  GtkWidget *event_box;
  GtkWidget *color_ebox;
  GtkWidget *hbox;
  GtkWidget *entrybox;

  NotificationBannerEntry *entries;
  guint timeout_id;
  gboolean scrolling;
} NotificationBanner;

typedef struct {
  gint banner_width;
  GtkAdjustment *adj;
} ScrollingData;


static void       notification_banner_create(GSList*);
static gboolean   scroller(gpointer data);
static GtkWidget* create_entrybox(GSList*);
static gboolean   notification_banner_move_press(GtkWidget*, GdkEventButton*,
						 gpointer);
static gboolean   notification_banner_configure(GtkWidget*, GdkEventConfigure*,
						gpointer);

static NotificationBanner banner;
static ScrollingData      sdata;

/* Corresponding mutexes */
G_LOCK_DEFINE_STATIC(banner);
G_LOCK_DEFINE_STATIC(sdata);


void notification_banner_show(GSList *msg_list)
{
  G_LOCK(banner);
  if((notify_config.banner_show != NOTIFY_BANNER_SHOW_NEVER) &&
     (g_slist_length(msg_list) ||
      (notify_config.banner_show == NOTIFY_BANNER_SHOW_ALWAYS)))
    notification_banner_create(msg_list);
  else 
    notification_banner_destroy();
  G_UNLOCK(banner);
}

void notification_banner_destroy(void)
{
  if(banner.window) {
    if(banner.entries) {
      g_free(banner.entries);
      banner.entries = NULL;
    }
    gtk_widget_destroy(banner.window);
    banner.window = NULL;
    G_LOCK(sdata);
    sdata.adj = NULL;
    sdata.banner_width = 0;
    G_UNLOCK(sdata);
    if(banner.timeout_id) {
      g_source_remove(banner.timeout_id);
      banner.timeout_id = 0;
    }
  }
}

static void notification_banner_create(GSList *msg_list)
{
  GtkRequisition requisition, requisition_after;
  GdkColor bg;

  /* Window */
  if(!banner.window) {
    banner.window = gtkut_window_new(GTK_WINDOW_TOPLEVEL, "notification_banner");
    gtk_window_set_decorated(GTK_WINDOW(banner.window), FALSE);
    gtk_widget_set_size_request(banner.window, gdk_screen_width(), -1);
    gtk_window_set_keep_above(GTK_WINDOW(banner.window), TRUE);
    gtk_window_set_accept_focus(GTK_WINDOW(banner.window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(banner.window), TRUE);
    gtk_window_move(GTK_WINDOW(banner.window),
		    notify_config.banner_root_x, notify_config.banner_root_y);
    g_signal_connect(banner.window, "button-press-event",
		     G_CALLBACK(notification_banner_move_press), NULL);
    g_signal_connect(banner.window, "configure-event",
		     G_CALLBACK(notification_banner_configure), NULL);

  }
  else {
    if(banner.entries) {
      g_free(banner.entries);
      banner.entries = NULL;
    }
    gtk_widget_destroy(banner.event_box);
  }
  if(notify_config.banner_sticky)
    gtk_window_stick(GTK_WINDOW(banner.window));
  else
    gtk_window_unstick(GTK_WINDOW(banner.window));

  /* Event box */
  banner.event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(banner.window), banner.event_box);

  /* Scrolled window */
  banner.scrolled_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(banner.scrolled_win),
				 GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_container_add(GTK_CONTAINER(banner.event_box), banner.scrolled_win);

  banner.color_ebox = gtk_event_box_new();
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW
					(banner.scrolled_win),
					banner.color_ebox);


  /* Hbox */
  banner.hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(banner.color_ebox), banner.hbox);

  /* Entrybox */
  banner.entrybox = create_entrybox(msg_list);
  gtk_box_pack_start(GTK_BOX(banner.hbox), banner.entrybox, FALSE, FALSE, 0);

  /* Color */
  if(notify_config.banner_enable_colors) {
    gtkut_convert_int_to_gdk_color(notify_config.banner_color_bg,&bg);
    gtk_widget_modify_bg(banner.color_ebox,GTK_STATE_NORMAL,&bg);
  }
  
  gtk_widget_show_all(banner.window);

  /* Scrolling */
  gtk_widget_size_request(banner.hbox, &requisition);
  if(requisition.width > gdk_screen_width()) {
    /* Line is too big for screen! */
    /* Double the entrybox into hbox */
    GtkWidget *separator, *second_entrybox;

    separator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(banner.hbox), separator,
		       FALSE, FALSE, 0);
    second_entrybox = create_entrybox(msg_list);
    gtk_box_pack_start(GTK_BOX(banner.hbox), second_entrybox, FALSE, FALSE, 0);

    gtk_widget_show_all(banner.window);
    gtk_widget_size_request(banner.hbox, &requisition_after);

    G_LOCK(sdata);
    sdata.banner_width = requisition_after.width - requisition.width;
    sdata.adj =
      gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW
					  (banner.scrolled_win));
    G_UNLOCK(sdata);
    
    banner.scrolling = TRUE;
    if(banner.timeout_id) {
      g_source_remove(banner.timeout_id);
      banner.timeout_id = 0;
    }
    banner.timeout_id = 
      g_timeout_add(notify_config.banner_speed, scroller, NULL);
  }
  else {
    banner.scrolling = FALSE;
    if(banner.timeout_id) {
      g_source_remove(banner.timeout_id);
      banner.timeout_id = 0;
    }
    G_LOCK(sdata);
    sdata.banner_width = 0;
    sdata.adj = NULL;
    G_UNLOCK(sdata);
  }
}

static gboolean notification_banner_move_press(GtkWidget *widget,
					       GdkEventButton *event,
					       gpointer data)
{
  if(event->type == GDK_BUTTON_PRESS) {
    if(event->button == 1) {
      gtk_window_begin_move_drag(GTK_WINDOW(gtk_widget_get_toplevel(widget)), 
				 event->button, event->x_root, event->y_root,
				 event->time);
    }
    else if(event->button == 2) {
      gint pos_x, pos_y;
      gtk_window_get_position(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
			      &pos_x, &pos_y);
      gtk_window_move(GTK_WINDOW(gtk_widget_get_toplevel(widget)), 0, pos_y);
    }
  }

  return TRUE;
}

static gboolean notification_banner_configure(GtkWidget *widget,
					      GdkEventConfigure *event,
					      gpointer data)
{
  gtk_window_get_position(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
			  &(notify_config.banner_root_x),
			  &(notify_config.banner_root_y));
  return TRUE;
}

static gboolean scroller(gpointer data)
{
  G_LOCK(sdata);
  if(sdata.adj && GTK_IS_ADJUSTMENT(sdata.adj)) {
    if(sdata.adj->value != sdata.banner_width)
      gtk_adjustment_set_value(sdata.adj, sdata.adj->value + 1);
    else
      gtk_adjustment_set_value(sdata.adj, 0);
  }
  G_UNLOCK(sdata);
  return banner.scrolling;
}

static GtkWidget* create_entrybox(GSList *msg_list)
{
  GtkWidget *entrybox;
  GdkColor fg;
  gint list_length;

  list_length = g_slist_length(msg_list);

  if(banner.entries) {
    g_free(banner.entries);
    banner.entries = NULL;
  }

  entrybox = gtk_hbox_new(FALSE, 5);
  if(list_length) {
    GSList *walk;
    gint ii = 0;
    banner.entries = g_new(NotificationBannerEntry, list_length);
    for(walk = msg_list; walk != NULL; walk = g_slist_next(walk)) {
      CollectedMsg *cmsg = walk->data;
      GtkWidget *label1;
      GtkWidget *label2;
      GtkWidget *label3;
      GtkWidget *label4;
      GtkWidget *label5;
      GtkWidget *label6;
      
      if(ii > 0) {
	GtkWidget *separator;
	separator = gtk_vseparator_new();
	gtk_box_pack_start(GTK_BOX(entrybox), separator,
			   FALSE, FALSE, 0);
      }

      banner.entries[ii].table = gtk_table_new(3, 2, FALSE);

      label1 = gtk_label_new(prefs_common.trans_hdr ? _("From:") : "From:");
      gtk_misc_set_alignment(GTK_MISC(label1), 0, 0.5);
      gtk_table_attach_defaults(GTK_TABLE(banner.entries[ii].table), 
				label1, 0, 1, 0, 1);
      label2 = gtk_label_new(prefs_common.trans_hdr ? _("Subject:") : "Subject:");
      gtk_misc_set_alignment(GTK_MISC(label2), 0, 0.5);
      gtk_table_attach_defaults(GTK_TABLE(banner.entries[ii].table),
				label2, 0, 1, 1, 2);
      label3 = gtk_label_new(_("Folder:"));
      gtk_misc_set_alignment(GTK_MISC(label3), 0, 0.5);
      gtk_table_attach_defaults(GTK_TABLE(banner.entries[ii].table),
				label3, 0, 1, 2, 3);
      
      label4 = gtk_label_new(cmsg->from);
      gtk_misc_set_alignment(GTK_MISC(label4), 0, 0.5);
      gtk_table_attach_defaults(GTK_TABLE(banner.entries[ii].table),
				label4, 1, 2, 0, 1);
      label5 = gtk_label_new(cmsg->subject);
      gtk_misc_set_alignment(GTK_MISC(label5), 0, 0.5);
      gtk_table_attach_defaults(GTK_TABLE(banner.entries[ii].table),
				label5, 1, 2, 1, 2);
      label6 = gtk_label_new(cmsg->folderitem_name);
      gtk_misc_set_alignment(GTK_MISC(label6), 0, 0.5);
      gtk_table_attach_defaults(GTK_TABLE(banner.entries[ii].table),
				label6, 1, 2, 2, 3);
      gtk_table_set_col_spacings(GTK_TABLE(banner.entries[ii].table), 5);
      gtk_box_pack_start(GTK_BOX(entrybox), banner.entries[ii++].table,
			 FALSE, FALSE, 0);
      /* Color */
      if(notify_config.banner_enable_colors) {
	gtkut_convert_int_to_gdk_color(notify_config.banner_color_fg,&fg);
	gtk_widget_modify_fg(label1,GTK_STATE_NORMAL,&fg);
	gtk_widget_modify_fg(label2,GTK_STATE_NORMAL,&fg);
	gtk_widget_modify_fg(label3,GTK_STATE_NORMAL,&fg);
	gtk_widget_modify_fg(label4,GTK_STATE_NORMAL,&fg);
	gtk_widget_modify_fg(label5,GTK_STATE_NORMAL,&fg);
	gtk_widget_modify_fg(label6,GTK_STATE_NORMAL,&fg);
      }
    }
  }
  else {
    /* We have an empty list -- create an empty dummy element */
    GtkWidget *label;

    banner.entries = g_new(NotificationBannerEntry, 1);
    banner.entries[0].table = gtk_table_new(3, 1, FALSE);
    label = gtk_label_new("");
    gtk_table_attach_defaults(GTK_TABLE(banner.entries[0].table), 
			      label, 0, 1, 0, 1);
    label = gtk_label_new("");
    gtk_table_attach_defaults(GTK_TABLE(banner.entries[0].table), 
			      label, 0, 1, 1, 2);
    label = gtk_label_new("");
    gtk_table_attach_defaults(GTK_TABLE(banner.entries[0].table), 
			      label, 0, 1, 2, 3);
    gtk_box_pack_start(GTK_BOX(entrybox), banner.entries[0].table,
		       FALSE, FALSE, 0);
  }
  return entrybox;
}

#endif /* NOTIFICATION_BANNER */
