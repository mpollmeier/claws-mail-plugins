plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = att_remover.la

att_remover_la_SOURCES = \
	att_remover.c

att_remover_la_LDFLAGS = \
	-avoid-version -module

att_remover_la_LIBADD = \
	$(GTK_LIBS)

INCLUDES = \
	-I$(claws_dir)/ \
	-I$(claws_dir)/src/ \
	-I$(claws_dir)/src/common \
	-I$(claws_dir)/src/gtk

AM_CPPFLAGS = \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
