BEGIN {$INC{'SylpheedClaws/Utils.pm'} = 1;}
package SylpheedClaws::Utils;
use base qw(Exporter);
our @EXPORT = (
    	       qw(SA_is_spam extract_addresses move_to_trash abort),
    	       qw(addr_in_addressbook from_in_addressbook),
    	       qw(get_attribute_value),
    	       );
# Spam
sub SA_is_spam {
    return not SylpheedClaws::Filter::Matcher::test('spamc -c < %F > /dev/null');
}
# simple extract email addresses from a header field
sub extract_addresses {
    my $hf = shift; return undef unless defined($hf);
    my @addr = ();
    while($hf =~ m/[-.+\w]+\@[-.+\w]+/) {
	$hf =~ s/^.*?([-.+\w]+\@[-.+\w]+)//;
	push @addr,$1;
    }
    push @addr,"" unless @addr;
    return @addr;
}
# move to trash
sub move_to_trash {SylpheedClaws::C::move_to_trash();SylpheedClaws::Filter::Action::stop();}
# abort: stop() and do not continue with built-in filtering
sub abort {SylpheedClaws::C::abort();SylpheedClaws::Filter::Action::stop();}
# addressbook query
sub addr_in_addressbook {
    return SylpheedClaws::C::addr_in_addressbook(@_) if @_;
    return 0;
}
sub from_in_addressbook {
    my ($from) = extract_addresses(SylpheedClaws::Filter::Matcher::header("from"));
    return 0 unless $from;
    return addr_in_addressbook($from,@_);
}
sub get_attribute_value {
    my $email = shift; my $key = shift;
    return "" unless ($email and $key);
    return SylpheedClaws::C::get_attribute_value($email,$key,@_);
}
1;
