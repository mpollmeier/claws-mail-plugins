/* 
 * Claws Mail -- A GTK+ based, lightweight, and fast e-mail client
 * Copyright(C) 1999-2009 the Claws Mail Team
 * == Fancy Plugin ==
 * This file Copyright (C) 2009 Salvatore De Paolis
 * <iwkse@claws-mail.org>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write tothe Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtkutils.h"
#include "utils.h"
#include "prefs.h"
#include "prefs_gtk.h"
#include "prefswindow.h"
#include "combobox.h"
#include "addressbook.h"

#include "fancy_prefs.h"

#define PREFS_BLOCK_NAME "fancy"

FancyPrefs fancy_prefs;

typedef struct _FancyPrefsPage FancyPrefsPage;

struct _FancyPrefsPage {
    PrefsPage page;
    GtkWidget *auto_load_images;
    GtkWidget *block_links;
    GtkWidget *enable_scripts;
    GtkWidget *enable_plugins;
    GtkWidget *open_external;
};

static PrefParam param[] = {
        {"auto_load_images", "FALSE", &fancy_prefs.auto_load_images, P_BOOL, 
         NULL, NULL, NULL},
        {"block_links", "TRUE", &fancy_prefs.block_links, P_BOOL, NULL, 
         NULL, NULL},
        {"enable_scripts", "FALSE", &fancy_prefs.enable_scripts, P_BOOL, NULL, 
         NULL, NULL},
        {"enable_plugins", "FALSE", &fancy_prefs.enable_plugins, P_BOOL, NULL, 
         NULL, NULL},
        {"open_external", "FALSE", &fancy_prefs.open_external, P_BOOL, NULL, 
         NULL, NULL},
        {0,0,0,0}
};

static FancyPrefsPage fancy_prefs_page;

static void create_fancy_prefs_page     (PrefsPage *page, GtkWindow *window, 
                                         gpointer   data);
static void destroy_fancy_prefs_page    (PrefsPage *page);
static void save_fancy_prefs            (PrefsPage *page);

void fancy_prefs_init(void)
{
    static gchar *path[3];
    gchar *rcpath;

    path[0] = _("Plugins");
    path[1] = "Fancy";
    path[2] = NULL;

    prefs_set_default(param);
    rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
    prefs_read_config(param, PREFS_BLOCK_NAME, rcpath, NULL);
    g_free(rcpath);
        
    fancy_prefs_page.page.path = path;
    fancy_prefs_page.page.create_widget = create_fancy_prefs_page;
    fancy_prefs_page.page.destroy_widget = destroy_fancy_prefs_page;
    fancy_prefs_page.page.save_page = save_fancy_prefs;
    fancy_prefs_page.page.weight = 30.0;
    prefs_gtk_register_page((PrefsPage *) &fancy_prefs_page);
}

void fancy_prefs_done(void)
{
    prefs_gtk_unregister_page((PrefsPage *) &fancy_prefs_page);
}

static void create_fancy_prefs_page(PrefsPage *page, GtkWindow *window, 
                                    gpointer data)
{
    FancyPrefsPage *prefs_page = (FancyPrefsPage *) page;

    GtkWidget *vbox;
    GtkWidget *checkbox1;
    GtkWidget *checkbox2;
    GtkWidget *checkbox3;
    GtkWidget *checkbox4;
    GtkWidget *checkbox5;

    vbox = gtk_vbox_new(FALSE, 3);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), VBOX_BORDER);
    gtk_widget_show(vbox);
        
    checkbox1 = gtk_check_button_new_with_label(_("Auto-Load images"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox1),
                                 fancy_prefs.auto_load_images);
    gtk_box_pack_start(GTK_BOX(vbox), checkbox1, FALSE, FALSE, 0);
    gtk_widget_show(checkbox1);

    checkbox2 = gtk_check_button_new_with_label(_("Block external links"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox2),
                                 fancy_prefs.block_links);
    gtk_box_pack_start(GTK_BOX(vbox), checkbox2, FALSE, FALSE, 0);
    gtk_widget_show(checkbox2);
    
    checkbox3 = gtk_check_button_new_with_label(_("Enable Javascript"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox3),
                                 fancy_prefs.enable_scripts);
    gtk_box_pack_start(GTK_BOX(vbox), checkbox3, FALSE, FALSE, 0);
    gtk_widget_show(checkbox3);

    checkbox4 = gtk_check_button_new_with_label(_("Enable Plugins"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox4),
                                 fancy_prefs.enable_plugins);
    gtk_box_pack_start(GTK_BOX(vbox), checkbox4, FALSE, FALSE, 0);
    gtk_widget_show(checkbox4);

    checkbox5 = gtk_check_button_new_with_label(_("Open links with external browser"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox5),
                                 fancy_prefs.open_external);
    gtk_box_pack_start(GTK_BOX(vbox), checkbox5, FALSE, FALSE, 0);
    gtk_widget_show(checkbox5);

    prefs_page->auto_load_images = checkbox1;
    prefs_page->block_links = checkbox2;
    prefs_page->enable_scripts = checkbox3;
    prefs_page->enable_plugins = checkbox4;
    prefs_page->open_external = checkbox5;
    prefs_page->page.widget = vbox;
}

static void destroy_fancy_prefs_page(PrefsPage *page)
{
    /* Do nothing! */
}

static void save_fancy_prefs(PrefsPage *page)
{
        FancyPrefsPage *prefs_page = (FancyPrefsPage *) page;
        PrefFile *pref_file;
        gchar *rc_file_path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
                                          COMMON_RC, NULL);
        
        fancy_prefs.auto_load_images = gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(prefs_page->auto_load_images));
        fancy_prefs.block_links = gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(prefs_page->block_links));
        fancy_prefs.enable_scripts = gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(prefs_page->enable_scripts));
        fancy_prefs.enable_plugins = gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(prefs_page->enable_plugins));
        fancy_prefs.open_external = gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(prefs_page->open_external));

        pref_file = prefs_write_open(rc_file_path);
        g_free(rc_file_path);
        
        if (!(pref_file) ||
        (prefs_set_block_label(pref_file, PREFS_BLOCK_NAME) < 0))
          return;
        
        if (prefs_write_param(param, pref_file->fp) < 0) {
          g_warning("failed to write Fancy Plugin configuration\n");
          prefs_file_close_revert(pref_file);
          return;
        }

        if (fprintf(pref_file->fp, "\n") < 0) {
        FILE_OP_ERROR(rc_file_path, "fprintf");
        prefs_file_close_revert(pref_file);
    } else
            prefs_file_close(pref_file);
}
