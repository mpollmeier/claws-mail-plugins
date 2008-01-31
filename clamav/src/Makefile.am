plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = clamav.la

clamav_la_SOURCES = \
         clamav_plugin.c \
	 clamav_plugin_gtk.c \
	 clamav_plugin.h \
	 gettext.h

clamav_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(CLAMAV_LIBS)

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLAMAV_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
