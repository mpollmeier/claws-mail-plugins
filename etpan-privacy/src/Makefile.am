plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = etpan-privacy.la

## etPan! Privacy folder plugin

etpan_privacy_la_SOURCES = \
	plugin.c \
	mailprivacy.c mailprivacy.h \
	mailprivacy_gnupg.c mailprivacy_gnupg.h \
	mailprivacy_smime.c mailprivacy_smime.h \
	mailprivacy_tools.c mailprivacy_tools.h \
	mailprivacy_types.h \
	etpan_privacy.c etpan_privacy.h \
	prefs_privacy.c prefs_privacy.h

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

