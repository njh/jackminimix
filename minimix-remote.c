/*
	mini_mixer_client.c
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

#include <lo/lo.h>
#include <getopt.h>
#include "config.h"


static
void error_handler(int num, const char *msg, const char *path)
{
    fprintf(stderr, "liblo error %d in path %s: %s\n", num, path, msg);
}


/* Display how to use this program */
static int usage(  )
{
	printf("jack-minimix-remote version %s\n\n", PACKAGE_VERSION);
	printf("Usage: jmmc [-h <hostname>] [-p <port>] <command> [<params>]\n");
	printf("Available commands:\n");
	printf("    quit\n");
	printf("    channel_gain <channel> <gain>\n");
	printf("    output_gain <gain>\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	char *port = NULL;
	char *url = NULL;
	lo_address addr = NULL;
	lo_server serv = NULL;
	int opt;

	// Parse Switches
	while ((opt = getopt(argc, argv, "p:u:h")) != -1) {
		switch (opt) {
			case 'p':
				port = optarg;
				break;
				
			case 'u':
				url = optarg;
				break;
				
			default:
				usage( );
				break;
		}
	}
	
	// Need either a port or URL
	if (!port && !url) {
		fprintf(stderr, "Either URL or Port argument is required.\n");
		usage( );
	}
	
	// Check remaining arguments
    argc -= optind;
    argv += optind;
    if (argc<1) usage( );
    
    
	// Create address structure to send on
	if (port) 	addr = lo_address_new(NULL, port);
	else		addr = lo_address_new_from_url(url);

	
	// Create a server for receiving replies on
    serv = lo_server_new(NULL, error_handler);
	//lo_server_add_method( serv, "/deck/state", "s", state_handler, addr);
	//lo_server_add_method( serv, "/deck/position", "f", position_handler, addr);
	//lo_server_add_method( serv, "/pong", "", ping_handler, addr);

	// Send command to the server
	if (strcasecmp( argv[0], "quit" )==0) {
		lo_send( addr, "/quit", "");
	} else if (strcasecmp( argv[0], "channel_gain" )==0) {
		lo_send( addr, "/channel/gain", "if", atoi(argv[0]), atof(argv[1]));
	} else if (strcasecmp( argv[0], "output_gain" )==0) {
		lo_send( addr, "/output/gain", "f", atof(argv[0]));
	} else {
		fprintf(stderr, "Unknown command '%s'.\n", argv[0]);
		usage();
	}
	
	
	/* free memory used by the address */
	if (addr) {
		lo_address_free( addr );
		addr = NULL;
	}

	return 0;
}

