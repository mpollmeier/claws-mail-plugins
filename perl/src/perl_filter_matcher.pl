BEGIN {$INC{'SylpheedClaws/Filter/Matcher.pm'} = 1;}
package SylpheedClaws::Filter::Matcher;
use locale;
use base qw(Exporter);
use strict;
our @EXPORT =   (qw(filepath header body),
		 qw(all marked unread deleted new replied),
		 qw(forwarded locked colorlabel match matchcase),
		 qw(regexp regexpcase test),
		 qw(to cc subject from to_or_cc newsgroups inreplyto),
		 qw(references body_part headers_part message),
		 qw(size_greater size_smaller size_equal),
		 qw(score_greater score_lower score_equal),
		 qw(age_greater age_lower partial $permanent));
# Global Variables
our(%header,$body,%msginfo,$mail_done);
our %colors = ('none'     =>  0,'orange'   =>  1,'red'  =>  2,
   	       'pink'     =>  3,'sky blue' =>  4,'blue' =>  5,
    	       'green'    =>  6,'brown'    =>  7);
# For convenience
sub to           { return "to";            }
sub cc           { return "cc";            }
sub from         { return "from";          }
sub subject      { return "subject";       }
sub to_or_cc     { return "to_or_cc";      }
sub newsgroups   { return "newsgroups";    }
sub inreplyto    { return "in-reply-to";   }
sub references   { return "references";    }
sub body_part    { return "body_part";     }
sub headers_part { return "headers_part";  }
sub message      { return "message";       }
# access the mail directly
sub header {
    my $key = shift;
    if(not defined $key) {
	init_();
	return keys %header;
    }
    $key = lc $key; $key =~ s/:$//;
    init_()              unless exists $header{$key};
    if(exists $header{$key}) {
	wantarray ? return @{$header{$key}} : return $header{$key}->[-1];
    }
    return undef;
}
sub body {init_();return $body;}
sub filepath {return $msginfo{"filepath"};}
# Public Matcher Tests
sub all           { return 1; }
sub marked        { return SylpheedClaws::C::check_flag(1);}
sub unread        { return SylpheedClaws::C::check_flag(2);}
sub deleted       { return SylpheedClaws::C::check_flag(3);}
sub new           { return SylpheedClaws::C::check_flag(4);}
sub replied       { return SylpheedClaws::C::check_flag(5);}
sub forwarded     { return SylpheedClaws::C::check_flag(6);}
sub locked        { return SylpheedClaws::C::check_flag(7);}
sub ignore_thread { return SylpheedClaws::C::check_flag(8);}
sub age_greater  {return SylpheedClaws::C::age_greater(@_);}
sub age_lower    {return SylpheedClaws::C::age_lower(@_);  }
sub score_equal {
    my $my_score = shift;
    return 0 unless (defined($msginfo{"score"}) and defined($my_score));
    $my_score == $msginfo{"score"} ? return 1 : return 0;
}
sub score_greater {
    my $my_score = shift;
    return 0 unless (defined($msginfo{"score"}) and defined($my_score));
    $msginfo{"score"} > $my_score ? return 1 : return 0;
}
sub score_lower {
    my $my_score = shift;
    return 0 unless (defined($msginfo{"score"}) and defined($my_score));
    $msginfo{"score"} < $my_score ? return 1 : return 0;
}
sub colorlabel {
    my $color = shift;
    $color = lc $color;
    $color = $colors{$color} if exists $colors{$color};
    $color = 0 if $color =~ m/\D/;
    return SylpheedClaws::C::colorlabel($color);
}
sub size_greater {
    my $my_size = shift;
    return 0 unless (defined($msginfo{"size"}) and defined($my_size));
    $msginfo{"size"} > $my_size ? return 1 : return 0;
}
sub size_smaller {
    my $my_size = shift;
    return 0 unless (defined($msginfo{"size"}) and defined($my_size));
    $msginfo{"size"} < $my_size ? return 1 : return 0;
}
sub size_equal {
    my $my_size = shift;
    return 0 unless (defined($msginfo{"size"}) and defined($my_size));
    $msginfo{"size"} == $my_size ? return 1 : return 0;
}
sub partial {
    return 0 unless defined($msginfo{"total_size"})
	and defined($msginfo{"size"});
    return ($msginfo{"total_size"} != 0
	    && $msginfo{"size"} != $msginfo{"total_size"});
}
sub test {
   $_ = shift; my $command = ""; my $hl=""; my $re="";
   s/\"/"/g; #fool stupid emacs perl mode";
   s/([^%]*)//; $command .= $1;
    while($_) {
	if   (/^%%/){s/^%%([^%]*)//;$command .= "\\%".$1; next;}
    	elsif(/^%s/){s/^%s([^%]*)//;$hl=header("subject");$re=$1;}
    	elsif(/^%f/){s/^%f([^%]*)//;$hl=header("from");$re=$1;}
    	elsif(/^%t/){s/^%t([^%]*)//;$hl=header("to");$re=$1;}
    	elsif(/^%c/){s/^%c([^%]*)//;$hl=header("cc");$re=$1;}
    	elsif(/^%d/){s/^%d([^%]*)//;$hl=header("date");$re=$1;}
    	elsif(/^%i/){s/^%i([^%]*)//;$hl=header("message-id");$re=$1;}
    	elsif(/^%n/){s/^%n([^%]*)//;$hl=header("newsgroups");$re=$1;}
    	elsif(/^%r/){s/^%r([^%]*)//;$hl=header("references");$re=$1;}
	elsif(/^%F/){s/^%F([^%]*)//;$hl=filepath();$re=$1;}
	else        {s/^(%[^%]*)//; $command .= $1;}
	$command .= "\Q$hl\E" if defined $hl;$hl="";
	$command .= $re;$re="";
    }
   return !(system($command)>>8);
}
sub matchcase    { return match_(@_,"i");  }
sub match        { return match_(@_);      }
sub regexpcase   { return match_(@_,"ri"); }
sub regexp       { return match_(@_,"r");  }
# Internals
sub add_header_entries_ {
    my($key,@values) = @_; $key = lc $key; $key =~ s/:$//;
    $header{$key} = [] unless exists $header{$key};
    push @{$header{$key}},@values;
}
sub init_ {
    return 0 if $mail_done;
    SylpheedClaws::C::open_mail_file();
    read_headers_();
    read_body_();
    SylpheedClaws::C::close_mail_file();
    $mail_done = 1;
}
sub filter_init_ {
    %header = (); %msginfo = (); undef $body; $mail_done = 0;
    $msginfo{"size"}               = SylpheedClaws::C::filter_init( 1);
    add_header_entries_("date",      SylpheedClaws::C::filter_init( 2));
    add_header_entries_("from",      SylpheedClaws::C::filter_init( 3));
    add_header_entries_("to",        SylpheedClaws::C::filter_init( 4));
    add_header_entries_("cc",        SylpheedClaws::C::filter_init( 5));
    add_header_entries_("newsgroups",SylpheedClaws::C::filter_init( 6));
    add_header_entries_("subject",   SylpheedClaws::C::filter_init( 7));
    add_header_entries_("msgid",     SylpheedClaws::C::filter_init( 8));
    add_header_entries_("inreplyto", SylpheedClaws::C::filter_init( 9));
    add_header_entries_("xref",      SylpheedClaws::C::filter_init(10));
    add_header_entries_("xface",     SylpheedClaws::C::filter_init(11));
    add_header_entries_("dispositionnotificationto",
			             SylpheedClaws::C::filter_init(12));
    add_header_entries_("returnreceiptto",
			             SylpheedClaws::C::filter_init(11));
    add_header_entries_("references",SylpheedClaws::C::filter_init(11));
    $msginfo{"score"}              = SylpheedClaws::C::filter_init(15);
    $msginfo{"threadscore"}        = SylpheedClaws::C::filter_init(16);
    $msginfo{"plaintext_file"}     = SylpheedClaws::C::filter_init(17);
    $msginfo{"hidden"}             = SylpheedClaws::C::filter_init(19);
    $msginfo{"filepath"}           = SylpheedClaws::C::filter_init(20);
    $msginfo{"partial_recv"}       = SylpheedClaws::C::filter_init(21);
    $msginfo{"total_size"}         = SylpheedClaws::C::filter_init(22);
    $msginfo{"account_server"}     = SylpheedClaws::C::filter_init(23);
    $msginfo{"account_login"}      = SylpheedClaws::C::filter_init(24);
    $msginfo{"planned_download"}   = SylpheedClaws::C::filter_init(25);
} 
sub read_headers_ {
    my($key,$value);
    %header = ();
    while(($key,$value) = SylpheedClaws::C::get_next_header()) {
	next unless $key =~ /:$/;
	add_header_entries_($key,$value);
    }
}
sub read_body_ {
    my $line;
    while(defined($line = SylpheedClaws::C::get_next_body_line())) {
	$body .= $line;
    }    
}
sub match_ {
    my ($where,$what,$modi) = @_; $modi ||= "";
    my $nocase=""; $nocase = "1" if (index($modi,"i") != -1);
    my $regexp=""; $regexp = "1" if (index($modi,"r") != -1);
    if($where eq "to_or_cc") {
	if(not $regexp) { 
    	    return ((index(header("to"),$what) != -1) or
    		    (index(header("cc"),$what) != -1)) unless $nocase;
    	    return ((index(lc header("to"),lc $what) != -1) or
    		    (index(lc header("cc"),lc $what) != -1))
    	    } else {
    		return ((header("to") =~ m/$what/) or
    			(header("cc") =~ m/$what/)) unless $nocase;
    		return ((header("to") =~ m/$what/i) or
    			(header("cc") =~ m/$what/i));
    	    }
    } elsif($where eq "body_part") {
	my $mybody = body(); $mybody =~ s/\s+/ /g;
    	if(not $regexp) {
    	    return (index($mybody,$what) != -1) unless $nocase;
    	    return (index(lc $mybody,lc $what) != -1);
    	} else {
    	    return ($body =~ m/$what/) unless $nocase;
    	    return ($body =~ m/$what/i);
    	}
    } elsif($where eq "headers_part") {
	my $myheader = header_as_string_();
    	if(not $regexp) {
    	    $myheader =~ s/\s+/ /g;
    	    return (index($myheader,$what) != -1) unless $nocase;
    	    return (index(lc $myheader,lc $what) != -1);
    	} else {
    	    return ($myheader =~ m/$what/) unless $nocase;
    	    return ($myheader =~ m/$what/i);
   	}
    } elsif($where eq "message") {
	my $message = header_as_string_();
	$message .= "\n".body();
    	if(not $regexp) {
    	    $message =~ s/\s+/ /g;
    	    return (index($message,$what) != -1) unless $nocase;
    	    return (index(lc $message,lc $what) != -1);
    	} else {
    	    return ($message =~ m/$what/) unless $nocase;
    	    return ($message =~ m/$what/i);
    	}
    } else {
	$where = lc $where;
	my $myheader = header(lc $where); $myheader ||= "";
	return 0 unless $myheader;
    	if(not $regexp) {		
    	    return (index(header($where),$what) != -1) unless $nocase;
    	    return (index(lc header($where),lc $what) != -1);
    	} else {
    	    return (header($where) =~ m/$what/) unless $nocase;
    	    return (header($where) =~ m/$what/i);
	} 
    }
}
sub header_as_string_ {
    my $headerstring="";
    my @headerkeys = header(); my(@fields,$field);
    foreach $field (@headerkeys) {
	@fields = header($field);
	foreach (@fields) {
	    $headerstring .= $field.": ".$_."\n";
	}
    }
    return $headerstring;
}
our $permanent = "";
1;
