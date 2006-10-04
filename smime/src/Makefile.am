plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = smime.la
plugin_DATA = smime.deps

smime_la_SOURCES = \
	plugin.c \
	smime.c prefs_smime.c

smime_la_LDFLAGS = \
	-avoid-version -module

smime_la_LIBADD = \
	$(GTK_LIBS) \
	$(GPGME_LIBS)

INCLUDES = \
	-I$(sylpheed_dir)/ \
	-I$(sylpheed_dir)/src/ \
	-I$(sylpheed_dir)/src/common \
	-I$(sylpheed_dir)/src/gtk

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GPGME_CFLAGS)

EXTRA_DIST = \
	smime.h smime.deps
