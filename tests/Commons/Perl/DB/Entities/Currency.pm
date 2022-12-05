
package DB::CurrencyExchangeRate;

use warnings;
use strict;
use DB::Entity::PQ;

our @ISA = qw(DB::Entity::PQ);

use constant STRUCT => 
{
  currency_exchange_id =>  DB::Entity::Type::int(unique => 1),
  currency_id => DB::Entity::Type::link('DB::Currency', unique => 1),
  rate => DB::Entity::Type::float()
};

1;

package DB::Currency;

use warnings;
use strict;
use DB::Entity::PQ;

use constant DEFAULT_FRACTION_DIGITS => 2;

our @ISA = qw(DB::Entity::PQ);


use constant STRUCT => 
{
  currency_id => DB::Entity::Type::sequence(),
  currency_code =>  DB::Entity::Type::string(unique => 1),
  fraction_digits => DB::Entity::Type::int(default => DEFAULT_FRACTION_DIGITS),
  source => 'M',
  
  # Private
  rate => DB::Entity::Type::float(private => 1)
};

sub precreate_
{
  my ($self, $ns) = @_;
  $self->{currency_code} = select_currency_code_($ns, $self);
}

sub postcreate_
{
  my ($self, $ns) = @_;
  
  my $stmt = $ns->pq_dbh->prepare_cached(
    q|SELECT currency_exchange_id FROM CurrencyExchange|, 
    undef, 1);
  $stmt->execute;
  while (my $res = $stmt->fetchrow_arrayref)
  {
    $ns->create(CurrencyExchangeRate => {
      currency_id => $self->{currency_id},
      currency_exchange_id => $res->[0],
      rate => $self->{rate} });
  }
}

sub select_currency_code_ {
  my ($ns, $args) = @_;
  my %args_copy = %$args;
  {
    # Try find currency with the same fraction & rate
    my $fraction_digits = $args_copy{fraction_digits};
    my $rate = $args_copy{rate};
    if (defined $fraction_digits && defined $rate)
    {
      my $stmt = 
          $ns->pq_dbh->prepare_cached(q|
                                   SELECT a.currency_code FROM Currency a
                                   LEFT JOIN CurrencyExchangeRate b USING(currency_id)
                                   LEFT JOIN (SELECT currency_id, 
                                              MAX(currency_exchange_id) as max_exchange_id
                                              FROM CurrencyExchangeRate
                                              GROUP BY currency_id) c USING(currency_id)
                                   WHERE a.fraction_digits=? AND b.rate = ?
                                   AND b.currency_exchange_id = c.max_exchange_id|, undef, 1);
      $stmt->execute($fraction_digits, $rate);
      my @result_row = $stmt->fetchrow_array();
      if ( @result_row )
      {
        return $result_row[0];
      }
    }
  }
  # Else return first not busy currency from allowed list
  my @allowed_codes = (
                  "SHP", # St. Helena Pounds
                  "KMF", # Comoros Francs
                  "LRD", # Liberia Dollars
                  "TJS", # Tajikistan Somoni
                  "STD", # Sao Tome and Principe Dobras
                  "LSL", # Lesotho Maloti
                  "SBD", # Solomon Islands Dollars
                  "FKP", # Falkland Islands Pounds
                  "GNF", # Guinea Francs
                  "SRD", # Suriname Dollars
                  "BIF", # Burundi Francs
                  "BTN", # Bhutan Ngultrum
                  "DJF", # Djibouti Francs
                  "CVE", # Cape Verde Escudos
                  "SPL", # Seborga Luigini
                  "TOP", # Tonga Pa'anga
                  "GYD", # Guyana Dollars
                  "WST", # Samoa Tala
                  "AOA", # Angola Kwanza
                  "RWF", # Rwanda Francs
                  "BND", # Brunei Dollar
                  "BGN", # Bulgarian Lev
                  "KHR"  # Riel
                    );

  my $codes_string = "'" . join("', '", @allowed_codes, "'");
  my $stmt = $ns->pq_dbh->prepare_cached("SELECT currency_code FROM Currency "
                                      . "WHERE currency_code IN ($codes_string)");

  
  $stmt->execute();
  my @result_row;
  while (@result_row = $stmt->fetchrow_array())
  {
    @allowed_codes = grep({ $_ ne $result_row[0] } @allowed_codes)
  }
  die "All allowed currency codes busy. Cann't create currency."
      if (!scalar @allowed_codes);
  return $allowed_codes[0];
}

1;

