AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = jack-minimix jack-minimix-remote

jack_minimix_SOURCES = minimix.c
jack_minimix_CFLAGS = -Wall @JACK_CFLAGS@ @LIBLO_CFLAGS@
jack_minimix_LDFLAGS = -lm @JACK_LIBS@ @LIBLO_LIBS@

jack_minimix_remote_SOURCES = minimix-remote.c
jack_minimix_remote_CFLAGS = -Wall @LIBLO_CFLAGS@
jack_minimix_remote_LDFLAGS = @LIBLO_LIBS@

EXTRA_DIST = autogen.sh TODO
