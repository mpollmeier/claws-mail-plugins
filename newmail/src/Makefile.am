plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = newmail.la

newmail_la_SOURCES = \
	newmail.c \
	plugin_version.h

newmail_la_LDFLAGS = \
	-avoid-version -module

AM_CPPFLAGS = \
        $(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

EXTRA_DIST = \
	plugin_version.h.in
