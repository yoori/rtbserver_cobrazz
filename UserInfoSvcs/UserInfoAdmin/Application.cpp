#include <list>
#include <vector>
#include <iterator>
#include <iostream>

#include <Generics/MemBuf.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaAlgs.hpp>

#include <UserInfoSvcs/UserInfoCommons/ChannelMatcher.hpp>
#include <UserInfoSvcs/UserInfoCommons/UserFreqCapProfile.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerSessionFactory.hpp>
#include <UserInfoSvcs/UserInfoExchanger/UserInfoExchanger.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/exception/all.hpp>
#include <boost/lexical_cast.hpp>

#include "Application.hpp"

namespace
{
  const char TIME_FORMAT[] = "%d-%m-%Y:%H-%M-%S.%q";
  const char DAY_TIME_FORMAT[] = "%H-%M-%S.%q";
}

namespace
{
  const char USAGE[] =
    "\nUsage: \nUserInfoAdmin <command> <command arguments>\n\n"
    "Synopsis 1:\n"
    "UserInfoAdmin match --uid=<user id in base64 format> "
      "--page-channels=<channel ids - channel_id1,trigger_channel_id2;channel_id1,trigger_channel_id2;...> "
      "--search-channels=<channel ids - channel_id1,trigger_channel_id2;channel_id1,trigger_channel_id2;...> "
      "--url-channels=<channel ids - channel_id1,trigger_channel_id2;channel_id1,trigger_channel_id2;...> "
      "--persistent-channels=<channel ids - channel_id1,channel_id2,...>"
      "-r[--reference=]<user_info_manager_corba_ref>|"
      "<user_info_manager_controller_corba_ref> "
      "--time=<DD-MM-YYYY:HH-MM-SS> \n\n"
    "Synopsis 2:\n"
    "UserInfoAdmin [print|print-freq-cap|print-base|print-add|print-history] "
    "user_id=<user id in base64 format> "
      "[--expand | -e] [--align | -a] [--plain | -p] "
      "-r[--reference=]<user_info_manager_corba_ref>|<user_info_manager_controller_corba_ref> \n\n"
    "Debug commands:\n"
    "Synopsis 4:\n"
    "UserInfoAdmin request "
      "--uid=<user id in base64 format> "
      "--customer_id=<colo id> "
      "--provider_id=<user info owner colo id> "
      "--exchanger=<user info exchanger corba ref>\n\n"
    "UserInfoAdmin response "
      "--uid=<user id in base64 format> "
      "--provider_id=<user info owner colo id> "
      "--user_info=<test user info> "
      "--user_history_info=<test history user info>\n\n"
    "UserInfoAdmin delete-old-profiles "
      "(-r[--reference=]<user_info_manager_corba_ref>|"
      "<user_info_manager_controller_corba_ref>) "
      "--sync --time=<DD-MM-YYYY:HH-MM-SS> --portion=<number>\n\n"
    "UserInfoAdmin generate "
      "--keys-dir=<path to key directory> --persistent\n\n"
    "UserInfoAdmin get-config-timestamp "
    "(-r[--reference=]<user_info_manager_corba_ref>) \n\n";
  
  const unsigned long PRINT_BASE = 0x01;
  const unsigned long PRINT_ADD = 0x02;
  const unsigned long PRINT_HISTORY = 0x04;
  const unsigned long PRINT_ALL = -1;
}

Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::get_profile_(
  AdServer::UserInfoSvcs::UserInfoMatcher* user_info_matcher,
  const AdServer::Commons::UserId& user_id,
  const AdServer::Commons::UserId& temp_user_id,
  bool print_plain,
  bool print_expand,
  bool print_align,
  unsigned long print_kind)
  noexcept
{
  try
  {
    AdServer::UserInfoSvcs::ProfilesRequestInfo profiles_request;
    profiles_request.base_profile = (print_kind & PRINT_BASE) != 0;
    profiles_request.add_profile = (print_kind & PRINT_ADD) != 0;
    profiles_request.history_profile = (print_kind & PRINT_HISTORY) != 0;
    profiles_request.freq_cap_profile = false;

    AdServer::UserInfoSvcs::UserProfiles_var user_profile;

    user_info_matcher->get_user_profile(
      CorbaAlgs::pack_user_id(!user_id.is_null() ? user_id : temp_user_id),
      user_id.is_null(),
      profiles_request,
      user_profile.out());

    if (profiles_request.base_profile)
    {
      const CORBACommons::OctSeq& base_profile = user_profile->base_user_profile;
      
      std::cout << "Base profile:" << std::endl;
      
      if(print_plain)
      {
        print_plain_(
          std::cout,
          base_profile.get_buffer(),
          base_profile.length(),
          " ");
      }
      
      ChannelsMatcher::print(
        base_profile.get_buffer(),
        base_profile.length(),
        std::cout,
        print_expand,
        print_align);
    }

    if (profiles_request.add_profile)
    {
      const CORBACommons::OctSeq& add_profile = user_profile->add_user_profile;
      
      std::cout << "Additional profile:" << std::endl;
      
      if (print_plain)
      {
        print_plain_(
          std::cout,
          add_profile.get_buffer(),
          add_profile.length(),
          " ");
      }
      
      ChannelsMatcher::print(
        add_profile.get_buffer(),
        add_profile.length(),
        std::cout,
        print_expand,
        print_align);
    }
    
    if (profiles_request.history_profile)
    {
      const CORBACommons::OctSeq& history_profile = user_profile->history_user_profile;
      
      std::cout << "History profile:" << std::endl;
      
      if (print_plain)
      {
        print_plain_(
          std::cout,
          history_profile.get_buffer(),
          history_profile.length(),
          " ");
      }
      
      ChannelsMatcher::history_print(
        history_profile.get_buffer(),
        history_profile.length(),
        std::cout,
        print_align);
    }

    std::cout << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::
        UserInfoMatcher::ImplementationException& e)
  {
    std::cerr << "Caught UserInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::
        UserInfoMatcher::NotReady& e)
  {
    std::cerr << "Caught UserInfoMatcher::NotReady. :" << 
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound& e)
  {
    std::cerr << "Caught UserInfoManager::ChunkNotFound: " <<
      e.description << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void
Application_::print_freq_cap_profile_(
  AdServer::UserInfoSvcs::UserInfoMatcher* user_info_matcher,
  const AdServer::Commons::UserId& user_id,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::UserInfoSvcs::ProfilesRequestInfo profiles_request;
    profiles_request.base_profile = false;
    profiles_request.add_profile = false;
    profiles_request.history_profile = false;
    profiles_request.freq_cap_profile = true;

    AdServer::UserInfoSvcs::UserProfiles_var user_profile;

    user_info_matcher->get_user_profile(
      CorbaAlgs::pack_user_id(user_id),
      false,
      profiles_request,
      user_profile.out());

    const CORBACommons::OctSeq& fc_profile = user_profile->freq_cap;

    if(print_plain)
    {
      print_plain_(
        std::cout,
        fc_profile.get_buffer(),
        fc_profile.length(),
        " ");
    }

    Generics::ConstSmartMemBuf_var mb(
      new Generics::ConstSmartMemBuf(fc_profile.get_buffer(), fc_profile.length()));
    UserFreqCapProfile freq_cap_profile(mb);
    freq_cap_profile.print(std::cout, 0);
  }
  catch(const AdServer::UserInfoSvcs::
        UserInfoMatcher::ImplementationException& e)
  {
    std::cerr << "Caught UserInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::
        UserInfoMatcher::NotReady& e)
  {
    std::cerr << "Caught UserInfoMatcher::NotReady. :" << 
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound& e)
  {
    std::cerr << "Caught UserInfoManager::ChunkNotFound: " <<
      e.description << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void Application_::tokenize_channels(
  std::set<unsigned long>& matched_channel_ids,
  const String::SubString& matched_channels)
  /*throw(eh::Exception)*/
{
  String::StringManip::Splitter<
    const String::AsciiStringManip::Char2Category<',', ' '> >
    input_channels_tokenizer(matched_channels);

  String::SubString channel;
  while (input_channels_tokenizer.get_token(channel))
  {
    int value;
    if (!String::StringManip::str_to_int(channel, value))
    {
      value = 0;
    }
    matched_channel_ids.insert(value);
  }
}

void Application_::tokenize_channels(
  std::set<ChannelMatch>& matched_channel_ids,
  const String::SubString& matched_channels)
{
  if (matched_channels.empty())
  {
    return;
  }

  std::vector<std::string> pair_ids;
  boost::split(
    pair_ids,
    matched_channels.str(),
    boost::is_any_of(";"),
    boost::token_compress_on);
  for (const auto& pair_id : pair_ids)
  {
    std::vector<std::string> ids;
    boost::split(
      ids,
      pair_id,
      boost::is_any_of(","),
      boost::token_compress_on);
    if (ids.size() != 2)
    {
      std::ostringstream ostream;
      ostream << "There must be exactly two numbers in a pair. Received:"
              << pair_id;
      throw std::runtime_error(ostream.str());
    }

    boost::trim(ids[0]);
    boost::trim(ids[1]);

    matched_channel_ids.insert(
      ChannelMatch(
        boost::lexical_cast<unsigned long>(ids[0]),
        boost::lexical_cast<unsigned long>(ids[1])));
  }
}

void
Application_::match_(
  AdServer::UserInfoSvcs::UserInfoMatcher* user_info_matcher,
  const AdServer::Commons::UserId& user_id,
  const AdServer::Commons::UserId& temp_user_id,
  const String::SubString& opt_time,
  const String::SubString& matched_page_channels,
  const String::SubString& matched_search_channels,
  const String::SubString& matched_url_channels,
  const String::SubString& matched_persistent_channels)
  noexcept
{
  try
  {
    /* fill match params */
    AdServer::UserInfoSvcs::UserInfo user_info;
    user_info.user_id = CorbaAlgs::pack_user_id(
      !user_id.is_null() ? user_id : temp_user_id);
    user_info.temporary = user_id.is_null();
    user_info.last_colo_id = -1;
    user_info.current_colo_id = 1;

    user_info.time = Generics::Time(opt_time,
      "%d-%m-%Y:%H-%M-%S").tv_sec;

    std::set<ChannelMatch> matched_page_channel_ids;
    std::set<ChannelMatch> matched_search_channel_ids;
    std::set<ChannelMatch> matched_url_channel_ids;
    std::set<unsigned long> matched_persistent_channel_ids;
    tokenize_channels(matched_page_channel_ids, matched_page_channels);
    tokenize_channels(matched_search_channel_ids, matched_search_channels);
    tokenize_channels(matched_url_channel_ids, matched_url_channels);
    tokenize_channels(matched_persistent_channel_ids, matched_persistent_channels);

    AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams match_params;
    match_params.silent_match = false;
    match_params.no_match = false;
    match_params.no_result = false;
    match_params.publishers_optin_timeout =
      CorbaAlgs::pack_time(Generics::Time::ZERO);
    
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var match_result;

    unsigned long i = 0;
    match_params.page_channel_ids.length(matched_page_channel_ids.size());
    for (std::set<ChannelMatch>::const_iterator it = matched_page_channel_ids.begin();
         it != matched_page_channel_ids.end(); ++it)
    {
      match_params.page_channel_ids[i].channel_id = it->channel_id;
      match_params.page_channel_ids[i].channel_trigger_id = it->channel_trigger_id;

      ++i;
    }

    i = 0;
    match_params.search_channel_ids.length(matched_search_channel_ids.size());
    for (std::set<ChannelMatch>::const_iterator it = matched_search_channel_ids.begin();
         it != matched_search_channel_ids.end(); ++it)
    {
      match_params.search_channel_ids[i].channel_id = it->channel_id;
      match_params.search_channel_ids[i].channel_trigger_id = it->channel_trigger_id;

      ++i;
    }

    i = 0;
    match_params.url_channel_ids.length(matched_url_channel_ids.size());
    for (std::set<ChannelMatch>::const_iterator it = matched_url_channel_ids.begin();
         it != matched_url_channel_ids.end(); ++it)
    {
      match_params.url_channel_ids[i].channel_id = it->channel_id;
      match_params.url_channel_ids[i].channel_trigger_id = it->channel_trigger_id;

      ++i;
    }

    CorbaAlgs::fill_sequence(
      matched_persistent_channel_ids.begin(),
      matched_persistent_channel_ids.end(),
      match_params.persistent_channel_ids);
    
    std::cout << "matching for user_id='" << user_id << "'" << std::endl <<
      "Input Channels: " << std::endl << "  Page: ";
    CorbaAlgs::print_sequence_fields(
      std::cout,
      match_params.page_channel_ids,
      &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_id,
      &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_trigger_id,
      ";",
      ",");
    std::cout << std::endl << "  Search: ";
    CorbaAlgs::print_sequence_fields(
      std::cout,
      match_params.search_channel_ids,
      &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_id,
      &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_trigger_id,
      ";",
      ",");
    std::cout << std::endl << "  Url: ";
    CorbaAlgs::print_sequence_fields(
      std::cout,
      match_params.url_channel_ids,
      &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_id,
      &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_trigger_id,
      ";",
      ",");
    std::cout << std::endl << "  Persistent: ";
    CorbaAlgs::print_sequence(std::cout, match_params.persistent_channel_ids);
    std::cout << std::endl;;
    
    user_info_matcher->match(user_info, match_params, match_result);

    std::cout << "Output Channels: ";
    const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq&
      output_channels = match_result->channels;

    for(CORBA::ULong i = 0; i < output_channels.length(); ++i)
    {
      std::cout << (i != 0 ? "," : "") << output_channels[i].channel_id;
    }

    std::cout << std::endl;
  }
  catch (const boost::exception& exc)
  {
    std::cerr << "Caught exception: "
              << boost::diagnostic_information(exc);
  }
  catch(const AdServer::UserInfoSvcs::
        UserInfoMatcher::ImplementationException& e)
  {
    std::cerr << "Caught UserInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::
        UserInfoMatcher::NotReady& e)
  {
    std::cerr << "Caught UserInfoMatcher::NotReady. ";
  }
  catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound& e)
  {
    std::cerr << "Caught UserInfoManager::ChunkNotFound: " <<
      e.description << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}
          
void
Application_::remove_user_profile_(
  AdServer::UserInfoSvcs::UserInfoMatcher* user_info_matcher,
  const AdServer::Commons::UserId& user_id)
  noexcept
{
  try
  {
    CORBACommons::UserIdInfo user_id_info = CorbaAlgs::pack_user_id(user_id);
    user_info_matcher->remove_user_profile(user_id_info);
  }
  catch(const AdServer::UserInfoSvcs::
    UserInfoMatcher::ImplementationException& e)
  {
    std::cerr << "Caught UserInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::UserInfoMatcher::NotReady& e)
  {
    std::cerr << "Caught UserInfoMatcher::NotReady. ";
  }
  catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound& e)
  {
    std::cerr << "Caught UserInfoManager::ChunkNotFound: " <<
      e.description << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
}

void Application_::delete_old_profiles_(
  AdServer::UserInfoSvcs::UserInfoMatcher* user_info_matcher,
  bool sync,
  const Generics::Time& cleanup_time,
  long portion)
  noexcept
{
  try
  {
    user_info_matcher->clear_expired(
      sync,
      CorbaAlgs::pack_time(cleanup_time),
      portion);
  }
  catch(const AdServer::UserInfoSvcs::
        UserInfoMatcher::ImplementationException& e)
  {
    std::cerr << "Caught UserInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::UserInfoSvcs::
        UserInfoMatcher::NotReady& e)
  {
    std::cerr << "Caught UserInfoMatcher::NotReady. ";
  }
  catch(const AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound& e)
  {
    std::cerr << "Caught UserInfoManager::ChunkNotFound: " <<
      e.description << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::CheckOption opt_plain;
  Generics::AppUtils::CheckOption opt_expand;
  Generics::AppUtils::CheckOption opt_align;
  
  Generics::AppUtils::Option<std::string> opt_user_id;
  Generics::AppUtils::Option<std::string> opt_temp_user_id;
  Generics::AppUtils::Option<std::string> opt_ui_exchanger_ref;
  Generics::AppUtils::Option<std::string> opt_user_info_manager_ref;

  // match options
  Generics::AppUtils::Option<std::string> opt_matched_page_channels("");
  Generics::AppUtils::Option<std::string> opt_matched_search_channels("");
  Generics::AppUtils::Option<std::string> opt_matched_url_channels("");
  Generics::AppUtils::Option<std::string> opt_matched_persistent_channels("");
  
  Generics::AppUtils::Option<unsigned long> opt_provider_id;
  Generics::AppUtils::Option<std::string> opt_customer_id;
  Generics::AppUtils::Option<std::string> opt_user_info;
  Generics::AppUtils::Option<std::string> opt_user_history_info;
  Generics::AppUtils::Option<std::string> opt_keys_directory;
  Generics::AppUtils::CheckOption opt_persistent;
  Generics::AppUtils::CheckOption opt_sync;

  Generics::AppUtils::Option<std::string> opt_time;
  Generics::AppUtils::Args args(-1);

  Generics::AppUtils::Option<unsigned long> opt_portion;
  
  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);

  args.add(
    Generics::AppUtils::equal_name("plain") ||
    Generics::AppUtils::short_name("p"),
    opt_plain);

  args.add(
    Generics::AppUtils::equal_name("expand") ||
    Generics::AppUtils::short_name("e"),
    opt_expand);

  args.add(
    Generics::AppUtils::equal_name("align") ||
    Generics::AppUtils::short_name("a"),
    opt_align);
  
  args.add(
    Generics::AppUtils::equal_name("user-id") ||
    Generics::AppUtils::equal_name("uid") ||
    Generics::AppUtils::short_name("u"),
    opt_user_id);

  args.add(
    Generics::AppUtils::equal_name("temp-user-id") ||
    Generics::AppUtils::equal_name("tuid") ||
    Generics::AppUtils::short_name("t"),
    opt_temp_user_id);

  args.add(Generics::AppUtils::equal_name("provider-id"), opt_provider_id);
  args.add(Generics::AppUtils::equal_name("customer-id"), opt_customer_id);

  args.add(
    Generics::AppUtils::equal_name("reference") ||
    Generics::AppUtils::short_name("r"),
    opt_user_info_manager_ref);

  args.add(
    Generics::AppUtils::equal_name("exchanger") ||
    Generics::AppUtils::short_name("re"),
    opt_ui_exchanger_ref);

  args.add(Generics::AppUtils::equal_name("page-channels"),
    opt_matched_page_channels);
  args.add(Generics::AppUtils::equal_name("search-channels"),
    opt_matched_search_channels);
  args.add(Generics::AppUtils::equal_name("url-channels"),
    opt_matched_url_channels);
  args.add(Generics::AppUtils::equal_name("persistent-channels"),
    opt_matched_persistent_channels);
  args.add(Generics::AppUtils::equal_name("user-info"), opt_user_info);
  args.add(Generics::AppUtils::equal_name("user-history-info"), opt_user_history_info);
  args.add(Generics::AppUtils::equal_name("keys-dir"), opt_keys_directory);
  args.add(Generics::AppUtils::equal_name("persistent"), opt_persistent);
  args.add(Generics::AppUtils::equal_name("time"), opt_time);
  args.add(Generics::AppUtils::equal_name("sync"), opt_sync);
  args.add(Generics::AppUtils::equal_name("portion"), opt_portion);
  
  args.parse(argc - 1, argv + 1);
 
  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() || *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  std::string command = *commands.begin();

  if(command == "generate" || command == "sign")
  {
    std::string keys_dir;

    if(opt_keys_directory.installed())
    {
      keys_dir = opt_keys_directory->c_str();
    }
    else
    {
      const char* unix_commons_env = getenv("unix_commons_root");
      keys_dir = unix_commons_env ? unix_commons_env : "/opt/foros/server";
      keys_dir += "/share/uuid_keys"; 
    }

    keys_dir += opt_persistent.enabled() ? "/private.der" : "/private_temp.der";

    Generics::SignedUuidGenerator generator(keys_dir.c_str());

    if(command == "generate")
    {
      Generics::SignedUuid id = generator.generate();
      std::cout << "'" << id.str() << "'" << std::endl;
    }
    else
    {
      Generics::SignedUuid id = generator.sign(
        AdServer::Commons::UserId(opt_user_id->c_str()));
      std::cout << "'" << id.str() << "'" << std::endl;
    }

    return;
  }

  if(!opt_user_info_manager_ref.installed())
  {
    Stream::Error ostr;
    ostr << "Not defined parameters: reference to the manager or controller must be specifed.";
    throw Exception(ostr);
  }

  CORBACommons::CorbaClientAdapter_var corba_client_adapter(
    new CORBACommons::CorbaClientAdapter());

  if(command == "print" ||
     command == "print-freq-cap" ||
     command == "print-base" ||
     command == "print-add" ||
     command == "print-history" ||
     command == "match" ||
     command == "remove" ||
     command == "delete-old-profiles" ||
     command == "get-config-timestamp")
  {
    AdServer::UserInfoSvcs::UserInfoMatcher_var user_info_matcher;

    bool success = false;
    try
    {
      AdServer::UserInfoSvcs::
        UserInfoManagerSessionFactory::init(*corba_client_adapter);

      CORBACommons::CorbaObjectRef corba_object_ref(
        opt_user_info_manager_ref->c_str());

      AdServer::UserInfoSvcs::UserInfoManagerController_var
        user_info_manager_controller =
          corba_client_adapter->resolve_object<
            AdServer::UserInfoSvcs::UserInfoManagerController>(corba_object_ref);

      user_info_matcher =
        user_info_manager_controller->get_session();
      success = true;
    }
    catch(const CORBACommons::CorbaClientAdapter::Exception&)
    {}
    catch(const CORBA::SystemException&)
    {}

    if(!success)
    {
      try
      {
        CORBA::Object_var user_info_matcher_obj =
          corba_client_adapter->resolve_object(
            CORBACommons::CorbaObjectRef(opt_user_info_manager_ref->c_str()));

        user_info_matcher = AdServer::UserInfoSvcs::UserInfoManager::_narrow(
          user_info_matcher_obj);

        if(CORBA::is_nil(user_info_matcher.in()))
        {
          throw Exception("_narrow for user_info_manager return nil reference.");
        }
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << "Caught CORBA::SystemException: " << ex;
        throw Exception(ostr);
      }
    }
    
    if(command != "delete-old-profiles" &&
       command != "get-config-timestamp" &&
       !opt_user_id.installed() && !opt_temp_user_id.installed())
    {
      Stream::Error ostr;
      ostr << "Not defined parameter user-id or temp-user-id.";
      throw Exception(ostr);
    }

    AdServer::Commons::UserId user_id =
      !opt_user_id->empty() ? AdServer::Commons::UserId(opt_user_id->c_str()) :
      AdServer::Commons::UserId();
    AdServer::Commons::UserId temp_user_id =
      !opt_temp_user_id->empty() ? AdServer::Commons::UserId(opt_temp_user_id->c_str()) :
      AdServer::Commons::UserId();
    
    if(command == "print")
    {
      get_profile_(
        user_info_matcher,
        user_id,
        temp_user_id,
        opt_plain.enabled(),
        opt_expand.enabled(),
        opt_align.enabled(),
        PRINT_ALL);
    }
    if(command == "print-freq-cap")
    {
      print_freq_cap_profile_(
        user_info_matcher,
        user_id,
        opt_plain.enabled());
    }
    else if (command == "print-base")
    {
      get_profile_(
        user_info_matcher,
        user_id,
        temp_user_id,
        opt_plain.enabled(),
        opt_expand.enabled(),
        opt_align.enabled(),
        PRINT_BASE);
    }
    else if (command == "print-add")
    {
      get_profile_(
        user_info_matcher,
        user_id,
        temp_user_id,
        opt_plain.enabled(),
        opt_expand.enabled(),
        opt_align.enabled(),
        PRINT_ADD);
    }
    else if (command == "print-history")
    {
      get_profile_(
        user_info_matcher,
        user_id,
        temp_user_id,
        opt_plain.enabled(),
        opt_expand.enabled(),
        opt_align.enabled(),
        PRINT_HISTORY);
    }
    else if(command == "match")
    {
      match_(
        user_info_matcher,
        user_id,
        temp_user_id,
        *opt_time,
        *opt_matched_page_channels,
        *opt_matched_search_channels,
        *opt_matched_url_channels,
        *opt_matched_persistent_channels);
    }
    else if(command == "remove")
    {
      remove_user_profile_(
        user_info_matcher,
        user_id);
    }
    else if (command == "get-config-timestamp")
    {
      CORBACommons::TimestampInfo_var master_stamp;
      user_info_matcher->get_master_stamp(master_stamp);

      Generics::Time timestamp = CorbaAlgs::unpack_time(master_stamp);
      std::cout << "Config timestamp : "
                << timestamp.get_gm_time().format(TIME_FORMAT)
                << std::endl;
    }
    else if(command == "delete-old-profiles")
    {
      delete_old_profiles_(
        user_info_matcher,
        opt_sync.enabled(),
        opt_time.installed() ?
          Generics::Time(*opt_time, "%H-%M") :
          Generics::Time(-1),
        opt_portion.installed() ? *opt_portion : -1);
    }
  }
  else if(command == "request" || command == "response")
  {
    try
    {
      /* parse commons for request and response arguments */
      AdServer::UserInfoSvcs::UserInfoExchanger_var  user_info_exchanger =
        corba_client_adapter->resolve_object<
          AdServer::UserInfoSvcs::UserInfoExchanger>(
            opt_ui_exchanger_ref->c_str());

      if(!opt_user_id.installed() || !opt_provider_id.installed())
      {
        throw Exception("user_id or provider_id isn't presented");
      }
      
      if(command == "request")
      {
        std::cout << "Request user info. " << std::endl;

        if(!opt_customer_id.installed() || !opt_provider_id.installed())
        {
          throw Exception("customer_id or provider_id isn't defined");
        }

        try
        {
          AdServer::UserInfoSvcs::ColoUsersRequestSeq user_request;
          user_request.length(1);
          user_request[0].colo_id = *opt_provider_id;
          user_request[0].users.length(1);
          user_request[0].users[0] = opt_user_id->c_str();

          user_info_exchanger->register_users_request(
            opt_customer_id->c_str(), user_request);
        }
        catch(AdServer::UserInfoSvcs::
              UserInfoExchanger::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << "Caught ImplementationException from register_users_request(..): " <<
            ex.description;
          throw Exception(ostr);
        }
        catch(CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << "Caught CORBA::SystemException from register_users_request(..): " << ex;
          throw Exception(ostr);
        }

        std::cout << "Wait & receive user info..." << std::endl;

        try
        {
          bool stop = false;

          do
          {
            AdServer::UserInfoSvcs::UserProfileSeq_var user_profile;
            AdServer::UserInfoSvcs::ReceiveCriteria rec_cr;
            rec_cr.max_response_plain_size = 10*1024*1024;
            rec_cr.common_chunks_number = 1;
            rec_cr.chunk_ids.length(1);
            rec_cr.chunk_ids[0] = 0;

            user_info_exchanger->receive_users(
              opt_customer_id->c_str(),
              user_profile,
              rec_cr);

            if(user_profile->length() != 0)
            {
              for(CORBA::ULong i = 0; i < user_profile->length(); ++i)
              {
                const AdServer::UserInfoSvcs::UserProfile&
                  up_to_print = (*user_profile)[i];

                std::string plain_up_string(
                  (const char*)up_to_print.plain_profile.get_buffer(),
                  up_to_print.plain_profile.length());

                std::string plain_uhp_string(
                  (const char*)up_to_print.plain_history_profile.get_buffer(),
                  up_to_print.plain_history_profile.length());

                std::cout
                  << "User Info received: " << std::endl
                  << "  user_id='" << up_to_print.user_id << "'" << std::endl
                  << "  colo_id='" << up_to_print.colo_id << "'" << std::endl
                  << "  plain_profile='" << plain_up_string << "'" << std::endl
                  << "  plain_history_profile='" << plain_uhp_string << "'" << std::endl;

                if(*opt_user_id == (const char*)up_to_print.user_id)
                {
                  stop = true;
                }
              }
            }
            else
            {
              sleep(1);
            }
          }
          while(!stop);
        }
        catch(const AdServer::UserInfoSvcs::
              UserInfoExchanger::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << "Caught ImplementationException from receive_users(..): " <<
            ex.description;
          throw Exception(ostr);
        }
        catch(const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << "Caught CORBA::SystemException from receive_users(..): " << ex;
          throw Exception(ostr);
        }
      }
      else if(command == "response")
      {
        std::cout << "Wait for user request..." << std::endl;

        if(!opt_user_info.installed() || !opt_user_history_info.installed())
        {
          throw Exception("user-info or user-history-info undefined");
        }
        
        /* wait for user request */
        try
        {
          bool stop = false;

          do
          {
            AdServer::UserInfoSvcs::UserIdSeq_var requested_users;

            user_info_exchanger->get_users_requests(
              opt_customer_id->c_str(),
              requested_users);

            if(requested_users->length() != 0)
            {
              for(CORBA::ULong i = 0; i < requested_users->length(); ++i)
              {
                std::cout << "User requested: " << std::endl <<
                  "  user_id='" << (*requested_users)[i] << "'" << std::endl;

                if(*opt_user_id == (const char*)((*requested_users)[i]))
                {
                  stop = true;
                }
              }
            }
            else
            {
              sleep(1);
            }
          }
          while(!stop);
        }
        catch(AdServer::UserInfoSvcs::
              UserInfoExchanger::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << "Caught ImplementationException from get_users_requests(..): " <<
            ex.description;
          throw Exception(ostr);
        }
        catch(CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << "Caught CORBA::SystemException from get_users_requests(..): " << ex;
          throw Exception(ostr);
        }

        std::cout << "To send requested user." << std::endl;

        try
        {
          /* send requested user */
          AdServer::UserInfoSvcs::UserProfileSeq user_profile;
          user_profile.length(1);
          user_profile[0].colo_id = *opt_provider_id;
          user_profile[0].user_id = opt_user_id->c_str();

          {
            user_profile[0].plain_profile.length(opt_user_info->length());

            memcpy(
              user_profile[0].plain_profile.get_buffer(),
              opt_user_info->c_str(),
              opt_user_info->length());
          }

          {
            user_profile[0].plain_history_profile.length(opt_user_history_info->length());

            memcpy(
              user_profile[0].plain_history_profile.get_buffer(),
              opt_user_history_info->c_str(),
              opt_user_history_info->length());
          }

          user_info_exchanger->send_users(
            opt_customer_id->c_str(),
            user_profile);

          std::cout << "User info sended successfully." << std::endl;
        }
        catch(AdServer::UserInfoSvcs::
              UserInfoExchanger::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << "Caught ImplementationException from send_users(..): " <<
            ex.description;
          throw Exception(ostr);
        }
        catch(CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << "Caught CORBA::SystemException from send_users(..): " << ex;
          throw Exception(ostr);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      std::cout << "Caught eh::Exception: " << ex.what() << std::endl;
    }
    catch(const CORBA::SystemException& ex)
    {
      std::cout << "Caught CORBA::SystemException: " << ex << std::endl;
    }
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

  ostr << std::endl << std::dec << std::setw(0);
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


