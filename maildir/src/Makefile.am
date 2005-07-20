plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)
plugin_DATA=maildir_gtk.deps
plugin_LTLIBRARIES = maildir.la maildir_gtk.la

EXTRA_DIST=maildir_gtk.deps

## Maildir folder plugin

maildir_la_SOURCES = \
	plugin.c \
	maildir.c maildir.h \
	uiddb.c uiddb.h

maildir_la_LDFLAGS = \
	-avoid-version -module

maildir_la_LIBADD = \
	-ldb

maildir_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

## Maildir GTK GUI support

maildir_gtk_la_SOURCES = \
	plugin_gtk.c \
	maildir_gtk.c maildir_gtk.h

maildir_gtk_la_LDFLAGS = \
	-avoid-version -module

maildir_gtk_la_LIBADD = \
	$(GTK_LIBS)

maildir_gtk_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
