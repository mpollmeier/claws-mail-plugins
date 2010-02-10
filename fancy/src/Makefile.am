plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = fancy.la

fancy_la_SOURCES = \
        fancy_viewer.c \
        fancy_viewer.h \
        fancy_prefs.c \
        fancy_prefs.h \
        gettext.h

fancy_la_LDFLAGS = \
    -avoid-version -module \
    $(GTK_LIBS) \
    $(GTKPRINTUNIX_LIBS) \
    $(WEBKIT_LIBS) \
    $(LIBSOUP_GNOME_LIBS)

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
    $(GTKPRINTUNIX_CFLAGS) \
    $(WEBKIT_CFLAGS) \
    $(LIBSOUP_GNOME_CFLAGS) \
    -DLOCALEDIR=\""$(localedir)"\"
