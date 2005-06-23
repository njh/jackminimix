bin_PROGRAMS = jack_mini_mixer jmmc

jack_mini_mixer_SOURCES = mini_mixer.c
jack_mini_mixer_CFLAGS = -g -Wall @JACK_CFLAGS@ @LIBLO_CFLAGS@
jack_mini_mixer_LDFLAGS = -lm @JACK_LIBS@ @LIBLO_LIBS@

jmmc_SOURCES = mini_mixer_client.c
jmmc_CFLAGS = -g -Wall @LIBLO_CFLAGS@
jmmc_LDFLAGS = @LIBLO_LIBS@
