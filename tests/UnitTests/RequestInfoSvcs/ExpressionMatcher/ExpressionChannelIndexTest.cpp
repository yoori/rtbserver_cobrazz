#include <map>
#include <sstream>

#include <Generics/AppUtils.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/ChannelMatcher.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelCorbaAdapter.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelIndex.hpp>
#include <CampaignSvcs/CampaignServer/ExpressionChannelParser.hpp>
#include <CampaignSvcs/CampaignServer/NonLinkedExpressionChannelCorbaAdapter.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

using namespace AdServer::CampaignSvcs;

struct TestExpressionChannelIndex: public ExpressionChannelIndex
{
  template<unsigned long CHECK_CHANNELS_COUNT>
  bool check(const char* test_name,
    unsigned long simple_channel_id,
    const unsigned long (&check_channels)[CHECK_CHANNELS_COUNT])
  {
    ExpressionChannelMatchMap::const_iterator ind_it =
      channels_.find(simple_channel_id);
    if(ind_it == channels_.end())
    {
      std::cerr << test_name << ": cell for #" << simple_channel_id <<
        " not found." << std::endl;
      return false;
    }

    ChannelIdSet etalon_channels;
    std::copy(check_channels, check_channels + CHECK_CHANNELS_COUNT,
      std::inserter(etalon_channels, etalon_channels.begin()));

    ChannelIdSet channels;
    std::copy(ind_it->second->matched_channels.begin(),
      ind_it->second->matched_channels.end(),
      std::inserter(channels, channels.begin()));

    for(auto eit = ind_it->second->check_channels.cbegin();
        eit != ind_it->second->check_channels.cend(); ++eit)
    {
      channels.insert(eit->channel_id);
    }

    if(channels.size() == etalon_channels.size() &&
       std::equal(etalon_channels.begin(), etalon_channels.end(), channels.begin()))
    {
      return true;
    }

    std::cerr << test_name << ": cell for #" << simple_channel_id << "(";
    Algs::print(std::cerr, channels.begin(), channels.end());
    std::cerr << ") not equal etalon: (";
    Algs::print(std::cerr, etalon_channels.begin(), etalon_channels.end());
    std::cerr << ")" << std::endl;
    return false;
  }

};

typedef ReferenceCounting::SmartPtr<TestExpressionChannelIndex>
  TestExpressionChannelIndex_var;

ExpressionChannelBase_var
parse_for_channel(
  const char* expression,
  const ChannelParams& channel_params,
  ExpressionChannelHolderMap& channels)
{
  NonLinkedExpressionChannel_var nl_ch =
    ExpressionChannelParser::parse(String::SubString(expression));
  nl_ch->params(channel_params);

  ExpressionChannelInfo channel_info;
  pack_non_linked_expression_channel(channel_info, nl_ch);
  return unpack_channel(channel_info, channels);
}

void
create_simple_channel(
  ExpressionChannelHolderMap& channels,
  unsigned long simple_channel_id)
{
  ChannelParams simple_channel_params;
//simple_channel_params.name = "TEST";
  simple_channel_params.channel_id = simple_channel_id;
  simple_channel_params.status = 'A';
  SimpleChannel_var simple_channel(new SimpleChannel(simple_channel_params));
  channels[simple_channel_id] = new ExpressionChannelHolder(simple_channel);
}

void
create_expr_channel(
  ExpressionChannelHolderMap& channels,
  unsigned long channel_id,
  const char* expr)
{
  ChannelParams ex_channel_params;
//ex_channel_params.name = "TEST";
  ex_channel_params.channel_id = channel_id;
  ex_channel_params.status = 'A';

  channels[channel_id] = new ExpressionChannelHolder(
    parse_for_channel(expr, ex_channel_params, channels));
}

// return max simple channel id
unsigned long
generate_channels(
  ExpressionChannelHolderMap& channels,
  unsigned long sc_number,
  unsigned long LEVEL_NUMBER = 2)
{
  unsigned long simple_channel_id = 1;

  unsigned long first_channel_id = 1;
  for(; simple_channel_id < sc_number + 1; ++simple_channel_id)
  {
    create_simple_channel(channels, simple_channel_id);
  }

  unsigned long last_channel_id = simple_channel_id - 1;
  for(unsigned long level_i = 1; level_i < LEVEL_NUMBER; ++level_i)
  {
    unsigned long channel_id = last_channel_id;
    unsigned long next_level_channel_id = first_channel_id;
    for(unsigned long i = 0; i < sc_number - level_i; ++i)
    {
      std::ostringstream eostr;
      eostr << next_level_channel_id << "|" <<
        (next_level_channel_id + 1);
      create_expr_channel(channels, channel_id++, eostr.str().c_str());
      ++next_level_channel_id;
    }
    first_channel_id = last_channel_id;
    last_channel_id = channel_id;
  }

  return simple_channel_id;
}

int test_balance_1()
{
  static const char* TEST_NAME = "test_balance_1";

  ExpressionChannelHolderMap channels;

  create_simple_channel(channels, 1);
  create_simple_channel(channels, 2);
  create_simple_channel(channels, 3);

  create_expr_channel(channels, 4, "1 & 2");
  create_expr_channel(channels, 5, "2 & 3");

  TestExpressionChannelIndex_var ch_index(new TestExpressionChannelIndex());
  ch_index->index(channels);

  const unsigned long CH_1[] = {1, 4};
  const unsigned long CH_2[] = {2, 5};
  const unsigned long CH_3[] = {3};

  // expect 1 => 1, 4; 2 => 2, 5; 3 => 3
  bool check = ch_index->check(TEST_NAME, 1, CH_1);
  check |= ch_index->check(TEST_NAME, 2, CH_2);
  check |= ch_index->check(TEST_NAME, 3, CH_3);

  if(check)
  {
    if(ch_index->size() == 3)
    {
      std::cout << TEST_NAME << ": success" << std::endl;
    }
    else
    {
      std::cerr << TEST_NAME << ": incorrect index size: " <<
        ch_index->size() << std::endl;
      return 1;
    }
    return 0;
  }

  return 1;
}

int test_balance_2()
{
  static const char* TEST_NAME = "test_balance_2";

  ExpressionChannelHolderMap channels;

  create_simple_channel(channels, 1);
  create_simple_channel(channels, 2);
  create_simple_channel(channels, 3);
  create_simple_channel(channels, 4);
  create_expr_channel(channels, 5, "1 & 2 | 3");
  create_expr_channel(channels, 6, "2 | 3 & 4");
  create_expr_channel(channels, 7, "1 & 2 | 3 & 4");
  create_expr_channel(channels, 8, "(2 | 3) & (1 | 4)");

  TestExpressionChannelIndex_var ch_index(new TestExpressionChannelIndex());
  ch_index->index(channels);

  const unsigned long CH_1[] = {1, 5, 7, 8};
  const unsigned long CH_2[] = {2, 6, 8};
  const unsigned long CH_3[] = {3, 5};
  const unsigned long CH_4[] = {4, 6, 7, 8};

  if(ch_index->check(TEST_NAME, 1, CH_1) &&
     ch_index->check(TEST_NAME, 2, CH_2) &&
     ch_index->check(TEST_NAME, 3, CH_3) &&
     ch_index->check(TEST_NAME, 4, CH_4))
  {
    if(ch_index->size() == 4)
    {
      std::cout << TEST_NAME << ": success" << std::endl;
    }
    else
    {
      std::cerr << TEST_NAME << ": incorrect index size: " <<
        ch_index->size() << std::endl;
      return 1;
    }
    return 0;
  }

  return 1;
}

int test_and_not()
{
  static const char* TEST_NAME = "test_and_not";

  ExpressionChannelHolderMap channels;

  create_simple_channel(channels, 1);
  create_simple_channel(channels, 2);
  create_simple_channel(channels, 3);
  create_simple_channel(channels, 4);
  create_simple_channel(channels, 5);
  create_expr_channel(channels, 10, "1 & 2 | 3 ^ 4");
  create_expr_channel(channels, 11, "(1 | 2 | 3) ^ 4");
  create_expr_channel(channels, 12, "((1 | 2) ^ 5) & ((1 | 3) ^ 4)");
  create_expr_channel(channels, 13, "((1 | 2) ^ 3) & ((1 | 3) ^ 4)");

  TestExpressionChannelIndex_var ch_index(new TestExpressionChannelIndex());
  ch_index->index(channels);

  const unsigned long CH_1[] = {1, 10, 11, 12, 13};
  const unsigned long CH_2[] = {2, 11, 12, 13};
  const unsigned long CH_3[] = {3, 10, 11};
  const unsigned long CH_4[] = {4};
  const unsigned long CH_5[] = {5};

  bool check = ch_index->check(TEST_NAME, 1, CH_1);
  check &= ch_index->check(TEST_NAME, 2, CH_2);
  check &= ch_index->check(TEST_NAME, 3, CH_3);
  check &= ch_index->check(TEST_NAME, 4, CH_4);
  check &= ch_index->check(TEST_NAME, 5, CH_5);
  if(check)
  {
    if(ch_index->size() == 5)
    {
      std::cout << TEST_NAME << ": success" << std::endl;
    }
    else
    {
      std::cerr << TEST_NAME << ": incorrect index size: " <<
        ch_index->size() << std::endl;
      return 1;
    }
    return 0;
  }

  return 1;
}

int perf_test(
  unsigned long SIMPLE_CHANNEL_COUNT,
  unsigned long MATCH_CHANNEL_COUNT,
  unsigned long expected_result_size = 0,
  unsigned long LEVEL_NUMBER = 2)
{
//const unsigned long REQUESTS_NUMBER = 1000;

  if(expected_result_size == 0)
  {
    expected_result_size = 2 * MATCH_CHANNEL_COUNT + 1;
  }

  ExpressionChannelHolderMap expression_channels;
  /*unsigned long max_sc_id = */
  generate_channels(
    expression_channels, SIMPLE_CHANNEL_COUNT, LEVEL_NUMBER);

  /*
  ExpressionChannelIndex_var ch_index(new ExpressionChannelIndex());
  ch_index->index(expression_channels);
  */

  /*
  Generics::Timer timer;
  timer.start();
  ChannelIdSet hist_channels;
  unsigned long base = ::rand() % (max_sc_id - MATCH_CHANNEL_COUNT - 1) + 1;
  for(unsigned long i = base; i < base + MATCH_CHANNEL_COUNT; ++i)
  {
    hist_channels.insert(i);
  }

  for(unsigned long i = 0; i < REQUESTS_NUMBER; ++i)
  {
    ChannelIdSet res_channels;
    ch_index->match(res_channels, hist_channels);

    if(res_channels.size() != expected_result_size)
    {
      std::cerr << "result size incorrect : " << res_channels.size() << std::endl;
      return 1;
    }
  }
  timer.stop();

  std::cout << "perf_test(" <<
    "reqs=" << REQUESTS_NUMBER <<
    ", s-channels=" << SIMPLE_CHANNEL_COUNT <<
    ", m-channels=" << MATCH_CHANNEL_COUNT <<
    ", r-channels=" << expected_result_size <<
    ", lev=" << LEVEL_NUMBER <<
    "): " << timer.elapsed_time() << ", indexing time = " << itimer.elapsed_time() <<
    std::endl <<
    " index traits: size = " <<
    ch_index->size() << ", avg seq size = " <<
    ch_index->avg_seq_size_() << std::endl;
  */

  return 0;
}

int main(int /*argc*/, char* /*argv*/[]) noexcept
{
  int res = 0;
  try
  {
    /*
    res += test_balance_1();
    res += test_balance_2();
    res += test_and_not();
    */

    /*
    {
      const unsigned long SIMPLE_CHANNEL_COUNT = 200000;
      ExpressionChannelHolderMap expression_channels;
      unsigned long max_sc_id = generate_channels(
        expression_channels, SIMPLE_CHANNEL_COUNT, 1);

      ExpressionChannelIndex_var ch_index(new ExpressionChannelIndex());
      ch_index->index(expression_channels);
    }

    ::sleep(10000);
    */

//  res += perf_test(1, 1);

    res += perf_test(200000, 1); // 345396, 164060
    ::sleep(500);

    // res += perf_test(100, 1); // 20560
    // res += perf_test(400000, 1); // 670344

    /*
    res += perf_test(1000, 30);
    res += perf_test(5000, 60);
    res += perf_test(50000, 90);
    res += perf_test(50000, 90, 366, 4);
    res += perf_test(100000, 10);
    */
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return res;
}
