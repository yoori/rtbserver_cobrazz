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
const Generics::Time history_period = Generics::Time(3600);
const Generics::Time session_timeout = Generics::Time(30);

ProfileProperties pps;
ProfileMatchParams pmp;

struct ChannelMatchEq
{
  bool operator()(
    const std::pair<unsigned long, unsigned long>& left,
    const std::pair<unsigned long, unsigned long>& right) const
  {
    return left.first == right.first &&
      left.second == right.second;
  }
};

bool compare_channel_match_maps(
  const ChannelMatchMap& left, const ChannelMatchMap& right)
{
  return left.size() == right.size() &&
    std::equal(left.begin(), left.end(), right.begin(), ChannelMatchEq());
}

void print_match_result(std::ostream& out, const ChannelMatchMap& result)
{
  for(ChannelMatchMap::const_iterator res_it = result.begin();
      res_it != result.end(); ++res_it)
  {
    out << "  " << res_it->first << " : " << res_it->second << std::endl;
  }
}

/*
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
*/

struct MatchWrapper
{
  MatchWrapper(const ChannelDictionary& dict_val, const char* test_name_val)
    : dict(dict_val),
      base_profile(new SmartMemBuf),
      add_profile(new SmartMemBuf),
      history_profile(new SmartMemBuf),
      cm(base_profile.in(), add_profile.in()),
      test_name(test_name_val),
      match_ind(0)
  {}

  int match(
    const Generics::Time& time,
    const ChannelMatchPack& cid,
    const ChannelMatchMap& etalon)
  {
    if (cm.need_history_optimization(time, history_period,
      Generics::Time::ZERO))
    {
      /*
      std::cout << "===>" << std::endl;
      cm.history_print(history_profile->membuf(), std::cout, "");
      std::cout << ">>>>" << std::endl;
      cm.print(base_profile->membuf(), std::cout, "");
      */
      cm.history_optimize(history_profile.in(), time,
        Generics::Time::ZERO, dict);
      /*
      std::cout << "<===" << std::endl;
      cm.history_print(history_profile->membuf(), std::cout, "");
      std::cout << "<<<<" << std::endl;
      cm.print(base_profile->membuf(), std::cout, "");
      */
    }

    ChannelMatchMap result;

    cm.match(result, time, cid, dict, pmp, pps, session_timeout, false);
/*    
    std::cout << "-------------- " << match_ind << std::endl;
    cm.print(base_profile->membuf(), std::cout, "");
    std::cout << "-------------- " << match_ind << std::endl;
*/    
    ++match_ind;

    if(!compare_channel_match_maps(result, etalon))
    {
      std::cerr << test_name << ": incorrect match result(" << match_ind << "): " << std::endl;
      print_match_result(std::cerr, result);
      if(etalon.empty())
      {
        std::cerr << "Instead empty" << std::endl;
      }
      else
      {
        std::cerr << "Instead:" << std::endl;
        print_match_result(std::cerr, etalon);
      }

      return 1;
    }

    return 0;
  }

  void print(std::ostream& out) const noexcept
  {
    cm.print(base_profile->membuf(), out, "  ");
  }

  const ChannelDictionary& dict;
  SmartMemBuf_var base_profile;
  SmartMemBuf_var add_profile;
  SmartMemBuf_var history_profile;
  ChannelsMatcher cm;
  std::string test_name;
  int match_ind;
};

int reverse_order_match_test()
{
  static const char* FUN = "reverse_order_match_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(60), 2, 2));
    channel_rules->page_channels[1] = page;
  }

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->long_intervals.insert(
      ChannelInterval(Generics::Time(86400), Generics::Time(2*86400), 2, 2));
    channel_rules->page_channels[2] = page;
  }

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->long_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(2*86400), 2, 2));
    channel_rules->page_channels[3] = page;
  }

  int res = 0;

  {
    ChannelMatchPack cid;
    cid.page_channels.push_back(2);

    MatchWrapper matcher(*channel_rules, FUN);

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 23:59:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-11 00:00:30"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    ChannelMatchMap etalon;
    //etalon[2] = 2;

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 10:00:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      etalon);
  }

  {
    ChannelMatchPack cid;
    cid.page_channels.push_back(2);

    MatchWrapper matcher(*channel_rules, FUN);

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 23:59:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-09 23:59:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    ChannelMatchMap etalon;
    etalon[2] = 2;

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-11 10:00:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      etalon);
  }

  {
    ChannelMatchPack cid;
    cid.page_channels.push_back(1);

    MatchWrapper matcher(*channel_rules, FUN);

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 23:59:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    ChannelMatchMap etalon;
    etalon[1] = 2;

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-09 23:59:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      etalon);
  }

  {
    ChannelMatchPack cid;
    cid.page_channels.push_back(3);

    MatchWrapper matcher(*channel_rules, FUN);

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 23:59:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 00:00:30"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    ChannelMatchMap etalon;
    etalon[3] = 2;

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-11 10:00:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      etalon);

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-05 23:59:40"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      etalon);
  }

  return res;
}

int persistent_match_test()
{
  static const char* FUN = "persistent_match_test()";

  pmp.provide_persistent_channels = true;

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  int res = 0;

  MatchWrapper matcher(*channel_rules, FUN);
  Generics::Time tm = Generics::Time::get_time_of_day();

  {
    ChannelMatchPack cid;
    cid.persistent_channels.push_back(1);
    cid.persistent_channels.push_back(3);

    ChannelMatchMap etalon;
    etalon[1] = 1;
    etalon[3] = 1;

    res += matcher.match(tm, cid, etalon);
  }

  {
    ChannelMatchPack cid;
    cid.persistent_channels.push_back(1);
    cid.persistent_channels.push_back(2);

    ChannelMatchMap etalon;
    etalon[1] = 1;
    etalon[2] = 1;
    etalon[3] = 1;

    res += matcher.match(tm, cid, etalon);
  }

  {
    ChannelMatchPack cid;
    cid.persistent_channels.push_back(3);
    cid.persistent_channels.push_back(4);
    cid.persistent_channels.push_back(6);

    ChannelMatchMap etalon;
    etalon[1] = 1;
    etalon[2] = 1;
    etalon[3] = 1;
    etalon[4] = 1;
    etalon[6] = 1;

    res += matcher.match(tm, cid, etalon);
  }

  tm += 86400;

  {
    ChannelMatchPack cid;

    ChannelMatchMap etalon;
    etalon[1] = 1;
    etalon[2] = 1;
    etalon[3] = 1;
    etalon[4] = 1;
    etalon[6] = 1;

    res += matcher.match(tm, cid, etalon);
  }

  {
    ChannelMatchPack cid;
    cid.persistent_channels.push_back(5);
    cid.persistent_channels.push_back(7);

    ChannelMatchMap etalon;
    etalon[5] = 1;
    etalon[7] = 1;

    MatchWrapper add_matcher(*channel_rules, FUN);
    res += add_matcher.match(tm, cid, etalon);

    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);
    SmartMemBuf_var add_history_profile(new SmartMemBuf);

    ChannelsMatcher cm(matcher.base_profile.in(), add_profile.in());

    cm.merge(
      history_profile.in(),
      add_matcher.base_profile->membuf(),
      add_history_profile->membuf(),
      *channel_rules,
      pmp);

    ChannelMatchPack empty_cid;
    etalon[1] = 1;
    etalon[2] = 1;
    etalon[3] = 1;
    etalon[4] = 1;
    etalon[5] = 1;
    etalon[6] = 1;
    etalon[7] = 1;

    res += matcher.match(tm, empty_cid, etalon);
  }

  return res;
}

int session_match_test()
{
  static const char* FUN = "session_match_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(
      
      ChannelInterval(Generics::Time(Generics::Time::ZERO), Generics::Time(Generics::Time(60)), 1, 2));
    channel_rules->page_channels[1] = page;
  }

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(
      ChannelInterval(Generics::Time(30), Generics::Time(120), 2, 2));
    channel_rules->page_channels[2] = page;
  }

  int res = 0;

  {
    ChannelMatchPack cid;
    cid.page_channels.push_back(1);

    MatchWrapper matcher(*channel_rules, FUN);

    ChannelMatchMap etalon;
    etalon[1] = 2;

    res += matcher.match(
      Generics::Time::get_time_of_day(),
      cid,
      etalon);
  }

  {
    ChannelMatchPack cid;
    cid.page_channels.push_back(2);

    MatchWrapper matcher(*channel_rules, FUN);

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 10:12:50"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 10:13:20"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      ChannelMatchMap());

    ChannelMatchMap etalon;
    etalon[2] = 2;

    res += matcher.match(
      Generics::Time(String::SubString("2006-10-10 10:14:00"),
        "%Y-%m-%d %H:%M:%S"),
      cid,
      etalon);
  }

  return res;
}

int session_match_test_2()
{
  static const char* FUN = "session_match_test_2()";

  ChannelIntervalsPack_var page = new ChannelIntervalsPack();
  page->short_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(7200), 2, 1));

  ChannelDictionary_var channel_rules(new ChannelDictionary);
  channel_rules->page_channels[1] = page;

  ChannelMatchPack cid;
  cid.page_channels.push_back(1);

  int res = 0;

  MatchWrapper matcher(*channel_rules, FUN);

  res += matcher.match(
    Generics::Time(String::SubString("2006-01-01 12:12:12"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  ChannelMatchMap etalon;
  etalon[1] = 1;

  res += matcher.match(
    Generics::Time(String::SubString("2006-01-01 14:12:10"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon);

  return res;
}

int session_match_test_3()
{
  static const char* FUN = "session_match_test_3()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(
      ChannelInterval(Generics::Time(30), Generics::Time(120), 2, 2));
    channel_rules->page_channels[1] = page;
  }

  ChannelMatchPack cid;
  cid.page_channels.push_back(1);

  MatchWrapper matcher(*channel_rules, FUN);

  int res = 0;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:10:10"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:10:20"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:10:30"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:10:40"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  ChannelMatchMap etalon;
  etalon[1] = 2;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:10:50"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon);

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:12:50"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:13:20"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:14:00"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon);

  return res;
}

int session_match_test_4()
{
  static const char* FUN = "session_match_test_4()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(300), 2, 2));
    channel_rules->page_channels[1] = page;
  }

  ChannelMatchPack cid;
  cid.page_channels.push_back(1);

  int res = 0;

  MatchWrapper matcher(*channel_rules, FUN);

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 23:59:10"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  ChannelMatchMap etalon;
  etalon[1] = 2;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 23:59:10"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon);

  return res;
}

int session_few_intervals_test()
{
  static const char* FUN = "session_few_intervals_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(
      ChannelInterval(Generics::Time(60), Generics::Time(180), 1, 5));
    page->short_intervals.insert(
      ChannelInterval(Generics::Time(120), Generics::Time(180), 2, 6));
    channel_rules->page_channels[1] = page;
  }

  ChannelMatchPack cid;
  cid.page_channels.push_back(1);

  int res = 0;
  MatchWrapper matcher(*channel_rules, FUN);

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 00:00:01"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 00:00:59"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  ChannelMatchMap etalon1;
  etalon1[1] = 5;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 00:02:00"),
      "%Y-%m-%d %H:%M:%S"),
    ChannelMatchPack(),
    etalon1);

  ChannelMatchMap etalon2;
  etalon2[1] = 11;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 00:03:00"),
      "%Y-%m-%d %H:%M:%S"),
    ChannelMatchPack(),
    etalon2);

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 00:04:00"),
      "%Y-%m-%d %H:%M:%S"),
    ChannelMatchPack(),
    ChannelMatchMap());

  return res;
}

int session_delete_excess_test()
{
  static const char* FUN = "session_delete_excess_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(
      ChannelInterval(Generics::Time(30), Generics::Time(120), 2, 2));
    channel_rules->page_channels[1] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    ChannelMatchPack cid;
    cid.page_channels.push_back(1);

    ChannelMatchMap result;

    ChannelsMatcher cm(base_profile.in(), add_profile.in());

    cm.match(result, Generics::Time(String::SubString("2006-10-10 10:10:10"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    cm.match(result, Generics::Time(String::SubString("2006-10-10 10:10:11"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    cm.match(result, Generics::Time(String::SubString("2006-10-10 10:10:12"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    cm.match(result, Generics::Time(String::SubString("2006-10-10 10:10:13"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    cm.match(result, Generics::Time(String::SubString("2006-10-10 10:10:14"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    cm.match(result, Generics::Time(String::SubString("2006-10-10 10:10:50"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);

    if(result.size() != 1 || !(
         result.begin()->first == 1 &&
         result.begin()->second == 2))
    {
      std::cerr << FUN << ": (1) incorrect match result: " << std::endl;
      print_match_result(std::cerr, result);
      std::cerr << "Instead 1 : 2" << std::endl;
      return 1;
    }

    /*
    // check that excess session visits cleared with using partly match
    PartlyMatchResult partly_result;
    cm.partly_match(
      partly_result,
      Generics::Time(String::SubString("2006-10-10 10:10:50"),
        "%Y-%m-%d %H:%M:%S"),
      *channel_rules);

    if(partly_result.from_to_matches.size() != 1 || !(
         partly_result.from_to_matches.begin()->first == 1 &&
         partly_result.from_to_matches.begin()->second.visits == 4 &&
         partly_result.from_to_matches.begin()->second.minimum_visits == 2))
    {
      std::cerr << FUN << ": (1) incorrect partly match result: " << std::endl;
      print_partly_match_result(std::cerr, partly_result);
      return 1;
    }
    */
  }

  return 0;
}

int history_today_match_test()
{
  static const char* FUN = "history_today_match_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->today_long_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(172800), 3, 2));
    channel_rules->page_channels[1] = page;
    channel_rules->page_channels[2] = page;
  }

  ChannelMatchPack cid;
  cid.page_channels.push_back(1);
  cid.page_channels.push_back(2);

  MatchWrapper matcher(*channel_rules, FUN);

  int res = 0;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:10:20"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 10:10:20"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-11 10:10:20"),
      "%Y-%m-%d %H:%M:%S"),
    ChannelMatchPack(),
    ChannelMatchMap());

  ChannelMatchMap etalon;
  etalon[1] = 2;
  etalon[2] = 2;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-11 10:10:30"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon);

  return res;
}

int history_today_few_intervals_test()
{
  static const char* FUN = "history_today_few_intervals_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->today_long_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(24*60*60), 1, 1));
    page->today_long_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(2*24*60*60), 2, 2));
    page->today_long_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(4*24*60*60), 4, 4));
    channel_rules->page_channels[1] = page;
  }

  ChannelMatchPack cid;
  cid.page_channels.push_back(1);

  int res = 0;
  MatchWrapper matcher(*channel_rules, FUN);

  ChannelMatchMap etalon1;
  etalon1[1] = 1;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-10 00:00:01"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon1);

  ChannelMatchMap etalon2;
  etalon2[1] = 3;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-11 00:00:59"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon2);

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-11 00:00:59"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon2);

  ChannelMatchMap etalon3;
  etalon3[1] = 7;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-11 00:00:59"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon3);

  ChannelMatchMap etalon4;
  etalon4[1] = 1 + 4;

  res += matcher.match(
    Generics::Time(String::SubString("2006-10-14 00:00:59"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon4);

  return res;
}

int history_match_test()
{
  static const char* FUN = "history_match_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->long_intervals.insert(
      ChannelInterval(Generics::Time(86400), Generics::Time(259200), 1, 1));
    channel_rules->page_channels[1] = page;
  }

  SmartMemBuf_var base_profile(new SmartMemBuf);
  SmartMemBuf_var add_profile(new SmartMemBuf);
  SmartMemBuf_var history_profile(new SmartMemBuf);

  ChannelMatchPack cid;
  cid.page_channels.push_back(1);

  MatchWrapper matcher(*channel_rules, FUN);

  int res = 0;

  res += matcher.match(
    Generics::Time(String::SubString("2010-05-20 04:01:00"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    ChannelMatchMap());

  ChannelMatchMap etalon;
  etalon[1] = 1;

  res += matcher.match(
    Generics::Time(String::SubString("2010-05-22 04:01:00"),
      "%Y-%m-%d %H:%M:%S"),
    cid,
    etalon);

  return res;
}

int history_match_test_2()
{
  static const char* FUN = "history_match_test_2()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->long_intervals.insert(
      ChannelInterval(Generics::Time(86400), Generics::Time(259200), 2, 1));
    channel_rules->page_channels[1] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    ChannelMatchPack cid;
    cid.page_channels.push_back(1);

    ChannelMatchMap result;

    ChannelsMatcher cm(base_profile.in(), add_profile.in());

    cm.match(result, Generics::Time(String::SubString("2010-05-20 04:01:00"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps, session_timeout, false);
    result.clear();

    {
      Generics::Time now(String::SubString("2010-05-21 04:01:00"), "%Y-%m-%d %H:%M:%S");

      if (cm.need_history_optimization(now, history_period,
        Generics::Time::ZERO))
      {
        cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
          *channel_rules);
      }

//    cm.history_print(history_profile->membuf(), std::cout, "");

      cm.match(result, now, cid, *channel_rules, pmp, pps,
        session_timeout, false);

      result.clear();
    }

    {
      Generics::Time now(String::SubString("2010-05-22 04:01:00"), "%Y-%m-%d %H:%M:%S");

      if (cm.need_history_optimization(now, history_period,
        Generics::Time::ZERO))
      {
        cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
          *channel_rules);
      }

//    cm.history_print(history_profile->membuf(), std::cout, "");

      cm.match(result, now, ChannelMatchPack(), *channel_rules, pmp, pps,
        session_timeout, false);
    }

    if(result.size() != 1 || !(
         result.begin()->first == 1 &&
         result.begin()->second == 1))
    {
      std::cerr << FUN << ": incorrect match result: " << std::endl;
      print_match_result(std::cerr, result);
      std::cerr << "Instead 1 : 1" << std::endl;
      return 1;
    }
  }

  return 0;
}

int history_match_test_3()
{
  static const char* FUN = "history_match_test_3()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->long_intervals.insert(
      ChannelInterval(Generics::Time(86400), Generics::Time(86400*5), 2, 1));
    channel_rules->page_channels[1] = page;
  }

  {
    SmartMemBuf_var base_profile(new SmartMemBuf);
    SmartMemBuf_var add_profile(new SmartMemBuf);
    SmartMemBuf_var history_profile(new SmartMemBuf);

    ChannelMatchPack cid;
    cid.page_channels.push_back(1);

    ChannelMatchMap result;

    ChannelsMatcher cm(base_profile.in(), add_profile.in());

    cm.match(result, Generics::Time(String::SubString("2010-05-20 04:01:00"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps,
             session_timeout, false);
    result.clear();

    {
      Generics::Time now(String::SubString("2010-05-21 04:01:00"), "%Y-%m-%d %H:%M:%S");

      if (cm.need_history_optimization(now, history_period,
        Generics::Time::ZERO))
      {
        cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
          *channel_rules);
      }

      //cm.history_print(history_profile->membuf(), std::cout, "");

      cm.match(result, now, cid, *channel_rules, pmp, pps,
        session_timeout, false);
    }

    {
      Generics::Time now(String::SubString("2010-05-23 04:01:00"), "%Y-%m-%d %H:%M:%S");

      if (cm.need_history_optimization(now, history_period,
        Generics::Time::ZERO))
      {
        cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
          *channel_rules);
      }

      //cm.history_print(history_profile->membuf(), std::cout, "");

      cm.match(result, now, ChannelMatchPack(), *channel_rules, pmp, pps,
        session_timeout, false);

      result.clear();
    }

    {
      {
        ChannelIntervalsPack_var page = new ChannelIntervalsPack();
        page->long_intervals.insert(
      ChannelInterval(Generics::Time(86400), Generics::Time(86400*3), 2, 1));
        channel_rules->page_channels[1] = page;
      }

      Generics::Time now(String::SubString("2010-05-23 05:02:00"), "%Y-%m-%d %H:%M:%S");

      if (cm.need_history_optimization(now, history_period,
        Generics::Time::ZERO))
      {
        cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
          *channel_rules);
      }

      //cm.history_print(history_profile->membuf(), std::cout, "");

      cm.match(result, now, ChannelMatchPack(), *channel_rules, pmp, pps,
        session_timeout, false);
    }

    if(result.size() != 0)
    {
      std::cerr << FUN << ": incorrect match result: " << std::endl;
      print_match_result(std::cerr, result);
      std::cerr << "Instead 1 : 1" << std::endl;
      return 1;
    }
  }

  return 0;
}

int merge_test()
{
  static const char* FUN = "merge_test()";

  int res = 0;

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->today_long_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(86400*3), 2, 1));
    channel_rules->page_channels[1] = page;
  }

  SmartMemBuf_var base_profile(new SmartMemBuf);
  SmartMemBuf_var add_profile(new SmartMemBuf);
  SmartMemBuf_var history_profile(new SmartMemBuf);

  SmartMemBuf_var base_profile2(new SmartMemBuf);
  SmartMemBuf_var add_profile2(new SmartMemBuf);
  SmartMemBuf_var history_profile2(new SmartMemBuf);

  {
    ChannelMatchPack cid;
    cid.page_channels.push_back(1);

    ChannelMatchMap result;

    ChannelsMatcher cm(base_profile.in(), add_profile.in());

    cm.match(result, Generics::Time(String::SubString("2006-10-10 00:00:10"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps,
             session_timeout, false);
    result.clear();

    cm.match(result, Generics::Time(String::SubString("2006-10-10 00:00:10"),
      "%Y-%m-%d %H:%M:%S"),
             cid, *channel_rules, pmp, pps,
             session_timeout, false);

    if(result.size() != 1 &&
       !(result.begin()->first == 1 &&
         result.begin()->second == 1))
    {
      std::cerr << FUN << ": incorrect match result: " << std::endl;
      print_match_result(std::cerr, result);
      std::cerr << "Instead 1 : 1" << std::endl;
      return 1;
    }
  }

  {
    ChannelsMatcher cm2(base_profile2.in(), add_profile2.in());

    ChannelMatchPack cid;
    cid.page_channels.push_back(1);

    ChannelMatchMap result;

    cm2.match(result, Generics::Time(String::SubString("2006-10-11 00:00:10"),
      "%Y-%m-%d %H:%M:%S"),
              cid, *channel_rules, pmp, pps,
              session_timeout, false);

    if(!result.empty())
    {
      std::cerr << FUN << ": incorrect match result: " << std::endl;
      print_match_result(std::cerr, result);
      std::cerr << "Instead 1 : 1" << std::endl;
      return 1;
    }
  }

  {
    Generics::Time now(String::SubString("2006-10-12 04:01:00"), "%Y-%m-%d %H:%M:%S");

    ChannelsMatcher cm(base_profile.in(), add_profile.in());
    ChannelsMatcher cm2(base_profile2.in(), add_profile2.in());

    if (cm.need_history_optimization(now, history_period,
      Generics::Time::ZERO))
    {
      cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
        *channel_rules);
    }

    if(cm2.need_history_optimization(now, history_period, Generics::Time::ZERO))
    {
      cm2.history_optimize(history_profile2.in(), now, Generics::Time::ZERO,
        *channel_rules);
    }

    /*
    std::cout << "Source history profile: " << std::endl;
    cm.history_print(history_profile->membuf(), std::cout, "  ");
    std::cout << "Target history profile: " << std::endl;
    cm.history_print(history_profile2->membuf(), std::cout, "  ");
    */

    cm2.merge(
      history_profile2.in(),
      base_profile->membuf(),
      history_profile->membuf(),
      *channel_rules,
      pmp);

    ChannelMatchMap result;

    cm.match(result, now, ChannelMatchPack(), *channel_rules, pmp, pps,
      session_timeout, false);

    if(result.size() != 1 &&
       !(result.begin()->first == 1 &&
         result.begin()->second == 1))
    {
      std::cerr << FUN << ": incorrect match result: " << std::endl;
      print_match_result(std::cerr, result);
      std::cerr << "Instead 1 : 1" << std::endl;
      res = 1;
    }

    HistoryUserProfileReader hrdr(
      history_profile->membuf().data(),
      history_profile->membuf().size());

    if (hrdr.minor_version() != CURRENT_HISTORY_MINOR_PROFILE_VERSION ||
        hrdr.major_version() != CURRENT_HISTORY_MAJOR_PROFILE_VERSION)
    {
      std::cerr << FUN << ": incorrect merge result: " << std::endl;
      std::cerr << "Incorrect history profile version after merging." << std::endl;

      res = 1;
    }
  }

  return res;
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
    page->short_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(60), 1, 2));
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
    Generics::CPUTimer cpu_timer;
    timer.start();
    cpu_timer.start();

    for(unsigned long iteration = 0; iteration < ITERATION_COUNT; ++iteration)
    {
      ChannelMatchMap result;
      ChannelsMatcher cm(base_profile.in(), add_profile.in());
      cm.match(result, tm, cid, *channel_rules, pmp, pps,
        session_timeout, false);
    }

    cpu_timer.stop();
    timer.stop();

    std::cout << FUN << ": "
      "cpu time = " << cpu_timer.elapsed_time() <<
      ", sum time = " << timer.elapsed_time() <<
      ", avg time = " << timer.elapsed_time() / ITERATION_COUNT <<
      " for " << CHANNEL_NUMBER << " channels." << std::endl;
  }

  return 0;
}

int unique_channels_test()
{
  static const char* FUN = "unique_channels_test()";

  ChannelDictionary_var channel_rules(new ChannelDictionary);

  for(unsigned long i = 1; i < 11; ++i)
  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->short_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(60), 1, 2));
   ChannelFeatures cf(i%2 == 0, 0);
    channel_rules->page_channels[i] = page;
    channel_rules->channel_features.insert(
      std::make_pair(i, ChannelFeatures(i%2 == 0, 0)));
  }

  for(unsigned long i = 11; i < 21; ++i)
  {
    ChannelIntervalsPack_var search = new ChannelIntervalsPack();
    search->short_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(60), 2, 2));
    
    channel_rules->search_channels[i] = search;
     channel_rules->channel_features.insert(
       std::make_pair(i, ChannelFeatures(i%2 == 0, 0)));
  }

  for(unsigned long i = 15; i < 25; ++i)
  {
    ChannelIntervalsPack_var url = new ChannelIntervalsPack();
    url->short_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(60), 3, 2));

    channel_rules->url_channels[i] = url;
    channel_rules->channel_features.insert(
      std::make_pair(i, ChannelFeatures(i%2 == 0, 0)));
  }

  for(unsigned long i = 5; i < 15; ++i)
  {
    ChannelIntervalsPack_var search = new ChannelIntervalsPack();
    search->today_long_intervals.insert(
      ChannelInterval(Generics::Time::ZERO, Generics::Time(86400), 3, 2));

    channel_rules->search_channels[i] = search;
    channel_rules->channel_features.insert(
      std::make_pair(i, ChannelFeatures(i%2 == 0, 0)));   
  }

  for(unsigned long i = 8; i < 28; ++i)
  {
    ChannelIntervalsPack_var page = new ChannelIntervalsPack();
    page->long_intervals.insert(
      ChannelInterval(Generics::Time(86400), Generics::Time(3*86400), 3, 2));

    channel_rules->page_channels[i] = page;
    channel_rules->channel_features.insert(
      std::make_pair(i, ChannelFeatures(i%2 == 0, 0)));
  }

  SmartMemBuf_var base_profile(new SmartMemBuf);
  SmartMemBuf_var add_profile(new SmartMemBuf);
  SmartMemBuf_var history_profile(new SmartMemBuf);

  ChannelMatchPack cid;
  for(unsigned long channel_i = 0; channel_i < 30; ++channel_i)
  {
    cid.page_channels.push_back(channel_i);
    cid.search_channels.push_back(channel_i);
    cid.url_channels.push_back(channel_i);
  }

  Generics::Time now(String::SubString("2006-10-11 04:01:00"), "%Y-%m-%d %H:%M:%S");

  ChannelsMatcher cm(base_profile.in(), add_profile.in());
  ChannelMatchMap result;

  cm.match(result, now, cid, *channel_rules, pmp, pps,
    session_timeout, false);
  result.clear();

  now += 86400;

  ChannelMatchPack tcid;
  for(unsigned long channel_i = 0; channel_i < 30; ++channel_i)
  {
    tcid.page_channels.push_back(3*channel_i);
    tcid.search_channels.push_back(3*channel_i);
    tcid.url_channels.push_back(3*channel_i);
  }

  if (cm.need_history_optimization(now, history_period,
    Generics::Time::ZERO))
  {
    cm.history_optimize(history_profile.in(), now, Generics::Time::ZERO,
      *channel_rules);
  }

  cm.match(result, now, tcid, *channel_rules, pmp, pps,
    session_timeout, false);

  UniqueChannelsResult ucr;
  cm.unique_channels(
    base_profile->membuf(),
    &history_profile->membuf(),
    *channel_rules,
    ucr);
/*
  cm.history_print(history_profile->membuf(), std::cout, "");
  cm.print(base_profile->membuf(), std::cout, "");

  std::cout << "Simple channels: " << ucr.simple_channels << ", "
            << ucr.session_simple_channels << ", "
            << ucr.history_simple_channels << "."
            << std::endl;

  std::cout << "Discover channels: " << ucr.discover_channels << ", "
            << ucr.session_discover_channels << ", "
            << ucr.history_discover_channels << "."
            << std::endl;
*/

  UniqueChannelsResult etalon;
  etalon.simple_channels = 10;
  etalon.history_simple_channels = 10;
  etalon.discover_channels = 11;
  etalon.history_discover_channels = 11;

  if (ucr != etalon)
  {
    std::cerr << FUN << ": incorrect test result: " << std::endl;
    std::cerr << "Result: (" << ucr.simple_channels << ", "
              << ucr.history_simple_channels << ", "
              << ucr.discover_channels << ", "
              << ucr.history_discover_channels << ")" << std::endl
              << "instead of etalon: ("
              << etalon.simple_channels << ", "
              << etalon.history_simple_channels << ", "
              << etalon.discover_channels << ", "
              << etalon.history_discover_channels << ")" << std::endl;
    return 1;
  }

  return 0;
}

int main() noexcept
{
  int ret = 0;

  pmp.filter_contextual_triggers = false;
  pmp.repeat_trigger_timeout = Generics::Time::ZERO;
  
  ret += persistent_match_test();
  ret += session_match_test();
  ret += session_match_test_2();
  ret += session_match_test_3();
  ret += session_match_test_4();
  ret += session_few_intervals_test();
  ret += session_delete_excess_test();

  ret += history_today_match_test();
  ret += history_today_few_intervals_test();

  ret += history_match_test();
  ret += history_match_test_2();
  ret += history_match_test_3();

  ret += merge_test();

  ret += reverse_order_match_test();

  ret += unique_channels_test();

  ret += session_match_performance_test();

  return ret;
}
