plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = cachesaver.la

## cachesaver folder plugin

cachesaver_la_SOURCES = \
	plugin.c

cachesaver_la_LDFLAGS = \
	-avoid-version -module

cachesaver_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
