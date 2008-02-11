plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = gtkhtml2_viewer.la

gtkhtml2_viewer_la_SOURCES = \
         gtkhtml2_viewer.c \
	 gtkhtml2_prefs.c \
	 gtkhtml2_prefs.h \
	 gettext.h

gtkhtml2_viewer_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(GTKPRINTUNIX_LIBS) \
	$(GTKHTML2_LIBS)

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk \
	-I$(top_srcdir)

gtkhtml2_viewer_la_LIBADD = \
        $(top_srcdir)/libgtkhtml/libgtkhtml-2.la

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GTKPRINTUNIX_CFLAGS) \
	$(OPENSSL_CFLAGS) \
	$(GTKHTML2_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
