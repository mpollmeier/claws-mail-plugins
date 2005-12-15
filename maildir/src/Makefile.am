plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)
plugin_LTLIBRARIES = maildir.la

## Maildir folder plugin

maildir_la_SOURCES = \
	plugin.c \
	maildir.c maildir.h \
	maildir_gtk.c maildir_gtk.h \
	plugin_version.h \
	uiddb.c uiddb.h

maildir_la_LDFLAGS = \
	-avoid-version -module

maildir_la_LIBADD = \
	-ldb

maildir_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

maildir_gtk_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

EXTRA_DIST = \
	plugin_version.h.in
