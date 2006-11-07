plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = att_remover.la

att_remover_la_SOURCES = \
	att_remover.c

att_remover_la_LDFLAGS = \
	-avoid-version -module

att_remover_la_LIBADD = \
	$(GTK_LIBS)

INCLUDES = \
	-I$(sylpheed_dir)/ \
	-I$(sylpheed_dir)/src/ \
	-I$(sylpheed_dir)/src/common \
	-I$(sylpheed_dir)/src/gtk

AM_CPPFLAGS = \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
