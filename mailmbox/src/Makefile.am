plugindir = $(libdir)/sylpheed/plugins

plugin_LTLIBRARIES = mailmbox.la mailmbox_gtk.la

## Mailmbox folder plugin

mailmbox_la_SOURCES = \
	plugin.c \
	carray.c carray.h \
	chash.c chash.h \
	clist.c clist.h \
	mailimf.c mailimf.h \
	mailimf_types.c mailimf_types.h \
	mailimf_types_helper.c mailimf_types_helper.h \
	mailimf_write.c mailimf_write.h \
	maillock.c maillock.h \
	mailmbox.c mailmbox.h \
	mailmbox_folder.c mailmbox_folder.h \
	mailmbox_parse.c mailmbox_parse.h \
	mailmbox_types.c mailmbox_types.h \
	mmapstring.c mmapstring.h

mailmbox_la_LDFLAGS = \
	-avoid-version -module

mailmbox_la_LIBADD = \
	-ldb

mailmbox_la_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS)

## Mailmbox GTK GUI support

mailmbox_gtk_la_SOURCES = \
	plugin_gtk.c

mailmbox_gtk_la_LDFLAGS = \
	-avoid-version -module

mailmbox_gtk_la_LIBADD = \
	$(GTK_LIBS)

mailmbox_gtk_la_CPPFLAGS = \
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
