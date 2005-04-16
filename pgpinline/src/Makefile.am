plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = pgpinline.la

pgpinline_la_SOURCES = \
	plugin.c \
	pgpinline.c pgpinline.h 

pgpinline_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(GPGME_LIBS)

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GPGME_CFLAGS)
