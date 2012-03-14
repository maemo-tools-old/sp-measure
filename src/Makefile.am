CFLAGS ?= -O2 -g
CFLAGS += -Wall

lib_LTLIBRARIES = libspmeasure.la 

libspmeasure_la_SOURCES = sp_measure_system.c sp_measure_process.c measure_utils.c
libspmeasure_la_LDFLAGS=$(VERSION_INFO)

DISTCLEANFILES = Makefile.in
