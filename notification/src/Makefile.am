CFLAGS += "-Wall"

plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = notification_plugin.la

notification_plugin_la_SOURCES = \
	notification_plugin.c notification_plugin.h \
	notification_core.c notification_core.h \
	notification_popup.c notification_popup.h \
	notification_banner.c notification_banner.h \
	notification_command.c notification_command.h \
	notification_prefs.c notification_prefs.h

notification_plugin_la_LDFLAGS = \
	-avoid-version -module

notification_plugin_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
