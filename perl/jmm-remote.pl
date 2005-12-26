#!/usr/bin/perl
#
# madjack-remote.pl
# Perl based Terminal interface for MadJACK
#
# Nicholas J. Humfrey <njh@aelius.com>
#

use Audio::MadJACK;
use Term::ReadKey;
use strict;

# Create MadJACK object for talking to the deck
my $madjack = new Audio::MadJACK(@ARGV);
exit(-1) unless (defined $madjack);

# Display the URL of the MadJACK deck we connected to
print "URL of madjack server: ".$madjack->get_url()."\n";


# Change terminal mode
ReadMode(3);
$|=1;

my $running = 1;
while( $running ) {
	# Get player state
	my $state = $madjack->get_state();
	#last unless (defined $state);

	# Wait for 1/5 second for key-press
	my $key = ReadKey( 0.2 );
	if (defined $key) {
		if ($key eq 'q') {
			$running=0;
		} elsif ($key eq 'l') {
			ReadMode(0);
			print "Enter name of file to load: ";
			my $filename = <STDIN>;
			chomp($filename);
			$madjack->load( $filename );
			ReadMode(3);
		} elsif ($key eq 's') {
			$madjack->stop()
		} elsif ($key eq 'f') {
			print "Filename: ".$madjack->get_filename();
			print " (".$madjack->get_filepath().")\n";
		} elsif ($key eq 'c') {
			$madjack->cue()
		} elsif ($key eq 'e') {
			$madjack->eject()
		} elsif ($key eq 'p') {
			if ($state eq 'PLAYING') { $madjack->pause(); }
			else { $madjack->play(); }
		} else {
			warn "Unknown key command ('$key')\n";
		}
	}
	
	# Display state and time
	my $pos = $madjack->get_position();
	printf("%s [%1.1f]                  \r", $state, $pos);
}


# Restore terminate settings
ReadMode(0);

