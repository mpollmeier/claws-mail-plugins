package SylpheedClaws::Persistent;

use strict;
our %Cache;
use Symbol qw(delete_package);

sub valid_package_name {
    my($string) = @_;
    $string =~ s/([^A-Za-z0-9\/])/sprintf("_%2x",unpack("C",$1))/eg;
    # second pass only for words starting with a digit
    $string =~ s|/(\d)|sprintf("/_%2x",unpack("C",$1))|eg;
    
    # Dress it up as a real package name
    $string =~ s|/|::|g;
    return "SylpheedClaws" . $string;
}

sub eval_file {
    my($file, $delete) = @_;
    my $package = valid_package_name($file);
    my $mtime = -M $file;
    if(!(defined $Cache{$package}{mtime} &&
	 $Cache{$package}{mtime} <= $mtime)) {
    	delete_package($package) if defined $Cache{$package}{mtime};
	local *FH;
	open FH, $file or die "Failed to open '$file': $!";
	local($/) = undef;
	my $sub = <FH>;
	close FH;
	#wrap the code into a subroutine inside our unique package
	my $eval = qq{package $package;
		      use SylpheedClaws::Filter::Matcher;
		      use SylpheedClaws::Filter::Action;
		      use SylpheedClaws::Utils;
		      sub handler { $sub; }};
	{
	    # hide our variables within this block
	    my($file,$mtime,$package,$sub);
	    eval $eval;
	}
	die $@ if $@;
	#cache it unless we're cleaning out each time
	$Cache{$package}{mtime} = $mtime unless $delete;
    }
    eval {$package->handler;};
    die $@ if $@;
    delete_package($package) if $delete;
}
