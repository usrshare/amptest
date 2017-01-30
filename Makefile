OBJDIR=obj
SRCDIR=src

PREFIX=i686-w64-mingw32-

CC=$(PREFIX)gcc
LD=$(PREFIX)ld
CFLAGS=-std=c11 -I/usr/share/mingw-w64/include -mwindows
LIBS=-lcomctl32

_OBJS = main.o

ifdef RELEASE
DEFS += -xSSE3 -O3 -DNDEBUG
else
DEFS += -g
endif

OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

hello.exe: $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFS)

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -rf $(OBJDIR)/*.o hello.exe
