plugindir = $(libdir)/sylpheed/plugins

plugin_LTLIBRARIES = maildir.la maildir_gtk.la

## Maildir folder plugin

maildir_la_SOURCES = \
	plugin.c \
	maildir.c maildir.h

maildir_la_LDFLAGS = \
	-avoid-version -module

maildir_la_LIBADD = \
	-ldb

maildir_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS)

## Maildir GTK GUI support

maildir_gtk_la_SOURCES = \
	plugin_gtk.c

maildir_gtk_la_LDFLAGS = \
	-avoid-version -module

maildir_gtk_la_LIBADD = \
	$(GTK_LIBS)

maildir_gtk_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
