/*
 * acpi_notifier -- for Sylpheed-Claws
 *
 * Copyright (C) 2005 Colin Leroy <colin@colino.net>
 *
 * Sylpheed is a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2005 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "defs.h"
#include "sylpheed.h"
#include "version.h"
#include "prefs.h"
#include "prefs_gtk.h"
#include "menu.h"
#include "hooks.h"

#define PREFS_BLOCK_NAME "AcpiNotifier"

typedef struct _PredefinedAcpis {
	gchar *name;
	gchar *on_param;
	gchar *off_param;
	gchar *file_path;
} PredefinedAcpis;

/**
 * Add your implementation here (and send me the patch!) 
 */
PredefinedAcpis known_implementations[] = {
	{"Other", "", "", ""},
	{"ACER", "1", "0", "/proc/driver/acerhk/led"},
	{"ASUS", "1", "0", "/proc/acpi/asus/mled"},
	{"IBM", "7 on", "7 off", "/proc/acpi/ibm/led"},
	{NULL, NULL, NULL, NULL}
};

static guint hook_id;

struct AcpiNotifierPage
{
	PrefsPage page;
	
	GtkWidget *no_mail_off_btn;
	GtkWidget *no_mail_blink_btn;
	GtkWidget *no_mail_on_btn;
	GtkWidget *unread_mail_off_btn;
	GtkWidget *unread_mail_blink_btn;
	GtkWidget *unread_mail_on_btn;
	GtkWidget *new_mail_off_btn;
	GtkWidget *new_mail_blink_btn;
	GtkWidget *new_mail_on_btn;
	GtkWidget *default_implementations_optmenu;
	GtkWidget *on_value_entry;
	GtkWidget *off_value_entry;
	GtkWidget *file_entry;
	GtkWidget *hbox_acpi_file;
	GtkWidget *hbox_acpi_values;
	GtkWidget *warning_label;
};

typedef struct _AcpiNotifierPrefs AcpiNotifierPrefs;

struct _AcpiNotifierPrefs
{
	gint		 no_mail_action;
	gint		 unread_mail_action;
	gint		 new_mail_action;
	gchar 		*on_param;	
	gchar 		*off_param;	
	gchar 		*file_path;	
};

AcpiNotifierPrefs acpiprefs;

static struct AcpiNotifierPage acpi_prefs_page;

enum {
	OFF = 0,
	BLINK,
	ON
} BlinkType;

static PrefParam param[] = {
	{"no_mail_action", "0", &acpiprefs.no_mail_action, P_INT,
	 NULL, NULL, NULL},
	{"unread_mail_action", "0", &acpiprefs.unread_mail_action, P_INT,
	 NULL, NULL, NULL},
	{"new_mail_action", "1", &acpiprefs.new_mail_action, P_INT,
	 NULL, NULL, NULL},
	{"on_param", NULL, &acpiprefs.on_param, P_STRING,
	 NULL, NULL, NULL},
	{"off_param", NULL, &acpiprefs.off_param, P_STRING,
	 NULL, NULL, NULL},
	{"file_path", NULL, &acpiprefs.file_path, P_STRING,
	 NULL, NULL, NULL},
	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static void type_activated(GtkMenuItem *menuitem, gpointer data)
{
	GtkWidget *menu, *item;
	struct AcpiNotifierPage *page = (struct AcpiNotifierPage *)data;
	gint selected;

	if (page->file_entry == NULL)
		return;
		
	menu = gtk_option_menu_get_menu(
		GTK_OPTION_MENU(page->default_implementations_optmenu));
	item = gtk_menu_get_active(GTK_MENU(menu));
	selected = GPOINTER_TO_INT
		(g_object_get_data(G_OBJECT(item), MENU_VAL_ID));

	if (selected != 0) {
		gtk_widget_hide(page->hbox_acpi_file);
		gtk_widget_hide(page->hbox_acpi_values);
		gtk_entry_set_text(GTK_ENTRY(page->file_entry), 
			known_implementations[selected].file_path);
		gtk_entry_set_text(GTK_ENTRY(page->on_value_entry), 
			known_implementations[selected].on_param);
		gtk_entry_set_text(GTK_ENTRY(page->off_value_entry), 
			known_implementations[selected].off_param);
		if (!is_file_exist(known_implementations[selected].file_path))
			gtk_widget_show(page->warning_label);
		else
			gtk_widget_hide(page->warning_label);
	} else {
		gtk_widget_show_all(page->hbox_acpi_file);
		gtk_widget_show_all(page->hbox_acpi_values);
	}
}
static void file_entry_changed (GtkWidget *entry, gpointer data)
{
	struct AcpiNotifierPage *page = (struct AcpiNotifierPage *)data;
	if (!page->warning_label)
		return;

	if (!is_file_exist(gtk_entry_get_text(GTK_ENTRY(entry))))
		gtk_widget_show(page->warning_label);
	else
		gtk_widget_hide(page->warning_label);
}

static void acpi_prefs_create_widget_func(PrefsPage * _page,
					   GtkWindow * window,
					   gpointer data)
{
	struct AcpiNotifierPage *page = (struct AcpiNotifierPage *) _page;

	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *hbox_acpi_file;
	GtkWidget *hbox_acpi_values;
	GtkWidget *start_label;
	GtkWidget *no_mail_label;
	GtkWidget *no_mail_off_btn;
	GtkWidget *no_mail_blink_btn;
	GtkWidget *no_mail_on_btn;
	GtkWidget *unread_mail_label;
	GtkWidget *unread_mail_off_btn;
	GtkWidget *unread_mail_blink_btn;
	GtkWidget *unread_mail_on_btn;
	GtkWidget *new_mail_label;
	GtkWidget *new_mail_off_btn;
	GtkWidget *new_mail_blink_btn;
	GtkWidget *new_mail_on_btn;
	GtkWidget *default_implementations_optmenu;
	GtkWidget *default_implementations_menu;
	GtkWidget *on_value_entry;
	GtkWidget *off_value_entry;
	GtkWidget *file_entry;
	GtkWidget *menuitem;
	GtkWidget *warning_label;
	int i;
	int found = 0;

	vbox = gtk_vbox_new(FALSE, 6);
	
	no_mail_label = gtk_label_new(_(" : no new or unread mail"));
	unread_mail_label = gtk_label_new(_(" : unread mail"));
	new_mail_label = gtk_label_new(_(" : new mail"));

	no_mail_off_btn = gtk_radio_button_new_with_label(NULL, _("off"));
	no_mail_blink_btn = gtk_radio_button_new_with_label_from_widget(
		GTK_RADIO_BUTTON(no_mail_off_btn), _("blinking"));
	no_mail_on_btn = gtk_radio_button_new_with_label_from_widget(
		GTK_RADIO_BUTTON(no_mail_off_btn), _("on"));

	unread_mail_off_btn = gtk_radio_button_new_with_label(NULL, _("off"));
	unread_mail_blink_btn = gtk_radio_button_new_with_label_from_widget(
		GTK_RADIO_BUTTON(unread_mail_off_btn), _("blinking"));
	unread_mail_on_btn = gtk_radio_button_new_with_label_from_widget(
		GTK_RADIO_BUTTON(unread_mail_off_btn), _("on"));

	new_mail_off_btn = gtk_radio_button_new_with_label(NULL, _("off"));
	new_mail_blink_btn = gtk_radio_button_new_with_label_from_widget(
		GTK_RADIO_BUTTON(new_mail_off_btn), _("blinking"));
	new_mail_on_btn = gtk_radio_button_new_with_label_from_widget(
		GTK_RADIO_BUTTON(new_mail_off_btn), _("on"));

	on_value_entry = gtk_entry_new();
	off_value_entry = gtk_entry_new();
	file_entry = gtk_entry_new();
	gtk_widget_set_size_request(on_value_entry, 40, -1);
	gtk_widget_set_size_request(off_value_entry, 40, -1);

	default_implementations_optmenu = gtk_option_menu_new ();
	default_implementations_menu = gtk_menu_new();
        gtk_option_menu_set_menu (
			GTK_OPTION_MENU(default_implementations_optmenu), 
			default_implementations_menu);
	for (i = 0; known_implementations[i].name != NULL; i++) {
		MENUITEM_ADD (default_implementations_menu, 
				menuitem, known_implementations[i].name, i);
                g_signal_connect(G_OBJECT(menuitem), "activate",
                                 G_CALLBACK(type_activated),
                                 page);
	}
	
	hbox = gtk_hbox_new(FALSE, 6);
	start_label = gtk_label_new(_("LED "));
	gtk_box_pack_start(GTK_BOX(hbox), start_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), no_mail_off_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), no_mail_blink_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), no_mail_on_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), no_mail_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 6);
	start_label = gtk_label_new(_("LED "));
	gtk_box_pack_start(GTK_BOX(hbox), 
			start_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			unread_mail_off_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			unread_mail_blink_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			unread_mail_on_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			unread_mail_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), 
			hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 6);
	start_label = gtk_label_new(_("LED "));
	gtk_box_pack_start(GTK_BOX(hbox), 
			start_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			new_mail_off_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			new_mail_blink_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			new_mail_on_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			new_mail_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), 
			hbox, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 6);
	start_label = gtk_label_new(_("ACPI type: "));
	gtk_box_pack_start(GTK_BOX(hbox), 
			start_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), 
			default_implementations_optmenu, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), 
			hbox, FALSE, FALSE, 0);

	hbox_acpi_file = gtk_hbox_new(FALSE, 6);
	start_label = gtk_label_new(_("ACPI file: "));
	gtk_box_pack_start(GTK_BOX(hbox_acpi_file), 
			start_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_acpi_file), 
			file_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), 
			hbox_acpi_file, FALSE, FALSE, 0);
        g_signal_connect(G_OBJECT(file_entry), "changed",
                         G_CALLBACK(file_entry_changed), page);

	hbox_acpi_values = gtk_hbox_new(FALSE, 6);
	start_label = gtk_label_new(_("values - On: "));
	gtk_box_pack_start(GTK_BOX(hbox_acpi_values), 
			start_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_acpi_values), 
			on_value_entry, FALSE, FALSE, 0);
	start_label = gtk_label_new(_(" - Off: "));
	gtk_box_pack_start(GTK_BOX(hbox_acpi_values), 
			start_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_acpi_values), 
			off_value_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), 
			hbox_acpi_values, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 6);
	warning_label = gtk_label_new(
			_("Warning: control file doesn't exist."));
	gtk_box_pack_start(GTK_BOX(hbox), warning_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);
	gtk_widget_hide(warning_label);

	switch (acpiprefs.no_mail_action) {
	case OFF: 	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(no_mail_off_btn), TRUE); 
		break;
	case BLINK:	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(no_mail_blink_btn), TRUE); 
		break;
	case ON:	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(no_mail_on_btn), TRUE); 
		break;
	}
	
	switch (acpiprefs.unread_mail_action) {
	case OFF: 	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(unread_mail_off_btn), TRUE); 
		break;
	case BLINK:	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(unread_mail_blink_btn), TRUE); 
		break;
	case ON:	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(unread_mail_on_btn), TRUE); 
		break;
	}
	
	switch (acpiprefs.new_mail_action) {
	case OFF: 	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(new_mail_off_btn), TRUE); 
		break;
	case BLINK:	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(new_mail_blink_btn), TRUE); 
		break;
	case ON:	
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(new_mail_on_btn), TRUE); 
		break;
	}
	
	if (acpiprefs.file_path != NULL) {
		for (i = 0; known_implementations[i].name != NULL; i++) {
			if (!strcmp(acpiprefs.file_path, 
				    known_implementations[i].file_path)) {
				gtk_option_menu_set_history(
					GTK_OPTION_MENU(
					default_implementations_optmenu), i);
				found = i;
			}
		}
	}
	if (found == 0) {
		for (i = 0; known_implementations[i].name != NULL; i++) {
			if (is_file_exist(known_implementations[i].file_path)) {
				gtk_option_menu_set_history(
					GTK_OPTION_MENU(
					default_implementations_optmenu), i);
				found = i;
			}
		}
	}
	if (found != 0) {
		gtk_widget_hide(hbox_acpi_file);
		gtk_widget_hide(hbox_acpi_values);
		gtk_entry_set_text(GTK_ENTRY(file_entry), 
			known_implementations[found].file_path);
		gtk_entry_set_text(GTK_ENTRY(on_value_entry), 
			known_implementations[found].on_param);
		gtk_entry_set_text(GTK_ENTRY(off_value_entry), 
			known_implementations[found].off_param);
		
		if (!is_file_exist(known_implementations[found].file_path))
			gtk_widget_show(warning_label);
	} else {
		gtk_option_menu_set_history(
			GTK_OPTION_MENU(default_implementations_optmenu), 0);
		gtk_widget_show_all(hbox_acpi_file);
		gtk_widget_show_all(hbox_acpi_values);
		if (acpiprefs.file_path != NULL)
			gtk_entry_set_text(GTK_ENTRY(file_entry), 
					acpiprefs.file_path);
		if (acpiprefs.on_param != NULL)
			gtk_entry_set_text(GTK_ENTRY(on_value_entry), 
					acpiprefs.on_param);
		if (acpiprefs.off_param != NULL)
			gtk_entry_set_text(GTK_ENTRY(off_value_entry), 
					acpiprefs.off_param);
		if (!acpiprefs.file_path || !is_file_exist(acpiprefs.file_path))
			gtk_widget_show(warning_label);
	}
	
	page->page.widget = vbox;

	page->no_mail_off_btn = no_mail_off_btn;
	page->no_mail_blink_btn = no_mail_blink_btn;
	page->no_mail_on_btn = no_mail_on_btn;
	page->unread_mail_off_btn = unread_mail_off_btn;
	page->unread_mail_blink_btn = unread_mail_blink_btn;
	page->unread_mail_on_btn = unread_mail_on_btn;
	page->new_mail_off_btn = new_mail_off_btn;
	page->new_mail_blink_btn = new_mail_blink_btn;
	page->new_mail_on_btn = new_mail_on_btn;
	page->default_implementations_optmenu = default_implementations_optmenu;
	page->on_value_entry = on_value_entry;
	page->off_value_entry = off_value_entry;
	page->file_entry = file_entry;
	page->hbox_acpi_file = hbox_acpi_file;
	page->hbox_acpi_values = hbox_acpi_values;
	page->warning_label = warning_label;
}

static void acpi_prefs_destroy_widget_func(PrefsPage *_page)
{
}

static void acpi_prefs_save_func(PrefsPage * _page)
{
	struct AcpiNotifierPage *page = (struct AcpiNotifierPage *) _page;
	PrefFile *pfile;
	gchar *rcpath;
	GtkWidget *menu;
	GtkWidget *menuitem;
	gint selected = 0;

	g_free(acpiprefs.file_path);
	acpiprefs.file_path = gtk_editable_get_chars(
			GTK_EDITABLE(page->file_entry), 0, -1);
	g_free(acpiprefs.on_param);
	acpiprefs.on_param = gtk_editable_get_chars(
			GTK_EDITABLE(page->on_value_entry), 0, -1);
	g_free(acpiprefs.off_param);
	acpiprefs.off_param = gtk_editable_get_chars(
			GTK_EDITABLE(page->off_value_entry), 0, -1);

	if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->no_mail_off_btn)))
		acpiprefs.no_mail_action = OFF;
	else if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->no_mail_blink_btn)))
		acpiprefs.no_mail_action = BLINK;
	else if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->no_mail_on_btn)))
		acpiprefs.no_mail_action = ON;
	
	if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->unread_mail_off_btn)))
		acpiprefs.unread_mail_action = OFF;
	else if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->unread_mail_blink_btn)))
		acpiprefs.unread_mail_action = BLINK;
	else if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->unread_mail_on_btn)))
		acpiprefs.unread_mail_action = ON;
	
	if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->new_mail_off_btn)))
		acpiprefs.new_mail_action = OFF;
	else if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->new_mail_blink_btn)))
		acpiprefs.new_mail_action = BLINK;
	else if (gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(page->new_mail_on_btn)))
		acpiprefs.new_mail_action = ON;
	
	menu = gtk_option_menu_get_menu(
		GTK_OPTION_MENU(page->default_implementations_optmenu));
	menuitem = gtk_menu_get_active(GTK_MENU(menu));
	selected = GPOINTER_TO_INT
		(g_object_get_data(G_OBJECT(menuitem), MENU_VAL_ID));

	if (selected != 0) {
		g_free(acpiprefs.file_path);
		acpiprefs.file_path = g_strdup(
				known_implementations[selected].file_path);
		g_free(acpiprefs.on_param);
		acpiprefs.on_param = g_strdup(
				known_implementations[selected].on_param);
		g_free(acpiprefs.off_param);
		acpiprefs.off_param = g_strdup(
				known_implementations[selected].off_param);
	}

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	pfile = prefs_write_open(rcpath);
	g_free(rcpath);
	if (!pfile || (prefs_set_block_label(pfile, PREFS_BLOCK_NAME) < 0))
		return;

	if (prefs_write_param(param, pfile->fp) < 0) {
		g_warning("failed to write " PREFS_BLOCK_NAME 
			  " configuration to file\n");
		prefs_file_close_revert(pfile);
		return;
	}
	fprintf(pfile->fp, "\n");

	prefs_file_close(pfile);
}

static void acpi_set(gboolean on)
{
	FILE *fp = NULL;
	if (!acpiprefs.file_path)
		return;
	if (!is_file_exist(acpiprefs.file_path))
		return;
	if (!acpiprefs.on_param || !acpiprefs.off_param)
		return;
		
	fp = fopen(acpiprefs.file_path, "wb");
	if (fp == NULL)
		return;

	if (on) {
		fwrite(acpiprefs.on_param, 1, strlen(acpiprefs.on_param), fp);
	} else {
		fwrite(acpiprefs.off_param, 1, strlen(acpiprefs.off_param), fp);
	}
	fclose(fp);
}

static guint should_quit = FALSE;
static GThread *thread = NULL;
static guint my_new = 0, my_unread = 0;
static gpointer update_led_thread(gpointer data)
{
	int last_blink = 0;
	int action = 0;

	while (!should_quit) {
		sleep(1);
		
		if (my_new > 0) {
			action = acpiprefs.new_mail_action;
		} else if (my_unread > 0) {
			action = acpiprefs.unread_mail_action;
		} else {
			action = acpiprefs.no_mail_action;
		}
		
		switch (action) {
		case ON: 
			acpi_set(TRUE); 
			break;
		case BLINK:	
			acpi_set(last_blink); 
			last_blink = !last_blink; 
			break;
		case OFF:	
			acpi_set(FALSE); 
			break;
		}
	}
	acpi_set(FALSE);
	return NULL;
}

static gboolean acpi_update_hook(gpointer source, gpointer data)
{
	gint new, unread, unreadmarked, marked, total;
	folder_count_total_msgs(&new, &unread, &unreadmarked, &marked, &total);

	my_new = new;
	my_unread = unread;
	
	return FALSE;
}

void acpi_prefs_init(void)
{
	static gchar *path[3];
	gchar *rcpath;
	GError *error;

	path[0] = _("Plugins");
	path[1] = _("Acpi Notifier");
	path[2] = NULL;

	prefs_set_default(param);
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	prefs_read_config(param, PREFS_BLOCK_NAME, rcpath, NULL);
	g_free(rcpath);

	acpi_prefs_page.page.path = path;
	acpi_prefs_page.page.create_widget = acpi_prefs_create_widget_func;
	acpi_prefs_page.page.destroy_widget = acpi_prefs_destroy_widget_func;
	acpi_prefs_page.page.save_page = acpi_prefs_save_func;

	prefs_gtk_register_page((PrefsPage *) &acpi_prefs_page);
	hook_id = hooks_register_hook (FOLDER_ITEM_UPDATE_HOOKLIST, 
			acpi_update_hook, NULL);
	
	should_quit = FALSE;
	my_new = 0;
	my_unread = 0;
	
	thread = g_thread_create(update_led_thread, NULL, TRUE, &error);
}

void acpi_prefs_done(void)
{
	prefs_gtk_unregister_page((PrefsPage *) &acpi_prefs_page);
	hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_id);
	should_quit = TRUE;
	g_thread_join(thread);
	thread = NULL;
}


void acpi_init(void)
{
	acpi_prefs_init();
}

void acpi_done(void)
{
	acpi_prefs_done();
}

gint plugin_init(gchar **error)
{
	if((sylpheed_get_version() > VERSION_NUMERIC)) {
		*error = g_strdup("Your sylpheed version is newer than the "
				  "version the plugin was built with");
		return -1;
	}
	if((sylpheed_get_version() < MAKE_NUMERIC_VERSION(1, 9, 13, 0))) {
		*error = g_strdup("Your sylpheed version is too old");
		return -1;
	}
	acpi_init();
	return 0;
}

void plugin_done(void)
{
	acpi_done();
}

const gchar *plugin_name(void)
{
	return _("Acpi notifier");
}

const gchar *plugin_desc(void)
{
	return _("This plugin handles various ACPI mail LEDs.");
}

const gchar *plugin_type(void)
{
	return "GTK2";
}
