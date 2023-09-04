package CsvUtils::CompanyUtils;

use Encode;
use List::MoreUtils qw/uniq/;
use utf8;

sub normalize_company_name_form
{
  my ($name) = @_;

  $name = lc $name;
  $name = " " . $name . " ";
  $name =~ s/^(.*)$/ $1/;
  $name =~ s/^(.*)$/$1 /;
  $name =~ s/«/ /g;
  $name =~ s/»/ /g;
  $name =~ s|/| |g;
  $name =~ s/[(][^)]*[)]/ /g;
  $name =~ s/[(),."]/ /g;
  $name =~ s/–\s+/-/g;
  $name =~ s/-\s+/-/g;
  $name =~ s/\s+-/-/g;
  $name =~ s/\s+/ /g;
  $name =~ s/ инн [0-9]{6,} / /g;
  $name =~ s/\s+/ /g;
  $name =~ s/ сбер\s+банк / сбербанк /g;
  $name =~ s/ ([^ ]+)сервис / $1 сервис /g;

  if($name =~ m/ муниципальное / && $name =~ m/ казенное / && $name =~ m/ предприятие /)
  {
    $name =~ s/ (муниципальное|казенное|предприятие) / /g;
    $name .= ' мкп ';
    $name =~ s/\s+/ /g;
  }

  $name =~ s/ ационерное общество / ао /g;
  $name =~ s/ акционерное общество / ао /g;
  $name =~ s/ общество с ограниченной ответственностью / ооо /g;
  $name =~ s/ общество с ограниченной ответсвенностью / ооо /g;
  $name =~ s/ общество с ограниченной отвественностью / ооо /g;
  $name =~ s/ общесвтво с ограниченной ответственностью / ооо /g;
  $name =~ s/ общество с органиченной ответственностью / ооо /g;
  $name =~ s/ товарищество с органиченной ответственностью / тоо /g;
  $name =~ s/ федеральное государственное учреждение / фгу /g;
  $name =~ s/ закрытое акционерное общество / зао /g;
  $name =~ s/ индивидуальный предприниматель / ип /g;
  $name =~ s/ публичное акционерное общество / пао /g;
  $name =~ s/ муниципальное автономное учреждение / пао /g;
  $name =~ s/ государственное автономное учреждение / гау /g;
  $name =~ s/ муниципальное предприятие / мп /g;
  $name =~ s/ публичное ао / пао /g;
  $name =~ s/ закрытое ао / зао /g;
  $name =~ s/ открытое ао / оао /g;
  $name =~ s/ предприниматель / ип /g;

  return $name;
}

sub normalize_company_name
{
  my ($name) = @_;

  Encode::_utf8_on($name);

  my $nf = normalize_company_name_form($name);

  if($nf !~ m/ ип /)
  {
    while($name =~ m/((?:\p{Uppercase}\s*[.]\s*)+)/)
    {
      my $rep = $1;
      my $res = lc($rep);
      $res =~ s/[.]//g;
      $res =~ s/\s+//g;
      $name =~ s/\Q$rep/$res /;
    }

    $name = normalize_company_name_form($name);
  }
  else
  {
    $name = $nf;
  }

  $name =~ s/-/ /g; $name =~ s/\s+/ /g;
  $name =~ s/ два / 2 /g; $name =~ s/\s+/ /g;
  $name =~ s/ гпб / газпромбанк /g; $name =~ s/\s+/ /g;
  $name =~ s/ торговый дом / тд /g; $name =~ s/\s+/ /g;
  $name =~ s/ дальневосточное производственное объединение / двпо /g; $name =~ s/\s+/ /g;

  $name =~ s/^(.*)банк(.*)$/$1 банк $2/g; $name =~ s/\s+/ /g;
  $name =~ s/^(.*)сервис(.*)$/$1 сервис $2/g; $name =~ s/\s+/ /g;
  $name =~ s/^(.*)ойл(.*)$/$1 ойл $2/g; $name =~ s/\s+/ /g;
  $name =~ s/^(.*)авто(.*)$/$1 авто $2/g; $name =~ s/\s+/ /g;
  $name =~ s/ то / /g; $name =~ s/\s+/ /g;
  $name =~ s/ запчать / запчасть /g; $name =~ s/\s+/ /g;
  $name =~ s/ промой / промоил /g; $name =~ s/\s+/ /g;
  $name =~ s/ промой / промоил /g; $name =~ s/\s+/ /g;
  $name =~ s/ ([^ ]+)trade / $1 trade /g; $name =~ s/\s+/ /g;
  $name =~ s/ trade / trading /g; $name =~ s/\s+/ /g;
  $name =~ s/\s+/ /g;
  $name =~ s/ транс авто / транс-авто /g;

  my $type = '';

  my @words = split(/\s+/, $name);
  @words = uniq(sort(@words));

  my @res_words;
  foreach my $w(@words)
  {
    Encode::_utf8_on($w);

    if($w =~ m/^(мкоу|мку|фгку|мбоу|нп|ип|ооо|ао|оао|зао|сооо|гау|фгау|мп|пао|фгу|гку|муп|маоу|тоо|ou|ltd|фгуп)$/)
    {
      $type = $1;
    }

    $w =~ s/^(мкоу|мку|фгку|мбоу|нп|ип|ооо|ао|оао|зао|сооо|гау|фгау|мп|пао|фгу|гку|муп|маоу|тоо|ou|ltd|фгуп)$//;
    $w =~ s/^корпорация$//;
    $w =~ s/^фирма$//;
    $w =~ s/^компания$//;
    $w =~ s/^предприятие$//;
    $w =~ s/^филиал$//;
    $w =~ s/^too$//;
    $w =~ s/^co$//;
    $w =~ s/^and$//;

    $w =~ s/^калужский$//;
    $w =~ s/^новгород$//;
    $w =~ s/^краснодар$//;
    $w =~ s/^челябинск$//;
    $w =~ s/^нижний$//;
    $w =~ s/^тюмень$//;
    $w =~ s/^ноябрьск$//;
    $w =~ s/^нск$//;
    $w =~ s/^(москве|москва|столичный)$//;

    if(length($w) > 1 || $w =~ m/^\d+$/)
    {
      push(@res_words, $w);
    }
  }

  return (uc($type), join(' ', @res_words));
}

1;
