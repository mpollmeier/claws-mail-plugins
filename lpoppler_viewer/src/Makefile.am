plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = poppler_viewer.la

poppler_viewer_la_SOURCES = \
         poppler_viewer.c \
	 gettext.h

poppler_viewer_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(POPPLER_LIBS)

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
	$(POPPLER_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
