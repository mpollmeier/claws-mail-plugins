/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto & the Sylpheed-Claws team
 * This file (C) 2005 Colin Leroy <colin@colino.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#  include "pluginconfig.h"
#endif

#include "defs.h"

#include <glib.h>

#include "common/utils.h"
#include "prefs.h"
#include "prefs_gtk.h"
#include "prefswindow.h"
#include "gettext.h"

#include "vcal_prefs.h"

#define PREFS_BLOCK_NAME "VCalendar"

struct VcalendarPage
{
	PrefsPage page;
	
	GtkWidget *alert_enable_btn;
	GtkWidget *alert_delay_entry;
	GtkWidget *export_enable_btn;
	GtkWidget *export_path_entry;
	GtkWidget *export_command_entry;
};

VcalendarPrefs vcalprefs;
static struct VcalendarPage vcal_prefs_page;

static PrefParam param[] = {
	{"alert_delay", "10", &vcalprefs.alert_delay, P_INT,
	 NULL, NULL, NULL},
	{"alert_enable", "FALSE", &vcalprefs.alert_enable, P_BOOL,
	 NULL, NULL, NULL},
	{"export_enable", "FALSE", &vcalprefs.export_enable, P_BOOL,
	 NULL, NULL, NULL},
	{"export_path", NULL, &vcalprefs.export_path, P_STRING,
	 NULL, NULL, NULL},
	{"export_command", NULL, &vcalprefs.export_command, P_STRING,
	 NULL, NULL, NULL},
	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static void vcal_prefs_create_widget_func(PrefsPage * _page,
					   GtkWindow * window,
					   gpointer data)
{
	struct VcalendarPage *page = (struct VcalendarPage *) _page;

	/* ------------------ code made by glade -------------------- */
	GtkWidget *table;
	GtkWidget *hbox;
	GtkWidget *alert_enable_btn;
	GtkWidget *alert_delay_entry;
	GtkWidget *export_enable_btn;
	GtkWidget *export_path_entry;
	GtkWidget *export_command_entry;
	GtkWidget *label;
	gchar *delay = NULL;
	
	table = gtk_table_new(3, 1, FALSE);
	gtk_widget_show(table);
	gtk_container_set_border_width(GTK_CONTAINER(table), 8);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

/* alert stuff */
	alert_enable_btn = gtk_check_button_new_with_label(_("Alert me "));
	gtk_widget_show(alert_enable_btn);
	
	alert_delay_entry = gtk_entry_new();
	
	gtk_widget_set_size_request(alert_delay_entry, 30, -1);
	
	gtk_widget_show(alert_delay_entry);
	
	label = gtk_label_new(_(" minutes before an event"));

	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), alert_enable_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), alert_delay_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	gtk_widget_show(hbox);

	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 0, 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(alert_enable_btn), 
			vcalprefs.alert_enable);
	delay = g_strdup_printf("%d", vcalprefs.alert_delay);
	gtk_entry_set_text(GTK_ENTRY(alert_delay_entry), delay);
	g_free(delay);
	
	page->alert_enable_btn = alert_enable_btn;
	page->alert_delay_entry = alert_delay_entry;

/* export stuff */
	export_enable_btn = gtk_check_button_new_with_label(_("Automatically export calendar to "));
	gtk_widget_show(export_enable_btn);
	
	export_path_entry = gtk_entry_new();
		
	gtk_widget_show(export_path_entry);
	
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), export_enable_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), export_path_entry, FALSE, FALSE, 0);

	gtk_widget_show(hbox);

	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 1, 2,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);

	label = gtk_label_new(_("Command to run after export: "));

	gtk_widget_show(label);

	export_command_entry = gtk_entry_new();
		
	gtk_widget_show(export_command_entry);
	
	hbox = gtk_hbox_new(FALSE, 6);	
	
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), export_command_entry, FALSE, FALSE, 0);
	
	gtk_widget_show(hbox);

	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 2, 3,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(export_enable_btn), 
			vcalprefs.export_enable);
	
	if (vcalprefs.export_path == NULL)
		vcalprefs.export_path = g_strconcat(get_rc_dir(), 
					G_DIR_SEPARATOR_S,
                                        "sylpheed-claws.ics", NULL);
	if (vcalprefs.export_command == NULL)
		vcalprefs.export_command = g_strdup("");

	gtk_entry_set_text(GTK_ENTRY(export_path_entry), 
			vcalprefs.export_path);
	gtk_entry_set_text(GTK_ENTRY(export_command_entry), 
			vcalprefs.export_command);
	
	page->export_enable_btn = export_enable_btn;
	page->export_path_entry = export_path_entry;
	page->export_command_entry = export_command_entry;

	page->page.widget = table;
}

static void vcal_prefs_destroy_widget_func(PrefsPage *_page)
{
}

static void vcal_prefs_save_func(PrefsPage * _page)
{
	struct VcalendarPage *page = (struct VcalendarPage *) _page;
	PrefFile *pfile;
	gchar *rcpath;

	vcalprefs.alert_enable =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (page->alert_enable_btn));
	vcalprefs.alert_delay =
	    atoi(gtk_entry_get_text(GTK_ENTRY(page->alert_delay_entry)));

	vcalprefs.export_enable = 
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (page->export_enable_btn));
	
	g_free(vcalprefs.export_path);
	vcalprefs.export_path =
	    gtk_editable_get_chars(GTK_EDITABLE(page->export_path_entry), 0, -1);

	g_free(vcalprefs.export_command);
	vcalprefs.export_command =
	    gtk_editable_get_chars(GTK_EDITABLE(page->export_command_entry), 0, -1);
	
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	pfile = prefs_write_open(rcpath);
	g_free(rcpath);
	if (!pfile || (prefs_set_block_label(pfile, PREFS_BLOCK_NAME) < 0))
		return;

	if (prefs_write_param(param, pfile->fp) < 0) {
		g_warning("failed to write Vcalendar configuration to file\n");
		prefs_file_close_revert(pfile);
		return;
	}
	fprintf(pfile->fp, "\n");

	prefs_file_close(pfile);
}

void vcal_prefs_init(void)
{
	static gchar *path[3];
	gchar *rcpath;

	path[0] = _("Plugins");
	path[1] = _("vCalendar");
	path[2] = NULL;

	prefs_set_default(param);
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	prefs_read_config(param, PREFS_BLOCK_NAME, rcpath, NULL);
	g_free(rcpath);

	vcal_prefs_page.page.path = path;
	vcal_prefs_page.page.create_widget = vcal_prefs_create_widget_func;
	vcal_prefs_page.page.destroy_widget = vcal_prefs_destroy_widget_func;
	vcal_prefs_page.page.save_page = vcal_prefs_save_func;

	prefs_gtk_register_page((PrefsPage *) &vcal_prefs_page);
}

void vcal_prefs_done(void)
{
	prefs_gtk_unregister_page((PrefsPage *) &vcal_prefs_page);
}
