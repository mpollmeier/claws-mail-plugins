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
	GtkWidget *alert_delay_spinbtn;
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

	GtkWidget *vbox1, *vbox2;
	GtkWidget *hbox1, *hbox2, *hbox3;
	GtkWidget *alert_enable_checkbtn;
	GtkObject *alert_enable_spinbtn_adj;
	GtkWidget *alert_enable_spinbtn;
	GtkWidget *label_alert_enable;
	GtkWidget *export_enable_checkbtn;
	GtkWidget *export_path_entry;
	GtkWidget *export_command_label;
	GtkWidget *export_command_entry;

	vbox1 = gtk_vbox_new (FALSE, VSPACING);
	gtk_widget_show (vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), VBOX_BORDER);

	vbox2 = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vbox2);
	gtk_box_pack_start(GTK_BOX (vbox1), vbox2, FALSE, FALSE, 0);


/* alert stuff */
	hbox1 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox1);
	gtk_box_pack_start(GTK_BOX (vbox2), hbox1, TRUE, TRUE, 0);

	alert_enable_checkbtn = gtk_check_button_new_with_label(_("Alert me "));
	gtk_widget_show (alert_enable_checkbtn);
	gtk_box_pack_start(GTK_BOX (hbox1), alert_enable_checkbtn, FALSE, FALSE, 0);

	alert_enable_spinbtn_adj = gtk_adjustment_new (10, 1, 24*60, 1, 10, 10);
	alert_enable_spinbtn = gtk_spin_button_new
		(GTK_ADJUSTMENT (alert_enable_spinbtn_adj), 1, 0);
	gtk_widget_set_size_request (alert_enable_spinbtn, 64, -1);
	gtk_widget_show (alert_enable_spinbtn);
	gtk_box_pack_start(GTK_BOX (hbox1), alert_enable_spinbtn, FALSE, FALSE, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (alert_enable_spinbtn), TRUE);

	label_alert_enable = gtk_label_new(_(" minutes before an event"));
	gtk_widget_show (label_alert_enable);
	gtk_box_pack_start(GTK_BOX (hbox1), label_alert_enable, FALSE, FALSE, 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(alert_enable_checkbtn), 
			vcalprefs.alert_enable);
	SET_TOGGLE_SENSITIVITY(alert_enable_checkbtn, alert_enable_spinbtn);


/* export enable + path stuff */
	hbox2 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox2);
	gtk_box_pack_start(GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

	export_enable_checkbtn = gtk_check_button_new_with_label(_("Automatically export calendar to "));
	gtk_widget_show(export_enable_checkbtn);
	gtk_box_pack_start(GTK_BOX (hbox2), export_enable_checkbtn, FALSE, FALSE, 0);

	export_path_entry = gtk_entry_new();
	gtk_widget_show(export_path_entry);
	gtk_box_pack_start(GTK_BOX(hbox2), export_path_entry, TRUE, TRUE, 0);
	SET_TOGGLE_SENSITIVITY(export_enable_checkbtn, export_path_entry);


/* run-command after export stuff */
	hbox3 = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox3);
	gtk_box_pack_start(GTK_BOX (vbox2), hbox3, TRUE, TRUE, 0);

	export_command_label = gtk_label_new(_("Command to run after export: "));
	gtk_widget_show(export_command_label);
	gtk_box_pack_start(GTK_BOX (hbox3), export_command_label, FALSE, FALSE, 0);

	export_command_entry = gtk_entry_new();
	gtk_widget_show(export_command_entry);
	gtk_box_pack_start(GTK_BOX (hbox3), export_command_entry, TRUE, TRUE, 0);


	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(export_enable_checkbtn), 
			vcalprefs.export_enable);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(alert_enable_spinbtn),
			vcalprefs.alert_delay);
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

	page->alert_enable_btn = alert_enable_checkbtn;
	page->alert_delay_spinbtn = alert_enable_spinbtn;
	page->export_enable_btn = export_enable_checkbtn;
	page->export_path_entry = export_path_entry;
	page->export_command_entry = export_command_entry;

	page->page.widget = vbox1;
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
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
						 (page->alert_delay_spinbtn));

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
