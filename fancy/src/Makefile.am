plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = fancy_plugin.la

fancy_plugin_la_SOURCES = \
        fancy_viewer.c \
		fancy_viewer.h \
		fancy_prefs.c \
		fancy_prefs.h \
	 	gettext.h

fancy_plugin_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(WEBKIT_LIBS)

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(OPENSSL_CFLAGS) \
	$(WEBKIT_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
