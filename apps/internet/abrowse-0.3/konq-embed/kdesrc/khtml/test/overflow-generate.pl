#!/usr/bin/perl -w
# dnaber@mini.gt.owl.de, 2000-09-18

if( ! ($ARGV[0] && $ARGV[1]) ) {
	print STDERR "Usage: overflow-make.pl <length_of_strings> filename> >resultfile";
	print STDERR "Replaces 'XXX' in input file with long ASCII strings to trigger buffer overflows.";
	exit;
}

print STDERR "Loading file...\n";
open(INPUT, $ARGV[1]) || die "Cannot open '$ARGV[1]': $!";
undef $/;
$str = (<INPUT>);
close(INPUT);

$overflow_chars = $ARGV[0];
print STDERR "Making random data...\n";
my $overflow;
for( $i = 0; $i <= $overflow_chars; $i++ ) {
	$overflow .= rand_char();
}
print STDERR "Substituting 'XXX'...\n";
$str =~ s/XXX/$overflow/igs;

print $str;
exit;

sub rand_char() {
	$ch = chr(65+rand(26));
	if( rand(2) < 1 ) {
		$ch = "\L$ch";
	}
	return $ch;
}
