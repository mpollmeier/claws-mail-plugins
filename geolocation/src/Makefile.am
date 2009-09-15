plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = geolocation_plugin.la

geolocation_plugin_la_SOURCES = \
	geolocation_plugin.c \
	gettext.h

geolocation_plugin_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(LIBXML_LIBS) \
	$(CHAMPLAIN_LIBS)


INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBXML_CFLAGS) \
	$(CHAMPLAIN_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
