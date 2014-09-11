#!/usr/bin/env perl
use Neutrino::Hub;

my $h = new Neutrino::Hub({"debug" => 1});
#$h->test_msg();
#$h->reset();
#sleep 2;
#$h->clear_relays();
#sleep 2;
#$h->set_relays({cool=>1});
#sleep 2;
#$h->set_relays({heat=>1});
#sleep 2;
#$h->clear_relays();
$colors = [];
foreach(0..11) {
    $colors[$_] = [200,0,0];
}
$h->set_leds($colors);

