plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = spamreport.la

spamreport_la_SOURCES = \
         spam_report.c \
	 spam_report_prefs.c \
	 spam_report_prefs.h \
	 gettext.h

spamreport_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
