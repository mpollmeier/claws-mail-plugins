/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2006 Hiroyuki Yamamoto and the Claws Mail Team
 * Copyright (C) 2006 Ricardo Mones
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

#include <gtk/gtk.h>
#include "defs.h"
#include "gettext.h"
#include "attachwarner_prefs.h"
#include "prefs_common.h"
#include "prefs_gtk.h"

#define PREFS_BLOCK_NAME "AttachWarner"

AttachWarnerPrefs attwarnerprefs;

struct AttachWarnerPrefsPage
{
	PrefsPage page;
	
	GtkWidget *regexp_text;
};

struct AttachWarnerPrefsPage attwarnerprefs_page;

static PrefParam param[] = {
	{"match_strings", N_("attach"), &attwarnerprefs.match_strings, P_STRING,
	 NULL, NULL, NULL},
	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static void attwarner_prefs_create_widget_func(PrefsPage * _page,
					   GtkWindow * window,
					   gpointer data)
{
	struct AttachWarnerPrefsPage *page = (struct AttachWarnerPrefsPage *) _page;
	GtkWidget *vbox, *hbox;;
	GtkWidget *label;
	GtkWidget *scrolledwin;
	GtkTextBuffer *buffer;

	vbox = gtk_vbox_new(FALSE, 6);
	hbox = gtk_hbox_new(FALSE, 6);
	
	label = gtk_label_new(_("Warn when matching the following regular expressions:\n(one per line)"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	page->regexp_text = gtk_text_view_new();
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->regexp_text));
	gtk_text_buffer_set_text(buffer, attwarnerprefs.match_strings, -1);
	
	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy
		(GTK_SCROLLED_WINDOW (scrolledwin),
		 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type
		(GTK_SCROLLED_WINDOW (scrolledwin), GTK_SHADOW_IN);

	gtk_container_add(GTK_CONTAINER(scrolledwin), page->regexp_text);
	gtk_widget_set_size_request(page->regexp_text, -1, 100);
	gtk_box_pack_start(GTK_BOX(vbox), scrolledwin, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 6);
	gtk_widget_show_all(hbox);
	
	page->page.widget = hbox;

}

static void attwarner_prefs_destroy_widget_func(PrefsPage *_page)
{
}

static void attwarner_save_config(void)
{
	PrefFile *pfile;
	gchar *rcpath;

	debug_print("Saving AttachWarner Page\n");

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	pfile = prefs_write_open(rcpath);
	g_free(rcpath);
	if (!pfile || (prefs_set_block_label(pfile, PREFS_BLOCK_NAME) < 0))
		return;

	if (prefs_write_param(param, pfile->fp) < 0) {
		g_warning("Failed to write AttachWarner configuration to file\n");
		prefs_file_close_revert(pfile);
		return;
	}
	fprintf(pfile->fp, "\n");

	prefs_file_close(pfile);
}


static void attwarner_prefs_save_func(PrefsPage * _page)
{
	struct AttachWarnerPrefsPage *page = (struct AttachWarnerPrefsPage *) _page;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *tmp;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page->regexp_text));
	
	g_free(attwarnerprefs.match_strings);
	
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	
	tmp = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	
	attwarnerprefs.match_strings = g_malloc(2*strlen(tmp)+1);
	pref_get_escaped_pref(attwarnerprefs.match_strings, tmp);
	attwarner_save_config();
	g_free(attwarnerprefs.match_strings);
	attwarnerprefs.match_strings = tmp;
}

void attachwarner_prefs_init(void)
{
	static gchar *path[3];
	gchar *rcpath;
	gchar *tmp;
	
	path[0] = _("Plugins");
	path[1] = _("Attach Warner");
	path[2] = NULL;

	prefs_set_default(param);
	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	prefs_read_config(param, PREFS_BLOCK_NAME, rcpath, NULL);
	g_free(rcpath);

	tmp = g_malloc(strlen(attwarnerprefs.match_strings)+1);
	pref_get_unescaped_pref(tmp, attwarnerprefs.match_strings);
	
	g_free(attwarnerprefs.match_strings);
	attwarnerprefs.match_strings = tmp;
	
	attwarnerprefs_page.page.path = path;
	attwarnerprefs_page.page.create_widget = attwarner_prefs_create_widget_func;
	attwarnerprefs_page.page.destroy_widget = attwarner_prefs_destroy_widget_func;
	attwarnerprefs_page.page.save_page = attwarner_prefs_save_func;

	prefs_gtk_register_page((PrefsPage *) &attwarnerprefs_page);
}

void attachwarner_prefs_done(void)
{
	prefs_gtk_unregister_page((PrefsPage *) &attwarnerprefs_page);

}
