
CC ?= gcc
DEBUG = -g3
WARN = -W -Wall
CFLAGS ?= -std=c99 -O3

all:	xatmas2

xatmas2:	xatmas2.c
	$(CC) -o xatmas2 xatmas2.c $(DEBUG) $(WARN) $(CFLAGS)

clean:
	rm -f xatmas2 AUTORUN.SYS

distclean: clean
	rm -f *~ */*~

test:	xatmas2
	./xatmas2 tst/OPCTEST.SRC
	diff -s tst/OPCTEST.OK AUTORUN.SYS
	./xatmas2 tst/ATMAS.SRC
	diff -s tst/ATMAS.OK AUTORUN.SYS
