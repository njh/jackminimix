AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = jack-minimixer jack-minimixer-remote

jack_minimixer_SOURCES = minimixer.c
jack_minimixer_CFLAGS = -Wall @JACK_CFLAGS@ @LIBLO_CFLAGS@
jack_minimixer_LDFLAGS = -lm @JACK_LIBS@ @LIBLO_LIBS@

jack_minimixer_remote_SOURCES = minimixer-remote.c
jack_minimixer_remote_CFLAGS = -Wall @LIBLO_CFLAGS@
jack_minimixer_remote_LDFLAGS = @LIBLO_LIBS@

EXTRA_DIST = autogen.sh TODO
