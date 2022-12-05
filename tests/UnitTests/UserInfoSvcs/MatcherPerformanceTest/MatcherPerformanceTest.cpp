#include <string>
#include <iostream>
#include <vector>
#include <list>

#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>
#include <UserInfoSvcs/UserInfoCommons/ChannelMatcher.hpp>
#include <UserInfoSvcs/UserInfoCommons/Allocator.hpp>

using namespace AdServer::UserInfoSvcs;

const long GMT_OFFSET = -3;
const Generics::Time session_timeout = Generics::Time(30);

ProfileProperties pps;
ProfileMatchParams pmp;

int size_assumption_test()
{
  static const char* FUN = "size_assuption_test()";
  const unsigned long CHANNEL_NUMBER = 10001;
  const unsigned long ITERATION_COUNT = 1000;

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  for(unsigned long channel_i = 1; channel_i < CHANNEL_NUMBER; ++channel_i)
  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();

    page->short_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(3600), 4, 2));
    page->short_intervals.insert(ChannelInterval(
      Generics::Time(3600), Generics::Time(7200), 2, 2));
    page->short_intervals.insert(ChannelInterval(
      Generics::Time(7200), Generics::Time(14400), 1, 2));
    page->short_intervals.insert(ChannelInterval(
      Generics::Time(14400), Generics::Time(28800), 8, 2));

    page->long_intervals.insert(ChannelInterval(
      Generics::Time(86400), Generics::Time(2*86400), 2, 2));

    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(86400), 2, 2));
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(2*86400), 4, 2));
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(4*86400), 1, 2));
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(8*86400), 8, 2));

    channel_rules->page_channels[channel_i] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    Generics::Time tm = Generics::Time::get_time_of_day();

    ChannelMatchPack cid;
    for(unsigned long channel_i = 0; channel_i < CHANNEL_NUMBER; ++channel_i)
    {
      cid.page_channels.push_back(channel_i);
    }

    for(unsigned long iteration = 0; iteration < ITERATION_COUNT; ++iteration)
    {
      std::cout << iteration << std::endl;

      ChannelMatchPack new_cid;
      for(unsigned long i = 1; i <= 1000; ++i)
      {
        unsigned long chid = 0;

        while (chid == 0)
        {
          chid = rand()%CHANNEL_NUMBER;
        }

        new_cid.page_channels.push_back(chid);
      }

      tm += 4*3600/ITERATION_COUNT;
      ChannelMatchMap result;
      ChannelsMatcher cm(base_profile.in(), add_profile.in());

      if (cm.need_history_optimization(tm, Generics::Time::ZERO,
        Generics::Time::ZERO))
      {
        cm.history_optimize(history_profile.in(), tm, Generics::Time::ZERO,
          *channel_rules);
      }

      cm.match(result, tm, new_cid, *channel_rules, pmp, pps, session_timeout, false);
    }

    ChannelsMatcher::print(base_profile->membuf(), std::cout, "  ");


    std::cout << FUN << ": base profile size = "
              << base_profile->membuf().size() << std::endl;
    std::cout << FUN << ": history profile size = "
              << history_profile->membuf().size() << std::endl;
  }

  return 0;
}


int session_match_performance_test()
{
  static const char* FUN = "session_match_performance_test()";
  const unsigned long CHANNEL_NUMBER = 1000;
  const unsigned long ITERATION_COUNT = 100;

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  for(unsigned long channel_i = 1; channel_i < CHANNEL_NUMBER; ++channel_i)
  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(ChannelInterval(
      Generics::Time(20), Generics::Time(60), 10, 2));
    page->short_intervals.insert(ChannelInterval(
      Generics::Time(40), Generics::Time(120), 20, 2));
    page->short_intervals.insert(ChannelInterval(
      Generics::Time(90), Generics::Time(130), 30, 2));
    channel_rules->page_channels[channel_i] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    Generics::Time tm = Generics::Time::get_time_of_day();

    ChannelMatchPack cid;
    for(unsigned long channel_i = 0; channel_i < CHANNEL_NUMBER; ++channel_i)
    {
      cid.page_channels.push_back(channel_i);
    }

    Generics::Timer timer;
    timer.start();

    for(unsigned long iteration = 0; iteration < ITERATION_COUNT; ++iteration)
    {
      tm += 3;
      ChannelMatchMap result;
      ChannelsMatcher cm(base_profile.in(), add_profile.in());
      cm.match(result, tm, cid, *channel_rules, pmp, pps, session_timeout, false);
    }

    timer.stop();

    std::cout << FUN << ": sum time = " << timer.elapsed_time() << ", avg time = " <<
      timer.elapsed_time() / ITERATION_COUNT << " for " <<
      CHANNEL_NUMBER << " channels." << std::endl;
  }

  return 0;
}

int history_match_performance_test()
{
  static const char* FUN = "history_match_performance_test()";
  const unsigned long CHANNEL_NUMBER = 1000;
  const unsigned long ITERATION_COUNT = 100;

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  for(unsigned long channel_i = 1; channel_i < CHANNEL_NUMBER; ++channel_i)
  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->long_intervals.insert(ChannelInterval(
      Generics::Time(86400), Generics::Time(86400*2), 10, 2));
    page->long_intervals.insert(ChannelInterval(
      Generics::Time(86400*2), Generics::Time(86400*3), 20, 2));
    page->long_intervals.insert(ChannelInterval(
      Generics::Time(86400), Generics::Time(86400*3), 30, 2));
    channel_rules->page_channels[channel_i] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    Generics::Time tm = Generics::Time::get_time_of_day();

    ChannelMatchPack cid;
    for(unsigned long channel_i = 0; channel_i < CHANNEL_NUMBER; ++channel_i)
    {
      cid.page_channels.push_back(channel_i);
    }

    Generics::Timer timer;
    timer.start();

    for(unsigned long iteration = 0; iteration < ITERATION_COUNT; ++iteration)
    {
      tm += 3;
      ChannelMatchMap result;
      ChannelsMatcher cm(base_profile.in(), add_profile.in());
      cm.match(result, tm, cid, *channel_rules, pmp, pps, session_timeout, false);
    }

    timer.stop();

    std::cout << FUN << ": sum time = " << timer.elapsed_time() << ", avg time = " <<
      timer.elapsed_time() / ITERATION_COUNT << " for " <<
      CHANNEL_NUMBER << " channels." << std::endl;
  }

  return 0;
}

int
history_today_match_performance_test()
{
  static const char* FUN = "history_today_match_performance_test()";
  const unsigned long CHANNEL_NUMBER = 1000;
  const unsigned long ITERATION_COUNT = 100;

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  for(unsigned long channel_i = 1; channel_i < CHANNEL_NUMBER; ++channel_i)
  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(86400*4), 10, 2));
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(86400*3), 20, 2));
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(86400), 30, 2));
    channel_rules->page_channels[channel_i] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    Generics::Time tm = Generics::Time::get_time_of_day();

    ChannelMatchPack cid;
    for(unsigned long channel_i = 0; channel_i < CHANNEL_NUMBER; ++channel_i)
    {
      cid.page_channels.push_back(channel_i);
    }

    Generics::Timer timer;
    timer.start();

    for(unsigned long iteration = 0; iteration < ITERATION_COUNT; ++iteration)
    {
      tm += 3;
      ChannelMatchMap result;
      ChannelsMatcher cm(base_profile.in(), add_profile.in());
      cm.match(result, tm, cid, *channel_rules, pmp, pps, session_timeout, false);
    }

    timer.stop();

    std::cout << FUN << ": sum time = " << timer.elapsed_time() << ", avg time = " <<
      timer.elapsed_time() / ITERATION_COUNT << " for " <<
      CHANNEL_NUMBER << " channels." << std::endl;
  }

  return 0;
}

int
merge_performance_test()
{
  static const char* FUN = "history_today_match_performance_test()";
  const unsigned long CHANNEL_NUMBER = 1000;
  const unsigned long ITERATION_COUNT = 100;

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  for(unsigned long channel_i = 1; channel_i < CHANNEL_NUMBER; ++channel_i)
  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(86400*4), 10, 2));
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(86400*3), 20, 2));
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(86400), 30, 2));
    channel_rules->page_channels[channel_i] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);
    SmartMemBuf_var merge_base_profile(new SmartMemBuf);
    SmartMemBuf_var merge_add_profile(new SmartMemBuf);
    SmartMemBuf_var merge_history_profile(new SmartMemBuf);

    Generics::Time tm = Generics::Time::get_time_of_day();

    ChannelMatchPack cid;
    for(unsigned long channel_i = 0; channel_i < CHANNEL_NUMBER; ++channel_i)
    {
      cid.page_channels.push_back(channel_i);
    }

    Generics::Timer timer;
    timer.start();

    ChannelsMatcher matcher(base_profile.in(), add_profile.in());
    
    {
      // fill target profile
      for(unsigned long iteration = 0; iteration < ITERATION_COUNT; ++iteration)
      {
        tm += 3;
        ChannelMatchMap result;
        matcher.match(result, tm, cid, *channel_rules, pmp, pps, session_timeout, false);
      }
    }
    
    {
      // fill source profile
      ChannelsMatcher merge_matcher(merge_base_profile.in(), merge_add_profile.in());
    }

    timer.stop();

    std::cout << FUN << ": sum time = " << timer.elapsed_time() << ", avg time = " <<
      timer.elapsed_time() / ITERATION_COUNT << " for " <<
      CHANNEL_NUMBER << " channels." << std::endl;
  }

  return 0;
}

int main() noexcept
{
  int ret = 0;

  ret += history_today_match_performance_test();
  ret += session_match_performance_test();
  ret += history_match_performance_test();

//  ret += size_assumption_test();

  return ret;
}
