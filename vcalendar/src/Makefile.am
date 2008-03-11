plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = vcalendar.la

INCLUDES= \
	-I$(top_srcdir)/libical/libical

vcalendar_la_SOURCES = \
	plugin.c \
	vcalendar.c vcalendar.h \
	vcal_manager.c vcal_manager.h \
	vcal_folder.c vcal_folder.h \
	vcal_meeting_gtk.c vcal_meeting_gtk.h \
	vcal_prefs.c vcal_prefs.h \
	icaltime_as_local.c icaltime_as_local.h \
	gettext.h day-view.c month-view.c

vcalendarincludedir = $(includedir)/claws-mail/plugins/@PACKAGE@
vcalendarinclude_HEADERS = \
	vcal_interface.h
 
vcalendar_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

vcalendar_la_LIBADD = \
        $(top_srcdir)/libical/libical/libical.la

AM_CPPFLAGS = \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"
