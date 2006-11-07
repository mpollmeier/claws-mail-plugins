plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = smime.la
plugin_DATA = smime.deps

smime_la_SOURCES = \
	plugin.c \
	smime.c

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
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GPGME_CFLAGS)

EXTRA_DIST = \
	smime.h smime.deps
