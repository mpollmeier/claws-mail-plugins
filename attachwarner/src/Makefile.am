plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = attachwarner.la

attachwarner_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\" 

attachwarner_la_SOURCES = \
	attachwarner.c attachwarner.h \
	attachwarner_prefs.c attachwarner_prefs.h \
	gettext.h

