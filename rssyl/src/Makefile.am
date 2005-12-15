plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = rssyl.la

rssyl_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	-Wall \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

rssyl_la_SOURCES = \
	date.c date.h \
	feed.c feed.h \
	feedprops.c feedprops.h \
	gettext.h \
	parsers.c parsers.h \
	plugin.c \
	plugin_version.h \
	rssyl.c rssyl.h \
	rssyl_cb_gtk.c rssyl_cb_gtk.h \
	rssyl_cb_menu.c rssyl_cb_menu.h \
	rssyl_gtk.c rssyl_gtk.h \
	strreplace.c strreplace.h

EXTRA_DIST = \
	plugin_version.h.in

