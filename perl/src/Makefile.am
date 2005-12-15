plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = perl_plugin.la

perl_plugin_la_SOURCES = \
	perl_plugin.c perl_plugin.h \
	perl_gtk.c perl_gtk.h \
	plugin_version.h

perl_plugin_la_LDFLAGS = \
	-avoid-version -module

perl_plugin_la_LIBADD = \
	$(PERL_LDFLAGS) \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(PERL_CFLAGS)

EXTRA_DIST = \
	plugin_version.h.in
