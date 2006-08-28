plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = gtkhtml2_viewer.la

gtkhtml2_viewer_la_SOURCES = \
         gtkhtml2_viewer.c \
	 gtkhtml2_prefs.c \
	 gtkhtml2_prefs.h

gtkhtml2_viewer_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(GTKHTML2_LIBS)

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk

AM_CPPFLAGS = \
	-Wall \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(OPENSSL_CFLAGS) \
	$(GTKHTML2_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
