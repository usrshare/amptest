OBJDIR=obj
SRCDIR=src

PREFIX=i686-w64-mingw32-

CC=$(PREFIX)gcc
LD=$(PREFIX)ld
ifeq ($(shell which windres),)
WINDRES=$(PREFIX)windres
else
WINDRES=windres
endif

CFLAGS=-std=c11 -I/usr/share/mingw-w64/include -Wall -Wno-unknown-pragmas
LDFLAGS=
LIBS=-lcomctl32 -lgdi32 -mwindows

_OBJS = main.o plist.o wa_plugins.o ui.o ui_string.o ui_skin.o win_misc.o menus.rc.o 

ifdef RELEASE
DEFS += -xSSE3 -O3 -DNDEBUG
else
DEFS += -g
endif

OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

hello.exe: $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFS)

$(OBJDIR)/%.rc.o: $(SRCDIR)/%.rc $(OBJDIR)
	$(WINDRES) -i $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -rf $(OBJDIR)/*.o hello.exe
