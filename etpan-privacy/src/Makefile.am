plugindir = $(libdir)/sylpheed-claws/plugins

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

etpan_privacy_la_LIBADD = 

etpan_privacy_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

INCLUDES = \
	-I$(sylpheed_dir)/ \
	-I$(sylpheed_dir)/src/ \
	-I$(sylpheed_dir)/src/common \
	-I$(sylpheed_dir)/src/gtk

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

AM_LDDFLAGS = 