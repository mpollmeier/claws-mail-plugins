plugindir = $(SYLPHEED_CLAWS_PLUGINDIR)

plugin_LTLIBRARIES = vcalendar.la

vcalendar_la_SOURCES = \
	plugin.c \
	vcalendar.c vcalendar.h \
	vcal_manager.c vcal_manager.h \
	vcal_folder.c vcal_folder.h \
	vcal_meeting_gtk.c vcal_meeting_gtk.h \
	icaltime_as_local.c icaltime_as_local.h
 
vcalendar_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS)

AM_CPPFLAGS = \
	$(SYLPHEED_CLAWS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS)
