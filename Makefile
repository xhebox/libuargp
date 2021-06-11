prefix=/usr/local
bindir=$(prefix)/bin
includedir=$(prefix)/include
libdir=$(prefix)/lib

SRC = $(sort $(wildcard *.c))
OBJS = $(SRC:.c=.o)

SHLIB_EXT=.so
ALL_INCLUDES=argp.h
ALL_LIBS=libargp$(SHLIB_EXT) libargp.a

CFLAGS=-O0 -fPIC -Wall
BUILDCFLAGS=$(CFLAGS) -I.
LDFLAGS=

AR ?= $(CROSS_COMPILE)ar
RANLIB  ?= $(CROSS_COMPILE)ranlib
CC ?= $(CROSS_COMPILE)cc

INSTALL ?= ./install.sh

all: $(ALL_LIBS)

install: $(ALL_LIBS:lib%=$(DESTDIR)$(libdir)/lib%) $(ALL_INCLUDES:%=$(DESTDIR)$(includedir)/%)

clean:
	rm -f $(ALL_LIBS)
	rm -f $(OBJS)
	rm -f *.o *.so

%.o: %.c
	$(CC) $(BUILDCFLAGS) -c -o $@ $<

libargp.a: $(OBJS)
	rm -f $@
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

libargp$(SHLIB_EXT): $(OBJS)
	rm -f $@
	$(CC) $(LDFLAGS) -shared -o $@ $(OBJS) $(LDFLAGS)

$(DESTDIR)$(libdir)/%: %
	$(INSTALL) -D -m 755 $< $@

$(DESTDIR)$(includedir)/%.h: %.h
	$(INSTALL) -D -m 644 $< $@

.PHONY: all clean install
