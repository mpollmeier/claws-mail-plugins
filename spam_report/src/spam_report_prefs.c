/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 Colin Leroy <colin@colino.net>
 * and the Claws Mail Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"

#include <glib.h>
#include "gettext.h"
#include <gtk/gtk.h>

#include "gtkutils.h"
#include "utils.h"
#include "prefs.h"
#include "prefs_gtk.h"
#include "prefswindow.h"
#include "alertpanel.h"

#include "spam_report_prefs.h"

#define PREFS_BLOCK_NAME "SpamReport"

SpamReportPrefs spamreport_prefs;
void spamreport_clear_cache(void);

typedef struct _SpamReportPage SpamReportPage;

struct _SpamReportPage {
        PrefsPage page;
        GtkWidget *frame[INTF_LAST];
	GtkWidget *user_entry[INTF_LAST];
	GtkWidget *pass_entry[INTF_LAST];
};

static PrefParam param[] = {
        {"signalspam_user", "", &spamreport_prefs.user[INTF_SIGNALSPAM], P_STRING, NULL, NULL, NULL},
        {"signalspam_pass", "", &spamreport_prefs.pass[INTF_SIGNALSPAM], P_PASSWORD, NULL, NULL, NULL},
        {0,0,0,0,0,0,0}
};

static SpamReportPage spamreport_prefs_page;

static void create_spamreport_prefs_page	(PrefsPage *page,
					 GtkWindow *window,
					 gpointer   data);
static void destroy_spamreport_prefs_page	(PrefsPage *page);
static void save_spamreport_prefs		(PrefsPage *page);

void spamreport_prefs_init(void)
{
	static gchar *path[3];
	gchar *rcpath;

	path[0] = _("Plugins");
	path[1] = _("SpamReport");
	path[2] = NULL;

        prefs_set_default(param);
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
        prefs_read_config(param, PREFS_BLOCK_NAME, rcpath, NULL);
	g_free(rcpath);
        
        spamreport_prefs_page.page.path = path;
        spamreport_prefs_page.page.create_widget = create_spamreport_prefs_page;
        spamreport_prefs_page.page.destroy_widget = destroy_spamreport_prefs_page;
        spamreport_prefs_page.page.save_page = save_spamreport_prefs;
	spamreport_prefs_page.page.weight = 30.0;
        
        prefs_gtk_register_page((PrefsPage *) &spamreport_prefs_page);
}

void spamreport_prefs_done(void)
{
        prefs_gtk_unregister_page((PrefsPage *) &spamreport_prefs_page);
}

static void create_spamreport_prefs_page(PrefsPage *page,
				    GtkWindow *window,
                                    gpointer data)
{
        SpamReportPage *prefs_page = (SpamReportPage *) page;

        GtkWidget *vbox, *table;
	GtkWidget *tmp;
	int i = 0;
	
        vbox = gtk_vbox_new(FALSE, 3);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), VBOX_BORDER);
        
	for (i = 0; i < INTF_LAST; i++) {
		prefs_page->frame[i] = gtk_frame_new(spam_interfaces[i].name);
		gtk_box_pack_start(GTK_BOX(vbox), prefs_page->frame[i], FALSE, FALSE, 6);

		prefs_page->user_entry[i] = gtk_entry_new();
		prefs_page->pass_entry[i] = gtk_entry_new();
		gtk_entry_set_visibility(GTK_ENTRY(prefs_page->pass_entry[i]), FALSE);

		gtk_entry_set_text(GTK_ENTRY(prefs_page->user_entry[i]),
			spamreport_prefs.user[i] ? spamreport_prefs.user[i]:"");
		gtk_entry_set_text(GTK_ENTRY(prefs_page->pass_entry[i]),
			spamreport_prefs.pass[i] ? spamreport_prefs.pass[i]:"");


		table = gtk_table_new(2, 2, FALSE);

		gtk_container_add(GTK_CONTAINER(prefs_page->frame[i]), table);
		tmp = gtk_label_new(_("Username:"));
		gtk_table_attach(GTK_TABLE(table), tmp, 0, 1, 0, 1,
				0, 0, 
				0, 0);
		gtk_table_attach(GTK_TABLE(table), prefs_page->user_entry[i], 1, 2, 0, 1,
				GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 
				0, 0);

		tmp = gtk_label_new(_("Password:"));
		gtk_table_attach(GTK_TABLE(table), tmp, 0, 1, 1, 2,
				0, 0, 
				0, 0);
		gtk_table_attach(GTK_TABLE(table), prefs_page->pass_entry[i], 1, 2, 1, 2,
				GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 
				0, 0);
	}
	gtk_widget_show_all(vbox);
        prefs_page->page.widget = vbox;
}

static void destroy_spamreport_prefs_page(PrefsPage *page)
{
	/* Do nothing! */
}

static void save_spamreport_prefs(PrefsPage *page)
{
        SpamReportPage *prefs_page = (SpamReportPage *) page;
        PrefFile *pref_file;
        gchar *rc_file_path = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S,
                                          COMMON_RC, NULL);
        int i = 0;
	
	for (i = 0; i < INTF_LAST; i++) {

        	g_free(spamreport_prefs.user[i]);
		g_free(spamreport_prefs.pass[i]);

		spamreport_prefs.user[i] = gtk_editable_get_chars(
			GTK_EDITABLE(prefs_page->user_entry[i]), 0, -1);
		spamreport_prefs.pass[i] = gtk_editable_get_chars(
			GTK_EDITABLE(prefs_page->pass_entry[i]), 0, -1);
	}

        pref_file = prefs_write_open(rc_file_path);
        g_free(rc_file_path);
        
        if (!(pref_file) ||
	    (prefs_set_block_label(pref_file, PREFS_BLOCK_NAME) < 0))
          return;
        
        if (prefs_write_param(param, pref_file->fp) < 0) {
          g_warning("failed to write SpamReport Plugin configuration\n");
          prefs_file_close_revert(pref_file);
          return;
        }
        fprintf(pref_file->fp, "\n");
        prefs_file_close(pref_file);
}