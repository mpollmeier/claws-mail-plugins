plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

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
	plugin_version.h gettext.h
 
vcalendar_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

vcalendar_la_LIBADD = \
        $(top_srcdir)/libical/libical/.libs/libical.a

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)

EXTRA_DIST = \
	plugin_version.h.in
