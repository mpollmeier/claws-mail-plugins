plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)
plugin_DATA=fetchinfo_plugin_gtk.deps
plugin_LTLIBRARIES = fetchinfo_plugin.la fetchinfo_plugin_gtk.la

EXTRA_DIST=fetchinfo_plugin_gtk.deps

fetchinfo_plugin_la_SOURCES = \
	fetchinfo_plugin.c fetchinfo_plugin.h

fetchinfo_plugin_la_LDFLAGS = \
	-avoid-version -module

fetchinfo_plugin_gtk_la_SOURCES = \
	fetchinfo_plugin_gtk.c

fetchinfo_plugin_gtk_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
