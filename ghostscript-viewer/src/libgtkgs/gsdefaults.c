/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#ifdef HAVE_CONFIG_H
#  include <pluginconfig.h>
#endif

#include "gtkgs.h"
#include "gsdefaults.h"
#include "ggvutils.h"

/**
 * defaults for GtkGS widgets
 **/

/* Default values to pass to gtk_gs_init */
typedef struct _GtkGSDefaults 
{
        gboolean   antialiased;
        gboolean   override_size;
        gint       default_size;
	gboolean   watch_doc;
        gboolean   override_orientation;
        gboolean   respect_eof;
        gboolean   show_scroll_rect;
        gint       fallback_orientation;
        gfloat     zoom_factor;
        gchar      *interpreter_cmd;
        gchar      *alpha_params;
        gchar      *scan_pdf_cmd;
        gchar      *ungzip_cmd;
        gchar      *unbzip2_cmd;
} GtkGSDefaults;

static GtkGSDefaults gtk_gs_defaults = {
        TRUE, FALSE, 8, TRUE, FALSE, TRUE, TRUE, 0, 1.0, NULL, NULL, NULL, NULL,
};

void
gtk_gs_defaults_set_size(gint iNewPageSize)
{
        gtk_gs_defaults.default_size = iNewPageSize;
}

gint
gtk_gs_defaults_get_size()
{
        return gtk_gs_defaults.default_size;
}

void
gtk_gs_defaults_set_watch_doc(gint iNewWatchDoc)
{
        gtk_gs_defaults.watch_doc = iNewWatchDoc;
}

gint
gtk_gs_defaults_get_watch_doc()
{
        return gtk_gs_defaults.watch_doc;
}

void
gtk_gs_defaults_set_override_size(gboolean bOverrSize)
{
        gtk_gs_defaults.override_size = bOverrSize;
}

gboolean
gtk_gs_defaults_get_override_size()
{
        return gtk_gs_defaults.override_size;
}

void
gtk_gs_defaults_set_override_orientation(gboolean bOverOrien)
{
        gtk_gs_defaults.override_orientation = bOverOrien;
}

gboolean
gtk_gs_defaults_get_override_orientation()
{
        return gtk_gs_defaults.override_orientation;
}

void
gtk_gs_defaults_set_antialiased(gint iNewAntialiased)
{
        gtk_gs_defaults.antialiased = iNewAntialiased;
}

gboolean
gtk_gs_defaults_get_antialiased()
{
        return gtk_gs_defaults.antialiased;
}

void
gtk_gs_defaults_set_zoom_factor(gfloat fZoom)
{
        fZoom = MIN(fZoom, 30.0);
        fZoom = MAX(0.05, fZoom);
        gtk_gs_defaults.zoom_factor = fZoom;
}

gfloat
gtk_gs_defaults_get_zoom_factor()
{
        return gtk_gs_defaults.zoom_factor;
}

void
gtk_gs_defaults_set_orientation(gint iNewOrientation) {
        g_assert (
                  (iNewOrientation == GTK_GS_ORIENTATION_PORTRAIT) ||
                  (iNewOrientation == GTK_GS_ORIENTATION_LANDSCAPE) ||
                  (iNewOrientation == GTK_GS_ORIENTATION_UPSIDEDOWN) ||
                  (iNewOrientation == GTK_GS_ORIENTATION_SEASCAPE)
                  );
        gtk_gs_defaults.fallback_orientation = iNewOrientation;
}

gint
gtk_gs_defaults_get_orientation()
{
        return gtk_gs_defaults.fallback_orientation;
}

void
gtk_gs_defaults_set_respect_eof(gboolean resp)
{
        gtk_gs_defaults.respect_eof = resp;
}

gboolean
gtk_gs_defaults_get_respect_eof()
{
        return gtk_gs_defaults.respect_eof;
}

GtkGSPaperSize *
gtk_gs_defaults_get_paper_sizes()
{
        return ggv_paper_sizes;
}

gint
gtk_gs_defaults_get_paper_count()
{
        gint n = 0;

        while(ggv_paper_sizes[n].name != NULL)
                n++;

        return n;
}

gboolean
gtk_gs_defaults_get_show_scroll_rect()
{
        return gtk_gs_defaults.show_scroll_rect;
}

void
gtk_gs_defaults_set_show_scroll_rect(gboolean f)
{
        gtk_gs_defaults.show_scroll_rect = f;
}

const gchar *
gtk_gs_defaults_get_alpha_parameters()
{
        if(!gtk_gs_defaults.alpha_params)
                return ALPHA_PARAMS;
        return gtk_gs_defaults.alpha_params;
}

const gchar *
gtk_gs_defaults_get_interpreter_cmd()
{
        if(!gtk_gs_defaults.interpreter_cmd)
                return GS_PATH;
        return gtk_gs_defaults.interpreter_cmd;
}

const gchar *
gtk_gs_defaults_get_scan_pdf_cmd()
{
        if(!gtk_gs_defaults.scan_pdf_cmd)
                return GS_PATH
                        " -dNODISPLAY -dQUIET"
                        " -sPDFname=%s -sDSCname=%s"
                        " pdf2dsc.ps -c quit";
        return gtk_gs_defaults.scan_pdf_cmd;
}

const gchar *
gtk_gs_defaults_get_ungzip_cmd()
{
        if(!gtk_gs_defaults.ungzip_cmd)
                return "gzip -cd";
        return gtk_gs_defaults.ungzip_cmd;
}

const gchar *
gtk_gs_defaults_get_unbzip2_cmd()
{
        if(!gtk_gs_defaults.unbzip2_cmd)
                return "bzip2 -cd";
        return gtk_gs_defaults.unbzip2_cmd;
}

void
gtk_gs_defaults_set_interpreter_cmd(gchar *cmd)
{
        if(gtk_gs_defaults.interpreter_cmd)
                g_free(gtk_gs_defaults.interpreter_cmd);
        gtk_gs_defaults.interpreter_cmd = cmd;
}

void
gtk_gs_defaults_set_alpha_parameters(gchar *par)
{
        if(gtk_gs_defaults.alpha_params)
                g_free(gtk_gs_defaults.alpha_params);
        gtk_gs_defaults.alpha_params = par;
}

void
gtk_gs_defaults_set_scan_pdf_cmd(gchar *cmd)
{
        if(gtk_gs_defaults.scan_pdf_cmd)
                g_free(gtk_gs_defaults.scan_pdf_cmd);
        gtk_gs_defaults.scan_pdf_cmd = cmd;
}

void
gtk_gs_defaults_set_ungzip_cmd(gchar *cmd)
{
        if(gtk_gs_defaults.ungzip_cmd)
                g_free(gtk_gs_defaults.ungzip_cmd);
        gtk_gs_defaults.ungzip_cmd = cmd;
}

void
gtk_gs_defaults_set_unbzip2_cmd(gchar *cmd)
{
        if(gtk_gs_defaults.unbzip2_cmd)
                g_free(gtk_gs_defaults.unbzip2_cmd);
        gtk_gs_defaults.unbzip2_cmd = cmd;
}

void
gtk_gs_defaults_load()
{
}

void
gtk_gs_defaults_save()
{
}
