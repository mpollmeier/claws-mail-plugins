/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#ifndef __GS_DEFAULTS_H__
#define __GS_DEFAULTS_H__

#ifdef __cplusplus
{
#if 0 /* let's fool emacs */
}
#endif
#endif

#include "gtkgs.h"

/* defaults accessors */
void     gtk_gs_defaults_set_size(gint iNewPageSize); 
void     gtk_gs_defaults_set_override_size(gboolean bOverrSize);
void     gtk_gs_defaults_set_antialiased(gint iNewAntialiased);
void     gtk_gs_defaults_set_orientation(gint);
void     gtk_gs_defaults_set_override_orientation(gboolean bOverOrien);
void     gtk_gs_defaults_set_zoom_factor(gfloat fZoom);
void     gtk_gs_defaults_set_watch_doc(gint iNewWatchDoc);
void     gtk_gs_defaults_set_respect_eof(gboolean resp);
void     gtk_gs_defaults_set_show_scroll_rect(gboolean f);
gint     gtk_gs_defaults_get_size(void);
gboolean gtk_gs_defaults_get_override_size(void);
gboolean gtk_gs_defaults_get_antialiased(void);
gint     gtk_gs_defaults_get_orientation(void);
gboolean gtk_gs_defaults_get_override_orientation(void);
gfloat   gtk_gs_defaults_get_zoom_factor(void);
gint     gtk_gs_defaults_get_watch_doc(void) ;
gboolean gtk_gs_defaults_get_respect_eof(void);
gint     gtk_gs_defaults_get_paper_count(void);
gboolean gtk_gs_defaults_get_show_scroll_rect(void);
GtkGSPaperSize *gtk_gs_defaults_get_paper_sizes(void);
const gchar *gtk_gs_defaults_get_interpreter_cmd(void);
const gchar *gtk_gs_defaults_get_alpha_parameters(void);
const gchar *gtk_gs_defaults_get_scan_pdf_cmd(void);
const gchar *gtk_gs_defaults_get_ungzip_cmd(void);
const gchar *gtk_gs_defaults_get_unbzip2_cmd(void);
void gtk_gs_defaults_set_interpreter_cmd(gchar *);
void gtk_gs_defaults_set_alpha_parameters(gchar *);
void gtk_gs_defaults_set_scan_pdf_cmd(gchar *);
void gtk_gs_defaults_set_ungzip_cmd(gchar *);
void gtk_gs_defaults_set_unbzip2_cmd(gchar *);


#ifdef __cplusplus
}
#endif

#endif /* __GS_DEFAULTS_H__ */
