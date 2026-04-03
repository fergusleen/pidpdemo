PROG=pidpdemo
CC=cc
CFLAGS=-O
CPPFLAGS=
LDFLAGS=
LDLIBS=
PREFIX=$(HOME)/pidpdemo

OBJS=src/main.o src/menu.o src/pager.o src/actions.o

.PHONY: all clean install smoke run

# Plain build:
#   make
#
# Curses build, if the target has a usable curses library:
#   make clean
#   make CPPFLAGS='-DUSE_CURSES=1' LDLIBS='-lcurses -ltermcap'

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(PROG) $(OBJS) $(LDFLAGS) $(LDLIBS)

src/main.o: src/main.c src/menu.h src/actions.h src/compat.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/main.c -o src/main.o

src/menu.o: src/menu.c src/menu.h src/actions.h src/compat.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/menu.c -o src/menu.o

src/pager.o: src/pager.c src/pager.h src/menu.h src/compat.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/pager.c -o src/pager.o

src/actions.o: src/actions.c src/actions.h src/pager.h src/menu.h src/compat.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/actions.c -o src/actions.o

clean:
	rm -f $(PROG) $(OBJS)

run: $(PROG)
	./$(PROG) -p -d pages

install: $(PROG)
	test -d $(PREFIX) || mkdir $(PREFIX)
	test -d $(PREFIX)/bin || mkdir $(PREFIX)/bin
	test -d $(PREFIX)/pages || mkdir $(PREFIX)/pages
	cp $(PROG) $(PREFIX)/bin/$(PROG)
	cp pages/*.txt $(PREFIX)/pages/

smoke:
	sh scripts/smoke-test.sh
