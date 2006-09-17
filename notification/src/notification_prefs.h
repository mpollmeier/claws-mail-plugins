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

#ifndef NOTIFICATION_PREFS_H
#define NOTIFICATION_PREFS_H NOTIFICATION_PREFS_H

#include "pluginconfig.h"

#include <glib.h>

#include "prefs_gtk.h"

#include "notification_banner.h"

#ifdef NOTIFICATION_BANNER
typedef enum {
  NOTIFY_BANNER_SHOW_NEVER = 0,
  NOTIFY_BANNER_SHOW_ALWAYS,
  NOTIFY_BANNER_SHOW_NONEMPTY
} NotifyBannerShow;
#endif

typedef struct {
#ifdef NOTIFICATION_BANNER
  NotifyBannerShow banner_show;
  gint             banner_speed;
  gboolean         banner_include_unread;
  gboolean         banner_sticky;
  gint             banner_root_x;
  gint             banner_root_y;
  gboolean         banner_folder_specific;
  gboolean         banner_enable_colors;
  gulong           banner_color_bg;
  gulong           banner_color_fg;
#endif
#ifdef NOTIFICATION_POPUP
  gboolean         popup_show;
  gint             popup_timeout;
  gboolean         popup_folder_specific;
#ifndef HAVE_LIBNOTIFY
  gboolean         popup_sticky;
  gint             popup_root_x;
  gint             popup_root_y;
  gint             popup_width;
  gboolean         popup_enable_colors;
  gulong           popup_color_bg;
  gulong           popup_color_fg;
#endif /* !HAVE_LIBNOTIFY */
#endif
#ifdef NOTIFICATION_COMMAND
  gboolean         command_enabled;
  gint             command_timeout;
  gchar*           command_line;
#endif
} NotifyPrefs;

extern NotifyPrefs notify_config;
extern PrefParam   notify_param[];

void notify_gtk_init(void);
void notify_gtk_done(void);
void notify_save_config(void);

#endif /* NOTIFICATION_PREFS_H */
