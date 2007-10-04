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

#include <math.h>

#include "prefs.h"
#include "prefs_gtk.h"
#include "common/utils.h"
#include "common/defs.h"
#include "gtk/gtkutils.h"
#include "main.h"

#include "gettext.h"

#include "notification_prefs.h"
#include "notification_plugin.h"
#include "notification_core.h"

#include "notification_popup.h"
#include "notification_command.h"
#include "notification_lcdproc.h"
#include "notification_trayicon.h"

#include "notification_foldercheck.h"


typedef struct {
  PrefsPage page;
  GtkWidget *include_mail;
  GtkWidget *include_news;
  GtkWidget *include_rss;
  GtkWidget *include_calendar;
} NotifyPage;

NotifyPrefs notify_config;
NotifyPage  notify_page;

#ifdef NOTIFICATION_BANNER
typedef struct {
  PrefsPage page;
  GtkWidget *banner_show;
  GtkWidget *banner_speed;
  GtkWidget *banner_include_unread;
  GtkWidget *banner_max_msgs;
  GtkWidget *banner_sticky;
  GtkWidget *banner_folder_specific;
  GtkWidget *banner_enable_colors;
  GtkWidget *banner_color_bg;
  GtkWidget *banner_color_fg;
  GtkWidget *banner_cont_enable;
  GtkWidget *banner_cont_folder_specific;
  GtkWidget *banner_cont_color_sel;
} NotifyBannerPage;
NotifyBannerPage banner_page;
#endif /* NOTIFICATION_BANNER */

#ifdef NOTIFICATION_POPUP
typedef struct {
  PrefsPage page;
  GtkWidget *popup_show;
  GtkWidget *popup_timeout;
  GtkWidget *popup_folder_specific;
  GtkWidget *popup_cont_enable;
  GtkWidget *popup_cont_folder_specific;
#ifndef HAVE_LIBNOTIFY
  GtkWidget *popup_sticky;
  GtkWidget *popup_enable_colors;
  GtkWidget *popup_color_bg;
  GtkWidget *popup_color_fg;
  GtkWidget *popup_cont_color_sel;
#endif /* !HAVE_LIBNOTIFY */
} NotifyPopupPage;
NotifyPopupPage popup_page;
#endif /* NOTIFICATION_POPUP */

#ifdef NOTIFICATION_COMMAND
typedef struct {
  PrefsPage page;
  GtkWidget *command_enabled;
  GtkWidget *command_timeout;
  GtkWidget *command_folder_specific;
  GtkWidget *command_line;
  GtkWidget *command_cont_enable;
  GtkWidget *command_cont_folder_specific;
} NotifyCommandPage;
NotifyCommandPage command_page;
#endif /* NOTIFICATION_COMMAND */

#ifdef NOTIFICATION_LCDPROC
typedef struct {
  PrefsPage page;
  GtkWidget *lcdproc_enabled;
  GtkWidget *lcdproc_cont_enable;
  GtkWidget *lcdproc_hostname;
  GtkWidget *lcdproc_port;
} NotifyLCDProcPage;
NotifyLCDProcPage lcdproc_page;
#endif

#ifdef NOTIFICATION_TRAYICON
typedef struct {
  PrefsPage page;
  GtkWidget *trayicon_enabled;
  GtkWidget *trayicon_cont_enable;
  GtkWidget *trayicon_hide_at_startup;
  GtkWidget *trayicon_close_to_tray;
  GtkWidget *trayicon_hide_when_iconified;
  GtkWidget *trayicon_folder_specific;
  GtkWidget *trayicon_cont_folder_specific;
#ifdef HAVE_LIBNOTIFY
  GtkWidget *trayicon_popup_enabled;
  GtkWidget *trayicon_popup_cont_enable;
  GtkWidget *trayicon_popup_timeout;
#endif
} NotifyTrayiconPage;
NotifyTrayiconPage trayicon_page;
#endif

PrefParam notify_param[] = {

  {"include_mail", "TRUE", &notify_config.include_mail, P_BOOL, NULL,
   NULL, NULL},
  {"include_news", "TRUE", &notify_config.include_news, P_BOOL, NULL,
   NULL, NULL},
  {"include_rss", "TRUE", &notify_config.include_rss, P_BOOL, NULL,
   NULL, NULL},
  {"include_calendar", "TRUE", &notify_config.include_calendar, P_BOOL, NULL,
   NULL, NULL},

#ifdef NOTIFICATION_BANNER
  {"banner_show", "0", &notify_config.banner_show, P_INT, NULL, NULL, NULL},
  {"banner_speed", "30", &notify_config.banner_speed, P_INT, NULL, NULL, NULL},
  {"banner_include_unread", "FALSE", &notify_config.banner_include_unread,
   P_BOOL, NULL, NULL, NULL},
  {"banner_max_msgs", "100", &notify_config.banner_max_msgs, P_INT,
   NULL, NULL, NULL},
  {"banner_sticky", "FALSE", &notify_config.banner_sticky,
   P_BOOL, NULL, NULL, NULL},
  {"banner_root_x", "0", &notify_config.banner_root_x, P_INT,
   NULL, NULL, NULL},
  {"banner_root_y", "10", &notify_config.banner_root_y, P_INT,
   NULL, NULL, NULL},
  {"banner_folder_specific", "FALSE", &notify_config.banner_folder_specific,
   P_BOOL, NULL, NULL, NULL},
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
  {"popup_folder_specific", "FALSE", &notify_config.popup_folder_specific,
   P_BOOL, NULL, NULL, NULL},
#ifndef HAVE_LIBNOTIFY
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
#endif /* !HAVE_LIBNOTIFY */
#endif

#ifdef NOTIFICATION_COMMAND
  {"command_enabled", "FALSE", &notify_config.command_enabled, P_BOOL,
   NULL, NULL, NULL},
  {"command_timeout", "60000", &notify_config.command_timeout, P_INT,
   NULL, NULL, NULL},
  {"command_folder_specific", "FALSE", &notify_config.command_folder_specific,
   P_BOOL, NULL, NULL, NULL},
  {"command_line", "", &notify_config.command_line, P_STRING,
   NULL, NULL, NULL},
#endif

#ifdef NOTIFICATION_LCDPROC
  {"lcdproc_enabled", "FALSE", &notify_config.lcdproc_enabled, P_BOOL,
   NULL, NULL, NULL},
  {"lcdproc_hostname", "localhost", &notify_config.lcdproc_hostname, P_STRING,
   NULL, NULL, NULL},
  {"lcdproc_port", "13666", &notify_config.lcdproc_port, P_INT,
   NULL, NULL, NULL},
#endif

#ifdef NOTIFICATION_TRAYICON
  {"trayicon_enabled", "FALSE", &notify_config.trayicon_enabled, P_BOOL,
   NULL, NULL, NULL},
  {"trayicon_hide_at_startup", "FALSE",
   &notify_config.trayicon_hide_at_startup, P_BOOL, NULL, NULL, NULL},
  {"trayicon_close_to_tray", "FALSE",
   &notify_config.trayicon_close_to_tray, P_BOOL, NULL, NULL, NULL},
  {"trayicon_hide_when_iconified", "FALSE",
   &notify_config.trayicon_hide_when_iconified, P_BOOL, NULL, NULL, NULL},
  {"trayicon_folder_specific", "FALSE",
   &notify_config.trayicon_folder_specific,
   P_BOOL, NULL, NULL, NULL},
#ifdef HAVE_LIBNOTIFY
  {"trayicon_popup_enabled", "TRUE", &notify_config.trayicon_popup_enabled,
   P_BOOL, NULL, NULL, NULL},
  {"trayicon_popup_timeout", "5000", &notify_config.trayicon_popup_timeout,
   P_INT, NULL, NULL, NULL},
#endif /* HAVE_LIBNOTIFY */
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
static void notify_banner_enable_set_sensitivity(GtkComboBox*, gpointer);
static void notify_banner_color_sel_set_sensitivity(GtkToggleButton*,gpointer);
static void notify_banner_folder_specific_set_sensitivity(GtkToggleButton*,
							   gpointer);
#endif

#ifdef NOTIFICATION_POPUP
static void notify_create_popup_page(PrefsPage*, GtkWindow*, gpointer);
static void notify_destroy_popup_page(PrefsPage*);
static void notify_save_popup(PrefsPage*);
static void notify_popup_folder_specific_set_sensitivity(GtkToggleButton*,
							 gpointer);
static void notify_popup_enable_set_sensitivity(GtkToggleButton*, gpointer);
#ifndef HAVE_LIBNOTIFY
static void notify_popup_set_done_cb(GtkWidget*, gpointer);
static void notify_popup_set_cb(GtkWidget*, gpointer);
static void notify_popup_color_sel_set_sensitivity(GtkToggleButton*,gpointer);
#endif /* !HAVE_LIBNOTIFY */
#endif

#ifdef NOTIFICATION_COMMAND
static void notify_create_command_page(PrefsPage*, GtkWindow*, gpointer);
static void notify_destroy_command_page(PrefsPage*);
static void notify_save_command(PrefsPage*);
static void notify_command_enable_set_sensitivity(GtkToggleButton*, gpointer);
static void notify_command_folder_specific_set_sensitivity(GtkToggleButton*,
							   gpointer);
#endif

#ifdef NOTIFICATION_LCDPROC
static void notify_create_lcdproc_page(PrefsPage*, GtkWindow*, gpointer);
static void notify_destroy_lcdproc_page(PrefsPage*);
static void notify_save_lcdproc(PrefsPage*);
static void notify_lcdproc_enable_set_sensitivity(GtkToggleButton*, gpointer);
#endif

#ifdef NOTIFICATION_TRAYICON
static void notify_create_trayicon_page(PrefsPage*, GtkWindow*, gpointer);
static void notify_destroy_trayicon_page(PrefsPage*);
static void notify_save_trayicon(PrefsPage*);
static void notify_trayicon_enable_set_sensitivity(GtkToggleButton*, gpointer);
static void notify_trayicon_folder_specific_set_sensitivity(GtkToggleButton*,
							    gpointer);
#ifdef HAVE_LIBNOTIFY
static void notify_trayicon_popup_enable_set_sensitivity(GtkToggleButton*,
							 gpointer);
#endif
#endif

static gint conv_color_to_int(GdkColor*);

void notify_gtk_init(void)
{
  static gchar *path[3];

  path[0] = _("Plugins");
  path[1] = _("Notification");
  path[2] = NULL;

  notify_page.page.path = path;
  notify_page.page.create_widget  = notify_create_prefs_page;
  notify_page.page.destroy_widget = notify_destroy_prefs_page;
  notify_page.page.save_page      = notify_save_prefs;
  prefs_gtk_register_page((PrefsPage*) &notify_page);

#ifdef NOTIFICATION_BANNER
 {
   static gchar *banner_path[4];

   banner_path[0] = _("Plugins");
   banner_path[1] = _("Notification");
   banner_path[2] = _("Banner");
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

   popup_path[0] = _("Plugins");
   popup_path[1] = _("Notification");
   popup_path[2] = _("Popup");
   popup_path[3] = NULL;
   
   popup_page.page.path = popup_path;
   popup_page.page.create_widget  = notify_create_popup_page;
   popup_page.page.destroy_widget = notify_destroy_popup_page;
   popup_page.page.save_page      = notify_save_popup;
   prefs_gtk_register_page((PrefsPage*) &popup_page);
 }
#endif /* NOTIFICATION_POPUP */

#ifdef NOTIFICATION_COMMAND
 {
   static gchar *command_path[4];
   
   command_path[0] = _("Plugins");
   command_path[1] = _("Notification");
   command_path[2] = _("Command");
   command_path[3] = NULL;
   
   command_page.page.path = command_path;
   command_page.page.create_widget  = notify_create_command_page;
   command_page.page.destroy_widget = notify_destroy_command_page;
   command_page.page.save_page      = notify_save_command;
   prefs_gtk_register_page((PrefsPage*) &command_page);
 }
#endif /* NOTIFICATION_COMMAND */

#ifdef NOTIFICATION_LCDPROC
 {
   static gchar *lcdproc_path[4];
   
   lcdproc_path[0] = _("Plugins");
   lcdproc_path[1] = _("Notification");
   lcdproc_path[2] = _("LCD");
   lcdproc_path[3] = NULL;
   
   lcdproc_page.page.path = lcdproc_path;
   lcdproc_page.page.create_widget  = notify_create_lcdproc_page;
   lcdproc_page.page.destroy_widget = notify_destroy_lcdproc_page;
   lcdproc_page.page.save_page      = notify_save_lcdproc;
   prefs_gtk_register_page((PrefsPage*) &lcdproc_page);
 }
#endif /* NOTIFICATION_LCDPROC */

#ifdef NOTIFICATION_TRAYICON
 {
   static gchar *trayicon_path[4];
   
   trayicon_path[0] = _("Plugins");
   trayicon_path[1] = _("Notification");
   trayicon_path[2] = _("SysTrayicon");
   trayicon_path[3] = NULL;
   
   trayicon_page.page.path = trayicon_path;
   trayicon_page.page.create_widget  = notify_create_trayicon_page;
   trayicon_page.page.destroy_widget = notify_destroy_trayicon_page;
   trayicon_page.page.save_page      = notify_save_trayicon;
   prefs_gtk_register_page((PrefsPage*) &trayicon_page);
 }
#endif /* NOTIFICATION_TRAYICON */
}

void notify_gtk_done(void)
{
  if(claws_is_exiting())
    return;
  prefs_gtk_unregister_page((PrefsPage*) &notify_page);
#ifdef NOTIFICATION_BANNER
  prefs_gtk_unregister_page((PrefsPage*) &banner_page);
#endif
#ifdef NOTIFICATION_POPUP
  prefs_gtk_unregister_page((PrefsPage*) &popup_page);
#endif
#ifdef NOTIFICATION_COMMAND
  prefs_gtk_unregister_page((PrefsPage*) &command_page);
#endif
#ifdef NOTIFICATION_LCDPROC
  prefs_gtk_unregister_page((PrefsPage*) &lcdproc_page);
#endif
#ifdef NOTIFICATION_TRAYICON
  prefs_gtk_unregister_page((PrefsPage*) &trayicon_page);
#endif
}

void notify_save_config(void)
{
  PrefFile *pfile;
  gchar *rcpath;

  debug_print("Saving Notification plugin configuration...\n");

  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
  pfile = prefs_write_open(rcpath);
  g_free(rcpath);
  if(!pfile || (prefs_set_block_label(pfile, "NotificationPlugin") < 0))
    return;

  if(prefs_write_param(notify_param, pfile->fp) < 0) {
    debug_print("failed!\n");
    g_warning(_("\nNotification Plugin: Failed to write plugin configuration "
	      "to file\n"));
    prefs_file_close_revert(pfile);
    return;
  }
        if (fprintf(pfile->fp, "\n") < 0) {
		FILE_OP_ERROR(rcpath, "fprintf");
		prefs_file_close_revert(pfile);
	} else
	        prefs_file_close(pfile);
  debug_print("done.\n");
}

static void notify_create_prefs_page(PrefsPage *page, GtkWindow *window,
				     gpointer data)
{
  GtkWidget *pvbox;
  GtkWidget *vbox;
  GtkWidget *checkbox;
  GtkWidget *frame;
  GtkWidget *label;

  /* Page vbox */
  pvbox = gtk_vbox_new(FALSE, 0);

  /* Frame */
  frame = gtk_frame_new(_("Include folder types"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
  gtk_box_pack_start(GTK_BOX(pvbox), frame, FALSE, FALSE, 0);
  
  /* Frame vbox */
  vbox = gtk_vbox_new(FALSE, 4);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

  /* Include mail folders */
  checkbox = gtk_check_button_new_with_label(_("Mail folders"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.include_mail);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  notify_page.include_mail = checkbox;
  
  /* Include news folders */
  checkbox = gtk_check_button_new_with_label(_("News folders"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.include_news);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  notify_page.include_news = checkbox;

  /* Include RSS folders */
  checkbox = gtk_check_button_new_with_label(_("RSS folders"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.include_rss);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  notify_page.include_rss = checkbox;

  /* Include calendar folders */
  checkbox = gtk_check_button_new_with_label(_("Calendar folders"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.include_calendar);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  notify_page.include_calendar = checkbox;

  /* Warning-Label */
  label = gtk_label_new(_("These settings overwrite folder-specific "
			  "selections."));
  gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);  
  gtk_widget_show(label);

  /* Done. */
  gtk_widget_show(frame);
  gtk_widget_show(vbox);
  gtk_widget_show(pvbox);
  page->widget = pvbox;
}

static void notify_destroy_prefs_page(PrefsPage *page)
{
}

static void notify_save_prefs(PrefsPage *page)
{
  notify_config.include_mail = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(notify_page.include_mail));
  notify_config.include_news = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(notify_page.include_news));
  notify_config.include_rss = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(notify_page.include_rss));
  notify_config.include_calendar = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(notify_page.include_calendar));

  notification_core_global_includes_changed();
}

#ifdef NOTIFICATION_BANNER
static void notify_create_banner_page(PrefsPage *page, GtkWindow *window,
				      gpointer data)
{
  GtkRequisition requisition;
  GtkWidget *pvbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *checkbox;
  GtkWidget *button;
  GtkWidget *combo;
  GtkWidget *spinner;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *color_sel;
  GdkColor bg;
  GdkColor fg;

  pvbox = gtk_vbox_new(FALSE, 20);
  gtk_container_set_border_width(GTK_CONTAINER(pvbox), 10);

  /* Always / Never / Only when non-empty */
  hbox = gtk_hbox_new(FALSE, 20);
  gtk_box_pack_start(GTK_BOX(pvbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new(_("Show banner"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  combo = gtk_combo_box_new_text();
  gtk_combo_box_insert_text(GTK_COMBO_BOX(combo), NOTIFY_BANNER_SHOW_NEVER,
			    _("Never"));
  gtk_combo_box_insert_text(GTK_COMBO_BOX(combo), NOTIFY_BANNER_SHOW_ALWAYS,
			    _("Always"));
  gtk_combo_box_insert_text(GTK_COMBO_BOX(combo), NOTIFY_BANNER_SHOW_NONEMPTY,
			    _("Only when not empty"));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), notify_config.banner_show);
  gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(combo), "changed",
		   G_CALLBACK(notify_banner_enable_set_sensitivity), NULL);
  gtk_widget_show(combo);
  gtk_widget_show(hbox);
  banner_page.banner_show = combo;

  /* Container vbox for greying out everything */
  vbox = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(pvbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show(vbox);
  banner_page.banner_cont_enable = vbox;

  /* Banner speed */
  table = gtk_table_new(2, 3, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
  label = gtk_label_new(_("slow"));
  gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
  gtk_widget_show(label);
  label = gtk_label_new(_("fast"));
  gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 0, 1);
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0);
  gtk_widget_show(label);
  label = gtk_label_new(_("Banner speed"));
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
  gtk_widget_show(label);
  slider = gtk_hscale_new_with_range(10., 70., 10.);
  gtk_scale_set_digits(GTK_SCALE(slider), 0);
  gtk_widget_size_request(combo, &requisition);
  gtk_widget_set_size_request(slider, requisition.width, -1);
  gtk_range_set_increments(GTK_RANGE(slider), 10., 10.);
  gtk_range_set_inverted(GTK_RANGE(slider), TRUE);
  gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
  gtk_range_set_value(GTK_RANGE(slider), notify_config.banner_speed);
  gtk_table_attach_defaults(GTK_TABLE(table), slider, 1, 3, 1, 2);
  gtk_widget_show(slider);
  gtk_widget_show(table);
  banner_page.banner_speed = slider;

  /* Maximum number of messages in banner */
  hbox = gtk_hbox_new(FALSE, 10);
  label = gtk_label_new(_("Maximum number of messages (0 means unlimited)"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  spinner = gtk_spin_button_new_with_range(0., 1000., 1.);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spinner), 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), notify_config.banner_max_msgs);
  gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(spinner);
  gtk_widget_show(hbox);
  banner_page.banner_max_msgs = spinner;

  /* Include unread */
  checkbox = gtk_check_button_new_with_label(_("Include unread mails in banner"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.banner_include_unread);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  banner_page.banner_include_unread = checkbox;

  /* Check button sticky */
  checkbox = gtk_check_button_new_with_label(_("Make banner sticky"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.banner_sticky);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  banner_page.banner_sticky = checkbox;

  /* Check box for enabling folder specific selection */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  checkbox = gtk_check_button_new_with_label(_("Only include selected folders"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.banner_folder_specific);
  gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_banner_folder_specific_set_sensitivity),
		   NULL);
  gtk_widget_show(checkbox);
  banner_page.banner_folder_specific = checkbox;
  button = gtk_button_new_with_label(_("Select folders..."));
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(notification_foldercheck_sel_folders_cb),
		   BANNER_SPECIFIC_FOLDER_ID_STR);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  banner_page.banner_cont_folder_specific = button;
  gtk_widget_show(button);
  gtk_widget_show(hbox);

  /* Check box for enabling custom colors */
  checkbox = gtk_check_button_new_with_label(_("Use custom colors"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.banner_enable_colors);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_banner_color_sel_set_sensitivity), NULL);
  gtk_widget_show(checkbox);
  banner_page.banner_enable_colors = checkbox;

  /* Color selection dialogs for foreground and background color */
  /* foreground */
  table = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
  label = gtk_label_new(_("Foreground"));
  gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);
  gtk_widget_show(label);
  color_sel = gtk_color_button_new();
  gtkut_convert_int_to_gdk_color(notify_config.banner_color_fg,&fg);
  gtk_color_button_set_color(GTK_COLOR_BUTTON(color_sel),&fg);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(color_sel),_("Foreground color"));
  gtk_table_attach_defaults(GTK_TABLE(table),color_sel,1,2,0,1);
  gtk_widget_show(color_sel);
  banner_page.banner_color_fg = color_sel;
  /* background */
  label = gtk_label_new(_("Background"));
  gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,1,2);
  gtk_widget_show(label);
  color_sel = gtk_color_button_new();
  gtkut_convert_int_to_gdk_color(notify_config.banner_color_bg,&bg);
  gtk_color_button_set_color(GTK_COLOR_BUTTON(color_sel),&bg);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(color_sel), _("Background color"));
  gtk_table_attach_defaults(GTK_TABLE(table),color_sel,1,2,1,2);
  gtk_widget_show(color_sel);
  gtk_widget_show(table);
  banner_page.banner_color_bg = color_sel;
  banner_page.banner_cont_color_sel = table;

  notify_banner_color_sel_set_sensitivity
    (GTK_TOGGLE_BUTTON(banner_page.banner_enable_colors), NULL);
  notify_banner_folder_specific_set_sensitivity
    (GTK_TOGGLE_BUTTON(banner_page.banner_folder_specific), NULL);
  notify_banner_enable_set_sensitivity(GTK_COMBO_BOX(combo), NULL);
  gtk_widget_show(pvbox);
  banner_page.page.widget = pvbox;
}

static void notify_destroy_banner_page(PrefsPage *page)
{
}

static void notify_save_banner(PrefsPage *page)
{
  gdouble range_val;
  GdkColor color;

  notify_config.banner_show = 
    gtk_combo_box_get_active(GTK_COMBO_BOX(banner_page.banner_show));
  notify_config.banner_max_msgs = 
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(banner_page.banner_max_msgs));
  notify_config.banner_include_unread = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(banner_page.banner_include_unread));
  range_val= gtk_range_get_value(GTK_RANGE(banner_page.banner_speed));
  notify_config.banner_speed = (gint)ceil(range_val);

  notify_config.banner_sticky = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(banner_page.banner_sticky));

  notify_config.banner_folder_specific =
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(banner_page.banner_folder_specific));

  notify_config.banner_enable_colors = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(banner_page.banner_enable_colors));

  /* Color dialogs are a bit more complicated */
  gtk_color_button_get_color(GTK_COLOR_BUTTON(banner_page.banner_color_fg),
			     &color);
  notify_config.banner_color_fg = conv_color_to_int(&color);
  gtk_color_button_get_color(GTK_COLOR_BUTTON(banner_page.banner_color_bg),
			     &color);
  notify_config.banner_color_bg = conv_color_to_int(&color);

  notification_update_banner();
}

static void notify_banner_enable_set_sensitivity(GtkComboBox *combo,
						 gpointer data)
{
  NotifyBannerShow show;

  show = gtk_combo_box_get_active(GTK_COMBO_BOX(banner_page.banner_show));
  gtk_widget_set_sensitive(banner_page.banner_cont_enable,
			   (show == NOTIFY_BANNER_SHOW_NEVER) ? FALSE : TRUE);
}

static void notify_banner_color_sel_set_sensitivity(GtkToggleButton *button,
						    gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(banner_page.banner_enable_colors));
  gtk_widget_set_sensitive(banner_page.banner_cont_color_sel, active);
}

static void notify_banner_folder_specific_set_sensitivity(GtkToggleButton *bu,
							   gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(banner_page.banner_folder_specific));
  gtk_widget_set_sensitive(banner_page.banner_cont_folder_specific, active);
}
#endif /* NOTIFICATION_BANNER */

#ifdef NOTIFICATION_POPUP
static void notify_create_popup_page(PrefsPage *page, GtkWindow *window,
				     gpointer data)
{
  gdouble timeout;
  GtkWidget *pvbox;
  GtkWidget *vbox;
  GtkWidget *checkbox;
  GtkWidget *hbox;
  GtkWidget *spinner;
  GtkWidget *label;
  GtkWidget *button;
#ifndef HAVE_LIBNOTIFY
  GdkColor bg;
  GdkColor fg;
  GtkWidget *table;
  GtkWidget *color_sel;
#endif /* !HAVE_LIBNOTIFY */

  pvbox = gtk_vbox_new(FALSE, 20);
  gtk_container_set_border_width(GTK_CONTAINER(pvbox), 10);

  /* Enable popup */
  checkbox = gtk_check_button_new_with_label(_("Enable popup"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.popup_show);
  gtk_box_pack_start(GTK_BOX(pvbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_popup_enable_set_sensitivity), NULL);
  gtk_widget_show(checkbox);
  popup_page.popup_show = checkbox;

  /* Container vbox for greying out everything */
  vbox = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(pvbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show(vbox);
  popup_page.popup_cont_enable = vbox;

  /* Popup timeout */
  hbox = gtk_hbox_new(FALSE, 10);
  label = gtk_label_new(_("Popup timeout:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  spinner = gtk_spin_button_new_with_range(0.2, 60., 0.5);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spinner), 1);
  timeout = notify_config.popup_timeout/1000.;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), timeout);
  gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(spinner);
  label = gtk_label_new(_("seconds"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  gtk_widget_show(hbox);
  popup_page.popup_timeout = spinner;

  /* Check box for enabling folder specific selection */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  checkbox = gtk_check_button_new_with_label(_("Only include selected folders"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.popup_folder_specific);
  gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_popup_folder_specific_set_sensitivity),
		   NULL);
  gtk_widget_show(checkbox);
  popup_page.popup_folder_specific = checkbox;
  button = gtk_button_new_with_label(_("Select folders..."));
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(notification_foldercheck_sel_folders_cb),
		   POPUP_SPECIFIC_FOLDER_ID_STR);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  popup_page.popup_cont_folder_specific = button;
  gtk_widget_show(button);
  gtk_widget_show(hbox);

#ifndef HAVE_LIBNOTIFY
  /* Sticky check button */
  checkbox = gtk_check_button_new_with_label(_("Make popup sticky"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.popup_sticky);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  popup_page.popup_sticky = checkbox;

  /* Button to set size and position of popup window */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  button = gtk_button_new_with_label(_("Set popup window width and position"));
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(notify_popup_set_cb), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);
  label = gtk_label_new(_("(the window manager is free to ignore this)"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  gtk_widget_show(hbox);

  /* Check box for enabling custom colors */
  checkbox = gtk_check_button_new_with_label(_("Use custom colors"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.popup_enable_colors);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_popup_color_sel_set_sensitivity), NULL);
  gtk_widget_show(checkbox);
  popup_page.popup_enable_colors = checkbox;

  /* Color selection dialogs for foreground and background color */
  table = gtk_table_new(2,2,FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
  /* foreground */
  label = gtk_label_new(_("Foreground"));
  gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);
  gtk_widget_show(label);
  color_sel = gtk_color_button_new();
  gtkut_convert_int_to_gdk_color(notify_config.popup_color_fg,&fg);
  gtk_color_button_set_color(GTK_COLOR_BUTTON(color_sel),&fg);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(color_sel),_("Foreground color"));
  gtk_table_attach_defaults(GTK_TABLE(table),color_sel,1,2,0,1);
  gtk_widget_show(color_sel);
  popup_page.popup_color_fg = color_sel;
  /* background */
  label = gtk_label_new(_("Background"));
  gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,1,2);
  gtk_widget_show(label);
  color_sel = gtk_color_button_new();
  gtkut_convert_int_to_gdk_color(notify_config.popup_color_bg,&bg);
  gtk_color_button_set_color(GTK_COLOR_BUTTON(color_sel),&bg);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(color_sel),_("Background color"));
  gtk_table_attach_defaults(GTK_TABLE(table),color_sel,1,2,1,2);
  gtk_widget_show(color_sel);
  gtk_widget_show(table);
  popup_page.popup_color_bg = color_sel;
  popup_page.popup_cont_color_sel = table;

  notify_popup_color_sel_set_sensitivity
    (GTK_TOGGLE_BUTTON(popup_page.popup_enable_colors), NULL);
#endif /* !HAVE_LIBNOTIFY */

  notify_popup_enable_set_sensitivity
    (GTK_TOGGLE_BUTTON(popup_page.popup_show), NULL);
  notify_popup_folder_specific_set_sensitivity
    (GTK_TOGGLE_BUTTON(popup_page.popup_folder_specific), NULL);
  gtk_widget_show(pvbox);
  popup_page.page.widget = pvbox;
}

static void notify_destroy_popup_page(PrefsPage *page)
{
}

static void notify_save_popup(PrefsPage *page)
{
  gdouble timeout;

#ifndef HAVE_LIBNOTIFY
  GdkColor color;
#endif /* !HAVE_LIBNOTIFY */

  notify_config.popup_show = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(popup_page.popup_show));
  timeout =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(popup_page.popup_timeout));
  notify_config.popup_timeout = (gint)floor(timeout*1000+0.5);
  notify_config.popup_folder_specific =
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(popup_page.popup_folder_specific));

#ifndef HAVE_LIBNOTIFY
  notify_config.popup_sticky = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(popup_page.popup_sticky));
  notify_config.popup_enable_colors = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(popup_page.popup_enable_colors));

  /* Color dialogs are a bit more complicated */
  gtk_color_button_get_color(GTK_COLOR_BUTTON(popup_page.popup_color_fg),
			     &color);
  notify_config.popup_color_fg = conv_color_to_int(&color);
  gtk_color_button_get_color(GTK_COLOR_BUTTON(popup_page.popup_color_bg),
			     &color);
  notify_config.popup_color_bg = conv_color_to_int(&color);
#endif /* !HAVE_LIBNOTIFY */
}

#ifndef HAVE_LIBNOTIFY
static void notify_popup_set_cb(GtkWidget *widget, gpointer data)
{
  GtkWidget *win;
  GtkWidget *button;

  win = gtkut_window_new(GTK_WINDOW_TOPLEVEL, "notification_prefs");
  gtk_window_set_title(GTK_WINDOW(win), _("Sample popup window"));  
  gtk_window_set_modal(GTK_WINDOW(win), TRUE);
  gtk_window_set_keep_above(GTK_WINDOW(win), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(win), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(win), 5);

  button = gtk_button_new_with_label(_("Done"));
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

static void notify_popup_color_sel_set_sensitivity(GtkToggleButton *button,
						   gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(popup_page.popup_enable_colors));
  gtk_widget_set_sensitive(popup_page.popup_cont_color_sel, active);
}
#endif /* !HAVE_LIBNOTIFY */

static void notify_popup_enable_set_sensitivity(GtkToggleButton *button,
						gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(popup_page.popup_show));
  gtk_widget_set_sensitive(popup_page.popup_cont_enable, active);
}

static void notify_popup_folder_specific_set_sensitivity(GtkToggleButton *bu,
							 gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(popup_page.popup_folder_specific));
  gtk_widget_set_sensitive(popup_page.popup_cont_folder_specific, active);
}
#endif /* NOTIFICATION_POPUP */

#ifdef NOTIFICATION_COMMAND
static void notify_create_command_page(PrefsPage *page, GtkWindow *window,
				       gpointer data)
{
  GtkWidget *pvbox;
  GtkWidget *vbox;
  GtkWidget *checkbox;
  GtkWidget *hbox;
  GtkWidget *spinner;
  GtkWidget *entry;
  GtkWidget *label;
  GtkWidget *button;
  gdouble timeout;

  pvbox = gtk_vbox_new(FALSE, 20);
  gtk_container_set_border_width(GTK_CONTAINER(pvbox), 10);

  /* Enable command */
  checkbox = gtk_check_button_new_with_label(_("Enable command"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.command_enabled);
  gtk_box_pack_start(GTK_BOX(pvbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_command_enable_set_sensitivity), NULL);
  gtk_widget_show(checkbox);
  command_page.command_enabled = checkbox;

  /* Container vbox for greying out everything */
  vbox = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(pvbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show(vbox);
  command_page.command_cont_enable = vbox;

  /* entry field for command to execute */
  hbox = gtk_hbox_new(FALSE, 10);
  label = gtk_label_new(_("Command to execute:"));
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), notify_config.command_line);
  gtk_widget_show(entry);
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  command_page.command_line = entry;

  /* Spin button for command timeout */
  hbox = gtk_hbox_new(FALSE, 10);
  label = gtk_label_new(_("Block command after execution for"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  spinner = gtk_spin_button_new_with_range(0., 600., 1.);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spinner), 0);
  timeout = notify_config.command_timeout/1000.;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), timeout);
  gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(spinner);
  label = gtk_label_new(_("seconds"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  gtk_widget_show(hbox);
  command_page.command_timeout = spinner;

  /* Check box for enabling folder specific selection */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  checkbox = gtk_check_button_new_with_label(_("Only include selected folders"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.command_folder_specific);
  gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_command_folder_specific_set_sensitivity),
		   NULL);
  gtk_widget_show(checkbox);
  command_page.command_folder_specific = checkbox;
  button = gtk_button_new_with_label(_("Select folders..."));
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(notification_foldercheck_sel_folders_cb),
		   COMMAND_SPECIFIC_FOLDER_ID_STR);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  command_page.command_cont_folder_specific = button;
  gtk_widget_show(button);
  gtk_widget_show(hbox);

  notify_command_enable_set_sensitivity
    (GTK_TOGGLE_BUTTON(command_page.command_enabled), NULL);
  notify_command_folder_specific_set_sensitivity
    (GTK_TOGGLE_BUTTON(command_page.command_folder_specific), NULL);
  gtk_widget_show(pvbox);
  command_page.page.widget = pvbox;
}

static void notify_destroy_command_page(PrefsPage *page)
{
}

static void notify_save_command(PrefsPage *page)
{
  gdouble timeout;
  const gchar *tmp_str;

  notify_config.command_enabled = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(command_page.command_enabled));

  timeout =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(command_page.command_timeout));
  notify_config.command_timeout = (gint)floor(timeout*1000+0.5);
  notify_config.command_folder_specific =
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(command_page.command_folder_specific));

  tmp_str = gtk_entry_get_text(GTK_ENTRY(command_page.command_line));
  if(notify_config.command_line)
    g_free(notify_config.command_line);
  notify_config.command_line = g_strdup(tmp_str);
}

static void notify_command_enable_set_sensitivity(GtkToggleButton *button,
						  gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(command_page.command_enabled));
  gtk_widget_set_sensitive(command_page.command_cont_enable, active);
}

static void notify_command_folder_specific_set_sensitivity(GtkToggleButton *bu,
							   gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(command_page.command_folder_specific));
  gtk_widget_set_sensitive(command_page.command_cont_folder_specific, active);
}
#endif /* NOTIFICATION_COMMAND */

#ifdef NOTIFICATION_LCDPROC
static void notify_create_lcdproc_page(PrefsPage *page, GtkWindow *window,
				       gpointer data)
{
  GtkWidget *pvbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *spinner;
  GtkWidget *checkbox;
  GtkWidget *hbox;

  pvbox = gtk_vbox_new(FALSE, 20);
  gtk_container_set_border_width(GTK_CONTAINER(pvbox), 10);

  /* Enable lcdproc */
  checkbox = gtk_check_button_new_with_label(_("Enable LCD"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.lcdproc_enabled);
  gtk_box_pack_start(GTK_BOX(pvbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_lcdproc_enable_set_sensitivity), NULL);
  gtk_widget_show(checkbox);
  lcdproc_page.lcdproc_enabled = checkbox;

  /* Container vbox for greying out everything */
  vbox = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(pvbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show(vbox);
  lcdproc_page.lcdproc_cont_enable = vbox;

  /* Hostname and port information */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);
  label = gtk_label_new(_("Hostname:Port of LCDd server:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), notify_config.lcdproc_hostname);
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show(entry);
  gtk_widget_show(label);
  label = gtk_label_new(":");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  spinner = gtk_spin_button_new_with_range(1., 65535., 1.);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spinner), 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner),
			    notify_config.lcdproc_port);
  gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_widget_show(spinner);
  lcdproc_page.lcdproc_hostname = entry;
  lcdproc_page.lcdproc_port     = spinner;

  notify_lcdproc_enable_set_sensitivity
    (GTK_TOGGLE_BUTTON(lcdproc_page.lcdproc_enabled), NULL);
  gtk_widget_show(pvbox);
  lcdproc_page.page.widget = pvbox;
}

static void notify_destroy_lcdproc_page(PrefsPage *page)
{
}

static void notify_save_lcdproc(PrefsPage *page)
{
  notify_config.lcdproc_enabled = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(lcdproc_page.lcdproc_enabled));

  if(notify_config.lcdproc_hostname)
    g_free(notify_config.lcdproc_hostname);
  notify_config.lcdproc_hostname =
    g_strdup(gtk_entry_get_text(GTK_ENTRY(lcdproc_page.lcdproc_hostname)));

  notify_config.lcdproc_port = 
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lcdproc_page.lcdproc_port));

  if(notify_config.lcdproc_enabled)
    notification_lcdproc_connect();
  else
    notification_lcdproc_disconnect();
}

static void notify_lcdproc_enable_set_sensitivity(GtkToggleButton *button,
						  gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(lcdproc_page.lcdproc_enabled));
  gtk_widget_set_sensitive(lcdproc_page.lcdproc_cont_enable, active);
}
#endif /* NOTIFICATION_LCDPROC */

#ifdef NOTIFICATION_TRAYICON
static void notify_create_trayicon_page(PrefsPage *page, GtkWindow *window,
				       gpointer data)
{
  GtkWidget *pvbox;
  GtkWidget *vbox;
  GtkWidget *checkbox;
  GtkWidget *hbox;
  GtkWidget *button;

#ifdef HAVE_LIBNOTIFY
  GtkWidget *svbox;
  GtkWidget *ssvbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *spinner;
  gdouble timeout;
#endif
  
  pvbox = gtk_vbox_new(FALSE, 20);
  gtk_container_set_border_width(GTK_CONTAINER(pvbox), 10);

  /* Enable trayicon */
  checkbox = gtk_check_button_new_with_label(_("Enable Trayicon"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.trayicon_enabled);
  gtk_box_pack_start(GTK_BOX(pvbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_trayicon_enable_set_sensitivity), NULL);
  gtk_widget_show(checkbox);
  trayicon_page.trayicon_enabled = checkbox;

  /* Container vbox for greying out everything */
  vbox = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(pvbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show(vbox);
  trayicon_page.trayicon_cont_enable = vbox;

  /* Hide at startup */
  checkbox = gtk_check_button_new_with_label(_("Hide at start-up"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.trayicon_hide_at_startup);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  trayicon_page.trayicon_hide_at_startup = checkbox;

  /* Close to tray */
  checkbox = gtk_check_button_new_with_label(_("Close to tray"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.trayicon_close_to_tray);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  trayicon_page.trayicon_close_to_tray = checkbox;

  /* Hide when iconified */
  checkbox = gtk_check_button_new_with_label(_("Hide when iconified"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.trayicon_hide_when_iconified);
  gtk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  trayicon_page.trayicon_hide_when_iconified = checkbox;

  /* folder specific */
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);
  checkbox = gtk_check_button_new_with_label(_("Only include selected folders"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.trayicon_folder_specific);
  gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_trayicon_folder_specific_set_sensitivity),
		   NULL);
  gtk_widget_show(checkbox);
  trayicon_page.trayicon_folder_specific = checkbox;
  button = gtk_button_new_with_label(_("Select folders..."));
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(notification_foldercheck_sel_folders_cb),
		   TRAYICON_SPECIFIC_FOLDER_ID_STR);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  trayicon_page.trayicon_cont_folder_specific = button;
  gtk_widget_show(button);


#ifdef HAVE_LIBNOTIFY
  /* Frame for trayicon popup stuff */
  frame = gtk_frame_new(_("Passive toaster popup"));
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show(frame);

  /* vbox for frame */
  svbox = gtk_vbox_new(FALSE, 10);
  gtk_container_set_border_width(GTK_CONTAINER(svbox), 5);
  gtk_container_add(GTK_CONTAINER(frame), svbox);
  gtk_widget_show(svbox);

  /* Enable popup for the tray icon */
  checkbox = gtk_check_button_new_with_label(_("Enable Popup"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
			       notify_config.trayicon_popup_enabled);
  g_signal_connect(G_OBJECT(checkbox), "toggled",
		   G_CALLBACK(notify_trayicon_popup_enable_set_sensitivity),
		   NULL);
  gtk_box_pack_start(GTK_BOX(svbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show(checkbox);
  trayicon_page.trayicon_popup_enabled = checkbox;

  /* vbox for trayicon popup stuff enabled/disabled container */
  ssvbox = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(svbox), ssvbox, FALSE, FALSE, 0);
  gtk_widget_show(ssvbox);
  trayicon_page.trayicon_popup_cont_enable = ssvbox;

  /* timeout */
  hbox = gtk_hbox_new(FALSE, 10);
  label = gtk_label_new(_("Popup timeout:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  spinner = gtk_spin_button_new_with_range(0.2, 60., 0.5);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spinner), 1);
  timeout = notify_config.trayicon_popup_timeout/1000.;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner), timeout);
  gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ssvbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(spinner);
  label = gtk_label_new(_("seconds"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  gtk_widget_show(hbox);
  trayicon_page.trayicon_popup_timeout = spinner;
#endif

  notify_trayicon_enable_set_sensitivity
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_enabled), NULL);

#ifdef HAVE_LIBNOTIFY
  notify_trayicon_popup_enable_set_sensitivity
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_popup_enabled), NULL);
#endif

  notify_trayicon_folder_specific_set_sensitivity
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_folder_specific), NULL);

  gtk_widget_show(pvbox);
  trayicon_page.page.widget = pvbox;
}

static void notify_destroy_trayicon_page(PrefsPage *page)
{
}

static void notify_save_trayicon(PrefsPage *page)
{
#ifdef HAVE_LIBNOTIFY
  gdouble timeout;
#endif

  notify_config.trayicon_enabled = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_enabled));

  notify_config.trayicon_hide_at_startup = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_hide_at_startup));

  notify_config.trayicon_close_to_tray = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_close_to_tray));

  notify_config.trayicon_hide_when_iconified = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_hide_when_iconified));

#ifdef HAVE_LIBNOTIFY
  notify_config.trayicon_popup_enabled = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_popup_enabled));

  timeout =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON
			      (trayicon_page.trayicon_popup_timeout));
  notify_config.trayicon_popup_timeout = (gint)floor(timeout*1000+0.5);

  notify_config.trayicon_folder_specific = 
    gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_folder_specific));
#endif

  if(notify_config.trayicon_enabled)
    notification_update_msg_counts(NULL);
  else
    notification_trayicon_destroy();
}

static void notify_trayicon_enable_set_sensitivity(GtkToggleButton *button,
						   gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_enabled));
  gtk_widget_set_sensitive(trayicon_page.trayicon_cont_enable, active);
}

static void notify_trayicon_folder_specific_set_sensitivity(GtkToggleButton *bu,
								  gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_folder_specific));
  gtk_widget_set_sensitive(trayicon_page.trayicon_cont_folder_specific, active);
}

#ifdef HAVE_LIBNOTIFY
static void notify_trayicon_popup_enable_set_sensitivity(GtkToggleButton *bu,
							 gpointer data)
{
  gboolean active;
  active = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON(trayicon_page.trayicon_popup_enabled));
  gtk_widget_set_sensitive(trayicon_page.trayicon_popup_cont_enable, active);
}

#endif /* HAVE_LIBNOTIFY */

#endif /* NOTIFICATION_TRAYICON */

/* This feels so wrong... */
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
