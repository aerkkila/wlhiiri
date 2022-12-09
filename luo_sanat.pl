#!/bin/perl
use utf8;
use open ':std', ':encoding(UTF-8)';

@lista = [];
$pit = 0;
%onko;
open sisään, "<$ARGV[0]";
foreach my $rivi (<sisään>) {
    $rivi = lc($rivi);
    while ($rivi =~ m/[a-z.,öä]{2,4}/g) {
	if (!$onko{$&}++) {
	    @lista[$pit++] = $&;
	}
    }
}
close sisään;

print "@lista\n";
