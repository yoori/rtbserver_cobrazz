#include <list>
#include <vector>
#include <iterator>
#include <iostream>

#include <Generics/MemBuf.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>

#include <Commons/UserInfoManip.hpp>
#include <ProfilingCommons/PlainStorageAdapters.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

#include <UserInfoSvcs/UserInfoCommons/ChannelMatcher.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserActionProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserActionProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserCampaignReachProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserCampaignReachProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestInfoProfileUtils.hpp>

#include <RequestInfoSvcs/RequestInfoManager/Compatibility/RequestProfileAdapter.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/UserActionProfileAdapter.hpp>
#include <RequestInfoSvcs/RequestInfoManager/Compatibility/UserCampaignReachProfileAdapter.hpp>

#include <UserInfoSvcs/UserInfoManager/Compatibility/UserBaseProfileAdapter.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserHistoryProfileAdapter.hpp>
#include <UserInfoSvcs/UserInfoManager/Compatibility/UserFreqCapProfileAdapter.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfileUtils.hpp>

#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "\nUsage: \nProfileDump <COMMAND> <FILES> [-u <UID>]\n"
    "Commands: \n"
    "  print-base, print-add, print-history,\n"
    "  print-action, print-campaign-reach, print-inventory\n"
    "  print-keyword-match, print-request\n"
    "Sample: \n"
    "  ProfileDump print-inventory Inventory_.* -u z1e3xQ4ZT5mOn6Vz95CtFw..\n";
}

namespace
{
  std::string
  remove_uuid_escape(const char* uuid)
  {
    if(uuid[0] == '\\')
    {
      return std::string(uuid + 1);
    }

    return uuid;
  }
}

Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::print_profile_(
  ProfileType type,
  const char* user_id,
  const FileNameList& files)
  /*throw(eh::Exception)*/
{
  for(FileNameList::const_iterator it = files.begin();
      it != files.end(); ++it)
  {
    print_profile_from_file_(type, user_id, it->c_str());
  }
}

void
Application_::print_profile_from_file_(
  ProfileType profile_type,
  const char* user_id,
  const char* filename)
  /*throw(eh::Exception)*/
{
  typedef AdServer::ProfilingCommons::MemIndexProfileMap<
    AdServer::Commons::UserId,
    PlainStorage::FragmentBlockIndex,
    AdServer::ProfilingCommons::DefaultMapTraits<
      AdServer::Commons::UserId,
      AdServer::ProfilingCommons::UserIdAccessor,
      PlainStorage::FragmentBlockIndex,
      PlainStorage::FragmentBlockIndexSerializer> >
    MapT;

  ReferenceCounting::SmartPtr<MapT> user_map(new MapT(filename));

  if(user_id[0])
  {
    Generics::ConstSmartMemBuf_var mb =
      user_map->get_profile(AdServer::Commons::UserId(user_id));

    if(!mb)
    {
      std::cerr << "Not found" << std::endl;
    }
    else
    {
      print_profile_from_block_(profile_type, user_id, mb->membuf());
    }
  }
  else
  {
    std::list<AdServer::Commons::UserId> users;
    user_map->copy_keys(users);
    for(std::list<AdServer::Commons::UserId>::iterator it = users.begin();
        it != users.end(); ++it)
    {
      Generics::ConstSmartMemBuf_var mb = user_map->get_profile(*it);
      if(mb)
      {
        std::cout << ">>>>>>>>>> User '" << *it << "'(" << mb->membuf().size() << "):" << std::endl;
        print_profile_from_block_(profile_type, it->to_string().c_str(), mb->membuf());
      }
    }
  }
}

void Application_::print_profile_from_block_(
  ProfileType type,
  const char* user_id,
  const Generics::MemBuf& buf)
  noexcept
{
  try
  {
    ConstSmartMemBuf_var sm_buf;

    {
      SmartMemBuf_var mem_buf(new SmartMemBuf(buf.size()));
      memcpy(mem_buf->membuf().get<void>(), buf.data(), buf.size());
      sm_buf = Generics::transfer_membuf(mem_buf);
    }

    // print content
    if(type == PT_BASE || type == PT_ADD)
    {
      if (buf.size() != 0)
      {
        AdServer::UserInfoSvcs::BaseProfileAdapter base_adapter;
        sm_buf = base_adapter(sm_buf);
      }

      ChannelsMatcher::print(sm_buf->membuf().data(), sm_buf->membuf().size(), std::cout);
    }
    else if(type == PT_HISTORY)
    {
      if (buf.size() != 0)
      {
        AdServer::UserInfoSvcs::HistoryProfileAdapter history_adapter;
        sm_buf = history_adapter(sm_buf);
      }

      ChannelsMatcher::history_print(sm_buf->membuf().data(), sm_buf->membuf().size(), std::cout);
    }
    else if(type == PT_ACTION)
    {
      if (buf.size() != 0)
      {
        AdServer::RequestInfoSvcs::UserActionProfileAdapter act_adapter;
        sm_buf = act_adapter(sm_buf);
      }

      AdServer::RequestInfoSvcs::UserActionProfileReader act_reader(
        sm_buf->membuf().data(), sm_buf->membuf().size());
      AdServer::RequestInfoSvcs::print_user_action_profile(
        std::cout, act_reader, false);
    }
    else if(type == PT_CAMPAIGNREACH)
    {
      if (buf.size() != 0)
      {
        AdServer::RequestInfoSvcs::UserCampaignReachProfileAdapter camp_adapter;
        sm_buf = camp_adapter(sm_buf);
      }

      AdServer::RequestInfoSvcs::UserCampaignReachProfileReader
        campaign_reach_reader(sm_buf->membuf().data(), sm_buf->membuf().size());
      AdServer::RequestInfoSvcs::print_user_campaign_reach_profile(
        std::cout, campaign_reach_reader);
    }
    else if(type == PT_INVENTORY)
    {
      AdServer::RequestInfoSvcs::UserChannelInventoryProfileReader
        ch_inv_reader(buf.data(), buf.size());
      AdServer::RequestInfoSvcs::print_user_channel_inventory_profile(
        std::cout, user_id, ch_inv_reader);
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Can't print profile for User '" << user_id <<
      "': " << ex.what() << std::endl;
  }
}

void
Application_::dump_request_profiles_(const FileNameList& files) noexcept
{
  try
  {
    for(FileNameList::const_iterator fi = files.begin();
        fi != files.end(); ++fi)
    {
      typedef AdServer::ProfilingCommons::HashIndexProfileMap<
        AdServer::ProfilingCommons::RequestIdPackHashAdapter,
        PlainStorage::FragmentBlockIndex,
        AdServer::ProfilingCommons::DefaultMapTraits<
          AdServer::ProfilingCommons::RequestIdPackHashAdapter,
          AdServer::ProfilingCommons::RequestIdAccessor,
          PlainStorage::FragmentBlockIndex,
          PlainStorage::FragmentBlockIndexSerializer> >
        MapT;

      typedef AdServer::ProfilingCommons::ProfileMap<
        AdServer::ProfilingCommons::RequestIdPackHashAdapter>
        ProfileMap;

      ReferenceCounting::SmartPtr<MapT> base_map = new MapT(fi->c_str());
      ReferenceCounting::SmartPtr<ProfileMap> map(
        new AdServer::ProfilingCommons::AdaptProfileMap<
          AdServer::ProfilingCommons::RequestIdPackHashAdapter,
          AdServer::RequestInfoSvcs::RequestProfileAdapter>(base_map));
      std::list<AdServer::ProfilingCommons::RequestIdPackHashAdapter> requests_ids;
      map->copy_keys(requests_ids);

      for (std::list<AdServer::ProfilingCommons::RequestIdPackHashAdapter>::const_iterator  it =
           requests_ids.begin(); it != requests_ids.end(); ++it)
      {
        Generics::ConstSmartMemBuf_var mb =
          map->get_profile(*it);
        AdServer::RequestInfoSvcs::RequestInfoProfileReader reader(
          mb->membuf().data(), mb->membuf().size());
        AdServer::RequestInfoSvcs::print_request_info_profile(
          std::cout, reader);
      }
    }
  }
  catch (eh::Exception& ex)
  {
    std::cerr << "can't dump request profiles: " <<
      ex.what() << std::endl;
  }
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::CheckOption opt_plain;
  Generics::AppUtils::StringOption opt_user_id("");

  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);

  args.add(
    Generics::AppUtils::equal_name("plain") ||
    Generics::AppUtils::short_name("p"),
    opt_plain);

  args.add(
    Generics::AppUtils::equal_name("user-id") ||
    Generics::AppUtils::equal_name("uid") ||
    Generics::AppUtils::short_name("u"),
    opt_user_id);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  std::string command = *commands.begin();

  if(command == "print-base" ||
     command == "print-add" ||
     command == "print-history" ||
     command == "print-action" ||
     command == "print-campaign-reach" ||
     command == "print-inventory" ||
     command == "print-keyword-match" ||
     command == "print-request")
  {
    ProfileType profile_type;
    const char* print_profile_type = command.c_str() + 6;
    if(strcmp(print_profile_type, "base") == 0)
    {
      profile_type = PT_BASE;
    }
    else if(strcmp(print_profile_type, "add") == 0)
    {
      profile_type = PT_ADD;
    }
    else if(strcmp(print_profile_type, "history") == 0)
    {
      profile_type = PT_HISTORY;
    }
    else if(strcmp(print_profile_type, "action") == 0)
    {
      profile_type = PT_ACTION;
    }
    else if(strcmp(print_profile_type, "campaign-reach") == 0)
    {
      profile_type = PT_CAMPAIGNREACH;
    }
    else if(strcmp(print_profile_type, "inventory") == 0)
    {
      profile_type = PT_INVENTORY;
    }
    else if(strcmp(print_profile_type, "request") == 0)
    {
      profile_type = PT_REQUEST;
    }
    else
    {
      std::cout << USAGE << std::endl;
      return;
    }

    FileNameList files;
    std::copy(++commands.begin(), commands.end(), std::back_inserter(files));

    std::string user_id;

    if(!opt_user_id->empty())
    {
      user_id = remove_uuid_escape(opt_user_id->c_str());
    }

    if (profile_type == PT_REQUEST)
    {
      dump_request_profiles_(files);
    }
    else
    {
      print_profile_(
        profile_type,
        user_id.c_str(),
        files);
    }
  }
  else if(command == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }
  else
  {
    std::cerr << "Unknown command '" << command << "'. "
      "See help for more info." << std::endl;
  }
}

void
Application_::print_plain_(
  std::ostream& ostr,
  const void* buf,
  unsigned long size,
  const char* prefix)
  noexcept
{
  ostr << prefix;

  for(unsigned long i = 0; i < size; ++i)
  {
    ostr << "0x" << std::hex << std::setfill('0') << std::setw(2)
         << (int)*((const unsigned char*)buf + i) << " ";
    if(i && (i + 1) % 16 == 0)
    {
      ostr << std::endl << prefix;
    }
  }

  ostr << std::dec << std::setw(0);
}

int main(int argc, char** argv)
{
  Application_* app = 0;

  try
  {
    app = &Application::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}


