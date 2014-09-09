#!/usr/bin/env perl
use Neutrino::Hub;

my $h = new Neutrino::Hub({"debug" => 1});
$h->test_msg();
