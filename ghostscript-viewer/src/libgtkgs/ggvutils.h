/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#ifndef __GGV_UTILS_H__
#define __GGV_UTILS_H__

#include <gtkgs.h>

extern GtkGSPaperSize ggv_paper_sizes[];
extern char *ggv_orientation_labels[];
extern gfloat ggv_unit_factors[];
extern char *ggv_unit_labels[];

/* Split delimited string into a list of strings. */
GSList *ggv_split_string (const gchar *string, const gchar *delimiter);

/* Get index of a string from a list of them. */
gint ggv_get_index_of_string(gchar *string, gchar **strings);

/* Quote filename for system call */
gchar *ggv_quote_filename (const gchar *str);

/* If file exists and is a regular file then return its length, else -1 */
gint ggv_file_length (const gchar *filename);

/* Test if file exists, is a regular file and its length is > 0 */
gboolean ggv_file_readable (const char *filename);

/* Set a tooltip for a widget */
void ggv_set_tooltip(GtkWidget* w, const gchar* tip);

gfloat ggv_compute_zoom(gint zoom_spec);
gint ggv_compute_spec(gfloat zoom);

#endif
