plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = etpan-privacy.la

## etPan! Privacy folder plugin

etpan_privacy_la_SOURCES = \
	plugin.c \
	etpan_privacy.c etpan_privacy.h \
	prefs_privacy.c prefs_privacy.h \
	plugin_version.h

etpan_privacy_la_LDFLAGS = \
	-avoid-version -module
	$(GTK_LIBS)

etpan_privacy_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

EXTRA_DIST = \
	plugin_version.h.in

