/* Notification Plugin for Sylpheed-Claws
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

#include <math.h>

#include "prefs.h"
#include "prefs_gtk.h"
#include "common/utils.h"
#include "common/defs.h"
#include "gtk/gtkutils.h"

#include "notification_prefs.h"
#include "notification_plugin.h"


typedef struct {
  PrefsPage page;
} NotifyPage;


NotifyPrefs notify_config;
NotifyPage  notify_page;

#ifdef NOTIFICATION_BANNER
typedef struct {
  PrefsPage page;
  GtkWidget *banner_show;
  GtkWidget *banner_speed;
  GtkWidget *banner_include_unread;
  GtkWidget *banner_sticky;
  GtkWidget *banner_enable_colors;
  GtkWidget *banner_color_bg;
  GtkWidget *banner_color_fg;
} NotifyBannerPage;
NotifyBannerPage banner_page;
#endif /* NOTIFICATION_BANNER */

#ifdef NOTIFICATION_POPUP
typedef struct {
  PrefsPage page;
  GtkWidget *popup_show;
  GtkWidget *popup_timeout;
  GtkWidget *popup_sticky;
  GtkWidget *popup_enable_colors;
  GtkWidget *popup_color_bg;
  GtkWidget *popup_color_fg;
} NotifyPopupPage;
NotifyPopupPage popup_page;
#endif /* NOTIFICATION_POPUP */

PrefParam notify_param[] = {
#ifdef NOTIFICATION_BANNER
  {"banner_show", "0", &notify_config.banner_show, P_INT, NULL, NULL, NULL},
  {"banner_speed", "30", &notify_config.banner_speed, P_INT, NULL, NULL, NULL},
  {"banner_include_unread", "FALSE", &notify_config.banner_include_unread,
   P_BOOL, NULL, NULL, NULL},
  {"banner_sticky", "FALSE", &notify_config.banner_sticky,
   P_BOOL, NULL, NULL, NULL},
  {"banner_root_x", "0", &notify_config.banner_root_x, P_INT,
   NULL, NULL, NULL},
  {"banner_root_y", "10", &notify_config.banner_root_y, P_INT,
   NULL, NULL, NULL},
  {"banner_enable_colors", "FALSE", &notify_config.banner_enable_colors,
   P_BOOL, NULL, NULL, NULL},
  {"banner_color_bg", "0", &notify_config.banner_color_bg, P_COLOR,
   NULL, NULL, NULL},
  {"banner_color_fg", "16776704", &notify_config.banner_color_fg, P_COLOR,
   NULL, NULL, NULL},
#endif

#ifdef NOTIFICATION_POPUP
  {"popup_show", "TRUE", &notify_config.popup_show, P_BOOL, NULL, NULL, NULL},
  {"popup_timeout", "5000", &notify_config.popup_timeout,
   P_INT, NULL, NULL, NULL},
  {"popup_sticky", "TRUE", &notify_config.popup_sticky, P_BOOL,
   NULL, NULL, NULL},
  {"popup_root_x", "10", &notify_config.popup_root_x,
   P_INT, NULL, NULL, NULL},
  {"popup_root_y", "10", &notify_config.popup_root_y,
   P_INT, NULL, NULL, NULL},
  {"popup_width", "100", &notify_config.popup_width,
   P_INT, NULL, NULL, NULL},
  {"popup_enable_colors", "FALSE", &notify_config.popup_enable_colors, P_BOOL, 
   NULL, NULL, NULL},
  {"popup_color_bg", "0", &notify_config.popup_color_bg, P_COLOR,
   NULL, NULL, NULL},
  {"popup_color_fg", "16776704", &notify_config.popup_color_fg, P_COLOR,
   NULL, NULL, NULL},
#endif

  {NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static void notify_create_prefs_page(PrefsPage*, GtkWindow*, gpointer);
static void notify_destroy_prefs_page(PrefsPage*);
static void notify_save_prefs(PrefsPage*);

#ifdef NOTIFICATION_BANNER
static void notify_create_banner_page(PrefsPage*, GtkWindow*, gpointer);
static void notify_destroy_banner_page(PrefsPage*);
static void notify_save_banner(PrefsPage*);
#endif

#ifdef NOTIFICATION_POPUP
static void notify_create_popup_page(PrefsPage*, GtkWindow*, gpointer);
static void notify_destroy_popup_page(PrefsPage*);
static void notify_save_popup(PrefsPage*);
static void notify_popup_set_cb(GtkWidget*, gpointer);
static void notify_popup_set_done_cb(GtkWidget*, gpointer);
#endif

static gint conv_color_to_int(GdkColor*);

void notify_gtk_init(void)
{
  static gchar *path[3];

  path[0] = "Plugins";
  path[1] = "Notification";
  path[2] = NULL;

  notify_page.page.path = path;
  notify_page.page.create_widget  = notify_create_prefs_page;
  notify_page.page.destroy_widget = notify_destroy_prefs_page;
  notify_page.page.save_page      = notify_save_prefs;
  prefs_gtk_register_page((PrefsPage*) &notify_page);

#ifdef NOTIFICATION_BANNER
 {
   static gchar *banner_path[4];

   banner_path[0] = "Plugins";
   banner_path[1] = "Notification";
   banner_path[2] = "Banner";
   banner_path[3] = NULL;
   
   banner_page.page.path = banner_path;
   banner_page.page.create_widget  = notify_create_banner_page;
   banner_page.page.destroy_widget = notify_destroy_banner_page;
   banner_page.page.save_page      = notify_save_banner;
   prefs_gtk_register_page((PrefsPage*) &banner_page);
 }
#endif /* NOTIFICATION_BANNER */
#ifdef NOTIFICATION_POPUP
 {
   static gchar *popup_path[4];

   popup_path[0] = "Plugins";
   popup_path[1] = "Notification";
   popup_path[2] = "Popup";
   popup_path[3] = NULL;
   
   popup_page.page.path = popup_path;
   popup_page.page.create_widget  = notify_create_popup_page;
   popup_page.page.destroy_widget = notify_destroy_popup_page;
   popup_page.page.save_page      = notify_save_popup;
   prefs_gtk_register_page((PrefsPage*) &popup_page);
 }
#endif /* NOTIFICATION_POPUP */

}

void notify_gtk_done(void)
{
  prefs_gtk_unregister_page((PrefsPage*) &notify_page);
#ifdef NOTIFICATION_BANNER
  prefs_gtk_unregister_page((PrefsPage*) &banner_page);
#endif
#ifdef NOTIFICATION_POPUP
  prefs_gtk_unregister_page((PrefsPage*) &popup_page);
#endif
}

void notify_save_config(void)
{
  PrefFile *pfile;
  gchar *rcpath;

  debug_print("Saving Notification plugin configuration...");

  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
  pfile = prefs_write_open(rcpath);
  g_free(rcpath);
  if(!pfile || (prefs_set_block_label(pfile, "NotificationPlugin") < 0))
    return;

  if(prefs_write_param(notify_param, pfile->fp) < 0) {
    g_warning("\nNotification Plugin: Failed to write plugin configuration "
	      "to file");
    prefs_file_close_revert(pfile);
    return;
  }
  fprintf(pfile->fp, "\n");

  prefs_file_close(pfile);

  debug_print("done\n");
}

static void notify_create_prefs_page(PrefsPage *page, GtkWindow *window,
				     gpointer data)
{
  GtkWidget *label;
  label = gtk_label_new("Please choose from the submenus");
  gtk_widget_show(label);
  page->widget = label;
}

static void notify_destroy_prefs_page(PrefsPage *page)
{
}

static void notify_save_prefs(PrefsPage *page)
{
}

#ifdef NOTIFICATION_BANNER
static void notify_create_banner_page(PrefsPage *page, GtkWindow *window,
				      gpointer data)
{
  NotifyBannerPage *banner_page = (NotifyBannerPage*) page;
  GtkRequisition requisition;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *checkbox;
  GtkWidget *combo;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *color_sel;
  GdkColor bg;
  GdkColor fg;

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  table = gtk_table_new(8, 3, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);


  /* Combo box */
  label = gtk_label_new("Show banner");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
  gtk_widget_show(label);
  combo = gtk_combo_box_new_text();
  gtk_combo_box_insert_text(GTK_COMBO_BOX(combo), NOTIFY_BANNER_SHOW_NEVER,
			    "Never");
  gtk_combo_box_insert_text(GTK_COMBO_BOX(combo), NOTIFY_BANNER_SHOW_ALWAYS,
			    "Always");
  gtk_combo_box_insert_text(GTK_COMBO_BOX(combo), NOTIFY_BANNER_SHOW_NONEMPTY,
			    "Only when not empty");
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), notify_config.banner_show);
  gtk_table_attach_defaults(GTK_TABLE(table), combo, 1, 3, 0, 1);
  gtk_widget_show(combo);
  banner_page->banner_show = combo;

  /* Slider */
  label = gtk_label_new("slow");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
  gtk_widget_show(label);
  label = gtk_label_new("fast");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 1, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0);
  gtk_widget_show(label);
  label = gtk_label_new("Banner speed");
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
  gtk_widget_show(label);
  slider = gtk_hscale_new_with_range(10., 70., 10.);
  gtk_scale_set_digits(GTK_SCALE(slider), 0);
  gtk_widget_size_request(combo, &requisition);
  gtk_widget_set_size_request(slider, requisition.width, -1);
  gtk_range_set_increments(GTK_RANGE(slider), 10., 10.);
  gtk_range_set_inverted(GTK_RANGE(slider), TRUE);
  gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
  gtk_range_set_value(GTK_RANGE(slider), notify_config.banner_speed);
  gtk_table_attach_defaults(GTK_TABLE(table), slider, 1, 3, 2, 3);
  gtk_widget_show(slider);
  banner_page->banner_speed = slider;

  /* Check button include unread */
  checkbox = gtk_check_button_new_with_label("Include unread mails in banner");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.banner_include_unread);
  gtk_table_attach_defaults(GTK_TABLE(table), checkbox, 0, 2, 3, 4);
  gtk_widget_show(checkbox);
  banner_page->banner_include_unread = checkbox;

  /* Check button sticky */
  checkbox = gtk_check_button_new_with_label("Make banner sticky");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.banner_sticky);
  gtk_table_attach_defaults(GTK_TABLE(table), checkbox, 0, 2, 4, 5);
  gtk_widget_show(checkbox);
  banner_page->banner_sticky = checkbox;

  /* Check box for enabling custom colors */
  checkbox = gtk_check_button_new_with_label("Use custom colors");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.banner_enable_colors);
  gtk_table_attach_defaults(GTK_TABLE(table), checkbox, 0, 2, 5, 6);
  gtk_widget_show(checkbox);
  banner_page->banner_enable_colors = checkbox;

  /* Color selection dialogs for foreground and background color */
  /* foreground */
  label = gtk_label_new("Foreground");
  gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,6,7);
  gtk_widget_show(label);
  color_sel = gtk_color_button_new();
  gtkut_convert_int_to_gdk_color(notify_config.banner_color_fg,&fg);
  gtk_color_button_set_color(GTK_COLOR_BUTTON(color_sel),&fg);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(color_sel),"Foreground color");
  gtk_table_attach_defaults(GTK_TABLE(table),color_sel,1,2,6,7);
  gtk_widget_show(color_sel);
  banner_page->banner_color_fg = color_sel;
  /* background */
  label = gtk_label_new("Background");
  gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,7,8);
  gtk_widget_show(label);
  color_sel = gtk_color_button_new();
  gtkut_convert_int_to_gdk_color(notify_config.banner_color_bg,&bg);
  gtk_color_button_set_color(GTK_COLOR_BUTTON(color_sel),&bg);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(color_sel),"Background color");
  gtk_table_attach_defaults(GTK_TABLE(table),color_sel,1,2,7,8);
  gtk_widget_show(color_sel);
  banner_page->banner_color_bg = color_sel;

  /* Table configs */
  gtk_table_set_row_spacings(GTK_TABLE(table),20);
  gtk_table_set_row_spacing(GTK_TABLE(table),1,0);
  gtk_table_set_row_spacing(GTK_TABLE(table),6,0);

  gtk_widget_show(table);
  gtk_widget_show(vbox);
  banner_page->page.widget = vbox;
}

static void notify_destroy_banner_page(PrefsPage *page)
{
}

static void notify_save_banner(PrefsPage *page)
{
  NotifyBannerPage *banner_page = (NotifyBannerPage*) page;
  gdouble range_val;
  GdkColor color;

  notify_config.banner_show = 
    gtk_combo_box_get_active(GTK_COMBO_BOX(banner_page->banner_show));
  notify_config.banner_include_unread = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				 (banner_page->banner_include_unread));
  range_val= gtk_range_get_value(GTK_RANGE(banner_page->banner_speed));
  notify_config.banner_speed = (gint)ceil(range_val);
  notify_config.banner_sticky = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
				 (banner_page->banner_sticky));
  notify_config.banner_enable_colors = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(banner_page->banner_enable_colors));

  /* Color dialogs are a bit more complicated */
  gtk_color_button_get_color(GTK_COLOR_BUTTON(banner_page->banner_color_fg),
			     &color);
  notify_config.banner_color_fg = conv_color_to_int(&color);
  gtk_color_button_get_color(GTK_COLOR_BUTTON(banner_page->banner_color_bg),
			     &color);
  notify_config.banner_color_bg = conv_color_to_int(&color);

  notification_update_banner();
}
#endif /* NOTIFICATION_BANNER */

#ifdef NOTIFICATION_POPUP
static void notify_create_popup_page(PrefsPage *page, GtkWindow *window,
				     gpointer data)
{
  NotifyPopupPage *popup_page = (NotifyPopupPage*) page;
  GtkWidget *vbox;
  GtkWidget *checkbox;
  GtkWidget *hbox;
  GtkWidget *spinner;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *color_sel;
  GtkWidget *table;
  gdouble timeout;
  GdkColor bg;
  GdkColor fg;

  vbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

  /* Show check button for enabling the popup */
  checkbox = gtk_check_button_new_with_label("Enable popup");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.popup_show);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  popup_page->popup_show = checkbox;

  /* Spin button for popup timeout */
  hbox = gtk_hbox_new(FALSE, 10);
  label = gtk_label_new("Popup timeout (seconds)");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  spinner = gtk_spin_button_new_with_range(0.2, 60., 0.5);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spinner), 1.);
  timeout = notify_config.popup_timeout/1000.;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), timeout);
  gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(spinner);
  gtk_widget_show(hbox);
  popup_page->popup_timeout = spinner;

  /* Sticky check button */
  checkbox = gtk_check_button_new_with_label("Make popup sticky");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.popup_sticky);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  popup_page->popup_sticky = checkbox;

  /* Button to set size and position of popup window */
  button = gtk_button_new_with_label("Set popup window width and position");
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(notify_popup_set_cb), NULL);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);  
  gtk_widget_show(button);

  /* Check box for enabling custom colors */
  checkbox = gtk_check_button_new_with_label("Use custom colors");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.popup_enable_colors);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  popup_page->popup_enable_colors = checkbox;

  /* Color selection dialogs for foreground and background color */
  table = gtk_table_new(2,2,FALSE);
  /* foreground */
  label = gtk_label_new("Foreground");
  gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);
  gtk_widget_show(label);
  color_sel = gtk_color_button_new();
  gtkut_convert_int_to_gdk_color(notify_config.popup_color_fg,&fg);
  gtk_color_button_set_color(GTK_COLOR_BUTTON(color_sel),&fg);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(color_sel),"Foreground color");
  gtk_table_attach_defaults(GTK_TABLE(table),color_sel,1,2,0,1);
  gtk_widget_show(color_sel);
  popup_page->popup_color_fg = color_sel;
  /* background */
  label = gtk_label_new("Background");
  gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,1,2);
  gtk_widget_show(label);
  color_sel = gtk_color_button_new();
  gtkut_convert_int_to_gdk_color(notify_config.popup_color_bg,&bg);
  gtk_color_button_set_color(GTK_COLOR_BUTTON(color_sel),&bg);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(color_sel),"Background color");
  gtk_table_attach_defaults(GTK_TABLE(table),color_sel,1,2,1,2);
  gtk_widget_show(color_sel);
  popup_page->popup_color_bg = color_sel;
  /* table */
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
  gtk_widget_show(table);

  /* done */
  gtk_widget_show(vbox);
  popup_page->page.widget = vbox;
}

static void notify_destroy_popup_page(PrefsPage *page)
{
}

static void notify_save_popup(PrefsPage *page)
{
  NotifyPopupPage *popup_page = (NotifyPopupPage*) page;
  gdouble timeout;
  GdkColor color;

  notify_config.popup_show = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(popup_page->popup_show));
  timeout =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(popup_page->popup_timeout));
  notify_config.popup_timeout = (gint)floor(timeout*1000+0.5);
  notify_config.popup_sticky = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(popup_page->popup_sticky));
  notify_config.popup_enable_colors = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(popup_page->popup_enable_colors));
 
  /* Color dialogs are a bit more complicated */
  gtk_color_button_get_color(GTK_COLOR_BUTTON(popup_page->popup_color_fg),
			     &color);
  notify_config.popup_color_fg = conv_color_to_int(&color);
  gtk_color_button_get_color(GTK_COLOR_BUTTON(popup_page->popup_color_bg),
			     &color);
  notify_config.popup_color_bg = conv_color_to_int(&color);
}

static void notify_popup_set_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *win;
  GtkWidget *button;

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal(GTK_WINDOW(win), TRUE);
  gtk_window_set_keep_above(GTK_WINDOW(win), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(win), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(win), 5);

  button = gtk_button_new_with_label("done");
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(notify_popup_set_done_cb), win);
  gtk_container_add(GTK_CONTAINER(win), button);

  gtk_widget_show_all(win);
}

static void notify_popup_set_done_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *win = data;
  gint dummy;

  gtk_window_get_position(GTK_WINDOW(win),
			  &(notify_config.popup_root_x),
			  &(notify_config.popup_root_y));
  gtk_window_get_size(GTK_WINDOW(win),
		      &(notify_config.popup_width), &dummy);
  gtk_widget_destroy(win);
}

#endif /* NOTIFICATION_POPUP */

static gint conv_color_to_int(GdkColor *color)
{
  gint result;
  guint red,green,blue;

  red   = (guint) ((gdouble)(color->red)   /65535.*255.);
  green = (guint) ((gdouble)(color->green) /65535.*255.);
  blue  = (guint) ((gdouble)(color->blue)  /65535.*255.);

  result = (gint) (blue | (green<<8) | (red<<16));

  return result;
}
