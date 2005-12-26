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

$VERSION="0.01";
$ATTEMPTS=5;


sub new {
    my $class = shift;
    
    croak( "Missing JackMiniMix server port or URL" ) if (scalar(@_)<1);

    # Bless the hash into an object
    my $self = { 
    	pong => 0,
    	state => undef,
    	position => undef,
    	filename => undef,
    	filepath => undef
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
    $self->{lo}->add_method( '/deck/state', 's', \&_state_handler, $self );
    $self->{lo}->add_method( '/deck/position', 'd', \&_position_handler, $self );
    $self->{lo}->add_method( '/deck/filename', 's', \&_filename_handler, $self );
    $self->{lo}->add_method( '/deck/filepath', 's', \&_filepath_handler, $self );
    $self->{lo}->add_method( '/pong', '', \&_pong_handler, $self );
    
    # Check JackMiniMix server is there
    if (!$self->ping()) {
    	carp("JackMiniMix server is not responding");
    	return undef;
    }

   	return $self;
}

sub load {
	my $self=shift;
	my ($filename) = @_;
	return $self->_send( '/deck/load', 'LOADING|READY|ERROR', 's', $filename);
}


sub play {
	my $self=shift;
	return $self->_send( '/deck/play', 'PLAYING');
}

sub pause {
	my $self=shift;
	return $self->_send( '/deck/pause', 'PAUSED');
}

sub stop {
	my $self=shift;
	return $self->_send( '/deck/stop', 'STOPPED');
}

sub cue {
	my $self=shift;
	return $self->_send( '/deck/cue', 'LOADING|READY');
}

sub eject {
	my $self=shift;
	return $self->_send( '/deck/eject', 'EMPTY');
}


sub get_state {
	my $self=shift;
	$self->{state} = undef;
	$self->_wait_reply( '/deck/get_state' );
	return $self->{state};
}

sub _state_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{state}=$params[0];
	return 0; # Success
}

sub get_position {
	my $self=shift;
	$self->{postion} = undef;
	$self->_wait_reply( '/deck/get_position' );
	return $self->{position};
}

sub _position_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{position}=$params[0];
	return 0; # Success
}

sub get_filename {
	my $self=shift;
	$self->{filename} = undef;
	$self->_wait_reply( '/deck/get_filename' );
	return $self->{filename};
}

sub _filename_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{filename}=$params[0];
	return 0; # Success
}

sub get_filepath {
	my $self=shift;
	$self->{filepath} = undef;
	$self->_wait_reply( '/deck/get_filepath' );
	return $self->{filepath};
}

sub _filepath_handler {
	my ($serv, $mesg, $path, $typespec, $userdata, @params) = @_;
	$userdata->{filepath}=$params[0];
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


sub _send {
	my $self=shift;
	my ($path, $desired, $typespec, @params) = @_;
	my $state = undef;
	
	# Empty typespec if non specified
	$typespec = '' unless (defined $typespec);
	
	# Try a few times
	for(1..$ATTEMPTS) {
		my $result = $self->{lo}->send( $self->{addr}, $path, $typespec, @params );
		warn "Warning: failed to send '$path' OSC message.\n" if ($result<1);

		# Check what state the player is in now
		$state = $self->get_state();
		last if ($state =~ /^$desired$/i);
	}
	
	# Finally return true if we are in desired state
	if ($state =~ /^$desired$/i) { return 1 }
	else { return 0 }
}


sub _wait_reply {
	my $self=shift;
	my ($path) = @_;
	my $bytes = 0;
	
	# Throw away any old incoming messages
	for(1..$ATTEMPTS) { $self->{lo}->recv_noblock( 0 ); }

	# Try a few times
	for(1..$ATTEMPTS) {
	
		# Send Query
		my $result = $self->{lo}->send( $self->{addr}, $path, '' );
		if ($result<1) {
			warn "Failed to send message ($path): ".$self->{addr}->errstr()."\n";
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

Audio::JackMiniMix - Talk to JackMiniMix server using Object Oriented Perl

=head1 SYNOPSIS

  use Audio::JackMiniMix;

  my $mj = new Audio::JackMiniMix( 'osc.udp://jackminimix.example.net/' );
  $mj->load( 'Playlist_A/mymusic.mp3' );
  $mj->play();


=head1 DESCRIPTION

The Audio::JackMiniMix module uses Net::LibLO to talk to a 
JackMiniMix (MPEG Audio Deck) server. It has an Object Oriented style 
API making it simple to control multiple decks from a single script.


=over 4

=item B<new( oscurl )>

Connect to JackMiniMix deck specified by C<oscurl>.
A ping is sent to the JackMiniMix deck to check to see if it is there.
If a reply is not recieved or there was an error then C<undef> is returned.

=item B<load( filename )>

Send a message to the deck requesting that C<filename> is loaded.
Note: it is up to the developer to check to see if the file was 
successfully loaded, by calling the C<get_state()> method.

Returns 1 if command was successfully received or 0 on error.

=item B<play()>

Tell the deck to start playing the current track.

Returns 1 if command was successfully received or 0 on error.

=item B<pause()>

Tell the deck to pause the current track.

Returns 1 if command was successfully received or 0 on error.

=item B<stop()>

Tell the deck to stop decoding and playback of the current track.

Returns 1 if command was successfully received or 0 on error.

=item B<cue()>

Tell the deck to start decoding from the cue point.

Returns 1 if command was successfully received or 0 on error.

=item B<eject()>

Close the currect track loaded.

Returns 1 if command was successfully received or 0 on error.

=item B<get_state()>

Returns the current state of the JackMiniMix deck.
Returns one of the following strings:

   - PLAYING
   - PAUSED
   - READY
   - LOADING
   - STOPPED
   - EMPTY
   - ERROR
   
If no reply if received from the server or there is an error then 
C<undef> is returned.

=item B<get_position()>

Returns the deck's position (in seconds) in the current track. 
   
If no reply if received from the server or there is an error then 
C<undef> is returned.

=item B<get_filename()>

Returns the filename of the track currently loaded in the deck.
The filename is stripped of its path and suffix.

If no track is currently loaded then an empty string is returned.
If no reply if received from the server or there is an error then 
C<undef> is returned.

=item B<get_filepath()>

Returns the file path of the track currently loaded in the deck 
(path will be in the same form as originally passed to load()).

If no track is currently loaded then an empty string is returned.
If no reply if received from the server or there is an error then 
C<undef> is returned.

=item B<ping()>

Pings the remote deck to see if it is there.

Returns 1 if the server responds, or 0 if there is no reply.

=item B<get_url()>

Returns the OSC URL of the JackMiniMix deck.

=head1 SEE ALSO

L<Net::LibLO>

L<http://www.ecs.soton.ac.uk/~njh/jackminimix/>

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
