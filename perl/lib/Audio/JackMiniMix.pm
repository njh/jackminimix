package Audio::JackMiniMix;

################
#
# JackMiniMix: perl control interface
#
# Copyright 2005 Nicholas J. Humfrey <njh@aelius.com>
#

use Carp;

use Net::LibLO;
use strict;

use vars qw/$VERSION $ATTEMPTS/;

$VERSION="0.02";
$ATTEMPTS=5;


sub new {
    my $class = shift;
    
    croak( "Missing JackMiniMix server port or URL" ) if (scalar(@_)<1);

    # Bless the hash into an object
    my $self = { 
    	channel_count => 0,
    	channels => {},
    	pong => 0
    };
    bless $self, $class;

    # Create address of JackMiniMix server
    $self->{addr} = new Net::LibLO::Address( @_ );
    if (!defined $self->{addr}) {
    	carp("Error creating Net::LibLO::Address");
    	return undef;
    }
        
    # Create new LibLO instance
    $self->{lo} = new Net::LibLO();
    if (!defined $self->{lo}) {
    	carp("Error creating Net::LibLO");
    	return undef;
    }
    
    # Add reply handlers
    $self->{lo}->add_method( '/mixer/channel/gain', 'if', \&_channel_gain_handler, $self );
    $self->{lo}->add_method( '/mixer/channel/label', 'is', \&_channel_label_handler, $self );
    $self->{lo}->add_method( '/mixer/channel_count', 'i', \&_channel_count_handler, $self );
	$self->{lo}->add_method( '/pong', '', \&_pong_handler, $self );
    
    # Get the number of channels
    if (!$self->channel_count()) {
    	carp("JackMiniMix server is not responding");
    	return undef;
    }

   	return $self;
}

sub channel_count {
	my $self=shift;
	$self->{channel_count} = 0;
	$self->_wait_reply( '/mixer/get_channel_count' );
	return $self->{channel_count};
}

sub _channel_count_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{channel_count}=$params[0];
	return 0; # Success
}

sub get_channel_gain {
	my $self=shift;
	my ($channel) = @_;
	croak "Total number of channels is unknown" unless ($self->{channel_count});
	$self->{channels}->{$channel}->{gain} = undef;
	$self->_wait_reply( '/mixer/channel/get_gain', 'i', $channel );
	return $self->{channels}->{$channel}->{gain};
}

sub set_channel_gain {
	my $self=shift;
	my ($channel,$gain) = @_;
	croak "Total number of channels is unknown" unless ($self->{channel_count});
	$self->_wait_reply( '/mixer/channel/set_gain', 'if', $channel, $gain );
	return 1 if ($self->{channels}->{$channel}->{gain} == $gain);
	return 0; # Failed
}

sub _channel_gain_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	my ($channel, $gain) = @params;
	
	# Check channel exists
	if (!exists $userdata->{channels}->{$channel}) {
		$userdata->{channels}->{$channel} = {};
	}
	
	# Store the gain
	$userdata->{channels}->{$channel}->{gain} = $gain;
	
	return 0; # Success
}


sub get_channel_label {
	my $self=shift;
	my ($channel) = @_;
	croak "Total number of channels is unknown" unless ($self->{channel_count});
	$self->{channels}->{$channel}->{label} = undef;
	$self->_wait_reply( '/mixer/channel/get_label', 'i', $channel );
	return $self->{channels}->{$channel}->{label};
}

sub set_channel_label {
	my $self=shift;
	my ($channel,$label) = @_;
	croak "Total number of channels is unknown" unless ($self->{channel_count});
	$self->_wait_reply( '/mixer/channel/set_label', 'is', $channel, $label );
	return 1 if ($self->{channels}->{$channel}->{label} == $label);
	return 0; # Failed
}

sub _channel_label_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	my ($channel, $label) = @params;
	
	# Check channel exists
	if (!exists $userdata->{channels}->{$channel}) {
		$userdata->{channels}->{$channel} = {};
	}
	
	# Store the label
	$userdata->{channels}->{$channel}->{label} = $label;
	
	return 0; # Success
}

sub ping {
	my $self=shift;
	$self->{pong} = 0;
	$self->_wait_reply( '/ping' );
	return $self->{pong};
}

sub _pong_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{pong}++;
	return 0; # Success
}

sub get_url {
	my $self=shift;
	return $self->{addr}->get_url();
}


sub _wait_reply {
	my $self=shift;
	my (@args) = @_;
	my $bytes = 0;
	
	# Throw away any old incoming messages
	for(1..$ATTEMPTS) { $self->{lo}->recv_noblock( 0 ); }

	# Try a few times
	for(1..$ATTEMPTS) {
	
		# Send Query
		my $result = $self->{lo}->send( $self->{addr}, @args );
		if ($result<1) {
			warn "Failed to send message (".join(',',@args)."): ".$self->{addr}->errstr()."\n";
			sleep(1);
			next;
		}

		# Wait for reply within one second
		$bytes = $self->{lo}->recv_noblock( 1000 );
		if ($bytes<1) {
			warn "Timed out waiting for reply after one second.\n";
		} else { last; }
	}
	
	# Failed to get reply ?
	if ($bytes<1) {
		warn "Failed to get reply from JackMiniMix server after $ATTEMPTS attempts.\n";
	}
	
	return $bytes;
}


1;

__END__

=pod

=head1 NAME

Audio::JackMiniMix - Talk to JACK Mini Mixer using OSC

=head1 SYNOPSIS

  use Audio::JackMiniMix;

  my $mix = new Audio::JackMiniMix('osc.udp://host.example.net:3450/');
  $mix->set_channel_gain( 1, -50 );
  $mix->set_channel_gain( 2, -90 );


=head1 DESCRIPTION

The Audio::JackMiniMix module uses Net::LibLO to talk to a JackMiniMix server. 
It can be used to get and set the gains and labels for the channels of the mixer.

=over 4

=item B<new( oscurl )>

Connect to JackMiniMix process specified by C<oscurl>.
A channel_count() query is sent to the mixer, if the mixer does not 
respond, then undef is returned.


=item B<channel_count()>

Returns the number of stereo input channels that the mixer has.


=item B<get_channel_gain( channel )>

Returns the gain (in decibels) of channel.

C<channel> is the number of the channel (in range 1 to total number of channels).


=item B<set_channel_gain( channel, gain )>

Sets the gain of channel C<channel> to C<gain> dB.

C<channel> is the number of the channel (in range 1 to total number of channels).

C<gain> is the gain (in decibels) to set the channel to (in range -90 to 90 dB).


=item B<get_channel_label( channel )>

Returns the label (string) of channel number C<channel>.

C<channel> is the number of the channel (in range 1 to total number of channels).


=item B<set_channel_label( channel, label )>

Sets the label (string) of channel number C<channel> to C<label>.

C<channel> is the number of the channel (in range 1 to total number of channels).

C<label> is the new label for the channel.

=item B<ping()>

Pings the mixer to see if it is there.

Returns 1 if the server responds, or 0 if there is no reply.

=item B<get_url()>

Returns the OSC URL of the JackMiniMix deck.

=head1 SEE ALSO

L<Net::LibLO>

L<http://www.aelius.com/njh/jackminimix/>

=head1 AUTHOR

Nicholas J. Humfrey <njh@aelius.com>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2005 Nicholas J. Humfrey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

=cut
