#!/bin/perl
use utf8;
use open ':std', ':encoding(UTF-8)';

%onko;
open sisään, "<$ARGV[0]";
foreach my $rivi (<sisään>) {
    $rivi = lc($rivi);
    while ($rivi =~ m/[a-z.,öä]{2,4}/g) {
	if (!$onko{$&}++) {
	    print "$&\0";
	}
    }
}
close sisään;
