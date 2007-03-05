plugindir = $(CLAWS_MAIL_PLUGINDIR)

plugin_LTLIBRARIES = pdf_viewer.la

pdf_viewer_la_SOURCES = \
         poppler_viewer.c \
	 gettext.h

pdf_viewer_la_LDFLAGS = \
	-avoid-version -module \
	$(GTK_LIBS) \
	$(POPPLER_LIBS)

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/gtk

AM_CPPFLAGS = \
	-Wall \
	$(CLAWS_MAIL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(OPENSSL_CFLAGS) \
	$(POPPLER_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\"

EXTRA_DIST = \
	doc_info.xpm \
	doc_index.xpm \
	doc_index_close.xpm \
	first_arrow.xpm \
	last_arrow.xpm \
	left_arrow.xpm \
	right_arrow.xpm \
	rotate_left.xpm \
	rotate_right.xpm \
	zoom_fit.xpm \
	zoom_in.xpm \
	zoom_out.xpm \
	zoom_width.xpm
