#!/usr/bin/perl -I lib
#
# jmm_lsc.pl
# JackMiniMix channel list
#
# Nicholas J. Humfrey <njh@aelius.com>
#

use Audio::JackMiniMix;
use strict;

# Create MadJACK object for talking to the deck
my $minimix = new Audio::JackMiniMix(@ARGV);
exit(-1) unless (defined $minimix);

# Display the URL of the MadJACK deck we connected to
print "URL of JackMiniMix server: ".$minimix->get_url()."\n";



# Get the gain of each of the channels
foreach my $channel (1..$minimix->channel_count()) {
	my $gain = $minimix->get_channel_gain( $channel );
	my $label = $minimix->get_channel_label( $channel );
	print "[$channel] $label: $gain dB\n";
}
