CC=gcc

BASE=d:/dev
INC=$(BASE)/include
LIB=$(BASE)/lib
PROJ=$(BASE)/proj
SYLPHEED=$(PROJ)/sylpheed-claws

GLIB=-I$(INC) -I$(INC)/glib-2.0 -I$(LIB)/glib-2.0/include
GTK=-I$(INC) -I$(INC)/gdk -I$(INC)/gtk -I$(LIB)/gtk+/include
COMPAT=-I$(PROJ)/w32lib/src -I$(PROJ)/regex/src
SYLPHEED_INC=$(COMPAT) -I$(SYLPHEED)/src -I $(SYLPHEED)/src/common -I $(SYLPHEED)/src/gtk
SYLPHEED_LIB=$(SYLPHEED)/win32/sylpheed.dll

DEFINES=-DHAVE_CONFIG_H -D_FINDDATA_T_DEFINED -DHAVE_TOWLOWER
LIBRARIES=$(SYLPHEED_LIB) $(LIB)/libglib-2.0.dll.a $(LIB)/libgdk.dll.a $(LIB)/libgtk.dll.a
FLAGS=-shared $(DEFINES) $(SYLPHEED_INC) $(GLIB) $(GTK) $(LIBRARIES)

VPATH=.:..:../src

all: fetchinfo_plugin.dll fetchinfo_plugin_gtk.dll

fetchinfo_plugin.dll: fetchinfo_plugin.c fetchinfo_plugin.h
	$(CC) $< $(FLAGS) -o $@

fetchinfo_plugin_gtk.dll: fetchinfo_plugin_gtk.c fetchinfo_plugin.h
	$(CC) $< $(FLAGS) fetchinfo_plugin.dll -o $@
