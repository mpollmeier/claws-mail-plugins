plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = perl_plugin.la

perl_plugin_la_SOURCES = \
	perl_plugin.c

perl_plugin_la_LDFLAGS = \
	-avoid-version -module

perl_plugin_la_LIBADD = \
	$(PERL_LDFLAGS) \
	$(GTK_LIBS)

INCLUDES = \
	-I$(sylpheed_dir)/ \
	-I$(sylpheed_dir)/src/ \
	-I$(sylpheed_dir)/src/common \
	-I$(sylpheed_dir)/src/gtk

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(PERL_CFLAGS)