#ifndef __STRREPLACE_H
#define __STRREPLACE_H

gchar *rssyl_strreplace(gchar *source, gchar *pattern,
		gchar *replacement);
gchar *rssyl_sanitize_string(gchar *str, gboolean strip_nl);

#endif /* __STRREPLACE_H */
