#include <string>
#include <iostream>
#include <vector>
#include <list>

#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>
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

bool compare_partly_match_result(
  const PartlyMatchResult& left,
  const PartlyMatchResult& right)
{
  {
    PartlyMatchMap::const_iterator right_it = right.from_to_matches.begin();
    PartlyMatchMap::const_iterator left_it = left.from_to_matches.begin();

    for(; left_it != left.from_to_matches.end() &&
          right_it != right.from_to_matches.end();
        ++left_it, ++right_it)
    {
      if(left_it->first != right_it->first ||
         left_it->second.visits != right_it->second.visits ||
         left_it->second.minimum_visits != right_it->second.minimum_visits)
      {
        return false;
      }
    }

    if(right_it != right.from_to_matches.end() ||
       left_it != left.from_to_matches.end())
    {
      return false;
    }
  }

  {
    PartlyMatchMap::const_iterator right_it = right.now_to_matches.begin();
    PartlyMatchMap::const_iterator left_it = left.now_to_matches.begin();

    for(; left_it != left.now_to_matches.end() &&
          right_it != right.now_to_matches.end();
        ++left_it, ++right_it)
    {
      if(left_it->first != right_it->first ||
         left_it->second.visits != right_it->second.visits ||
         left_it->second.minimum_visits != right_it->second.minimum_visits)
      {
        return false;
      }
    }

    if(right_it != right.now_to_matches.end() ||
       left_it != left.now_to_matches.end())
    {
      return false;
    }
  }

  return true;
}

void print_partly_match_result(
  std::ostream& out, const PartlyMatchResult& result)
{
  out << "  from-to:" << std::endl;
  for(PartlyMatchMap::const_iterator res_it = result.from_to_matches.begin();
      res_it != result.from_to_matches.end(); ++res_it)
  {
    out << "    " << res_it->first << " : " << res_it->second.visits << "/" <<
      res_it->second.minimum_visits << std::endl;
  }

  out << "  now-to:" << std::endl;
  for(PartlyMatchMap::const_iterator res_it = result.now_to_matches.begin();
      res_it != result.now_to_matches.end(); ++res_it)
  {
    out << "    " << res_it->first << " : " << res_it->second.visits << "/" <<
      res_it->second.minimum_visits << std::endl;
  }
}

int session_match_test()
{
  static const char* FUN = "session_match_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(60), 1, 2));
    channel_rules->page_channels[1] = page;
  }

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(ChannelInterval(
      Generics::Time(30), Generics::Time(120), 2, 2));
    channel_rules->page_channels[2] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    Generics::Time tm = Generics::Time::get_time_of_day();

    ChannelIdPack cid;
    cid.page_channels.push_back(1);

    ChannelMatchMap result;
    PartlyMatchResult partly_result;

    ChannelsMatcher cm(base_profile.in(), add_profile.in());

    cm.match(result, tm, cid, *channel_rules, pmp, pps, session_timeout, false);
    cm.partly_match(partly_result, tm, *channel_rules);

    PartlyMatchResult etalon;
    etalon.from_to_matches.insert(std::make_pair(1, PartlyMatch(1, 1)));
    etalon.now_to_matches.insert(std::make_pair(1, PartlyMatch(1, 1)));

    if(!compare_partly_match_result(partly_result, etalon))
    {
      std::cerr << FUN << ": incorrect match result: " << std::endl;
      print_partly_match_result(std::cerr, partly_result);
      std::cerr << "instead: " << std::endl;
      print_partly_match_result(std::cerr, etalon);
      return 1;
    }
  }

  return 0;
}

int history_today_match_test()
{
  static const char* FUN = "history_today_match_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->today_long_intervals.insert(ChannelInterval(
      Generics::Time::ZERO, Generics::Time(172800), 3, 2));
    channel_rules->page_channels[1] = page;
    channel_rules->page_channels[2] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    ChannelIdPack cid;
    cid.page_channels.push_back(1);
    cid.page_channels.push_back(2);

    ChannelMatchMap result;

    ChannelsMatcher cm(base_profile.in(), add_profile.in());

    cm.match(result, Generics::Time(String::SubString("2006-10-10 10:10:10"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    {
      Generics::Time now(String::SubString("2006-10-11 10:10:20"), "%Y-%m-%d %H:%M:%S");
      if(cm.need_history_optimization(now, Generics::Time::ZERO,
        Generics::Time::ZERO))
      {
        cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
          *channel_rules);
      }
    }

    cm.match(result, Generics::Time(String::SubString("2006-10-11 10:10:20"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    {
      PartlyMatchResult partly_result;

      cm.partly_match(
        partly_result,
        Generics::Time(String::SubString("2006-10-10 10:10:30"),
          "%Y-%m-%d %H:%M:%S"),
        *channel_rules);

      PartlyMatchResult etalon;
      etalon.from_to_matches.insert(std::make_pair(1, PartlyMatch(2, 3)));
      etalon.from_to_matches.insert(std::make_pair(2, PartlyMatch(2, 3)));
      etalon.now_to_matches.insert(std::make_pair(1, PartlyMatch(2, 3)));
      etalon.now_to_matches.insert(std::make_pair(2, PartlyMatch(2, 3)));

      if(!compare_partly_match_result(partly_result, etalon))
      {
        std::cerr << FUN << ": incorrect match result: " << std::endl;
        print_partly_match_result(std::cerr, partly_result);
        std::cerr << "instead: " << std::endl;
        print_partly_match_result(std::cerr, etalon);
        return 1;
      }
    }

    cm.match(result, Generics::Time(String::SubString("2006-10-11 10:10:20"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();
    cm.match(result, Generics::Time(String::SubString("2006-10-11 10:10:20"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    {
      PartlyMatchResult partly_result;

      cm.partly_match(
        partly_result,
        Generics::Time(String::SubString("2006-10-10 10:10:30"),
          "%Y-%m-%d %H:%M:%S"),
        *channel_rules);

      PartlyMatchResult etalon;
      etalon.from_to_matches.insert(std::make_pair(1, PartlyMatch(4, 3)));
      etalon.from_to_matches.insert(std::make_pair(2, PartlyMatch(4, 3)));
      etalon.now_to_matches.insert(std::make_pair(1, PartlyMatch(4, 3)));
      etalon.now_to_matches.insert(std::make_pair(2, PartlyMatch(4, 3)));

      if(!compare_partly_match_result(partly_result, etalon))
      {
        std::cerr << FUN << ": incorrect match result (2): " << std::endl;
        print_partly_match_result(std::cerr, partly_result);
        std::cerr << "instead: " << std::endl;
        print_partly_match_result(std::cerr, etalon);
        return 1;
      }
    }
  }

  return 0;
}

int history_match_test()
{
  static const char* FUN = "history_match_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->long_intervals.insert(ChannelInterval(
      Generics::Time(86400), Generics::Time(259200), 1, 1));
    channel_rules->page_channels[1] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    ChannelIdPack cid;
    cid.page_channels.push_back(1);

    ChannelMatchMap result;

    ChannelsMatcher cm(base_profile.in(), add_profile.in());

    cm.match(result, Generics::Time(String::SubString("2010-05-20 04:01:00"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    PartlyMatchResult partly_result;

    {
      Generics::Time now(String::SubString("2010-05-22 04:01:00"), "%Y-%m-%d %H:%M:%S");


      if(cm.need_history_optimization(now, Generics::Time::ZERO,
        Generics::Time::ZERO))
      {
        cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
          *channel_rules);
      }

      ChannelIdPack empty_cid;
      cm.match(result, now, empty_cid, *channel_rules, pmp, pps, session_timeout, false);

      cm.partly_match(partly_result, now, *channel_rules);
    }

    PartlyMatchResult etalon;
    etalon.from_to_matches.insert(std::make_pair(1, PartlyMatch(1, 1)));
    etalon.now_to_matches.insert(std::make_pair(1, PartlyMatch(1, 1)));

    if(!compare_partly_match_result(partly_result, etalon))
    {
      std::cerr << FUN << ": incorrect match result: " << std::endl;
      print_partly_match_result(std::cerr, partly_result);
      std::cerr << "instead: " << std::endl;
      print_partly_match_result(std::cerr, etalon);
      return 1;
    }
  }

  return 0;
}

int main() noexcept
{
  int ret = 0;

  ret += session_match_test();
  ret += history_today_match_test();
  ret += history_match_test();

  return ret;
}
