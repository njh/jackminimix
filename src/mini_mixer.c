/*

	mini_mixer.c
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

#include <jack/jack.h>
#include <lo/lo.h>
#include <getopt.h>
#include "config.h"
#include "db.h"

#define		PROGRAM_NAME		"Jack Mini Mixer"
#define		DEFAULT_CLIENT_NAME	"minimixer"
#define		DEFAULT_OSC_PORT	"4444"


typedef struct {
	float current_gain;		// decibels
	float desired_gain;		// decibels
//	float fade_rate;		// dB per sec
	jack_port_t *left_port;
	jack_port_t *right_port;
} mm_channel_t;


jack_port_t *port_out_left = NULL;
jack_port_t *port_out_right = NULL;
jack_client_t *client = NULL;
 
int verbose = 0;
int running = 1;
int channel_count = 0;
float output_gain = 0.0f;		// decibels
mm_channel_t *channels = NULL;


static void
signal_handler (int signum)
{
	switch(signum) {
		case SIGTERM:	fprintf(stderr, "Got termination signal.\n"); break;
		case SIGINT:	fprintf(stderr, "Got interupt signal.\n"); break;
	}
	running=0;
}



static int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    if (verbose>1) printf("Received quit OSC message.\n");
    running = 0;

    return 0;
}

static int channel_gain_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	int chan = argv[0]->i;
	float db = argv[1]->f;

	if (verbose>1) {
    	printf("Received channel gain change OSC message ");
		printf("  (channel=%d, gain=%fdB)\n", chan, db);
	}
	
	if (chan < 1 || chan > channel_count) {
		printf("Error: channel number in OSC message is out of range\n");
		return 1;
	}
	
	// store the new value
	channels[chan-1].current_gain = db;

	return 0;
}


static int output_gain_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
	output_gain = argv[0]->f;

	if (verbose>1) {
    	printf("Received output gain change OSC message (%fdB).\n", output_gain);
	}

	return 0;
}


static int process_jack_audio(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *out_left =
		jack_port_get_buffer(port_out_left, nframes);
	jack_default_audio_sample_t *out_right =
		jack_port_get_buffer(port_out_right, nframes);
	float out_gain = db2lin( output_gain );
	jack_nframes_t n=0;
	int ch;
	
	// Put silence into the outputs
	for ( n=0; n<nframes; n++ ) {
		out_left[ n ] = 0;
		out_right[ n ] = 0;
	}

	// Mix each input into the output buffer
	for ( ch=0; ch < channel_count ; ch++ ) {
		jack_default_audio_sample_t *in_left =
			jack_port_get_buffer(channels[ch].left_port, nframes);
		jack_default_audio_sample_t *in_right =
			jack_port_get_buffer(channels[ch].right_port, nframes);
		
		float mix_gain = db2lin( channels[ch].current_gain );
		for ( n=0; n<nframes; n++ ) {
			out_left[ n ] += (in_left[ n ] * mix_gain);
			out_right[ n ] += (in_right[ n ] * mix_gain);
		}
		
	}

	// Adjust gain on output
	for ( n=0; n<nframes; n++ ) {
		out_left[ n ] *= out_gain;
		out_right[ n ] *= out_gain;
	}

	return 0;
}

static void mm_error(int num, const char *msg, const char *path)
{
    printf("%s error %d in path %s: %s\n", PROGRAM_NAME, num, path, msg);
}


static lo_server_thread setup_osc( const char * port ) 
{
	// Create OSC server
    lo_server_thread st = lo_server_thread_new(port, mm_error);

	// add path handlers
    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);
    lo_server_thread_add_method(st, "/channel/gain", "if", channel_gain_handler, NULL);
    lo_server_thread_add_method(st, "/output/gain", "f", output_gain_handler, NULL);

	// Set OSC Server running
    lo_server_thread_start(st);
    
    return st;
}

static void setup_jack( const char * client_name ) 
{
	// Register with Jack
	if ((client = jack_client_new(client_name)) == 0) {
		fprintf(stderr, "JACK server not running?\n");
		exit(1);
	}
	if (verbose>0) printf("Registering with JACK at '%s'.\n", client_name);

	// Create our pair of output ports
	if (!(port_out_left = jack_port_register(client, "out_left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0))) {
		fprintf(stderr, "Cannot register output port 'out_left'.\n");
		exit(1);
	}
	
	if (!(port_out_right = jack_port_register(client, "out_right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0))) {
		fprintf(stderr, "Cannot register output port 'out_right'.\n");
		exit(1);
	}

	// Register the peak audio callback
	jack_set_process_callback(client, process_jack_audio, 0);

}

static jack_port_t* create_input_port( const char* side, int chan_num )
{
	char port_name[255];
	jack_port_t *port;
	
	snprintf( port_name, 255, "in%d_%s", chan_num+1, side );
	port = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	if (!port) {
		fprintf(stderr, "Cannot register input port '%s'.\n", port_name);
		exit(1);
	}
	if (verbose>1) printf("Registered JACK port '%s'\n", port_name);
	
	return port;
}

static mm_channel_t* setup_channels( int chan_count ) 
{
	mm_channel_t* channels = (mm_channel_t*)malloc( 
			sizeof(mm_channel_t) * chan_count);
	int c;
	
	// Initialise each of the channels
	for(c=0; c<chan_count; c++) {
		
		// Faders start faded down
		channels[c].current_gain=-90.0f;
		channels[c].desired_gain=-90.0f;
		
		// Create the JACK input ports
		channels[c].left_port = create_input_port( "left", c );
		channels[c].right_port = create_input_port( "right", c );
	}
			
	return channels;
}



/* Display how to use this program */
static int usage( const char * progname )
{
	fprintf(stderr, "%s version %s\n\n", PROGRAM_NAME, VERSION);
	fprintf(stderr, "Usage %s -c <channel count> [-p <osc_port>] [-v]\n\n", progname);
	exit(1);
}


int main(int argc, char *argv[])
{
	lo_server_thread server_thread = NULL;
	char* client_name = DEFAULT_CLIENT_NAME;
	char* osc_port = DEFAULT_OSC_PORT;
	int opt;
	
	while ((opt = getopt(argc, argv, "c:p:vh")) != -1) {
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
	
	// Number of channels is not an optional parameter
	if (channel_count<1) {
		usage( argv[0] );
	}
	
	// Dislay welcoming message
	if (verbose>0) printf("Starting %s version %s with %d channels.\n",
							PROGRAM_NAME, VERSION, channel_count);

	// Set signal handlers
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);


	// Setup JACK
	setup_jack( client_name );
	
	// Create the channel descriptors
	channels = setup_channels( channel_count );

	// Set JACK running
	if (jack_activate(client)) {
		fprintf(stderr, "Cannot activate client.\n");
		exit(1);
	}

	// Setup OSC
	server_thread = setup_osc( osc_port );

	
	// Sleep until we are done (work is done in threads)
	while (running) {
		usleep(1000);
	}
	
	
	// Cleanup
	if (server_thread) {
		lo_server_thread_stop( server_thread );
		lo_server_thread_free( server_thread );
		server_thread = NULL;
	}

	//  ** close down JACK **
	//  ** free up channel memory **
	
	return 0;
}

