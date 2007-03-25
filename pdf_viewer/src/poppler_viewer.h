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
#ifndef POPPLER_VIEWER_H
#define POPPLER_VIEWER_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

//#include <unistd.h>
#include <stdio.h>
#include "gettext.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <poppler.h>
#include <version.h>
#include <plugin.h>
#include <messageview.h>
#include <alertpanel.h>

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

/*#ifdef USE_PTHREAD
 *#include <pthread.h>
 * #endif*/


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
	PopplerDocument			*pdf_doc;
	PopplerPage			*pdf_page;
	PopplerIndexIter		*pdf_index;
	PopplerRectangle		*last_rect;
	PageResult			*last_page_result;

	GtkTreeModel			*index_model;
	
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

#endif /* POPPLER_VIEWER_H */
