#! /bin/sh

./xatmas2 test/OPCTEST.SRC
diff -s test/OPCTEST.OK AUTORUN.SYS

./xatmas2 test/ATMAS.SRC
diff -s test/ATMAS.OK AUTORUN.SYS
