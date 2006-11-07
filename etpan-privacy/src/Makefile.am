plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = etpan-privacy.la

## etPan! Privacy folder plugin

etpan_privacy_la_SOURCES = \
	plugin.c \
	etpan_privacy.c etpan_privacy.h \
	prefs_privacy.c prefs_privacy.h

etpan_privacy_la_LDFLAGS = \
	-avoid-version -module
	$(GTK_LIBS)

etpan_privacy_la_CPPFLAGS = \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

AM_CPPFLAGS = \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
