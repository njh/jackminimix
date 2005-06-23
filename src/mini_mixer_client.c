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


#define		PROGRAM_NAME		"Jack Mini Mixer Client"
#define		DEFAULT_OSC_HOST	"localhost"
#define		DEFAULT_OSC_PORT	"4444"


/* Display how to use this program */
static int usage()
{
	fprintf(stderr, "%s version %s\n\n", PROGRAM_NAME, VERSION);
	fprintf(stderr, "Usage: jmmc [-h <hostname>] [-p <port>] <command> [<params>]\n");
	fprintf(stderr, "Available commands:\n");
	fprintf(stderr, "    quit\n");
	fprintf(stderr, "    channel_volume <channel> <volume>\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	char* osc_host = DEFAULT_OSC_HOST;
	char* osc_port = DEFAULT_OSC_PORT;
	char* cmd = NULL;
	lo_address* addr = NULL;
	int opt;
	
	while ((opt = getopt(argc, argv, "h:p:")) != -1) {
		switch (opt) {
			case 'h':
				osc_host = optarg;
				break;
			case 'p':
				osc_port = optarg;
				break;
			default:
				fprintf(stderr, "Unknown option '%c'.\n", (char)opt);
				usage( argv[0] );
				break;
		}
	}
	argc -= optind;
	argv += optind;
     
	/* Create a new liblo socket */
	addr = lo_address_new( osc_host, osc_port );
	
	
	/* Next argument is the command */
	if (argc<1) usage();
	cmd = argv[0]; argv++; argc--;
	
	/* Send command to the server */
	if (strcasecmp( cmd, "quit" )==0) {
		lo_send( addr, "/quit", "");
	} else if (strcasecmp( cmd, "channel_volume" )==0) {
		lo_send( addr, "/channel/volume", "if", atoi(argv[0]), atof(argv[1]));
	} else {
		fprintf(stderr, "Unknown command '%s'.\n", cmd);
		usage();
	}
	
	
	/* free memory used by the address */
	if (addr) {
		lo_address_free( addr );
		addr = NULL;
	}

	return 0;
}

