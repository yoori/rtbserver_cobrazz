#include <list>
#include <iostream>

#include <Commons/Algs.hpp>
#include <UserInfoSvcs/UserInfoCommons/ChannelDictionary.hpp>
#include <UserInfoSvcs/UserInfoCommons/ChannelMatcher.hpp>

using namespace AdServer::UserInfoSvcs;

typedef SessionMatchesWriter::timestamps_Container TimestampList;

int main()
{
  int ret = 0;
  
  {
    ChannelIntervalList channel_intervals;
    channel_intervals.insert(ChannelInterval(
      Generics::Time(0), Generics::Time(10), 1, 2));
    
    TimestampList timestamps;
    timestamps.push_back(1);
    timestamps.push_back(2);
    timestamps.push_back(2);
    timestamps.push_back(2);
    timestamps.push_back(3);
    timestamps.push_back(4);
    timestamps.push_back(10);
    ChannelsMatcher::delete_excess_timestamps_(timestamps, channel_intervals);

    TimestampList etalon;
    etalon.push_back(1);
    etalon.push_back(2);
    etalon.push_back(4);
    etalon.push_back(10);
    
    if(timestamps.size() != etalon.size() || !std::equal(timestamps.begin(), timestamps.end(), etalon.begin()))
    {
      std::cerr << "1. incorrect result:" << std::endl << "  ";
      Algs::print(std::cerr, timestamps.begin(), timestamps.end());
      std::cerr << std::endl << "instead:" << std::endl << "  ";
      Algs::print(std::cerr, etalon.begin(), etalon.end());
      std::cerr << std::endl;
      ret += 1;
    }
  }
  
  {
    ChannelIntervalList channel_intervals;
    channel_intervals.insert(ChannelInterval(
      Generics::Time(10), Generics::Time(20), 1, 2));
    
    TimestampList timestamps;
    timestamps.push_back(1);
    timestamps.push_back(4);
    timestamps.push_back(4);
    timestamps.push_back(4);
    timestamps.push_back(10);
    ChannelsMatcher::delete_excess_timestamps_(timestamps, channel_intervals);

    TimestampList etalon;
    etalon.push_back(1);
    etalon.push_back(4);
    etalon.push_back(4);
    etalon.push_back(10);
    
    if(timestamps.size() != etalon.size() || !std::equal(timestamps.begin(), timestamps.end(), etalon.begin()))
    {
      std::cerr << "2. incorrect result:" << std::endl << "  ";
      Algs::print(std::cerr, timestamps.begin(), timestamps.end());
      std::cerr << std::endl << "instead:" << std::endl << "  ";
      Algs::print(std::cerr, etalon.begin(), etalon.end());
      std::cerr << std::endl;
      ret += 1;
    }
  }

  {
    ChannelIntervalList channel_intervals;
    channel_intervals.insert(ChannelInterval(
      Generics::Time(10), Generics::Time(20), 1, 2));
    
    TimestampList timestamps;
    timestamps.push_back(1);
    timestamps.push_back(4);
    timestamps.push_back(4);
    timestamps.push_back(4);
    timestamps.push_back(12);
    ChannelsMatcher::delete_excess_timestamps_(timestamps, channel_intervals);

    TimestampList etalon;
    etalon.push_back(1);
    etalon.push_back(4);
    etalon.push_back(4);
    etalon.push_back(4);
    etalon.push_back(12);
    
    if(timestamps.size() != etalon.size() || !std::equal(timestamps.begin(), timestamps.end(), etalon.begin()))
    {
      std::cerr << "3. incorrect result:" << std::endl << "  ";
      Algs::print(std::cerr, timestamps.begin(), timestamps.end());
      std::cerr << std::endl << "instead:" << std::endl << "  ";
      Algs::print(std::cerr, etalon.begin(), etalon.end());
      std::cerr << std::endl;
      ret += 1;
    }
  }
  
  {
    ChannelIntervalList channel_intervals;
    channel_intervals.insert(ChannelInterval(
      Generics::Time(10), Generics::Time(20), 1, 2));
    
    TimestampList timestamps;
    timestamps.push_back(1);
    timestamps.push_back(3);
    timestamps.push_back(3);
    timestamps.push_back(3);
    timestamps.push_back(4);
    timestamps.push_back(14);
    timestamps.push_back(16);
    timestamps.push_back(16);
    timestamps.push_back(16);
    timestamps.push_back(17);
    
    ChannelsMatcher::delete_excess_timestamps_(timestamps, channel_intervals);

    TimestampList etalon;
    etalon.push_back(1);
    etalon.push_back(3);
    etalon.push_back(3);
    etalon.push_back(4);
    etalon.push_back(14);
    etalon.push_back(16);
    etalon.push_back(16);
    etalon.push_back(17);
    
    if(timestamps.size() != etalon.size() || !std::equal(timestamps.begin(), timestamps.end(), etalon.begin()))
    {
      std::cerr << "4. incorrect result:" << std::endl << "  ";
      Algs::print(std::cerr, timestamps.begin(), timestamps.end());
      std::cerr << std::endl << "instead:" << std::endl << "  ";
      Algs::print(std::cerr, etalon.begin(), etalon.end());
      std::cerr << std::endl;
      ret += 1;
    }
  }

  {
    ChannelIntervalList channel_intervals;
    channel_intervals.insert(ChannelInterval(
      Generics::Time(10), Generics::Time(30), 2, 2));
    
    TimestampList timestamps;
    timestamps.push_back(0);
    timestamps.push_back(1);
    timestamps.push_back(1);
    timestamps.push_back(2);
    timestamps.push_back(2);
    timestamps.push_back(2);
    timestamps.push_back(2);
    timestamps.push_back(3);
    timestamps.push_back(3);
    timestamps.push_back(4);
    timestamps.push_back(40);
    
    ChannelsMatcher::delete_excess_timestamps_(timestamps, channel_intervals);

    TimestampList etalon;
    etalon.push_back(0);
    etalon.push_back(1);
    etalon.push_back(1);
    etalon.push_back(2);
    etalon.push_back(2);
    etalon.push_back(3);
    etalon.push_back(3);
    etalon.push_back(4);
    etalon.push_back(40);
    
    if(timestamps.size() != etalon.size() || !std::equal(timestamps.begin(), timestamps.end(), etalon.begin()))
    {
      std::cerr << "5. incorrect result:" << std::endl << "  ";
      Algs::print(std::cerr, timestamps.begin(), timestamps.end());
      std::cerr << std::endl << "instead:" << std::endl << "  ";
      Algs::print(std::cerr, etalon.begin(), etalon.end());
      std::cerr << std::endl;
      ret += 1;
    }
  }

  {
    ChannelIntervalList channel_intervals;
    
    TimestampList timestamps;
    timestamps.push_back(0);
    timestamps.push_back(1);
    timestamps.push_back(1);
    timestamps.push_back(2);
    
    ChannelsMatcher::delete_excess_timestamps_(timestamps, channel_intervals);

    TimestampList etalon;
    
    if(timestamps.size() != etalon.size() || !std::equal(timestamps.begin(), timestamps.end(), etalon.begin()))
    {
      std::cerr << "6. incorrect result:" << std::endl << "  ";
      Algs::print(std::cerr, timestamps.begin(), timestamps.end());
      std::cerr << std::endl << "instead:" << std::endl << "  ";
      Algs::print(std::cerr, etalon.begin(), etalon.end());
      std::cerr << std::endl;
      ret += 1;
    }
  }

  {
    const unsigned long TS_ARRAY[] = {
      1275462988,
      1275462993,
      1275463103,
      1275463108,
      1275463113,
      1275463118,
      1275463123
    };
    
    const unsigned long TS_ETALON_ARRAY[] = {
      1275462988,
      1275462993,
      1275463103,
      1275463108,
      1275463118,
      1275463123
    };

    ChannelIntervalList channel_intervals;
    channel_intervals.insert(ChannelInterval(
      Generics::Time(0), Generics::Time(120), 1, 1));

    TimestampList timestamps;
    std::copy(
      TS_ARRAY,
      TS_ARRAY + sizeof(TS_ARRAY) / sizeof(TS_ARRAY[0]),
      std::back_inserter(timestamps));

    /*
    std::cout << ">>>> to clear ts list:" << std::endl;
    for(TimestampList::const_iterator tit = timestamps.begin();
        tit != timestamps.end(); ++tit)
    {
      std::cout << Generics::Time(*tit).get_gm_time() << std::endl;
    }
    std::cout << "<<<<" << std::endl;
    */
    ChannelsMatcher::delete_excess_timestamps_(timestamps, channel_intervals);

    TimestampList etalon;
    std::copy(
      TS_ETALON_ARRAY,
      TS_ETALON_ARRAY + sizeof(TS_ETALON_ARRAY) / sizeof(TS_ETALON_ARRAY[0]),
      std::back_inserter(etalon));
    
    if(timestamps.size() != etalon.size() ||
       !std::equal(timestamps.begin(), timestamps.end(), etalon.begin()))
    {
      std::cerr << "7. incorrect result:" << std::endl << "  ";
      Algs::print(std::cerr, timestamps.begin(), timestamps.end());
      std::cerr << std::endl << "instead:" << std::endl << "  ";
      Algs::print(std::cerr, etalon.begin(), etalon.end());
      std::cerr << std::endl;
      ret += 1;
    }
  }

  return ret;
}
