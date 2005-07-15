plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = synce_plugin.la

synce_plugin_la_SOURCES = \
	synce_plugin.c \
	synce_rapi.c synce_rapi.h \
	synce_gtk.c synce_gtk.h \
	synce_comp.c synce_comp.h \
	contact_ids.h

synce_plugin_la_LDFLAGS = \
	-avoid-version -module

synce_plugin_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)