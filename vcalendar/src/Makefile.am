EXTRA_DIST = claws.def plugin.def version.rc

if OS_WIN32

LTRCCOMPILE = $(LIBTOOL) --mode=compile $(RC) \
     `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) | \
     sed -e 's/-I/--include-dir /g;s/-D/--define /g'`

%.lo : %.rc
	$(LTRCCOMPILE) -i $< -o $@

plugin_res = version.lo
plugin_res_ldflag = -Wl,.libs/version.o

export_symbols = -export-symbols $(srcdir)/plugin.def

plugin_deps = libclaws.a $(plugin_res) plugin.def

libclaws.a: claws.def
	$(DLLTOOL) --output-lib $@ --def $<

plugin_ldadd = -L . -lclaws

else
plugin_res =
plugin_res_ldflag =
export_symbols =
plugin_deps =
plugin_ldadd =
endif

if PLATFORM_WIN32
no_undefined = -no-undefined
else
no_undefined =
endif

if CYGWIN
cygwin_export_lib = -L$(top_builddir)/src -lclaws-mail
else
cygwin_export_lib = 
endif

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
	gettext.h common-views.c common-views.h \
	day-view.c month-view.c

vcalendarincludedir = $(includedir)/claws-mail/plugins/@PACKAGE@
vcalendarinclude_HEADERS = \
	vcal_interface.h
 
vcalendar_la_LDFLAGS = \
	$(plugin_res_ldflag) $(no_undefined) $(export_symbols) \
	-avoid-version -module \
	$(GTK_LIBS)

vcalendar_la_DEPENDENCIES = $(plugin_deps)

vcalendar_la_LIBADD = $(plugin_ldadd) $(cygwin_export_lib) \
	$(GTK_LIBS) \
        $(top_builddir)/libical/libical/libical.la

AM_CPPFLAGS = \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"

clean-local:
	rm -f libclaws.a
