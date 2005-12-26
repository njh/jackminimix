/*

	minimix.c
	Copyright (C) 2005  Nicholas J. Humfrey
	
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include <jack/jack.h>
#include <lo/lo.h>
#include <getopt.h>
#include "config.h"
#include "db.h"


#define		DEFAULT_CLIENT_NAME		"minimixer"
#define		DEFAULT_CHANNEL_COUNT	(4)
#define		CHANNEL_LABEL_LEN		(12)
#define		FADE_RATE				(0.1)


typedef struct {
	char label[CHANNEL_LABEL_LEN];	// Label for Channel
	float current_gain;				// decibels
	float desired_gain;				// decibels
	jack_port_t *left_port;			// Left Input Port
	jack_port_t *right_port;		// Right Input Port
} jmm_channel_t;


jack_port_t *port_out_left = NULL;
jack_port_t *port_out_right = NULL;
jack_client_t *client = NULL;
 
int verbose = 0;
int quiet = 0;
int running = 1;
int channel_count = DEFAULT_CHANNEL_COUNT;
jmm_channel_t *channels = NULL;


static
void signal_handler (int signum)
{
	if (!quiet) {
		switch(signum) {
			case SIGTERM:	fprintf(stderr, "Got termination signal.\n"); break;
			case SIGINT:	fprintf(stderr, "Got interupt signal.\n"); break;
		}
	}
	running=0;
}



static
int ping_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	lo_server serv = (lo_server)user_data;
	int result;
	
	// Display the address the ping came from
	if (verbose) {
		char *url = lo_address_get_url(src);
		printf( "Got ping from: %s\n", url);
		free(url);
	}

	// Send back reply
	result = lo_send_from( src, serv, LO_TT_IMMEDIATE, "/pong", "" );
	if (result<1) fprintf(stderr, "Error: sending reply failed: %s\n", lo_address_errstr(src));

    return 0;
}

static
int wildcard_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	if (verbose) {
		fprintf(stderr, "Warning: unhandled OSC message: '%s' with args '%s'.\n", path, types);
	}

    return -1;
}

static
int set_gain_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	lo_server serv = (lo_server)user_data;
	int chan = argv[0]->i;
	float db = argv[1]->f;
	int result;

	if (verbose) {
    	printf("Received channel gain change OSC message ");
		printf("  (channel=%d, gain=%fdB)\n", chan, db);
	}
	
	if (chan < 1 || chan > channel_count) {
		fprintf(stderr,"Warning: channel number in OSC message is out of range.\n");
		return 1;
	}
	
	// store the new value
	channels[chan-1].desired_gain = db;

	// Send back reply
	result = lo_send_from( src, serv, LO_TT_IMMEDIATE, "/mixer/channel/gain", "if", chan, channels[chan-1].desired_gain );
	if (result<1) fprintf(stderr, "Error: sending reply failed: %s\n", lo_address_errstr(src));

	return 0;
}

static
int get_gain_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	lo_server serv = (lo_server)user_data;
	int chan = argv[0]->i;
	int result;

	// Send back reply
	result = lo_send_from( src, serv, LO_TT_IMMEDIATE, "/mixer/channel/gain", "if", chan, channels[chan-1].desired_gain );
	if (result<1) fprintf(stderr, "Error: sending reply failed: %s\n", lo_address_errstr(src));

	return 0;
}

static
int set_label_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	lo_server serv = (lo_server)user_data;
	int chan = argv[0]->i;
	char* label = &argv[1]->s;
	int result;

	if (verbose) {
    	printf("Received channel label change OSC message ");
		printf("  (channel=%d, label='%s')\n", chan, label);
	}
	
	if (chan < 1 || chan > channel_count) {
		fprintf(stderr, "Warning: channel number in OSC message is out of range.\n");
		return 1;
	}
	
	// store the new value
	strncpy( channels[chan-1].label, label, CHANNEL_LABEL_LEN);

	// Send back reply
	result = lo_send_from( src, serv, LO_TT_IMMEDIATE, "/mixer/channel/label", "is", chan, channels[chan-1].label );
	if (result<1) fprintf(stderr, "Error: sending reply failed: %s\n", lo_address_errstr(src));

	return 0;
}

static
int get_label_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	lo_server serv = (lo_server)user_data;
	int chan = argv[0]->i;
	int result;

	// Send back reply
	result = lo_send_from( src, serv, LO_TT_IMMEDIATE, "/mixer/channel/label", "is", chan, channels[chan-1].label );
	if (result<1) fprintf(stderr, "Error: sending reply failed: %s\n", lo_address_errstr(src));

	return 0;
}

int get_channel_count_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 lo_message msg, void *user_data)
{
	lo_address src = lo_message_get_source( msg );
	lo_server serv = (lo_server)user_data;
	int result;

	// Send back reply
	result = lo_send_from( src, serv, LO_TT_IMMEDIATE, "/mixer/channel_count", "i", channel_count );
	if (result<1) fprintf(stderr, "Error: sending reply failed: %s\n", lo_address_errstr(src));

	return 0;
}


static
void error_handler(int num, const char *msg, const char *path)
{
    fprintf(stderr, "LibLO server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

static
void shutdown_callback_jack(void *arg)
{
	running = 0;
}


static
int process_jack_audio(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *out_left =
		jack_port_get_buffer(port_out_left, nframes);
	jack_default_audio_sample_t *out_right =
		jack_port_get_buffer(port_out_right, nframes);
	jack_nframes_t n=0;
	int ch;
	
	// Put silence into the outputs
	for ( n=0; n<nframes; n++ ) {
		out_left[ n ] = 0;
		out_right[ n ] = 0;
	}

	// Mix each input into the output buffer
	for ( ch=0; ch < channel_count ; ch++ ) {
		float mix_gain;
		jack_default_audio_sample_t *in_left =
			jack_port_get_buffer(channels[ch].left_port, nframes);
		jack_default_audio_sample_t *in_right =
			jack_port_get_buffer(channels[ch].right_port, nframes);
		
		// Adjust the gain ?
		channels[ch].current_gain = channels[ch].desired_gain;
		
		// Mix the audio
		mix_gain = db2lin( channels[ch].current_gain );
		for ( n=0; n<nframes; n++ ) {
			out_left[ n ] += (in_left[ n ] * mix_gain);
			out_right[ n ] += (in_right[ n ] * mix_gain);
		}
		
	}

	return 0;
}



static
lo_server_thread init_osc( const char * port ) 
{
	lo_server_thread st = NULL;
	lo_server serv = NULL;
	
	// Create new server
	st = lo_server_thread_new( port, error_handler );
	if (!st) return NULL;
	
	// Add the methods
	serv = lo_server_thread_get_server( st );
    lo_server_thread_add_method(st, "/mixer/get_channel_count", "", get_channel_count_handler, serv);
    lo_server_thread_add_method(st, "/mixer/channel/set_gain", "if", set_gain_handler, serv);
    lo_server_thread_add_method(st, "/mixer/channel/get_gain", "i", get_gain_handler, serv);
    lo_server_thread_add_method(st, "/mixer/channel/get_label", "i", get_label_handler, serv);
    lo_server_thread_add_method(st, "/mixer/channel/set_label", "is", set_label_handler, serv);
	lo_server_thread_add_method( st, "/ping", "", ping_handler, serv);

    // add method that will match any path and args
    lo_server_thread_add_method(st, NULL, NULL, wildcard_handler, serv);

	// Start the thread
	lo_server_thread_start(st);

	if (!quiet) {
		char *url = lo_server_thread_get_url( st );
		printf( "OSC server URL: %s\n", url );
		free(url);
	}
	
	return st;
}

static
void finish_osc( lo_server_thread st )
{
	if (verbose) printf( "Stopping OSC server thread.\n");

	lo_server_thread_stop( st );
	lo_server_thread_free( st );
	
}


static
void init_jack( const char * client_name ) 
{
	jack_status_t status;

	// Register with Jack
	if ((client = jack_client_open(client_name, JackNullOption, &status)) == 0) {
		fprintf(stderr, "Failed to start jack client: %d\n", status);
		exit(1);
	}
	if (!quiet) printf("JACK client registered as '%s'.\n", jack_get_client_name( client ) );

	// Create our pair of output ports
	if (!(port_out_left = jack_port_register(client, "out_left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0))) {
		fprintf(stderr, "Cannot register output port 'out_left'.\n");
		exit(1);
	}
	
	if (!(port_out_right = jack_port_register(client, "out_right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0))) {
		fprintf(stderr, "Cannot register output port 'out_right'.\n");
		exit(1);
	}

	// Register shutdown callback
	jack_on_shutdown (client, shutdown_callback_jack, NULL );

	// Register the peak audio callback
	jack_set_process_callback(client, process_jack_audio, 0);

}

static
void finish_jack( jack_client_t *client )
{
	// Leave the Jack graph
	jack_client_close(client);
}


static
jack_port_t* create_input_port( const char* side, int chan_num )
{
	int port_name_size = jack_port_name_size();
	char *port_name = malloc( port_name_size );
	jack_port_t *port;
	
	snprintf( port_name, port_name_size, "in%d_%s", chan_num+1, side );
	port = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	if (!port) {
		fprintf(stderr, "Cannot register input port '%s'.\n", port_name);
		exit(1);
	}
	
	return port;
}

static
jmm_channel_t* init_channels( int chan_count ) 
{
	jmm_channel_t* channels = (jmm_channel_t*)malloc( 
			sizeof(jmm_channel_t) * chan_count);
	int c;
	
	// Initialise each of the channels
	for(c=0; c<chan_count; c++) {
	
		snprintf( channels[c].label, CHANNEL_LABEL_LEN, "Channel %d", c+1 );
		
		// Faders start faded down
		channels[c].current_gain=-90.0f;
		channels[c].desired_gain=-90.0f;
		
		// Create the JACK input ports
		channels[c].left_port = create_input_port( "left", c );
		channels[c].right_port = create_input_port( "right", c );
	}
			
	return channels;
}

static
void finish_channels( jmm_channel_t* channels )
{

}

/* Display how to use this program */
static
int usage( const char * progname )
{
	printf("JackMiniMix version %s\n\n", PACKAGE_VERSION);
	printf("Usage: %s [options]\n", PACKAGE_NAME);
	printf("   -c <count>    Number of input channels (default 4)\n");
	printf("   -p <port>     Set the UDP port number for OSC\n");
	printf("   -n <name>     Name for this JACK client (default minimix)\n");
	printf("   -v            Enable verbose mode\n");
	printf("   -q            Enable quiet mode\n");
	printf("\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	lo_server_thread server_thread = NULL;
	char* client_name = DEFAULT_CLIENT_NAME;
	char* osc_port = NULL;
	int opt;
	
	while ((opt = getopt(argc, argv, "c:p:vqh")) != -1) {
		switch (opt) {
			case 'c':
				channel_count = atoi(optarg);
				break;
			case 'n':
				client_name = optarg;
				break;
			case 'v':
				verbose++;
				break;
			case 'q':
				quiet++;
				break;
			case 'p':
				osc_port = optarg;
				break;
			default:
				fprintf(stderr, "Unknown option '%c'.\n", (char)opt);
			case 'h':
				usage( argv[0] );
				break;
		}
	}
	
	// Not optional parameter
	if (channel_count<1) usage( argv[0] );
	
	// Dislay welcoming message
	if (verbose) printf("Starting JackMiniMix version %s with %d channels.\n",
							VERSION, channel_count);

	// Set signal handlers
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);


	// Setup JACK
	init_jack( client_name );
	
	// Create the channel descriptors
	channels = init_channels( channel_count );

	// Set JACK running
	if (jack_activate(client)) {
		fprintf(stderr, "Cannot activate client.\n");
		exit(1);
	}

	// Setup OSC
	server_thread = init_osc( osc_port );

	
	// Sleep until we are done (work is done in threads)
	while (running) {
		usleep(1000);
	}
	
	
	// Cleanup
	finish_osc( server_thread );
	finish_jack( client );
	finish_channels( channels );

	return 0;
}

