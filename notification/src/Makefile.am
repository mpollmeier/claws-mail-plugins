CFLAGS += "-Wall"

plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = notification_plugin.la

notification_plugin_la_SOURCES = \
	notification_plugin.c notification_plugin.h \
	notification_core.c notification_core.h \
	notification_popup.c notification_popup.h \
	notification_banner.c notification_banner.h \
	notification_command.c notification_command.h \
	notification_prefs.c notification_prefs.h \
	notification_foldercheck.c notification_foldercheck.h \
	notification_pixbuf.c notification_pixbuf.h \
	raw_sylpheed_claws_logo_64x64.h

notification_plugin_la_LDFLAGS = \
	-avoid-version -module

notification_plugin_la_LIBADD = \
	$(libnotify_LIBS)

notification_plugin_la_CPPFLAGS = \
	$(NOTIFY_CFLAGS) \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(libnotify_CFLAGS)
