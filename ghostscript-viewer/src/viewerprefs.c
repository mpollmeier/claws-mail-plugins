/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto and the Sylpheed-Claws Team
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
#  include "pluginconfig.h"
#endif
#include "config.h"

#include "defs.h"

#include <glib.h>
#include <gtk/gtk.h>

#include "common/intl.h"
#include "common/utils.h"
#include "common/prefs.h"
#include "prefs_gtk.h"
#include "gtk/prefswindow.h"

#include "viewerprefs.h"

#define PREFS_BLOCK_NAME "GhostscriptViewer"

struct GhostscriptViewerPage
{
	PrefsPage page;
	
	GtkWidget *antialiasing;
	GtkWidget *respect_eof;
};

GhostscriptViewerPrefs ghostscriptviewerprefs;

static PrefParam param[] = {
	{"antialiasing", "TRUE", &ghostscriptviewerprefs.antialiasing, P_BOOL,
	 NULL, NULL, NULL},
	{"respect_eof", "TRUE", &ghostscriptviewerprefs.respect_eof, P_BOOL,
	 NULL, NULL, NULL},

	{NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

static void ghostscriptviewer_create_widget_func(PrefsPage * _page,
					   GtkWindow * window,
					   gpointer data)
{
	struct GhostscriptViewerPage *page = (struct GhostscriptViewerPage *) _page;

	/* ------------------ code made by glade -------------------- */
	GtkWidget *table2;
	GtkWidget *label14;
	GtkWidget *antialiasing;
	GtkWidget *label15;
	GtkWidget *respect_eof;

	table2 = gtk_table_new(2, 2, FALSE);
	gtk_widget_show(table2);
	gtk_container_set_border_width(GTK_CONTAINER(table2), 8);
	gtk_table_set_row_spacings(GTK_TABLE(table2), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table2), 8);

	label14 = gtk_label_new(_("Antialiasing"));
	gtk_widget_show(label14);
	gtk_table_attach(GTK_TABLE(table2), label14, 0, 1, 0, 1,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label14), 0, 0.5);

	antialiasing = gtk_check_button_new_with_label("");
	gtk_widget_show(antialiasing);
	gtk_table_attach(GTK_TABLE(table2), antialiasing, 1, 2, 0, 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);

	label15 = gtk_label_new(_("Respect EOF"));
	gtk_widget_show(label15);
	gtk_table_attach(GTK_TABLE(table2), label15, 0, 1, 1, 2,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label15), 0, 0.5);

	respect_eof = gtk_check_button_new_with_label("");
	gtk_widget_show(respect_eof);
	gtk_table_attach(GTK_TABLE(table2), respect_eof, 1, 2, 1, 2,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	/* --------------------------------------------------------- */

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(antialiasing), 
				     ghostscriptviewerprefs.antialiasing);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(respect_eof), 
				     ghostscriptviewerprefs.respect_eof);

	page->antialiasing = antialiasing;
	page->respect_eof = respect_eof;

	page->page.widget = table2;
}

static void ghostscriptviewer_destroy_widget_func(PrefsPage *_page)
{
}

static void ghostscriptviewer_save_func(PrefsPage * _page)
{
	struct GhostscriptViewerPage *page = (struct GhostscriptViewerPage *) _page;
	PrefFile *pfile;
	gchar *rcpath;

	ghostscriptviewerprefs.antialiasing =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (page->antialiasing));
	ghostscriptviewerprefs.respect_eof =
	    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
					 (page->respect_eof));

	rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	pfile = prefs_write_open(rcpath);
	g_free(rcpath);
	if (!pfile || (prefs_set_block_label(pfile, PREFS_BLOCK_NAME) < 0))
		return;

	if (prefs_write_param(param, pfile->fp) < 0) {
		g_warning("failed to write GhostscriptViewer configuration to file\n");
		prefs_file_close_revert(pfile);
		return;
	}
	fprintf(pfile->fp, "\n");

	prefs_file_close(pfile);
}

static struct GhostscriptViewerPage ghostscriptviewer_page;

void ghostscript_viewer_prefs_init(void)
{
	prefs_set_default(param);
	prefs_read_config(param, PREFS_BLOCK_NAME, COMMON_RC);

	ghostscriptviewer_page.page.path = "Message View/Ghostscript Viewer";
	ghostscriptviewer_page.page.create_widget = ghostscriptviewer_create_widget_func;
	ghostscriptviewer_page.page.destroy_widget = ghostscriptviewer_destroy_widget_func;
	ghostscriptviewer_page.page.save_page = ghostscriptviewer_save_func;

	prefs_gtk_register_page((PrefsPage *) &ghostscriptviewer_page);
}

void ghostscript_viewer_prefs_done(void)
{
	prefs_gtk_unregister_page((PrefsPage *) &ghostscriptviewer_page);
}
