plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = rssyl.la

rssyl_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\" 

rssyl_la_SOURCES = \
	date.c date.h \
	feed.c feed.h \
	feedprops.c feedprops.h \
	gettext.h \
	opml.c opml.h \
	parsers.c parsers.h \
	plugin.c \
	rssyl.c rssyl.h \
	rssyl_cb_gtk.c rssyl_cb_gtk.h \
	rssyl_cb_menu.c rssyl_cb_menu.h \
	rssyl_gtk.c rssyl_gtk.h \
	rssyl_prefs.c rssyl_prefs.h \
	strreplace.c strreplace.h

