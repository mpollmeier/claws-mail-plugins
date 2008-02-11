libgtkhtmlincludedir = $(includedir)/gtkhtml-2.0/libgtkhtml/util

INCLUDES = \
	-I$(top_srcdir) \
	$(GTKHTML2_CFLAGS) \
	-DG_LOG_DOMAIN=\"HtmlUtil\"

noinst_LTLIBRARIES = libhtmlutil.la

libhtmlutil_la_SOURCES = \
	htmlatomlist.c \
	htmlglobalatoms.c \
	htmlstream.c \
	rfc1738.c \
	htmlstreambuffer.c

libgtkhtmlinclude_HEADERS = \
	htmlatomlist.h \
	htmlglobalatoms.h \
	htmlstreambuffer.h \
	htmlglobalatomlist.h \
	rfc1738.h \
	htmlstream.h

BUILT_SOURCES = htmlmarshal.h htmlmarshal.c

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-gmh xgen-gmc
CLEANFILES = $(gen_sources)

htmlmarshal.h: htmlmarshal.list
	echo "#ifndef __HTML_MARSHAL_H__" > xgen-gmh \
	&& echo "#define __HTML_MARSHAL_H__" >> xgen-gmh \
	&& glib-genmarshal --nostdinc --prefix=html_marshal $(srcdir)/htmlmarshal.list --header >> xgen-gmh \
	&& echo "#endif /* __HTML_MARSHAL_H__ */" >> xgen-gmh \
	&& (cmp -s xgen-gmh $(srcdir)/htmlmarshal.h || cp xgen-gmh $(srcdir)/htmlmarshal.h) \
	&& rm -f xgen-gmh xgen-gmh~
htmlmarshal.c: htmlmarshal.h
	echo "#include <glib.h>" > xgen-gmc \
	&& echo "#include <glib-object.h>" >> xgen-gmc \
	&& echo "#include \"htmlmarshal.h\"" >> xgen-gmc \
	&& glib-genmarshal --nostdinc --prefix=html_marshal $(srcdir)/htmlmarshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(srcdir)/htmlmarshal.c \
	&& rm -f xgen-gmc xgen-gmc~

EXTRA_DIST = htmlmarshal.list $(BUILT_SOURCES)

noinst_PROGRAMS = test-atoms

test_atoms_SOURCES = \
	test-atoms.c

test_atoms_LDADD = libhtmlutil.la $(GTKHTML2_LIBS)

MAINTAINERCLEANFILES =	\
	Makefile.in
