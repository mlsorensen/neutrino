package Neutrino::Hub;

use 5.014002;
use strict;
use warnings;
use JSON;
use Fcntl qw(:flock);
use Data::Dumper;

require Exporter;

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
	
);

our $VERSION = '0.01';


# Preloaded methods go here.

sub new {
    my ($class_name) = shift;
    my $self = {};
    bless($self, $class_name);

    if (host_type() eq 'bbb') {
        $self->{reset_pin}    = 51;
        $self->{serial_port}  = '/dev/ttyO1';
    } else {
	$self->{reset_pin}    = 20;
        $self->{serial_port}  = '/dev/ttyAMA0';
    }

    $self->{settings} = shift;

     set_up_screen();

    return $self;
}

sub set_up_screen {
    my $process = "/usr/bin/screen";
    my $args = "-dmS console /dev/ttyAMA0 57600";
    if (`ps -ef | grep -- "$args" | grep -v grep || echo -n setup` eq "setup") {
        print "need to set up screen\n";
        `$process $args`;
    }
}

sub test_msg {
    my $self = shift;
    my $msg = to_json({
        "msgtype" => "test"
    });

    send_msg($self,$msg);
    my $result = recv_msg($self);

    if ($result->{result}) {
        print("Test message success\n");
    } else {
        print("Test message failed: $result->{detail}\n");
    }
}

sub set_leds {
    my $self      = shift;
    my $ledcolors = shift;
    
    my $msg = to_json({"msgtype" => "ledcolor", "colors" => $ledcolors});
    send_msg($self,$msg);
    my $result = recv_msg($self);

    if ($result->{result}) {
        print("Color leds message success\n");
    } else {
        print("Color leds message failed: $result->{detail}\n");
    }
}

sub render_leds {
    my $self = shift;
    my $msg = to_json({"msgtype" => "renderleds"});

    send_msg($self,$msg);
    my $result = recv_msg($self);

    if ($result->{result}) {
        print("Render led message success\n");
    } else {
        print("Render led message failed: $result->{detail}\n");
    }
}

sub animate_leds {
    my $self = shift;
    my $params = shift;
    #valid names: "smiley_face"
    my $msg = to_json({"msgtype" => "animation", "name" => $params->{name}, "color" => $params->{color}});
    
    send_msg($self,$msg);
    my $result = recv_msg($self);

    if ($result->{result}) {
        print("Animate led message success\n");
    } else {
        print("Animate led message failed: $result->{detail}\n");
    }
}

sub heat {
    my $self = shift;
    set_relays($self, {heat => 1});
}

sub cool {
    my $self = shift;
    set_relays($self, {cool => 1});
}

sub humidify {
    my $self = shift;
    set_relays($self, {humidify => 1});
}

sub heat_humidify {
    my $self = shift;
    set_relays($self, {heat_humidify => 1});
}
sub fan {
    my $self = shift;
    set_relays($self, {fan => 1});
}

sub idle {
    my $self = shift;
    set_relays($self, {idle => 1});
}

sub set_relays {
    my $self   = shift;
    my $relays = shift; # list of relay states in order of: heat, cool, fan, humidify, heat2, cool2
    my $result;

    my $msg = to_json({
        "msgtype"     => "setrelays",
        "relaystates" => $relays
    });
    send_msg($self, $msg);
    $result = recv_msg($self);

    if ($result->{result}) {
        print("Set relays success\n");
    } else {
        print("Set relays failed: $result->{detail}\n");
    }
}

sub clear_relays {
    my $self   = shift;
    
    my $msg = to_json({ "msgtype" => "clearrelays"});
    send_msg($self, $msg);
    my $result = recv_msg($self);

    if ($result->{result}) {
        print("Clear relays success\n");
    } else {
        print("Clear relays failed: $result->{detail}\n");
    }
}
    

sub send_msg {
    my $self = shift;
    my $msg = shift . "\r\n";

    if ($self->{settings}->{debug}) {
        print "DEBUG: send_msg: sending $msg";
    }

    open(PORT, ">$self->{serial_port}");
    eval {
        local $SIG{ALRM} = sub { die "DEBUG: send_msg: Failed in FLOCK of $self->{serial_port}" };
        alarm 30;
        flock(PORT, LOCK_EX);
        print PORT $msg;
        alarm 0;
    };
    print $@ if $@;
    close PORT;

    if ($self->{settings}->{debug}) {
        print "DEBUG: send_msg: sent $msg";
    }
}

sub recv_msg {
    my $self = shift;

    if ($self->{settings}->{debug}) {
        print "DEBUG: recv_msg: receiving message\n";
    }

    my $jsonmsg;
    my $port;
    eval {
        local $SIG{ALRM} = sub { die "DEBUG: recv_msg: Failed in FLOCK of $self->{serial_port}" };
        alarm 30;
	open($port, "<$self->{serial_port}");
        flock($port, LOCK_EX);
        $jsonmsg = <$port>;
        alarm 0;
    };
    print $@ if $@;
    close $port;
    if ($self->{settings}->{debug}) {
        print "DEBUG: recv_msg: got $jsonmsg";
    }
    my $msg;
    eval {
        $msg = from_json($jsonmsg);
    };
    if ($@) {
        warn("CAUGHT JSON DESERIALZATION ERROR: $@");
        return;
    }
    return $msg;
}

sub reset {
    my $self = shift;
    set_gpio($self->{reset_pin}, "out", 0);
    set_gpio($self->{reset_pin}, "out", 1);
}

sub host_type {
    if (-e "/dev/ttyAMA0") {
        return "rpi";
    } else {
        return "bbb";
    }
}


sub set_gpio {
    my $pin       = shift;
    my $direction = shift;
    my $value     = shift;

    my $pindir = "/sys/class/gpio/gpio$pin";

    if (! -e $pindir) {
        open(EXPORT, ">/sys/class/gpio/export");
        print EXPORT $pin;
        close EXPORT;
    }

    if (! -e $pindir) {
        print "Failed to init gpio $pin\n";
        return;
    }

    open(FILE, ">$pindir/direction") or die "$^E\n";
    print FILE $direction;
    close FILE;

    open(FILE, ">$pindir/value");
    print FILE $value;
    close FILE;  
}

1;
__END__
# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

Neutrino::Hub - Perl extension for blah blah blah

=head1 SYNOPSIS

  use Neutrino::Hub;
  blah blah blah

=head1 DESCRIPTION

Stub documentation for Neutrino::Hub, created by h2xs. It looks like the
author of the extension was negligent enough to leave the stub
unedited.

Blah blah blah.

=head2 EXPORT

None by default.



=head1 SEE ALSO

Mention other useful documentation such as the documentation of
related modules or operating system documentation (such as man pages
in UNIX), or any relevant external documentation such as RFCs or
standards.

If you have a mailing list set up for your module, mention it here.

If you have a web site set up for your module, mention it here.

=head1 AUTHOR

root, E<lt>root@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2014 by root

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.14.2 or,
at your option, any later version of Perl 5 you may have available.


=cut
