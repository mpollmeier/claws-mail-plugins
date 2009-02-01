/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2006 Hiroyuki Yamamoto and the Claws Mail Team
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * The structure of this file has been borrowed from the structure of
 * the image_viewer plugin file. I also used it as an example of how to
 * build the preferences for the gtkhtml2 plugin.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include "gettext.h"
#include <gtk/gtk.h>

#include "gtkutils.h"
#include "prefs.h"
#include "prefs_gtk.h"
#include "prefswindow.h"
#include "alertpanel.h"
#include "utils.h"
#include "addressbook.h"
#include "gtkhtml2_prefs.h"
#include "combobox.h"

#define PREFS_BLOCK_NAME "gtkhtml2"

GtkHtmlBrowserPrefs gtkhtml_prefs;
void gtkhtml2_viewer_clear_cache(void);

typedef struct _GtkHtmlBrowserPage GtkHtmlBrowserPage;

struct _GtkHtmlBrowserPage {
        PrefsPage page;
        GtkWidget *local;
	GtkWidget *whitelist_ab;
	GtkWidget *whitelist_ab_folder_combo;
	GtkWidget *whitelist_ab_select_btn;
	GtkWidget *cache_images;
	GtkWidget *clear_cache;
	GtkWidget *empty_cache;
};

static PrefParam param[] = {
        {"local_browse", "TRUE", &gtkhtml_prefs.local, P_BOOL, NULL, NULL, NULL},
        {"cache_images", "TRUE", &gtkhtml_prefs.cache_images, P_BOOL, NULL, NULL, NULL},
        {"clear_cache", "TRUE", &gtkhtml_prefs.clear_cache, P_BOOL, NULL, NULL, NULL},
	{"whitelist_ab", "FALSE", &gtkhtml_prefs.whitelist_ab, P_BOOL,
	 NULL, NULL, NULL},
	{"whitelist_ab_folder", N_("Any"), &gtkhtml_prefs.whitelist_ab_folder, P_STRING,
	 NULL, NULL, NULL},
        {0,0,0,0,0,0,0}
};

static GtkHtmlBrowserPage gtkhtml_prefs_page;

static void gtkhtml_whitelist_ab_select_cb(GtkWidget *widget, gpointer data)
{
	GtkHtmlBrowserPage *page = (GtkHtmlBrowserPage *) data;
	const gchar *folderpath = NULL;
	gchar *new_path = NULL;

	folderpath = gtk_entry_get_text(GTK_ENTRY(GTK_BIN(page->whitelist_ab_folder_combo)->child));
	new_path = addressbook_folder_selection(folderpath);
	if (new_path) {
		gtk_entry_set_text(GTK_ENTRY(GTK_BIN(page->whitelist_ab_folder_combo)->child), new_path);
		g_free(new_path);
	}
}

static void create_gtkhtml_prefs_page	(PrefsPage *page,
					 GtkWindow *window,
					 gpointer   data);
static void destroy_gtkhtml_prefs_page	(PrefsPage *page);
static void save_gtkhtml_prefs		(PrefsPage *page);

void gtkhtml_prefs_init(void)
{
	static gchar *path[3];
	gchar *rcpath;

	path[0] = _("Plugins");
	path[1] = _("GtkHtml2 Viewer");
	path[2] = NULL;

        prefs_set_default(param);
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
        prefs_read_config(param, PREFS_BLOCK_NAME, rcpath, NULL);
	g_free(rcpath);
        
        gtkhtml_prefs_page.page.path = path;
        gtkhtml_prefs_page.page.create_widget = create_gtkhtml_prefs_page;
        gtkhtml_prefs_page.page.destroy_widget = destroy_gtkhtml_prefs_page;
        gtkhtml_prefs_page.page.save_page = save_gtkhtml_prefs;
	gtkhtml_prefs_page.page.weight = 30.0;
        
        prefs_gtk_register_page((PrefsPage *) &gtkhtml_prefs_page);
}

void gtkhtml_prefs_done(void)
{
        prefs_gtk_unregister_page((PrefsPage *) &gtkhtml_prefs_page);
}

static void local_checkbox_toggled(GtkToggleButton *button,
					  gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
        GtkHtmlBrowserPage *prefs_page = (GtkHtmlBrowserPage *) user_data;

	gtk_widget_set_sensitive(prefs_page->whitelist_ab, active);
	gtk_widget_set_sensitive(prefs_page->whitelist_ab_folder_combo, active && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_page->whitelist_ab)));
	gtk_widget_set_sensitive(prefs_page->whitelist_ab_select_btn, active && 
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_page->whitelist_ab)));
	
	active &= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_page->cache_images));
}

static void whitelist_checkbox_toggled(GtkToggleButton *button,
					  gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
        GtkHtmlBrowserPage *prefs_page = (GtkHtmlBrowserPage *) user_data;

	active &= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_page->local));

	gtk_widget_set_sensitive(prefs_page->whitelist_ab_folder_combo, active);
	gtk_widget_set_sensitive(prefs_page->whitelist_ab_select_btn, active);
}

static void cache_images_checkbox_toggled(GtkToggleButton *button,
					  gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
        GtkHtmlBrowserPage *prefs_page = (GtkHtmlBrowserPage *) user_data;

	gtk_widget_set_sensitive(prefs_page->clear_cache, active);
}

static void empty_cache_btn_clicked(GtkButton *button,
					  gpointer user_data)
{
	gtkhtml2_viewer_clear_cache();
	alertpanel_notice(_("Cache cleared."));
}

static void create_gtkhtml_prefs_page(PrefsPage *page,
				    GtkWindow *window,
                                    gpointer data)
{
        GtkHtmlBrowserPage *prefs_page = (GtkHtmlBrowserPage *) page;

        GtkWidget *vbox, *hbox;
        GtkWidget *local_checkbox;
	
	GtkWidget *whitelist_ab_checkbtn;
	GtkWidget *whitelist_ab_folder_combo;
	GtkWidget *whitelist_ab_select_btn;

        GtkWidget *cache_images_checkbox;
        GtkWidget *clear_cache_checkbox;
        GtkWidget *empty_cache_btn;
	GtkWidget *hbox_whitelist, *spacer;

        vbox = gtk_vbox_new(FALSE, 3);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), VBOX_BORDER);
        gtk_widget_show(vbox);
        
        local_checkbox = gtk_check_button_new_with_label
				(_("Automatically load remote images"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(local_checkbox),
                                     !gtkhtml_prefs.local);
        gtk_box_pack_start(GTK_BOX(vbox), local_checkbox, FALSE, FALSE, 0);
        gtk_widget_show(local_checkbox);
        
	hbox_whitelist = gtk_hbox_new(FALSE, 8);
	gtk_widget_show(hbox_whitelist);
	gtk_box_pack_start (GTK_BOX (vbox), hbox_whitelist, FALSE, FALSE, 0);
	
	spacer = gtk_hbox_new(FALSE, 0);
	gtk_widget_set_size_request(spacer, 12, -1);
	gtk_widget_show(spacer);
	gtk_box_pack_start(GTK_BOX(hbox_whitelist), spacer, FALSE, FALSE, 0);

	whitelist_ab_checkbtn = gtk_check_button_new_with_label(_("Only for senders found in address book"));
	gtk_widget_show(whitelist_ab_checkbtn);
	gtk_box_pack_start(GTK_BOX(hbox_whitelist), whitelist_ab_checkbtn, FALSE, FALSE, 0);

	whitelist_ab_folder_combo = combobox_text_new(TRUE, _("Any"), NULL);
	gtk_widget_set_size_request(whitelist_ab_folder_combo, 100, -1);
	gtk_box_pack_start (GTK_BOX (hbox_whitelist), whitelist_ab_folder_combo, TRUE, TRUE, 0);

	whitelist_ab_select_btn = gtk_button_new_with_label(_("Select ..."));
	gtk_widget_show (whitelist_ab_select_btn);
	gtk_box_pack_start (GTK_BOX (hbox_whitelist), whitelist_ab_select_btn, FALSE, FALSE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(whitelist_ab_checkbtn), gtkhtml_prefs.whitelist_ab);
	if (gtkhtml_prefs.whitelist_ab_folder != NULL) {
		/* translate "Any" (stored UNtranslated) */
		if (strcasecmp(gtkhtml_prefs.whitelist_ab_folder, "Any") == 0)
			gtk_entry_set_text(GTK_ENTRY(GTK_BIN(whitelist_ab_folder_combo)->child),
					gtkhtml_prefs.whitelist_ab_folder);
		else
		/* backward compatibility (when translated "Any" was stored) */
		if (g_utf8_collate(gtkhtml_prefs.whitelist_ab_folder, _("Any")) == 0)
			gtk_entry_set_text(GTK_ENTRY(GTK_BIN(whitelist_ab_folder_combo)->child),
					gtkhtml_prefs.whitelist_ab_folder);
		else
			gtk_entry_set_text(GTK_ENTRY(GTK_BIN(whitelist_ab_folder_combo)->child),
					gtkhtml_prefs.whitelist_ab_folder);
	}

        cache_images_checkbox = gtk_check_button_new_with_label
				(_("Cache remote images locally"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cache_images_checkbox),
                                     gtkhtml_prefs.cache_images);
        gtk_box_pack_start(GTK_BOX(vbox), cache_images_checkbox, FALSE, FALSE, 0);
        gtk_widget_show(cache_images_checkbox);
        
        clear_cache_checkbox = gtk_check_button_new_with_label
				(_("Clear image cache on exit"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clear_cache_checkbox),
                                     gtkhtml_prefs.clear_cache);
        gtk_box_pack_start(GTK_BOX(vbox), clear_cache_checkbox, FALSE, FALSE, 0);
        gtk_widget_show(clear_cache_checkbox);
        
        empty_cache_btn = gtk_button_new_with_label
				(_("Clear image cache now"));
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), empty_cache_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);
	gtk_widget_show(empty_cache_btn);
        
	g_signal_connect(G_OBJECT(local_checkbox), "toggled",
			 G_CALLBACK(local_checkbox_toggled),
			 prefs_page);

	g_signal_connect(G_OBJECT(whitelist_ab_checkbtn), "toggled",
			 G_CALLBACK(whitelist_checkbox_toggled),
			 prefs_page);

	gtk_widget_set_sensitive(whitelist_ab_checkbtn, !gtkhtml_prefs.local);
	gtk_widget_set_sensitive(whitelist_ab_folder_combo, !gtkhtml_prefs.local && gtkhtml_prefs.whitelist_ab);
	gtk_widget_set_sensitive(whitelist_ab_select_btn, !gtkhtml_prefs.local && gtkhtml_prefs.whitelist_ab);

	g_signal_connect(G_OBJECT(cache_images_checkbox), "toggled",
			 G_CALLBACK(cache_images_checkbox_toggled),
			 prefs_page);
	g_signal_connect(G_OBJECT(empty_cache_btn), "clicked",
			 G_CALLBACK(empty_cache_btn_clicked),
			 prefs_page);
	g_signal_connect(G_OBJECT (whitelist_ab_select_btn), "clicked",
			 G_CALLBACK(gtkhtml_whitelist_ab_select_cb), page);

        prefs_page->local = local_checkbox;
        prefs_page->cache_images = cache_images_checkbox;
        prefs_page->clear_cache = clear_cache_checkbox;
        prefs_page->empty_cache = empty_cache_btn;
	prefs_page->whitelist_ab = whitelist_ab_checkbtn;
	prefs_page->whitelist_ab_folder_combo = whitelist_ab_folder_combo;
	prefs_page->whitelist_ab_select_btn = whitelist_ab_select_btn;

        prefs_page->page.widget = vbox;
}

static void destroy_gtkhtml_prefs_page(PrefsPage *page)
{
	/* Do nothing! */
}

static void save_gtkhtml_prefs(PrefsPage *page)
{
        GtkHtmlBrowserPage *prefs_page = (GtkHtmlBrowserPage *) page;
        PrefFile *pref_file;
        gchar *rc_file_path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
                                          COMMON_RC, NULL);
        
        gtkhtml_prefs.local = !gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON(prefs_page->local));
        gtkhtml_prefs.cache_images = gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON(prefs_page->cache_images));
        gtkhtml_prefs.clear_cache = gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON(prefs_page->clear_cache));

	gtkhtml_prefs.whitelist_ab = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefs_page->whitelist_ab));
	g_free(gtkhtml_prefs.whitelist_ab_folder);
	gtkhtml_prefs.whitelist_ab_folder = gtk_editable_get_chars(
				GTK_EDITABLE(GTK_BIN(prefs_page->whitelist_ab_folder_combo)->child), 0, -1);
	/* store UNtranslated "Any" */
	if (g_utf8_collate(gtkhtml_prefs.whitelist_ab_folder, _("Any")) == 0) {
		g_free(gtkhtml_prefs.whitelist_ab_folder);
		gtkhtml_prefs.whitelist_ab_folder = g_strdup("Any");
	}

        pref_file = prefs_write_open(rc_file_path);
        g_free(rc_file_path);
        
        if (!(pref_file) ||
	    (prefs_set_block_label(pref_file, PREFS_BLOCK_NAME) < 0))
          return;
        
        if (prefs_write_param(param, pref_file->fp) < 0) {
          g_warning("failed to write GtkHtml Plugin configuration\n");
          prefs_file_close_revert(pref_file);
          return;
        }
        if (fprintf(pref_file->fp, "\n") < 0) {
		FILE_OP_ERROR(rc_file_path, "fprintf");
		prefs_file_close_revert(pref_file);
	} else
	        prefs_file_close(pref_file);
}
