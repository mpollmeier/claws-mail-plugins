plugindir = $(libdir)/sylpheed-claws/plugins

plugin_LTLIBRARIES = pgpinline.la

pgpinline_la_SOURCES = \
	plugin.c \
	pgpinline.c pgpinline.h \
	sgpgme.c sgpgme.h

pgpinline_la_LDFLAGS = \
	-avoid-version -module

INCLUDES = \
        -I$(sylpheed_dir)/ \
        -I$(sylpheed_dir)/src/ \
        -I$(sylpheed_dir)/src/common \
        -I$(sylpheed_dir)/src/gtk

AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GPGME_CFLAGS)
