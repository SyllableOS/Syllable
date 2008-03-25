package ExtUtils::MM_Syllable;

=head1 NAME

ExtUtils::MM_Syllable - methods to override UN*X behaviour in ExtUtils::MakeMaker

=head1 SYNOPSIS

 use ExtUtils::MM_Syllable;	# Done internally by ExtUtils::MakeMaker if needed

=head1 DESCRIPTION

See ExtUtils::MM_Unix for a documentation of the methods provided
there. This package overrides the implementation of these methods, not
the semantics.

=over 4

=cut 

use ExtUtils::MakeMaker::Config;
use File::Spec;
require ExtUtils::MM_Any;
require ExtUtils::MM_Unix;

use vars qw(@ISA $VERSION);
@ISA = qw( ExtUtils::MM_Any ExtUtils::MM_Unix );
$VERSION = '1.05';


=item os_flavor

Syllable is Syllable.

=cut

sub os_flavor {
    return('Syllable');
}

=item init_linker

libperl.a equivalent to be linked to dynamic extensions.

=cut

sub init_linker {
    my($self) = shift;

    $self->{PERL_ARCHIVE} ||= 
      File::Spec->catdir('$(PERL_INC)',$Config{libperl});
    $self->{PERL_ARCHIVE_AFTER} ||= '';
    $self->{EXPORT_LIST}  ||= '';
}

sub dynamic_lib {
    my($self, %attribs) = @_;
    return '' unless $self->needs_linking(); #might be because of a subdir

    return '' unless $self->has_link_code;

    my($otherldflags) = $attribs{OTHERLDFLAGS} || "";
    my($inst_dynamic_dep) = $attribs{INST_DYNAMIC_DEP} || "";
    my($armaybe) = $attribs{ARMAYBE} || $self->{ARMAYBE} || ":";
    my($ldfrom) = '$(LDFROM)';
    $armaybe = 'ar' if ($Is_OSF and $armaybe eq ':');
    my(@m);
    my $ld_opt = $Is_OS2 ? '$(OPTIMIZE) ' : '';	# Useful on other systems too?
    my $ld_fix = $Is_OS2 ? '|| ( $(RM_F) $@ && sh -c false )' : '';
    push(@m,'
# This section creates the dynamically loadable $(INST_DYNAMIC)
# from $(OBJECT) and possibly $(MYEXTLIB).
ARMAYBE = '.$armaybe.'
OTHERLDFLAGS = '.$ld_opt.$otherldflags.'
INST_DYNAMIC_DEP = '.$inst_dynamic_dep.'
INST_DYNAMIC_FIX = '.$ld_fix.'

$(INST_DYNAMIC): $(OBJECT) $(MYEXTLIB) $(BOOTSTRAP) $(INST_ARCHAUTODIR)$(DFSEP).exists $(EXPORT_LIST) $(PERL_ARCHIVE) $(PERL_ARCHIVE_AFTER) $(INST_DYNAMIC_DEP)
');
    if ($armaybe ne ':'){
	$ldfrom = 'tmp$(LIB_EXT)';
	push(@m,'	$(ARMAYBE) cr '.$ldfrom.' $(OBJECT)'."\n");
	push(@m,'	$(RANLIB) '."$ldfrom\n");
    }
    $ldfrom = "-all $ldfrom -none" if $Is_OSF;

    push(@m,'	$(RM_F) $@
');

    my $libs = '$(LDLOADLIBS)';

    push @m, sprintf <<'MAKE', $ld_run_path_shell, $ldrun, $ldfrom, $libs;
	%s$(LD) %s $(LDDLFLAGS) %s $(OTHERLDFLAGS) -o $@ $(MYEXTLIB)	\
	  -L$(PERL_INC) -lperl %s $(PERL_ARCHIVE_AFTER) $(EXPORT_LIST)	\
	  $(INST_DYNAMIC_FIX)
MAKE

    push @m, <<'MAKE';
	$(CHMOD) $(PERM_RWX) $@
MAKE

    return join('',@m);
}

=back

1;
__END__

