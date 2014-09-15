#!/usr/bin/env perl
use Neutrino::Hub;
use Data::Dumper;

my $h = new Neutrino::Hub({"debug" => 1});
#$h->test_msg();
#$h->reset();
#sleep 2;
#$h->cool();
#$h->cool();
#$h->humidify();
#sleep 3;
#$h->idle();
#$h->clear_relays();
#sleep 2;
#$h->set_relays({cool=>1});
#sleep 2;
#$h->set_relays({heat=>1});
#sleep 2;
#$h->clear_relays();
#my $colors = [[],[],[],[],[],[],[],[],[],[],[],[]];
$h->animate_leds({name => "dual_sweep_up", color => [0,60,30]});
$h->animate_leds({name => "dual_sweep_up", color => [60,20,0]});
#foreach my $color(0..255) {
#    $color = 255 - $color;
#    next unless $color % 2 == 0;
#    foreach my $led(0..11) {
#        $colors->[$led] = [$color,0,0];
#    }
#    $h->set_leds($colors);
#}
