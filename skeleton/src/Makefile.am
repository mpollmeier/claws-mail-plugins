plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = skeleton.la

## skeleton folder plugin

skeleton_la_SOURCES = \
	plugin.c

skeleton_la_LDFLAGS = \
	-avoid-version -module

skeleton_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
