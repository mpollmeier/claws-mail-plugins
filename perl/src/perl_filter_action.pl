BEGIN {$INC{'SylpheedClaws/Filter/Action.pm'} = 1;}
package SylpheedClaws::Filter::Action;
use base qw(Exporter);
our @EXPORT = (qw(mark unmark dele mark_as_unread mark_as_read),
	       qw(lock unlock move copy color execute),
	       qw(hide set_score change_score stop exit),
	       qw(forward forward_as_attachment redirect),
	       );
our %colors = ('none'     =>  0,'orange' =>  1,
    	       'red'      =>  2,'pink'   =>  3,
    	       'sky blue' =>  4,'blue'   =>  5,
    	       'green'    =>  6,'brown'  =>  7);
sub mark           { SylpheedClaws::C::set_flag  (1);}
sub unmark         { SylpheedClaws::C::unset_flag(1);}
sub mark_as_unread { SylpheedClaws::C::set_flag  (2);}
sub mark_as_read   { SylpheedClaws::C::unset_flag(2);}
sub lock           { SylpheedClaws::C::set_flag  (7);}
sub unlock         { SylpheedClaws::C::unset_flag(7);}
sub copy           { SylpheedClaws::C::copy     (@_);}
sub forward        { SylpheedClaws::C::forward(1,@_);}
sub forward_as_attachment {SylpheedClaws::C::forward(2,@_);}
sub redirect       { SylpheedClaws::C::redirect(@_); }
sub hide           { SylpheedClaws::C::hide();       }
sub exit           { stop();                         }
sub stop           { die 'intended';                 }
sub set_score {
    $SylpheedClaws::Filter::Matcher::msginfo{"score"} =
	SylpheedClaws::C::set_score(@_);
}
sub change_score   {
    $SylpheedClaws::Filter::Matcher::msginfo{"score"} =
	SylpheedClaws::C::change_score(@_);
}
sub execute        { SylpheedClaws::Filter::Matcher::test(@_);1;}
sub move           { SylpheedClaws::C::move(@_);stop();}
sub dele           { SylpheedClaws::C::delete();stop();}
sub color {
    ($color) = @_;$color = lc $color;
    $color = $colors{$color} if exists $colors{$color};
    $color = 0 if $color =~ m/\D/;
    SylpheedClaws::C::color($color);
}
1;
