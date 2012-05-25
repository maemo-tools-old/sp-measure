CFLAGS = -I../src -g
LDFLAGS = -L../src/.libs/
LDADD = ../src/.libs/libspmeasure.a

TESTS = test_sp_measure
check_PROGRAMS = test_sp_measure

test_sp_measure_SOURCES = test_sp_measure.c

distclean-local: clean
	-rm -f Makefile Makefile.in
	-rm -f *log



