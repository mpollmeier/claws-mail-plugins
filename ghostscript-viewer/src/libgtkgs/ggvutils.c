/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#ifdef HAVE_CONFIG_H
#  include "pluginconfig.h"
#endif
#include "config.h"

#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

#include "ggvutils.h"
#include "common/intl.h"

GtkGSPaperSize ggv_paper_sizes[] = {
        {"BBox",         0,  0},
        {"Letter",       612,  792,},
        {"Tabloid",      792, 1224,},
        {"Ledger",       1224,  792,},
        {"Legal",        612, 1008,},
        {"Statement",	 396,  612,},
        {"Executive",	 540,  720,},
        {"A3",		 842, 1190,},
        {"A4",		 595,  842,},
        {"A5",		 420,  595,},
        {"B4",		 729, 1032,},
        {"B5",		 516,  729,},
        {"Folio",        612,  936,},
        {"Quarto",       610,  780,},
        {"10x14",        720, 1008,},
        {NULL,		   0,    0}
};

gfloat ggv_unit_factors[] = {
        1.0,
        25.4,
        2.54,
        72.0
};

char *ggv_orientation_labels[] = {
        N_("Portrait"),
        N_("Landscape"),
        N_("Upside Down"),
        N_("Seascape"),
        NULL,
};

char *ggv_unit_labels[] = {
        N_("inch"),
        N_("mm"),
        N_("cm"),
        N_("point"),
        NULL
};

GSList *
ggv_split_string (const gchar *string, const gchar *delimiter)
{
	const gchar *ptr = string;
	int pos = 0, escape = 0;
	char buffer [BUFSIZ];
	GSList *list = NULL;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (delimiter != NULL, NULL);

	while (*ptr) {
		char c = *ptr++;
		const gchar *d;
		int found = 0;

		if (pos >= BUFSIZ) {
			g_warning ("string too long, aborting");
			return list;
		}

		if (escape) {
			buffer [pos++] = c;
			escape = 0;
			continue;
		}

		if (c == '\\') {
			escape = 1;
			continue;
		}

		for (d = delimiter; *d; d++) {
			if (c == *d) {
				buffer [pos++] = 0;
				list = g_slist_prepend (list, g_strdup (buffer));
				pos = 0; found = 1;
				break;
			}
		}

		if (!found)
			buffer [pos++] = c;
	}

	buffer [pos++] = 0;
	list = g_slist_prepend (list, g_strdup (buffer));

	return list;
}

gint
ggv_get_index_of_string(gchar *string, gchar **strings)
{
        guint idx = 0;

        while(strings[idx] != NULL) {
                if(strcmp(strings[idx], string) == 0)
                        return idx;
                idx++;
        }

        return -1;
}

/* Quote filename for system call */
gchar *ggv_quote_filename (const gchar *str)
{
        gchar c, *s, *t, *buf;

        s = (gchar *) str;
        t = buf = g_new (gchar, 2 * strlen (str) + 1);

        while ((c = *s++) != '\0') {
                switch (c) {
                case ' ':
                case '\t':
                case '\n': /* white space */
                case '\'':
                case '"':
                case '\\': /* quoting chars */
                case '|':
                case '&':
                case ';':  /* metacharacters */
                case '(':
                case ')':
                case '<':
                case '>':
                case '!':
                case '{':
                case '}':  /* reserved characters */
                case '*':
                case '[':
                case '?':
                case ']':  /* globbing chars */
                case '^':
                case '$':
                case '`':  /* expansion characters */
                case '%':
                        *t++ = '\\';
                        *t++ = c;
                        break;
                case '#':  /* comment char */
                        if (s == str)
                                *t++ = '\\';
                        *t++ = c;
                        break;
                default:
                        *t++ = c;
                        break;
                }
        }
        *t++ = '\0';
        return buf;
}

/* If file exists and is a regular file then return its length, else -1 */
gint ggv_file_length (const gchar *filename)
{
        struct stat stat_rec;

	if (filename && (stat (filename, &stat_rec) == 0)
	    && S_ISREG (stat_rec.st_mode))
		return stat_rec.st_size;
	else
                return -1;
}

/* Test if file exists, is a regular file and its length is > 0 */
gboolean ggv_file_readable (const char *filename)
{
        return (ggv_file_length (filename) > 0);
}

/* Set a tooltip for a widget */
void ggv_set_tooltip(GtkWidget* w, const gchar* tip)
{
	GtkTooltips* t = gtk_tooltips_new();

	gtk_tooltips_set_tip(t, w, tip, NULL);
}

gfloat ggv_compute_zoom(gint zoom_spec)
{
	return pow(1.2, zoom_spec); /* The Knuth magstep formula rules */
}

gint ggv_compute_spec(gfloat zoom)
{
        zoom = MAX(0.02, zoom);
        zoom = MIN(10.0, zoom);

        zoom = log(zoom)/log(1.2);
        return (gint)rint(zoom);
}

