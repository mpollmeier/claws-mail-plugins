/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2007 the Claws Mail Team
 * This file Copyright (C) 2007 Salvatore De Paolis 
 * <iwkse@claws-mail.org> 
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "gettext.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <poppler.h>
#include "claws.h"
#include "version.h"
#include "main.h"
#include "plugin.h"
#include "utils.h"
#include "mimeview.h"
#include "messageview.h"
#include "prefs_common.h"
#include "log.h"
#include "codeconv.h"
#include "menu.h"
#include "defs.h"
#include "gtkutils.h"
#include "alertpanel.h"

#include "first_arrow.xpm"
#include "last_arrow.xpm"
#include "left_arrow.xpm"
#include "right_arrow.xpm"
#include "zoom_fit.xpm"
#include "zoom_in.xpm"
#include "zoom_out.xpm"
#include "zoom_width.xpm"
#include "rotate_right.xpm"
#include "rotate_left.xpm"
#include "doc_info.xpm"
#include "doc_index.xpm"

#ifdef USE_PTHREAD
#include <pthread.h>
#endif


#define ZOOM_FACTOR 0.3
#define ROTATION 90 
#define ALPHA_CHANNEL 75
#define SELECTION_COLOR 0xff00ff

static gchar *msg = NULL;

struct _PageResult
{
	GList *results;
	gint page_num;
};

typedef struct _PageResult PageResult;

struct _PdfViewer
{
	MimeViewer			mimeviewer;
	GtkWidget			*buttons_table;
	GtkWidget			*vbox;
	GtkWidget			*hbox;
	GtkWidget			*frame_index;
	GtkWidget			*pdf_view;
	GtkWidget			*scrollwin;
	GtkWidget			*scrollwin_index;
	GtkWidget			*icon_type;
	GdkPixmap			*icon_pixmap;
	GdkBitmap			*icon_bitmap;
	GtkWidget			*doc_label;
	GtkWidget			*cur_page;
	GtkWidget			*doc_index_pane;
	GtkWidget			*index_list;
	/* begin GtkButtons */
	GtkWidget			*first_page;
	GtkWidget			*last_page;
	GtkWidget			*prev_page;
	GtkWidget			*next_page;
	GtkWidget			*zoom_in;
	GtkWidget			*zoom_out;
	GtkWidget			*zoom_fit;
	GtkWidget			*zoom_width;
	GtkWidget			*rotate_left;
	GtkWidget			*rotate_right;
	GtkWidget			*doc_info;
	GtkWidget			*doc_index;
	/* end GtkButtons */
	GtkTable			*table_doc_info;
	GtkTooltips			*button_bar_tips;
	PopplerDocument		*pdf_doc;
	PopplerPage			*pdf_page;
	PopplerIndexIter	*pdf_index;
	PopplerRectangle	*text_found_coord;
	GtkTreeModel		*index_model;
	
	GList				*page_results;
	GList				*text_found; /* GList of PageResults */
	gchar				*last_search;
	gint				 last_match;
	gint				 num_matches;

	gchar				*target_filename;
	gchar				*filename;
	gchar				*fsname;
	gchar				*doc_info_text;

	gint				res_cnt;
	gint				rotate;
	gint				num_pages;
	double				zoom;
	double				width;
	double				height;
	MimeInfo			*mimeinfo;
	MimeInfo			*to_load;
	
};
static gchar *content_types[] =
	{"application/pdf", 
	 "application/postscript", 
	 NULL};
typedef enum {
	TYPE_UNKNOWN,
	TYPE_PDF,
	TYPE_PS
} FileType;

enum {
	INDEX_NAME,
	INDEX_PAGE,
	INDEX_TOP,
	N_INDEX_COLUMNS
};

typedef struct _PdfViewer PdfViewer;

static FileType pdf_viewer_mimepart_get_type(MimeInfo *partinfo);
static MimeViewerFactory pdf_viewer_factory;

static void pdf_viewer_show_mimepart(MimeViewer *_viewer, const gchar *infile,
				MimeInfo *partinfo);
static gint pdf_viewer_show_mimepart_real(MimeViewer *_viewer);

static MimeViewer *pdf_viewer_create(void);
static void pdf_viewer_clear_viewer(MimeViewer *_viewer);
static void pdf_viewer_destroy_viewer(MimeViewer *_viewer);
static void pdf_viewer_update(MimeViewer *_viewer, gboolean reload_file, int page_num);

static GtkWidget *pdf_viewer_get_widget(MimeViewer *_viewer);

static void pdf_viewer_hide_index_pane(PdfViewer *viewer);
static void pdf_viewer_set_index_button_sensitive(PdfViewer *viewer);
static void pdf_viewer_scroll_to(PdfViewer *viewer, gfloat x, gfloat y);

static void search_matches_free(PdfViewer *viewer);
static gboolean	pdf_viewer_text_search (MimeViewer *_viewer, gboolean backward,
				     const gchar *str, gboolean case_sens);
static void pdf_viewer_render_selection(PdfViewer *viewer, GList *results, PageResult *page_results);


static char * pdf_viewer_get_document_format_data(GTime utime);
static void pdf_viewer_get_document_index(PdfViewer *viewer, PopplerIndexIter *index_iter, GtkTreeIter *parentiter);
static void pdf_viewer_index_row_activated(GtkTreeView		*tree_view,
				   	GtkTreePath		*path,
				   	GtkTreeViewColumn	*column,
				   	gpointer		 data);

static GtkTable * pdf_viewer_fill_info_table(PdfViewer *viewer);


/* Callbacks */
static void pdf_viewer_button_first_page_cb(GtkButton *button, PdfViewer *viewer);
static void pdf_viewer_button_last_page_cb(GtkButton *button, PdfViewer *viewer);
static void pdf_viewer_spin_change_page_cb(GtkSpinButton *button, PdfViewer *viewer);
static void pdf_viewer_button_zoom_in_cb(GtkButton *button, PdfViewer *viewer);
static void pdf_viewer_button_zoom_out_cb(GtkButton *button, PdfViewer *viewer);
static void pdf_viewer_button_zoom_fit_cb(GtkButton *button, PdfViewer *viewer);
static void pdf_viewer_button_zoom_width_cb(GtkButton *button, PdfViewer *viewer);
static void pdf_viewer_button_rotate_right_cb(GtkButton *button, PdfViewer *viewer);
static void pdf_viewer_button_rotate_left_cb(GtkButton *button, PdfViewer *viewer);
/* Show/Hide the index pane */
static void pdf_viewer_show_document_index_cb(GtkButton *button, PdfViewer *viewer);
static void pdf_viewer_button_document_info_cb(GtkButton *button, PdfViewer *viewer);

static void pdf_viewer_show_controls(PdfViewer *viewer, gboolean show);
static gboolean pdf_viewer_scroll_page(MimeViewer *_viewer, gboolean up);
static void pdf_viewer_scroll_one_line(MimeViewer *_viewer, gboolean up);
static void button_set_pixmap(GtkWidget *widg, char **button_image);


/** Claws-Mail Plugin functions*/
gint plugin_init(gchar **error);
void plugin_done(void);
const gchar *plugin_name(void);
const gchar *plugin_desc(void);
const gchar *plugin_type(void);
const gchar *plugin_licence(void);
const gchar *plugin_version(void);
struct PluginFeature *plugin_provides(void);




