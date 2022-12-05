#! /usr/bin/perl
#
use warnings;
use strict;

=head1 NAME

db_clean.pl - clean DB entities matching name prefix.

=head1 SYNOPSIS

  db_clean.pl OPTIONS

=head1 OPTIONS

=over

=item C<--db, -d db>

B<Required.>

=item C<--user, -u user>

B<Required.>

=item C<--password, -p password>

B<Required.>

=item C<--namespace namespace>

=item C<--prefix prefix>

=item C<--test test>

The script deletes records with names matching
I<namespace-prefix-test-%> SQL pattern.  By default namespace is
I<UT>, prefix is I<user_name>, and test is empty (when some
field is empty, its trailing I<'-'> is omitted too).  Thus, by default
you clean the data you created.  If you want to clean after particular
test, specify I<--test=name>.  If you want to clean all test data
after all users, specify I<--prefix=''> (empty string).  To clean the
namespace other than I<UT>, specify I<--namespace=name>.
Namespace name must not be empty to avoid accidental full clean.

=item C<--app-format name[,name]...>

List of AppFormat names to be deleted.  Default is
I<unit-test,unit-test-imp>.  Since AppFormats are shared among all
tests, the deletion happens only when I<--prefix=''>.  This option may
be given multiple times.

=back

=cut


use Getopt::Long qw(:config gnu_getopt);
use Pod::Usage;

(my $user_name = $ENV{USER}) =~ s/_/./;
my @test_app_format;
my %option = (namespace => 'UT',
              prefix => $user_name,
              test => '',
              'app-format' => \@test_app_format);
if (! GetOptions(\%option,
                 qw(db|d=s user|u=s password|p=s
                    namespace=s prefix:s test=s), 'app-format=s@')
    || @ARGV || (grep { not defined } @option{qw(db user password)})
    || $option{namespace} eq '') {
    pod2usage(1);
}

my $prefix = join('-',
                  grep({ $_ ne '' } @option{qw(namespace prefix test)}), '%');
@test_app_format = ('unit-test', 'unit-test-imp')
    unless @test_app_format;
@test_app_format = split(',', join(',', @test_app_format));


print "Deleting records with name prefix '$prefix'.\n";


use DBI;

my $dbh = DBI->connect("DBI:Oracle:$option{db}",
                       $option{user}, $option{password},
                       { AutoCommit => 0, PrintError => 0, RaiseError => 1 });


my $freq_cap_ids = $dbh->selectcol_arrayref(q{
    SELECT freq_cap_id FROM Site
    WHERE name LIKE :1
    UNION
    SELECT freq_cap_id FROM Campaign
    WHERE name LIKE :1
    UNION
    SELECT freq_cap_id FROM CampaignCreativeGroup
    WHERE name LIKE :1
    UNION
    SELECT freq_cap_id FROM CampaignCreative
    WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup
       WHERE name LIKE :1)
}, undef, $prefix);

my $keyword_ids = $dbh->selectcol_arrayref(q{
    SELECT keyword_id FROM Keyword WHERE keyword_id IN
      (SELECT keyword_id FROM CCGKeyword WHERE ccg_id IN
        (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1))
    AND keyword LIKE :1
}, undef, $prefix);


sub dbh_do {
    my ($sql) = @_;

    $sql =~ s/\A(?:^\s*\n)+//m;

    my @sql = split /(?:^\s*\n)+/m, $sql;

    foreach my $s (@sql) {
        $dbh->do($s, undef, $prefix);
    }
}


dbh_do <<'EOF;';

    DELETE FROM SiteRate WHERE site_rate_id IN
      (SELECT site_rate_id FROM TagPricing WHERE tag_id IN 
        (SELECT tag_id FROM Tags WHERE name LIKE :1))

    DELETE FROM TagPricing WHERE tag_id IN
      (SELECT tag_id FROM Tags WHERE name LIKE :1)

    DELETE FROM SiteReferrerStats WHERE tag_id IN
      (SELECT tag_id FROM Tags WHERE name LIKE :1)

    DELETE FROM Tags WHERE name LIKE :1

    DELETE FROM CCGSite WHERE site_id IN
      (SELECT site_id FROM Site WHERE name LIKE :1)

    DELETE FROM SiteAdvertiser WHERE site_id IN
      (SELECT site_id FROM Site WHERE name LIKE :1)
    OR account_id IN
      (SELECT account_id FROM Account WHERE name LIKE :1)

    DELETE FROM SiteCampaignApproval WHERE site_id IN
      (SELECT site_id FROM Site WHERE name LIKE :1)

    DELETE FROM SiteCreativeCategoryExclusion WHERE site_id IN
      (SELECT site_id FROM Site WHERE name LIKE :1)
    OR creative_category_id IN
      (SELECT creative_category_id FROM CreativeCategory WHERE name LIKE :1)

    DELETE FROM Site WHERE name LIKE :1

    DELETE FROM StatsHourly WHERE cc_id IN
      (SELECT cc_id FROM CampaignCreative WHERE ccg_id IN 
        (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1))
    OR colo_id IN
      (SELECT colo_id FROM Colocation WHERE name LIKE :1)

    DELETE FROM ExpressionPerformance WHERE cc_id IN
      (SELECT cc_id FROM CampaignCreative WHERE ccg_id IN 
        (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1))

    DELETE FROM CCGSite WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1)

    DELETE FROM CCGDailyRun WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1)

    DELETE FROM CCGCountry WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1)

    DELETE FROM ExcludeURL WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1)

    DELETE FROM CCGStats WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1)

    DELETE FROM CampaignCreative WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1)
    OR creative_id IN
      (SELECT creative_id FROM Creative WHERE name LIKE :1)

    DELETE FROM CCGKeywordStats WHERE ccg_keyword_id IN
      (SELECT ccg_keyword_id FROM CCGKeyword WHERE ccg_id IN
        (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1))

    DELETE FROM CCGKeyword WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1)

    UPDATE CampaignCreativeGroup SET ccg_rate_id = NULL WHERE name LIKE :1

    DELETE FROM CCGRate WHERE ccg_id IN
      (SELECT ccg_id FROM CampaignCreativeGroup WHERE name LIKE :1)

    DELETE FROM CampaignCreativeGroup WHERE name LIKE :1

    DELETE FROM SiteCampaignApproval WHERE campaign_id IN
      (SELECT campaign_id FROM Campaign WHERE name LIKE :1)

    DELETE FROM Campaign WHERE name LIKE :1

    DELETE FROM KeywordStats WHERE colo_id IN
      (SELECT colo_id FROM Colocation WHERE name LIKE :1)

EOF;


my $delete_freq_cap = $dbh->prepare(q{
    DELETE FROM FreqCap
    WHERE freq_cap_id = ?
});

foreach my $freq_cap_id (@$freq_cap_ids) {
    $delete_freq_cap->execute($freq_cap_id);
}


my $delete_keyword_stats = $dbh->prepare(q{
    DELETE FROM KeywordStats
    WHERE keyword_id = ?
});

my $delete_keyword = $dbh->prepare(q{
    DELETE FROM Keyword
    WHERE keyword_id = ?
});

foreach my $keyword_id (@$keyword_ids) {
    $delete_keyword_stats->execute($keyword_id);
    $delete_keyword->execute($keyword_id);
}


dbh_do <<'EOF;';

    DELETE FROM CreativeSizeOption WHERE size_id IN
      (SELECT size_id FROM CreativeSize WHERE name LIKE :1)

    DELETE FROM TemplateOptionValue WHERE creative_id IN
      (SELECT creative_id FROM Creative WHERE name LIKE :1)

    DELETE FROM CreativeCategory_Creative WHERE creative_id IN
      (SELECT creative_id FROM Creative WHERE name LIKE :1)
    OR creative_category_id IN
      (SELECT creative_category_id FROM CreativeCategory WHERE name LIKE :1)

    DELETE FROM CreativeCategory WHERE name LIKE :1

    DELETE FROM Creative WHERE name LIKE :1

    DELETE FROM TemplateOption WHERE template_id IN
      (SELECT template_id FROM Template WHERE name LIKE :1)

    DELETE FROM TemplateFile WHERE template_id IN
      (SELECT template_id FROM Template WHERE name LIKE :1)
    OR size_id IN (SELECT size_id FROM CreativeSize WHERE name LIKE :1)

    DELETE FROM Template WHERE name LIKE :1

    DELETE FROM CreativeSize WHERE name LIKE :1

EOF;


my $keyword_url_list_id = $dbh->prepare(q{
    SELECT keyword_list_id, url_list_id FROM Channel
    WHERE channel_id = :1
});

my $delete_channel_inventory = $dbh->prepare(q{
    DELETE FROM ChannelInventory
    WHERE channel_id = :1
});

my $delete_channel_inventory_by_cpm = $dbh->prepare(q{
    DELETE FROM ChannelInventoryByCPM
    WHERE channel_id = :1
});

my $delete_channel_trigger_stats = $dbh->prepare(q{
    DELETE FROM ChannelTriggerStats
    WHERE behav_params_id IN 
     (SELECT behav_params_id FROM BehavioralParameters 
      WHERE channel_id = :1)
});

my $delete_channel_performance = $dbh->prepare(q{
    DELETE FROM ChannelPerformance
    WHERE channel_id = :1
});

my $delete_channel = $dbh->prepare(q{
    DELETE FROM Channel
    WHERE channel_id = :1
});

my $delete_trigger_list = $dbh->prepare(q{
    DELETE FROM TriggerList
    WHERE trigger_list_id = :1
});

my $delete_behavioral_parameters = $dbh->prepare(q{
    DELETE FROM BehavioralParameters
    WHERE channel_id = :1
});

my $expressions = $dbh->selectcol_arrayref(q{
    SELECT expression FROM Channel
    WHERE name LIKE :1
}, undef, $prefix);

my @trigger_list_ids = @{$dbh->selectcol_arrayref(q{
    SELECT keyword_list_id FROM Channel
    WHERE name LIKE :1
    UNION
    SELECT url_list_id FROM Channel
    WHERE name LIKE :1
}, undef, $prefix)};

foreach my $expression (@$expressions) {
    next unless defined $expression;

    my @context_channel_ids = $expression =~ /(\d+)/g;
    foreach my $context_channel_id (@context_channel_ids) {
        $keyword_url_list_id->execute($context_channel_id);
        my ($keyword_id, $url_id) = $keyword_url_list_id->fetchrow_array;
        push @trigger_list_ids, $keyword_id
          if defined $keyword_id;
        push @trigger_list_ids, $url_id
          if defined $url_id;
    
        $delete_channel_inventory->execute($context_channel_id);
        $delete_channel_inventory_by_cpm->execute($context_channel_id);
        $delete_channel_trigger_stats->execute($context_channel_id);
        $delete_channel_performance->execute($context_channel_id);
        $delete_behavioral_parameters->execute($context_channel_id);
        $delete_channel->execute($context_channel_id);
    }
}


# Old channels

my $expressions_old = $dbh->selectcol_arrayref(q{
    SELECT expression FROM ChannelOld
    WHERE name LIKE :1
}, undef, $prefix);

my $delete_channel_old = $dbh->prepare(q{
    DELETE FROM ChannelOld
    WHERE channel_id = :1
});

my $trigger_list_id = $dbh->prepare(q{
    SELECT trigger_list_id FROM ChannelOld
    WHERE channel_id = :1
});

foreach my $expression (@$expressions_old) {
    next unless defined $expression;

    my @context_channel_ids = $expression =~ /(\d+)/g;
    foreach my $context_channel_id (@context_channel_ids) {
        $trigger_list_id->execute($context_channel_id);
        my ($trigger_id) = $trigger_list_id->fetchrow_array;
        push @trigger_list_ids, $trigger_id;
      
        $delete_channel_inventory->execute($context_channel_id);
        $delete_channel_inventory_by_cpm->execute($context_channel_id);
        $delete_channel_performance->execute($context_channel_id);
        $delete_channel_old->execute($context_channel_id);
    }
}


dbh_do <<'EOF;';

    DELETE FROM ChannelInventory WHERE channel_id IN
      (SELECT channel_id FROM Channel WHERE name LIKE :1)
    or channel_id IN 
      (SELECT channel_id FROM ChannelOld WHERE name LIKE :1)

    DELETE FROM ChannelInventoryByCPM WHERE channel_id IN
      (SELECT channel_id FROM Channel WHERE name LIKE :1)
    or channel_id IN 
      (SELECT channel_id FROM ChannelOld WHERE name LIKE :1)

    DELETE FROM ChannelTriggerStats WHERE behav_params_id IN
      (SELECT behav_params_id FROM BehavioralParameters WHERE channel_id IN
        (SELECT channel_id FROM Channel WHERE name LIKE :1))

    DELETE FROM ChannelPerformance WHERE channel_id IN
      (SELECT channel_id FROM Channel WHERE name LIKE :1) 
    or channel_id IN 
      (SELECT channel_id FROM ChannelOld WHERE name LIKE :1)


    DELETE FROM BehavioralParameters WHERE channel_id IN
      (SELECT channel_id FROM Channel WHERE name LIKE :1) 
 
    DELETE FROM Channel WHERE channel_id IN
      (SELECT channel_id FROM Channel WHERE name LIKE :1)

    DELETE FROM ChannelOld WHERE channel_id IN
      (SELECT channel_id FROM ChannelOld WHERE name LIKE :1)

    DELETE FROM OptOutStats WHERE colo_id IN
      (SELECT colo_id FROM Colocation WHERE name LIKE :1)

    DELETE FROM ColoUsers WHERE colo_id IN
      (SELECT colo_id FROM Colocation WHERE name LIKE :1)

    DELETE FROM UserProperties WHERE colo_id IN
      (SELECT colo_id FROM Colocation WHERE name LIKE :1)

    DELETE FROM KeywordStats WHERE colo_id IN
      (SELECT colo_id FROM Colocation WHERE name LIKE :1)

    UPDATE Colocation SET colo_rate_id = NULL WHERE name LIKE :1

    DELETE FROM ColocationRate WHERE colo_id IN
      (SELECT colo_id FROM Colocation WHERE name LIKE :1)

    DELETE FROM Colocation WHERE name LIKE :1

    DELETE FROM Advertiser WHERE name LIKE :1

    DELETE FROM Account WHERE name LIKE :1

    DELETE FROM AccountType WHERE name LIKE :1

    DELETE FROM CurrencyExchangeRate WHERE currency_id IN
      (SELECT currency_id FROM currency WHERE name LIKE :1)

    DELETE FROM Currency WHERE name LIKE :1

    DELETE FROM Country WHERE full_name LIKE :1

    DELETE FROM ReviewedTrigger WHERE trigger_word LIKE :1
    OR trigger_word LIKE 'http://webwise.com/'||:1

EOF;

foreach my $id (@trigger_list_ids) {
    $delete_trigger_list->execute($id);
}


if ($option{prefix} eq '' and $option{test} eq '') {
    $dbh->do(qq{
        DELETE FROM AppFormat WHERE name IN
          (@{[ join ', ', map { "'$_'" } @test_app_format ]})
    });
}


print "Committing...\n";
$dbh->commit;
print "Done\n";
