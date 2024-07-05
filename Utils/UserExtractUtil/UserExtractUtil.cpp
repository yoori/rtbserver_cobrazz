#include "UserExtractUtil.hpp"

#include <set>
#include <optional>

#include <Generics/AppUtils.hpp>
#include <String/Tokenizer.hpp>

#include <Commons/Algs.hpp>
#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <ProfilingCommons/ProfileMap/AdaptProfileMap.hpp>
#include <ProfilingCommons/PlainStorage3/LevelProfileMap.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserChannelInventoryProfileAdapter.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>
#include <TestCommons/ActiveObjectCallback.hpp>

namespace
{
  const char USAGE[] =
    "UserExtractUtil <OPTIONS>\n"
    "OPTIONS:\n"
    "  -h, --help : show this message.\n"
    "  --path : path to storage.\n"
    "  --channels : channels whitelist.\n"
    "  --min-req-time : the min request time for user filter.\n"
    "  --date : the day for collection of the data, if not set the last day section will be used.\n"
    "  --verbose : verbose output.\n"
    "Example:\n"
    "  UserExtractUtil --path=/tmp/cache/ --channels=1,2,3,4,5 --min-req-time=2015-10-13 --date=2015-10-23\n"
    "  out:\n"
    "  user_id1<tab>1,3,5\n"
    "  ...\n"
    "  user_idN<tab>1,2\n";

  typedef AdServer::ProfilingCommons::ProfileMap<AdServer::Commons::UserId> ProfileMap;

  typedef AdServer::ProfilingCommons::LevelProfileMap<
    AdServer::Commons::UserId, AdServer::ProfilingCommons::UserIdAccessor> BaseProfileMap;

  typedef AdServer::ProfilingCommons::ReadFileLevel<
    AdServer::Commons::UserId, AdServer::ProfilingCommons::UserIdAccessor> ReadFileLevel;

  class TimeOption : public Generics::AppUtils::Option<Generics::Time>
  {
  public:
    void
    set(
      const char*,
      const char* val)
      /*throw(Generics::AppUtils::InvalidParam)*/
    {
      try
      {
        Generics::Time ts;
        ts.set_f(String::SubString(val));
        set_value(ts);
      }
      catch (eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FNS << "Bad value '" << val << "'";
        throw Generics::AppUtils::InvalidParam(ostr);
      }
    }
  };

  class ChannelsOption : public Generics::AppUtils::OptionsSet<std::vector<unsigned long> >
  {
  public:
    void
    set(
      const char* opt_name,
      const char* val)
      /*throw(Generics::AppUtils::InvalidParam)*/
    {
      const String::SubString str(val);
      String::StringManip::SplitComma tokenizer(str);

      if (tokenizer.is_error())
      {
        Stream::Error ostr;
        ostr << FNS << "Bad value '" << val << "'";
        throw Generics::AppUtils::InvalidParam(ostr);
      }

      String::SubString token;

      while (tokenizer.get_token(token))
      {
        Generics::AppUtils::OptionsSet<std::vector<unsigned long> >::set(
          opt_name, token.str().c_str());
      }
    }
  };
}

void
print_channels(
  const AdServer::Commons::UserId& user_id,
  const std::set<unsigned long>& channel_whitelist,
  const AdServer::RequestInfoSvcs::ChannelInventoryDayReader& day,
  std::ostream& os)
  /*throw(eh::Exception)*/
{
  std::set<unsigned long> channels;

  const auto total_channel_list = day.total_channel_list();

  std::set_intersection(
    total_channel_list.begin(),
    total_channel_list.end(),
    channel_whitelist.begin(),
    channel_whitelist.end(),
    std::inserter(channels, channels.end()));

  if (!channels.empty())
  {
    os << user_id << '\t';
    Algs::print(os, channels.begin(), channels.end(), ",");
    os << '\n';
  }
}

void
extract_data(
  const Generics::Time& min_req_time,
  const std::optional<Generics::Time>& day,
  const std::set<unsigned long>& channel_whitelist,
  ProfileMap* profile_map_ptr,
  std::ostream& os,
  bool verbose = false)
  /*throw(eh::Exception)*/
{
  ReferenceCounting::SmartPtr<ProfileMap> profile_map =
    ReferenceCounting::add_ref(profile_map_ptr);

  ProfileMap::KeyList keys;
  profile_map->copy_keys(keys);

  if (verbose && keys.empty())
  {
    std::cerr << "WARNING: storage is empty" << std::endl;
  }

  std::size_t day_not_found = 0;
  std::size_t skipped_by_min_request_time_count = 0;

  for (auto it = keys.begin(); it != keys.end(); ++it)
  {
    Generics::ConstSmartMemBuf_var mem_buf = profile_map->get_profile(*it);

    if (mem_buf)
    {
      const AdServer::RequestInfoSvcs::UserChannelInventoryProfileReader
        profile_reader(mem_buf->membuf().data(), mem_buf->membuf().size());

      if (profile_reader.last_request_time() >= min_req_time.tv_sec)
      {
        const auto days = profile_reader.days();

        if (day)
        {
          auto day_it = days.begin();

          while (day_it != days.end())
          {
            if ((*day_it).date() == day->tv_sec)
            {
              break;
            }

            ++day_it;
          }

          if (day_it != days.end())
          {
            print_channels(*it, channel_whitelist, *day_it, os);
          }
          else
          {
            ++day_not_found;
          }
        }
        else if (!days.empty())
        {
          print_channels(*it, channel_whitelist, days.back(), os);
        }
        else
        {
          ++day_not_found;
        }
      }
      else
      {
        ++skipped_by_min_request_time_count;
      }
    }
  }

  if (verbose)
  {
    std::cerr << "INFO: day_not_found = " << day_not_found << std::endl;
    std::cerr << "INFO: skipped_by_min_request_time_count = " << skipped_by_min_request_time_count << std::endl;
  }
}

int
main(
  int argc,
  char** argv)
{
  try
  {
    using namespace Generics::AppUtils;

    CheckOption opt_help;
    StringOption path;
    ChannelsOption channel_whitelist;
    TimeOption min_req_time;
    TimeOption day;
    CheckOption verbose;

    Args args(-1);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.add(equal_name("path"), path);
    args.add(equal_name("channels"), channel_whitelist);
    args.add(equal_name("min-req-time"), min_req_time);
    args.add(equal_name("date"), day);
    args.add(equal_name("verbose"), verbose);

    args.parse(argc - 1, argv + 1);

    if (argc == 1 || opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    const std::size_t rw_buffer_size = 10 * 1024 * 1024;
    const std::size_t rwlevel_max_size = 100 * 1024 * 1024;;
    const std::size_t max_undumped_size = 2 * rwlevel_max_size;
    const std::size_t max_levels0 = 20;
    const char* PREFIX = "Inventory_";

    ReferenceCounting::SmartPtr<ProfileMap> base_map =
      new BaseProfileMap(
        Generics::ActiveObjectCallback_var(
          new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, "UserExtractUtil")),
        path->c_str(),
        PREFIX,
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          rw_buffer_size,
          rwlevel_max_size,
          max_undumped_size,
          max_levels0,
          Generics::Time::ZERO));

    ReferenceCounting::SmartPtr<ProfileMap> profile_map =
      new AdServer::ProfilingCommons::AdaptProfileMap<
        AdServer::Commons::UserId,
        AdServer::RequestInfoSvcs::UserChannelInventoryProfileAdapter>(
          base_map, AdServer::RequestInfoSvcs::UserChannelInventoryProfileAdapter());

    extract_data(
      *min_req_time,
      day.installed() ? std::optional<Generics::Time>(*day) : std::optional<Generics::Time>(),
      std::set<unsigned long>(channel_whitelist->begin(), channel_whitelist->end()),
      profile_map,
      std::cout,
      verbose.enabled());

    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "ERROR: " << ex.what() << std::endl;
  }

  return 1;
}
