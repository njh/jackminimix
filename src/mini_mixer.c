/*

	jackmeter.c
	Simple console based Digital Peak Meter for JACK
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


typedef struct {
	float current_db;
	float desired_db;
	jack_port_t *left_port;
	jack_port_t *right_port;
} mm_channel_t;


jack_port_t *port_out_left = NULL;
jack_port_t *port_out_right = NULL;
jack_client_t *client = NULL;

int verbose = 0;
int running = 1;
int channel_count = 0;
mm_channel_t *channels = NULL;



static int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    printf("Received quit OSC message.\n");
    running = 0;

    return 0;
}

static int process_audio(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in;

/*
	// just incase the port isn't registered yet
	if (input_port == NULL) {
		return 0;
	}


	// get the audio samples, and find the peak sample
	in = (jack_default_audio_sample_t *) jack_port_get_buffer(input_port, nframes);
	for (i = 0; i < nframes; i++) {
		const float s = fabs(in[i]);
		if (s > peak) {
			peak = s;
		}
	}
*/

	return 0;
}

static void mm_error(int num, const char *msg, const char *path)
{
    printf("mini mixer error %d in path %s: %s\n", num, path, msg);
}



/* Display how to use this program */
static int usage( const char * progname )
{
	fprintf(stderr, "jack_mini_mixer version %s\n\n", VERSION);
	fprintf(stderr, "Usage %s -c <channel count> [-p <osc_port>]\n\n", progname);
	exit(1);
}

static void setup_osc( const char * port ) 
{
	// Create OSC server
    lo_server_thread st = lo_server_thread_new(port, mm_error);

	// add method that will match the path /quit with no args
    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);

	// Set OSC Server running
    lo_server_thread_start(st);
}

static void setup_jack( const char * client_name ) 
{
	// Register with Jack
	if ((client = jack_client_new(client_name)) == 0) {
		fprintf(stderr, "JACK server not running?\n");
		exit(1);
	}
	if (verbose) printf("Registering as %s.\n", client_name);

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
	jack_set_process_callback(client, process_audio, 0);

}

static jack_port_t* create_input_port( const char* side, int chan_num )
{
	char port_name[255];
	jack_port_t *port;
	
	snprintf( port_name, 255, "in%d_%s", chan_num, side );
	if (!(port = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0))) {
		fprintf(stderr, "Cannot register input port '%s'.\n", port_name);
		exit(1);
	}
	if (verbose) printf("Registered port %s\n", port_name);
	
	return port;
}

static mm_channel_t* setup_channels( int chan_count ) 
{
	mm_channel_t* channels = (mm_channel_t*)malloc( 
			sizeof(mm_channel_t) * chan_count);
	int c;
	
	// Initialise each of the channels
	for(c=1; c<=chan_count; c++) {
		
		// Faders start faded down
		channels[c].current_db=0;
		channels[c].desired_db=0;
		
		// Create the JACK input ports
		channels[c].left_port = create_input_port( "left", c );
		channels[c].right_port = create_input_port( "right", c );
	}
			
	return channels;
}


int main(int argc, char *argv[])
{
	int opt;
	char* client_name = "minimixer";
	char* osc_port = "4444";
	
	while ((opt = getopt(argc, argv, "c:p:vh")) != -1) {
		switch (opt) {
			case 'c':
				channel_count = atof(optarg);
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
	setup_osc( osc_port );
	
	
	// Sleep until we are done (work is done in threads)
	while (running) {
		usleep(1000);
	}
	
	
	// ** Cleanup **
	//  - close down OSC server
	//  - close down JACK
	//  - free up channel memory
	
	return 0;
}

