plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = tnef_parse.la

tnef_parse_la_SOURCES = \
         tnef_parse.c \
         tnef_dump.c \
         tnef_dump.h \
	 gettext.h

tnef_parse_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
