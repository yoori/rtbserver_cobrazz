#! /usr/bin/perl
#
use warnings;
use strict;

=head1 NAME

text_advertising.pl - emulate server text advertising algorithm, see
                     https://confluence.ocslab.com/display/ADS/Text+Advertising

=head1 SYNOPSIS

  text_advertising.pl OPTIONS

=head2 OPTIONS

=over

=item C<--db, -d db>

=item C<--user, -u user>

=item C<--password, -p password>

B<Required.>

=item C<--tid tid>

=item C<--tag tag>

B<Required.>  Tag ID or name.  You should give either, but not both.

=item C<--keyword keyword[,keyword]...>

B<Required.>  This option may be given multiple times.

=back

=head2 BUGS

Frequency caps and other filters are not taken into account, and
likely never will.

All computations are performed with precision of 5 decimal places,
however resulting floating point values may not be equal to those
returned by the server, due to (server?) accumulated rounding errors.

When selecting display ccg TriggerList.trigger_list should be a single
keyword (no spaces, quotes, newlines), and Channel.expression should
be a single number.

=cut


use bignum qw(precision -5);


use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

my %option;
if (! GetOptions(\%option,
                 qw(db|d=s user|u=s password|p=s tid=i tag=s keyword=s@))
    || @ARGV || (grep { not defined }
                 @option{qw(db user password keyword)})
    || (exists $option{tid} == exists $option{tag})) {
    pod2usage(1);
}
@{$option{keyword}} = split(',', join(',', @{$option{keyword}}));

use DBI;

my $dbh = DBI->connect("DBI:Oracle:$option{db}",
                       $option{user}, $option{password},
                       { AutoCommit => 1, PrintError => 0, RaiseError => 1 });
$dbh->{LongReadLen} = 10 * 1024 * 1024;

my $trigger_ccg = $dbh->prepare(q{
    SELECT tl.trigger_list, ccg.ccg_id, ccg.name,
           (CASE WHEN ccg.realized_imp > 0
            THEN r.cpm + ((ccg.realized_clicks * NVL(r.cpc, 0)
                           + ccg.realized_actions * NVL(r.cpa, 0))
                          * 1000 / ccg.realized_imp)
            ELSE r.cpm END) display_ecpm
    FROM CampaignCreativeGroup ccg
    JOIN CCGSite s ON ccg.ccg_id = s.ccg_id
    JOIN Tags t ON s.site_id = t.site_id
    JOIN CCGRate r ON ccg.ccg_id = r.ccg_id,
    Channel c
    JOIN TriggerList tl ON (c.keyword_list_id = tl.trigger_list_id
                            OR c.url_list_id = tl.trigger_list_id)
    WHERE t.tag_id = :1 AND ccg.channel_id IS NOT NULL
    AND tl.trigger_list IS NOT NULL
    AND c.channel_id = (SELECT channel_id FROM Channel
                        WHERE keyword_list_id IS NOT NULL
                        OR url_list_id IS NOT NULL
                        CONNECT BY PRIOR expression = TO_CHAR(channel_id)
                        START WITH channel_id = ccg.channel_id)
});


my $matching_pair = $dbh->prepare(q{
    SELECT k.keyword_id keyword_id, cc.ccg_keyword_id ccg_keyword_id,
           cc.ccg_id ccg_id, c.account_id account_id,
           ccg.realized_imp ccg_imps, ccg.realized_clicks ccg_clicks,
           cc.max_cpc_bid max_cpc_bid, cc.imps kw_imps, cc.clicks kw_clicks,
           ccg.delivery_threshold delivery_threshold, ccg.name ccg_name,
           cur.fraction_digits
    FROM Keyword k JOIN CCGKeyword cc ON k.keyword_id = cc.keyword_id
    JOIN CCGSite ccgs ON cc.ccg_id = ccgs.ccg_id
    JOIN Tags t ON ccgs.site_id = t.site_id
    JOIN CampaignCreativeGroup ccg ON cc.ccg_id = ccg.ccg_id
    JOIN Campaign c ON ccg.campaign_id = c.campaign_id
    JOIN Account a ON a.account_id = c.account_id
    JOIN Currency cur ON cur.currency_id = a.currency_id
    WHERE t.tag_id = :1 AND k.keyword = :2
});

my $exchange_rate = $dbh->prepare(q{
    SELECT r.rate FROM CurrencyExchangeRate r
    JOIN Account a ON r.currency_id = a.currency_id
    WHERE r.currency_exchange_id = (SELECT MAX(currency_exchange_id)
                                    FROM CurrencyExchangeRate)
    AND a.account_id = :1
});

my $derived_data = $dbh->prepare(q{
    SELECT cs.max_text_creatives max_text_creatives, sr.cpm tag_cpm,
           s.account_id account_id, t.tag_id tid
    FROM Tags t JOIN TagPricing tp ON t.tag_id = tp.tag_id
    JOIN SiteRate sr ON tp.site_rate_id = sr.site_rate_id
    JOIN CreativeSize cs ON t.size_id = cs.size_id
    JOIN Site s ON t.site_id = s.site_id
    WHERE t.tag_id = :1 OR t.name = :2
});

my $margin = $dbh->prepare(q{
    SELECT param_value FROM ADSConfig
    WHERE param_name IN ('OIX_MIN_FIXED_MARGIN', 'OIX_MIN_MARGIN')
    ORDER BY param_name
});

my $keyword_ctr = $dbh->prepare(q{
    SELECT SUM(clicks) / SUM(imps)
    FROM CCGKeyword WHERE keyword_id = :1 HAVING SUM(imps) > 2000
});

my $global_ctr = $dbh->prepare(q{
    SELECT *
    FROM (SELECT SUM(clicks) / SUM(imps) ctr, keyword_id
          FROM CCGKeyword
          GROUP BY keyword_id HAVING SUM(imps) > 2000
          ORDER BY ctr DESC)
    WHERE rownum <= 1
});

sub fetch_ctr {
    my ($stmt, @args) = @_;

    $stmt->execute(@args);
    my $row = $stmt->fetchrow_arrayref;
    
    return ($row && defined $row->[0] ? @$row : 0);
}

my %keyword_ctr_cache;
my $global_ctr_cache;

sub compute_ctr {
    my ($row) = @_;

    print "\nComputing ctr:\n";

    print "does this (keyword, ccg) have imps > 2000?  ";
    if ($row->{kw_imps} > 2000) {
        print "yes\n";
        return $row->{kw_clicks} / $row->{kw_imps} + 0.0;
    }
    print "no\n";

    print "does this ccg have imps > 2000?  ";
    if ($row->{ccg_imps} > 2000) {
        print "yes\n";
        return $row->{ccg_clicks} / $row->{ccg_imps} + 0.0;
    }
    print "no\n";

    print "does this keyword have sum(imps) > 2000?  ";
    my $keyword_id = $row->{keyword_id};
    my $keyword_ctr_cache = \$keyword_ctr_cache{$keyword_id};
    unless (defined $$keyword_ctr_cache) {
        ($$keyword_ctr_cache) = fetch_ctr($keyword_ctr, $keyword_id);
    }
    if ($$keyword_ctr_cache) {
        print "yes\n";
        return $$keyword_ctr_cache + 0.0;
    }
    print "no\n";

    print "does any keyword have sum(imps) > 2000?  ";
    unless (defined $global_ctr_cache) {
        $global_ctr_cache = [fetch_ctr($global_ctr)];
    }
    if ($global_ctr_cache->[0]) {
        print "yes, using max crt for keyword_id = $global_ctr_cache->[1]\n";
        return $global_ctr_cache->[0] + 0.0;
    }
    print "no\n";

    print "using default ctr.\n";
    return 0.01;
}

my %exchange_rate_cache;

sub exchange_rate {
    my ($account_id) = @_;

    my $rate = \$exchange_rate_cache{$account_id};
    unless (defined $$rate) {
        $exchange_rate->execute($account_id);
        my $row = $exchange_rate->fetchrow_arrayref;
        $$rate = $row->[0];
    }

    return $$rate;
}


$derived_data->execute($option{tid}, $option{tag});
my ($max_text_creatives, $tag_cpm, $account_id, $tid) =
    $derived_data->fetchrow_array;
$option{tid} = $tid;

die "Tag pricing is not defined, exiting\n"
    unless defined $tag_cpm;

$tag_cpm += 0.0;
my $rate = exchange_rate($account_id);
my $tag_cpm_system = $tag_cpm / $rate;
print "Converting tag_cpm to system currency:"
    . " $tag_cpm / $rate = $tag_cpm_system\n";
$tag_cpm = $tag_cpm_system;
print "\nmax_text_creatives: $max_text_creatives\n";
print "tag_cpm: $tag_cpm\n";


my ($foros_min_fixed_margin, $oix_min_margin) =
    @{ $dbh->selectcol_arrayref($margin) };
print "foros_min_margin: $oix_min_margin (%)\n";
print "foros_min_fixed_margin: $oix_min_fixed_margin\n";


my $min_cpm = max($tag_cpm + $foros_min_fixed_margin,
                  $tag_cpm / (1 - $foros_min_margin / 100));
print "min_cpm: $min_cpm\n";


my %display = (
    ccg_id => 'none',
    name => '-',
    ecpm => 0,
);
my %trigger_ccg;
$trigger_ccg->execute($option{tid});
while (my $row = $trigger_ccg->fetchrow_arrayref) {
    my $t = \$trigger_ccg{$row->[0]};
    $$t = [@$row[1..3]] if ! $$t || $$t->[2] < $row->[3];
}
foreach my $kw (@{$option{keyword}}) {
    my $t = \$trigger_ccg{$kw};

    next unless defined $$t;

    if ($display{ecpm} < $$t->[2]) {
        @display{qw(ccg_id name ecpm)} = @$$t;
    }
}
print "display_ecpm: $display{ecpm}"
    . " (ccg_id = $display{ccg_id}, name = $display{name})\n";


sub print_pair {
    my ($pair) = @_;

    print "\n", "-" x 10, " ($pair->{keyword_id}, $pair->{ccg_id}) ", "-" x 10,
          "\n";
    foreach my $k (sort keys %$pair) {
        print "$k: ", $pair->{$k}, "\n";
    }
}


print "\nGetting (keyword, ccg) pairs:\n";
my @matching_pair;
foreach my $keyword (@{$option{keyword}}) {
    print "-" x 30, "\n";
    print "\nkeyword: $keyword\n";

    $matching_pair->execute($option{tid}, $keyword);
    while (my $row = $matching_pair->fetchrow_hashref('NAME_lc')) {
        my $max_cpc_bid = $row->{max_cpc_bid} + 0.0;
        $row->{max_cpc_bid_account_currency} = $max_cpc_bid;
        my $rate = exchange_rate($row->{account_id});
        $row->{account_exchange_rate} = $rate;
        my $max_cpc_bid_system = $max_cpc_bid / ($rate + 0.0);
        delete $row->{max_cpc_bid};

        $row->{bidding_step_account_currency} = 0.1 ** $row->{fraction_digits};

        print_pair($row);

        print "\nConverting max_cpc_bid to system currency:"
            . " $max_cpc_bid / $rate = $max_cpc_bid_system\n";
        $row->{max_cpc_bid} = $max_cpc_bid_system;
        print "max_cpc_bid: $max_cpc_bid_system\n";

        my $ctr = compute_ctr($row);
        $row->{ctr} = $ctr;
        $row->{max_ecpm_bid} = $row->{max_cpc_bid} * 1000 * $ctr;
        $row->{max_ecpm_bid} = int($row->{max_ecpm_bid} * 100) / 100;

        print "\nctr: $ctr\n";
        print "max_ecpm_bid: $row->{max_ecpm_bid}\n";

        push @matching_pair, $row;
    }
}
print "-" x 30, "\n";
print "\nTotal ", scalar(@matching_pair), " pairs selected\n";

sub max {
    my ($max, @vals) = @_;

    foreach my $v (@vals) {
        $max = $v if $max < $v;
    }

    return $max;
}

sub min {
    my ($min, @vals) = @_;

    foreach my $v (@vals) {
        $min = $v if $min > $v;
    }

    return $min;
}


sub min_ecpm {
    my ($N) = @_;

    return max($min_cpm, $display{ecpm}) / $N;
}

print "\nOrdering pairs by decreasing max_ecpm_bid\n";
@matching_pair =
    sort { $b->{max_ecpm_bid} <=> $a->{max_ecpm_bid} } @matching_pair;

print "\nRemoving pairs for the same account:\n";
my %account_seen;
@matching_pair =
    grep { (not $account_seen{$_->{account_id}}
            and $account_seen{$_->{account_id}} =
                  "($_->{keyword_id}, $_->{ccg_id})")
           or print("($_->{keyword_id}, $_->{ccg_id}) removed"
                    . " (account_id = $_->{account_id},"
                    . " same as for $account_seen{$_->{account_id}})\n"), 0 }
         @matching_pair;
print "done\n";

print "\n", scalar(@matching_pair), " pairs left:\n";
foreach my $pair (@matching_pair) {
    my $t = $pair->{max_ecpm_bid} * $pair->{delivery_threshold};

    print "($pair->{keyword_id}, $pair->{ccg_id}):"
        . " max_ecpm_bid = $pair->{max_ecpm_bid}, X delivery_threshold = $t\n";
}

print "\nSelecting (keyword, ccg) pairs:\n";
my $count = @matching_pair;
my $N = min($max_text_creatives, $count);
while ($N > 0) {
    print "-" x 10, " Trying to fit top $N pairs ", "-" x 10, "\n";
    print "min_ecpm($N): ", min_ecpm($N), "\n";
    @matching_pair =
        grep({ my $t = $_->{max_ecpm_bid} * $_->{delivery_threshold};
               $t >= min_ecpm($N)
               or print("($_->{keyword_id}, $_->{ccg_id}) filtered out"
                        . " (max_ecpm_bid*delivery_threshold = $t)\n"), 0 }
             @matching_pair);

    $count = min($count, $N, scalar(@matching_pair));
    print "$count pairs left\n";

    last if $count == $N;

    $N = min(--$N, $count);
}

unless ($N > 0) {
    print "\nHere we should have served display ccg"
      . " $display{ccg_id} ($display{name}), exiting\n";
    exit(0);
}

for (my $i = 0; $i < $count; ++$i) {
    my $pair = $matching_pair[$i];
    my $next_max_ecpm_bid = ($i < $#matching_pair
                             ? $matching_pair[$i+1]->{max_ecpm_bid}
                             : 0);
    if ($next_max_ecpm_bid != $pair->{max_ecpm_bid}) {
        if (not $pair->{in_ecpm_group}) {
            $pair->{actual_ecpm_bid} = max(min_ecpm($N), $next_max_ecpm_bid);
        } else {
            $pair->{actual_ecpm_bid} = $pair->{max_ecpm_bid};
        }
    } else {
        $pair->{actual_ecpm_bid} = $pair->{max_ecpm_bid};
        $matching_pair[$i+1]->{in_ecpm_group} =
          "($pair->{keyword_id}, $pair->{ccg_id})";
    }

    $pair->{actual_cpc} =
        $pair->{actual_ecpm_bid} / ($pair->{ctr} * 1000);

    my $rate = exchange_rate($pair->{account_id});
    $pair->{actual_cpc_account_currency} = ($pair->{actual_cpc} * $rate);

    use POSIX ();
    use constant EPSILON => 0.1 ** 10;
    $pair->{actual_cpc_account_currency_ceiled} =
        (POSIX::ceil($pair->{actual_cpc_account_currency}
                     / $pair->{bidding_step_account_currency} - EPSILON)
         * $pair->{bidding_step_account_currency});

    $pair->{actual_cpc_account_currency_result} =
        min($pair->{actual_cpc_account_currency_ceiled},
            $pair->{max_cpc_bid_account_currency});
}

print <<"EOF;";


---------- Result ----------

max_text_creatives: $max_text_creatives
selected N: $N
tag_exchange_rate: $rate
tag_cpm: $tag_cpm
min_cpm: $min_cpm
display_ecpm: $display{ecpm}
min_ecpm($N): @{[ min_ecpm($N) ]}
EOF;

for (my $i = 0; $i < $count; ++$i) {
    print_pair($matching_pair[$i]);
}
