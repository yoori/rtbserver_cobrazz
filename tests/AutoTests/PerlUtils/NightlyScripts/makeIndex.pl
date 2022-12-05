
=head1 NAME

makeIndex.pl - make HTML-index for test results

=head1 SYNOPSIS

  makeIndex.pl OPTIONS

=head1 OPTIONS

=over

=item C<--path, -p test results path>

B<Required.>

=back

=cut

use Getopt::Long qw(:config gnu_getopt bundling);
use Pod::Usage;
use TestHTML;

my %options = ();

if ( !GetOptions(\%options, qw(path|p=s)) 
    || (not defined $options{path}))
{
  pod2usage(1);
}

my $index = HTMLIndex->new($options{path});
$index->flush(join ("/", ($options{path}, "index.html")));


