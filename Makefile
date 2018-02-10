
CC ?= gcc
DEBUG = -g3
WARN = -W -Wall -Wextra
CFLAGS ?= -std=c99 -O3
DEFINES = -D_XOPEN_SOURCE=500

all:	xatmas2

xatmas2:	xatmas2.c
	$(CC) -o xatmas2 xatmas2.c $(DEBUG) $(WARN) $(CFLAGS) $(DEFINES)

clean:
	rm -f xatmas2 AUTORUN.SYS

distclean: clean
	rm -f *~ */*~ opctest.xex atmas.xex

test:	xatmas2
	./xatmas2 -o opctest.xex tst/OPCTEST.SRC
	diff -s tst/OPCTEST.OK opctest.xex
	./xatmas2 -o atmas.xex tst/ATMAS.SRC
	diff -s tst/ATMAS.OK atmas.xex
