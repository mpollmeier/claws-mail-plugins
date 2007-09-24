CFLAGS += "-Wall"
plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = address_dup_finder.la

address_dup_finder_la_SOURCES = \
	address_dup_finder.c \
	adf_prefs.c adf_prefs.h \
	adf_gtk.c adf_gtk.h \
	adf_finder.c adf_finder.h \
	gettext.h

address_dup_finder_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\" 


