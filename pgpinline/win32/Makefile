CC=gcc

BASE=d:/dev
INC=$(BASE)/include
LIB=$(BASE)/lib
PROJ=$(BASE)/proj
SYLPHEED=$(PROJ)/sylpheed-claws

GLIB_INC=-I$(INC) -I$(INC)/glib-2.0 -I$(LIB)/glib-2.0/include
GTK_INC=-I$(INC) -I$(INC)/gdk -I$(INC)/gtk -I$(LIB)/gtk+/include
GPGME_INC=-I$(PROJ)/gpgme/gpgme
GPGME_LIB=$(PROJ)/gpgme/gpgme.lib
COMPAT=-I$(PROJ)/w32lib/src -I$(PROJ)/regex/src
SYLPHEED_INC=$(COMPAT) -I$(SYLPHEED)/src -I $(SYLPHEED)/src/common -I $(SYLPHEED)/src/gtk
SYLPHEED_LIB=$(SYLPHEED)/win32/sylpheed.dll
PGPMIME_LIB=$(SYLPHEED)/win32/pgpmime.dll

DEFINES=-DHAVE_CONFIG_H
LIBRARIES=$(SYLPHEED_LIB) $(GPGME_LIB) $(PGPMIME_LIB) $(LIB)/libglib-2.0.dll.a $(LIB)/libgdk.dll.a $(LIB)/libgtk.dll.a
LDFLAGS=-mdll --export-all-symbols $(LIBRARIES)
CFLAGS=-fnative-struct $(DEFINES) -I. -I../src $(SYLPHEED_INC) $(GPGME_INC) $(GLIB_INC) $(GTK_INC)

VPATH=.:..:../src

all: pgpinline.dll

config.h:
	echo "#define USE_GPGME 1">>$@
	echo "#define DHAVE_BYTE_TYPEDEF 1">>$@
	echo "#define D_INTL_REDIRECT_INLINE 1">>$@
	echo "#define HAVE_TOWLOWER 1">>$@

pgpinline_OBJECTS=plugin.o pgpinline.o version.rc.o
plugin.o: plugin.c
pgpinline.o: pgpinline.c pgpinline.h
version.rc.o: version.rc
	windres -i $< -o $@
pgpinline.dll: config.h $(pgpinline_OBJECTS)
	dlltool -D $@ -z $@.def --export-all-symbols --exclude-symbols WinMain@16 $(pgpinline_OBJECTS)
	$(CC) -mdll -o junk.tmp -Wl,--base-file,$@.base $(pgpinline_OBJECTS) $(LIBRARIES)
	dlltool --dllname $@ --base-file $@.base --output-exp $@.exp --def $@.def
	$(CC) -mdll -o $@ $(pgpinline_OBJECTS) -Wl,$@.exp $(LIBRARIES)
	-rm $@.base $@.exp $@.lib junk.tmp

clean:
	-rm $(pgpinline_OBJECTS) pgpinline.dll config.h

