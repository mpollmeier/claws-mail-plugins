plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = newmail.la

newmail_la_SOURCES = \
	newmail.c

newmail_la_LDFLAGS = \
	-avoid-version -module

AM_CPPFLAGS = \
        $(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
