
package CreateNews;

use warnings;
use strict;

use RandWords;
use File::Spec;
use XMLEncoder;
use Digest::MD5 qw(md5_hex);

use constant DEFAULT_FILENAME_SIZE  => 10;
use constant DEFAULT_TTL  => 100;
use constant DEFAULT_TITLE_SIZE  => 100;
use constant DEFAULT_DSC_SIZE  => 4000;
use constant NEWS_IN_EVENT  => 4;

my @lang = ("eng", "rus", "kor");
my @country = ("USA", "GBR", "KOR");

sub create_news 
{
  my ($news_path, $query_count, $news_count, $urls) = @_;
  my @queries = ();
  for (my $f_idx = 0; $f_idx < $query_count; $f_idx++)
  {
    my $query    = RandWords::rand_string(DEFAULT_FILENAME_SIZE);
    my $filename = File::Spec->join($news_path, $query . ".xml");
    push (@queries, $query);
    create_news_file($filename, $news_count, $urls);
  }
  return @queries
}

sub create_news_file 
{
  my ($filepath, $news_count, $urls) = @_;
  open(my $XML, ">$filepath") || die "Cann't open file $filepath for write.\n";


  my $items_count = $news_count;
  
  my @news = create_items($items_count, $urls);

  print $XML XML::Encoder::GROUP_PARAM({"xmlns"=> "http://www.newsgate.com/XSearch/1.0",
                                        "version" => "1.0"},
                                        "result", [XML::Encoder::PARAM("ttl", DEFAULT_TTL), 
                                                   XML::Encoder::PARAM("reqDate", get_current_date()),
                                                   XML::Encoder::PARAM("total", $items_count),
                                                   XML::Encoder::GROUP_PARAM("items", \@news)]);

  close($XML);
}


sub create_items
{
  my ($count, $urls) = @_;
  my @items = ();
  my $event_id = genGUID();
  for (my $i = 0; $i < $count; $i++)
  {
    $event_id = $i % NEWS_IN_EVENT? $event_id: genGUID();
    push (@items, create_item($urls, $event_id));
  }
  return @items;
}

sub create_item
{
  my ($urls, $event_id) = @_;
  my $url = RandWords::rand_url("performance");
  push (@$urls, $url);
  return XML::Encoder::GROUP_PARAM("item", [XML::Encoder::PARAM("title", RandWords::rand_string(DEFAULT_TITLE_SIZE)),
                                            XML::Encoder::PARAM("description", RandWords::rand_string(DEFAULT_DSC_SIZE)),
                                            XML::Encoder::PARAM("pubDate", get_current_date()),
                                            XML::Encoder::PARAM("link", $url),
                                            XML::Encoder::GROUP_PARAM("guid", [XML::Encoder::PARAM("language", get_language())]),
                                            XML::Encoder::GROUP_PARAM("event", [XML::Encoder::PARAM("id", $event_id),
                                                                               XML::Encoder::PARAM("capacity", int(rand(100)))]),
                                            XML::Encoder::GROUP_PARAM("images", 
                                                                      [XML::Encoder::PARAM({src => RandWords::rand_url("performance"),
                                                                                            width => int(rand(799) + 1),
                                                                                            height => int(rand(599) + 1),
                                                                                            alt    => RandWords::rand_string(DEFAULT_TITLE_SIZE)}
                                                                                           , "image")]),
                                            XML::Encoder::GROUP_PARAM("source",  
                                                                      [XML::Encoder::PARAM("title", RandWords::rand_string(DEFAULT_TITLE_SIZE)),
                                                                       XML::Encoder::PARAM("link", RandWords::rand_url("performance")),
                                                                       XML::Encoder::PARAM("country" , get_country())])]);
}

sub get_current_date
{
  # TODO: Get real date here.
  return "Wed, 15 Apr 2009 20:00:28 GMT";
}

sub get_language
{
  return @lang[int(rand(scalar(@lang)))];
}

sub get_country
{
  return @country[int(rand(scalar(@country)))];
}

# Generate a GUID given a string
sub genGUID {
  my $t = time() * 1000;
  my $r1 = int(rand(100000000000000000));
  my $r2 = int(rand(100000000000000000));
  return uc md5_hex ($t . $r1 . $r2);
}



1;
