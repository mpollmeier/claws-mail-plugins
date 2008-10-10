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

plugin_LTLIBRARIES = gtkhtml2_viewer.la

gtkhtml2_viewer_la_SOURCES = \
         gtkhtml2_viewer.c \
	 gtkhtml2_prefs.c \
	 gtkhtml2_prefs.h \
	 gettext.h

gtkhtml2_viewer_la_LDFLAGS = \
	$(plugin_res_ldflag) $(no_undefined) $(export_symbols) \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(GTKPRINTUNIX_LIBS) \
	$(GTKHTML2_LIBS)

gtkhtml2_viewer_la_DEPENDENCIES = $(plugin_deps)

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk \
	-I$(top_srcdir)

gtkhtml2_viewer_la_LIBADD = $(plugin_ldadd) $(cygwin_export_lib) \
	$(GTK_LIBS) \
        $(top_builddir)/libgtkhtml/libgtkhtml-2.la

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GTKPRINTUNIX_CFLAGS) \
	$(GTKHTML2_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"

clean-local:
	rm -f libclaws.a

