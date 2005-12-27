AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = jack-minimix

jack_minimix_SOURCES = minimix.c
AM_CFLAGS = -Wall @JACK_CFLAGS@ @LIBLO_CFLAGS@
AM_LDFLAGS = -lm @JACK_LIBS@ @LIBLO_LIBS@

EXTRA_DIST = autogen.sh TODO
