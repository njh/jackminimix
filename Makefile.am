AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = jackminimix

jackminimix_SOURCES = minimix.c db.h
AM_CFLAGS = -Wall @JACK_CFLAGS@ @LIBLO_CFLAGS@
AM_LDFLAGS = -lm @JACK_LIBS@ @LIBLO_LIBS@

EXTRA_DIST = autogen.sh TODO
