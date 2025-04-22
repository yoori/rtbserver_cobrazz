#include <malloc.h>
#include <list>
#include <iterator>
#include <iostream>
#include <sstream>

#include <String/StringManip.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/ArrayAutoPtr.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <UtilCommons/Table.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/PassbackProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/PassbackProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/RequestProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserCampaignReachProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserCampaignReachProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserActionProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserActionProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserFraudProtectionProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserSiteReachProfile.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserTagRequestMergeProfile.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/RequestInfoProfileUtils.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/Algs.hpp>

#include <RequestInfoSvcs/RequestInfoManager/UserActionInfoContainer.hpp>

#include "Application.hpp"

namespace
{
  const char USAGE[] =
    "[OPTIONS] [COMMAND] [COMMAND ARGUMENTS]\n"
    "COMMANDS:\n"
    "  help\n"
    "  print-request [<request-id>]\n"
    "  print-reach [<user-id>]\n"
    "  print-action [<user-id>]\n"
    "  print-passback [<request-id>]\n"
    "  print-fraud [<user-id>]\n"
    "  print-site-reach [<user-id>]\n"
    "  print-tag-request-groups [<user-id>]\n"
    "  clear-expired\n"
    "OPTIONS:\n"
    "  -r, --ref : RequestInfoManager CORBA reference.\n"
    "  -P, --host_and_port : host:port.\n"
    "  -i, --rid, --request-id : request id.\n"
    "  -u, --uid, --user-id : user id.\n"
    "  --plain : print plain representation.\n"
    "  -s, --sync : clear profiles synchroniously (for clear-expired command).\n"
    "NOTES:\n"
    "  if value of uid or rid start with symbol '-' escape it with \\"
    "\n"
    "Sample:\n"
    "  RequestInfoAdmin print-request '\\-lgNQrDiQtOF50DRxzfdtA..'"
    " -r corbaloc::xen.ocslab.com:12116/RequestInfoManager\n";
}

namespace
{
  const Table::Column USER_FRAUD_TABLE_COLUMNS[] =
  {
    Table::Column("fraud_time", Table::Column::TEXT),
    Table::Column("requests", Table::Column::TEXT),
    Table::Column("rollback_requests", Table::Column::TEXT),
    Table::Column("clicks", Table::Column::TEXT),
  };

  const Table::Column USER_SITE_REACH_TABLE_COLUMNS[] =
  {
    Table::Column("appearance_list", Table::Column::TEXT),
  };

  const Table::Column USER_TAG_REQUEST_GROUP_TABLE_COLUMNS[] =
  {
    Table::Column("country", Table::Column::TEXT),
    Table::Column("colo_id", Table::Column::NUMBER),
    Table::Column("site_id", Table::Column::NUMBER),
    Table::Column("page_load_id", Table::Column::NUMBER),
    Table::Column("referer_hash", Table::Column::NUMBER),
    Table::Column("min_time", Table::Column::TEXT),
    Table::Column("max_time", Table::Column::TEXT),
    Table::Column("tags", Table::Column::TEXT),
    Table::Column("ad_shown", Table::Column::NUMBER),
  };
}

namespace
{
  using namespace AdServer::RequestInfoSvcs;

  std::string
  remove_uuid_escape(const char* uuid)
  {
    std::string res = uuid;

    if(uuid[0] == '\\')
    {
      res = std::string(uuid + 1);
    }

    return res;
  }

  std::string
  print_motion_list(
    const UserFraudProtectionProfileReader::requests_Container& motions,
    const char* space_align)
  {
    std::ostringstream ostr;
    for(UserFraudProtectionProfileReader::requests_Container::const_iterator it =
          motions.begin();
        it != motions.end();
        ++it)
    {
      ostr << std::endl << space_align <<
        "[ " << (*it).request_id() << ", " <<
        Generics::Time((*it).time()).get_gm_time() << " ]";
    }

    return ostr.str();
  }
}

using namespace AdServer::RequestInfoSvcs;

struct NullProcessor :
  public virtual RequestContainerProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
  virtual void
  process_request(const RequestInfo&) noexcept
  {}

  virtual void
  process_impression(const ImpressionInfo&)
    noexcept
  {}

  virtual void
  process_action(
    ActionType, const Generics::Time&, const AdServer::Commons::RequestId&)
    noexcept
  {}

  virtual void
  process_custom_action(const AdServer::Commons::RequestId&, const AdvCustomActionInfo&)
    /*throw(Exception)*/
  {}

  virtual void
  process_impression_post_action(
    const AdServer::Commons::RequestId&,
    const RequestPostActionInfo&)
    /*throw(Exception)*/
  {}
};

typedef ReferenceCounting::SmartPtr<NullProcessor>
  NullProcessor_var;

/**/
Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::print_plain_(
  std::ostream& ostr,
  const void* buf,
  unsigned long size,
  const char* prefix)
  noexcept
{
  ostr << prefix << std::hex << std::setfill('0') << std::setw(2);

  for(unsigned long i = 0; i < size; ++i)
  {
    ostr << "0x" << (int)*((const unsigned char*)buf + i) << " ";
    if(i && (i + 1) % 16 == 0)
    {
      ostr << std::endl << prefix;
    }
  }

  ostr << std::dec << std::setw(0);
}

int
Application_::print(
  AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
  const char* request_id_str,
  bool print_plain)
  noexcept
{
  using namespace AdServer::RequestInfoSvcs;

  try
  {
    AdServer::Commons::RequestId request_id(request_id_str);

    AdServer::RequestInfoSvcs::RequestProfile_var request_profile;

    if(request_info_manager->get_profile(
          request_id_str,
          request_profile.out()))
    {
      // print plain
      if(print_plain)
      {
        print_plain_(
          std::cout,
          request_profile->get_buffer(),
          request_profile->length(),
          "  ");

        std::cout << std::endl;
      }

      // print content
      RequestInfoProfileReader request_reader(
        request_profile->get_buffer(),
        request_profile->length());

      AdServer::RequestInfoSvcs::print_request_info_profile(
        std::cout, request_reader);
    }
    else
    {
      std::cout << "Profile for '" << request_id_str << "' not found." << std::endl;
    }
    return 0;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::ImplementationException& e)
  {
    std::cerr
      << "Caught RequestInfoManager::ImplementationException: "
      << e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::NotReady& e)
  {
    std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_campaign_reach(
  AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
  const char* user_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    AdServer::RequestInfoSvcs::UserProfile_var
      user_campaign_reach_profile;

    if(request_info_manager->get_user_campaign_reach_profile(
         user_id_str,
         user_campaign_reach_profile.out()))
    {
      // print plain
      if(print_plain)
      {
        print_plain_(
          std::cout,
          user_campaign_reach_profile->get_buffer(),
          user_campaign_reach_profile->length(),
          "  ");

        std::cout << std::endl;
      }

      AdServer::RequestInfoSvcs::UserCampaignReachProfileReader
        user_campaign_reach_reader(
          user_campaign_reach_profile->get_buffer(),
          user_campaign_reach_profile->length());

      AdServer::RequestInfoSvcs::print_user_campaign_reach_profile(
        std::cout, user_campaign_reach_reader);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::ImplementationException& e)
  {
    std::cerr
      << "Caught RequestInfoManager::ImplementationException: "
      << e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::NotReady& e)
  {
    std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

void
Application_::print_user_action_buf_(
  const void* buf,
  unsigned long buf_size,
  bool print_plain,
  bool align)
  /*throw(eh::Exception)*/
{
  // print plain
  if(print_plain)
  {
    print_plain_(std::cout, buf, buf_size, "  ");

    std::cout << std::endl;
  }

  AdServer::RequestInfoSvcs::UserActionProfileReader
    user_action_reader(buf, buf_size);

  AdServer::RequestInfoSvcs::print_user_action_profile(
    std::cout, user_action_reader, align);
}

int
Application_::print_user_action(
  AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
  const char* user_id_str,
  bool print_plain,
  bool align)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    AdServer::RequestInfoSvcs::UserProfile_var
      user_action_profile;

    if(request_info_manager->get_user_action_profile(
         user_id_str,
         user_action_profile.out()))
    {
      print_user_action_buf_(
        user_action_profile->get_buffer(),
        user_action_profile->length(),
        print_plain,
        align);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::ImplementationException& e)
  {
    std::cerr
      << "Caught RequestInfoManager::ImplementationException: "
      << e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::NotReady& e)
  {
    std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_fraud_protection(
  AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
  const char* user_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    AdServer::RequestInfoSvcs::UserProfile_var
      user_fraud_profile;

    if(request_info_manager->get_user_fraud_protection_profile(
         user_id_str,
         user_fraud_profile.out()))
    {
      // print plain
      if(print_plain)
      {
        print_plain_(
          std::cout,
          user_fraud_profile->get_buffer(),
          user_fraud_profile->length(),
          "  ");

        std::cout << std::endl;
      }

      AdServer::RequestInfoSvcs::UserFraudProtectionProfileReader
        user_fraud_reader(
          user_fraud_profile->get_buffer(),
          user_fraud_profile->length());

      unsigned long columns =
        sizeof(USER_FRAUD_TABLE_COLUMNS) /
        sizeof(USER_FRAUD_TABLE_COLUMNS[0]);

      Table table(columns);

      for(unsigned long i = 0; i < columns; i++)
      {
        table.column(i, USER_FRAUD_TABLE_COLUMNS[i]);
      }

      Table::Row row(table.columns());

      row.add_field(Generics::Time(
        user_fraud_reader.fraud_time()).gm_ft());

      std::string space_align(table.value_align(), ' ');

      row.add_field(print_motion_list(
        user_fraud_reader.requests(), space_align.c_str()));
      row.add_field(print_motion_list(
        user_fraud_reader.rollback_requests(), space_align.c_str()));
      row.add_field(print_motion_list(
        user_fraud_reader.clicks(), space_align.c_str()));

      table.add_row(row);
      table.dump(std::cout);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::ImplementationException& e)
  {
    std::cerr << "Caught RequestInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::NotReady& e)
  {
    std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_site_reach(
  AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
  const char* user_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    AdServer::RequestInfoSvcs::UserProfile_var
      user_site_reach_profile;

    if(request_info_manager->get_user_site_reach_profile(
         user_id_str,
         user_site_reach_profile.out()))
    {
      // print plain
      if(print_plain)
      {
        print_plain_(
          std::cout,
          user_site_reach_profile->get_buffer(),
          user_site_reach_profile->length(),
          "  ");

        std::cout << std::endl;
      }

      AdServer::RequestInfoSvcs::UserSiteReachProfileReader
        user_site_reach_reader(
          user_site_reach_profile->get_buffer(),
          user_site_reach_profile->length());

      unsigned long columns =
        sizeof(USER_SITE_REACH_TABLE_COLUMNS) /
        sizeof(USER_SITE_REACH_TABLE_COLUMNS[0]);

      Table table(columns);

      for(unsigned long i = 0; i < columns; i++)
      {
        table.column(i, USER_SITE_REACH_TABLE_COLUMNS[i]);
      }

      Table::Row row(table.columns());

      std::string space_align(table.value_align(), ' ');

      std::ostringstream ostr;
      print_appearance_list(
        ostr,
        user_site_reach_reader.appearance_list(),
        space_align.c_str());
      row.add_field(ostr.str());

      table.add_row(row);
      table.dump(std::cout);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::ImplementationException& e)
  {
    std::cerr << "Caught RequestInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::NotReady& e)
  {
    std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_tag_request_group(
  AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
  const char* user_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    AdServer::RequestInfoSvcs::UserProfile_var
      user_tag_request_group_profile;

    if(request_info_manager->get_user_tag_request_group_profile(
         user_id_str,
         user_tag_request_group_profile.out()))
    {
      // print plain
      if(print_plain)
      {
        print_plain_(
          std::cout,
          user_tag_request_group_profile->get_buffer(),
          user_tag_request_group_profile->length(),
          "  ");

        std::cout << std::endl;
      }

      unsigned long columns =
        sizeof(USER_TAG_REQUEST_GROUP_TABLE_COLUMNS) /
        sizeof(USER_TAG_REQUEST_GROUP_TABLE_COLUMNS[0]);

      Table table(columns);

      for(unsigned long i = 0; i < columns; i++)
      {
        table.column(i, USER_TAG_REQUEST_GROUP_TABLE_COLUMNS[i]);
      }

      AdServer::RequestInfoSvcs::UserTagRequestMergeProfileReader
        user_tag_request_group_reader(
          user_tag_request_group_profile->get_buffer(),
          user_tag_request_group_profile->length());

      for(UserTagRequestMergeProfileReader::tag_groups_Container::const_iterator tg_it =
            user_tag_request_group_reader.tag_groups().begin();
          tg_it != user_tag_request_group_reader.tag_groups().end(); ++tg_it)
      {
        Table::Row row(table.columns());
        row.add_field((*tg_it).country());
        row.add_field((*tg_it).colo_id());
        row.add_field((*tg_it).site_id());
        row.add_field((*tg_it).page_load_id());
        row.add_field((*tg_it).referer_hash());
        row.add_field(Generics::Time((*tg_it).min_time()).gm_ft());
        row.add_field(Generics::Time((*tg_it).max_time()).gm_ft());
        {
          std::ostringstream tags_ostr;
          Algs::print(tags_ostr, (*tg_it).tags().begin(), (*tg_it).tags().end());
          row.add_field(tags_ostr.str());
        }
        row.add_field((*tg_it).ad_shown());
        table.add_row(row);
      }

      table.dump(std::cout);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::ImplementationException& e)
  {
    std::cerr << "Caught RequestInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::NotReady& e)
  {
    std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_action_from_file(
  const char* file,
  const char* user_id_str,
  bool print_plain,
  bool debug_plain,
  bool align,
  bool is_level_enable,
  bool is_rocksdb_enable)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    Logging::Logger_var logger(new Logging::Null::Logger);
    NullProcessor_var act_processor(new NullProcessor());

    auto number_threads = std::thread::hardware_concurrency();
    if (number_threads == 0)
    {
      number_threads = 30;
    }

    UServerUtils::Grpc::RocksDB::Config config;
    config.event_queue_max_size = 10000000;
    config.io_uring_flags = IORING_SETUP_ATTACH_WQ | IORING_FEAT_FAST_POLL | IOSQE_ASYNC;
    config.io_uring_size = 1024 * 8;
    config.number_io_urings = 2 * number_threads;
    auto rocksdb_manager_pool = std::make_shared<UServerUtils::Grpc::RocksDB::DataBaseManagerPool>(
      config,
      logger.in());

    AdServer::ProfilingCommons::RocksDB::RocksDBParams rocksdb_params{
      10,
      1000000};

    UserActionInfoContainer_var act_container(
      new UserActionInfoContainer(
        logger,
        act_processor,
        file,
        "UserAction_",
        is_level_enable,
        is_rocksdb_enable,
        rocksdb_manager_pool,
        rocksdb_params,
        Generics::Time::ZERO, // action_ignore_time
        0, // cache
        Generics::Time(1000000), // expire time (sec)
        Generics::Time(200000)));

    Generics::ConstSmartMemBuf_var buf = act_container->get_profile(user_id);

    if(buf.in())
    {
      print_user_action_buf_(
        buf->membuf().data(),
        buf->membuf().size(),
        print_plain,
        align);

      if(debug_plain)
      {
        malloc_stats();

        AdServer::RequestInfoSvcs::UserActionProfileWriter user_action_writer;
        user_action_writer.init(buf->membuf().data(), buf->membuf().size());

        UserActionProfileWriter::action_markers_Container& lst = user_action_writer.action_markers();
        //       std::list<ActionMarkerWriter>& lst = user_action_writer.action_markers();
        std::cout << "number of action_markers from writer: " << lst.size() << std::endl;

        malloc_stats();
      }
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::ImplementationException& e)
  {
    std::cerr
      << "Caught RequestInfoManager::ImplementationException: "
      << e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::NotReady& e)
  {
    std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_passback(
  AdServer::RequestInfoSvcs::RequestInfoManager* request_info_manager,
  const char* request_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::RequestId request_id(request_id_str);

    AdServer::RequestInfoSvcs::PassbackProfile_var passback_profile;

    if(request_info_manager->get_passback_profile(
         request_id_str,
         passback_profile.out()))
    {
      // print plain
      if(print_plain)
      {
        print_plain_(
          std::cout,
          passback_profile->get_buffer(),
          passback_profile->length(),
          "  ");

        std::cout << std::endl;
      }

      // print content
      AdServer::RequestInfoSvcs::PassbackInfoReader
        passback_reader(
          passback_profile->get_buffer(),
          passback_profile->length());

      print_passback_profile(std::cout, passback_reader);
    }
    else
    {
      std::cout << "Profile for '" << request_id_str << "' not found." << std::endl;
    }
    return 0;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::ImplementationException& e)
  {
    std::cerr <<
      "Caught RequestInfoManager::ImplementationException: " <<
      e.description << std::endl;
  }
  catch(const AdServer::RequestInfoSvcs::
        RequestInfoManager::NotReady& e)
  {
    std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
  }
  catch(const CORBA::SystemException& e)
  {
    std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

void Application_::check_option_(
  const char *opt_name,
  Generics::AppUtils::Option<std::string>& option,
  const std::string& value)
  /*throw(InvalidParam)*/
{
  if(!option.installed())
  {
    if(!value.empty())
    {
      option.set_value(value);
    }
    else
    {
      Stream::Error err;
      err << "required option '" << opt_name << "' doesn't set";
      throw InvalidParam(err);
    }
  }
}


int
Application_::main(int& argc, char** argv) noexcept
{
  if (!::setlocale(LC_CTYPE, "en_US.utf8"))
  {
    std::cerr << "Cannot set locale." << std::endl;
  }

  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::Option<std::string> opt_ref;
  Generics::AppUtils::Option<std::string> opt_endpoint;
  Generics::AppUtils::Option<std::string> opt_request_id;
  Generics::AppUtils::Option<std::string> opt_user_id;
  Generics::AppUtils::CheckOption opt_user_temporary;
  Generics::AppUtils::Args args(-1);
  Generics::AppUtils::CheckOption opt_print_plain;
  Generics::AppUtils::CheckOption opt_debug_plain;
  Generics::AppUtils::CheckOption opt_sync;
  Generics::AppUtils::Option<std::string> opt_file;
  Generics::AppUtils::CheckOption opt_align;
  Generics::AppUtils::CheckOption opt_level;
  Generics::AppUtils::CheckOption opt_rocksdb;

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);

  args.add(
    Generics::AppUtils::equal_name("host_and_port") ||
    Generics::AppUtils::short_name("P"),
    opt_endpoint);

  args.add(
    Generics::AppUtils::equal_name("ref") ||
    Generics::AppUtils::short_name("r"),
    opt_ref);

  args.add(
    Generics::AppUtils::equal_name("rid") ||
    Generics::AppUtils::equal_name("request-id") ||
    Generics::AppUtils::short_name("i"),
    opt_request_id);

  args.add(
    Generics::AppUtils::equal_name("uid") ||
    Generics::AppUtils::equal_name("user-id") ||
    Generics::AppUtils::short_name("u"),
    opt_user_id);

  args.add(
    Generics::AppUtils::equal_name("temporary") ||
    Generics::AppUtils::short_name("t"),
    opt_user_temporary);

  args.add(
    Generics::AppUtils::short_name("f"),
    opt_file);

  args.add(
    Generics::AppUtils::equal_name("plain"),
    opt_print_plain);

  args.add(
    Generics::AppUtils::equal_name("align"),
    opt_align);

  args.add(
    Generics::AppUtils::equal_name("rocksdb"),
    opt_rocksdb);

  args.add(
    Generics::AppUtils::equal_name("level"),
    opt_level);

  args.add(
    Generics::AppUtils::equal_name("plain-debug"),
    opt_debug_plain);

  args.add(
    Generics::AppUtils::equal_name("sync") ||
    Generics::AppUtils::short_name("s"),
    opt_sync);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  std::string command = *commands.begin();
  std::string first_option;
  if(commands.size() > 1)
  {
    first_option = *(++commands.begin());
  }

  if (command == "print-request" ||
    command == "print-reach" ||
    command == "print-action" ||
    command == "print-passback" ||
    command == "print-fraud" ||
    command == "print-site-reach" ||
    command == "print-tag-request-groups" ||
    command == "clear-expired")
  {
    if(command == "print-action" && opt_file.installed())
    {
      check_option_("user-id", opt_user_id, first_option);
      return print_user_action_from_file(
        opt_file->c_str(),
        remove_uuid_escape(opt_user_id->c_str()).c_str(),
        opt_print_plain.enabled(),
        opt_debug_plain.enabled(),
        opt_align.enabled(),
        opt_level.enabled(),
        opt_rocksdb.enabled());
    }

    try
    {
      if(!opt_ref.installed())
      {
        check_option_("ref", opt_endpoint);
        std::string ref = "corbaloc::";
        ref += *opt_endpoint;
        ref += "/RequestInfoManager";
        opt_ref.set_value(ref);
      }

      CORBACommons::CorbaClientAdapter_var corba_client_adapter(
        new CORBACommons::CorbaClientAdapter());

      AdServer::RequestInfoSvcs::RequestInfoManager_var request_info_manager;

      CORBACommons::CorbaObjectRef corba_object_ref(opt_ref->c_str());

      request_info_manager = corba_client_adapter->resolve_object<
        AdServer::RequestInfoSvcs::RequestInfoManager>(corba_object_ref);

      if(command == "print-request")
      {
        check_option_("request-id", opt_request_id, first_option);

        return print(
          request_info_manager,
          remove_uuid_escape(
              opt_request_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if(command == "print-reach")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_campaign_reach(
          request_info_manager,
          remove_uuid_escape(
              opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if(command == "print-action")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_action(
          request_info_manager,
          remove_uuid_escape(
              opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled(),
          opt_align.enabled());
      }
      else if(command == "print-fraud")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_fraud_protection(
          request_info_manager,
          remove_uuid_escape(
              opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if(command == "print-passback")
      {
        check_option_("request-id", opt_request_id, first_option);

        return print_passback(
          request_info_manager,
          remove_uuid_escape(
              opt_request_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if(command == "print-site-reach")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_site_reach(
          request_info_manager,
          remove_uuid_escape(
              opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if(command == "print-tag-request-groups")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_tag_request_group(
          request_info_manager,
          remove_uuid_escape(
              opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if(command == "clear-expired")
      {
        try
        {
          request_info_manager->clear_expired(opt_sync.enabled());
          return 0;
        }
        catch(const CORBA::SystemException& e)
        {
          std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
        }
      }
      return 1;
    }
    catch(const InvalidParam& e)
    {
      std::cerr << e.what() << std::endl;
    }
    catch(const AdServer::RequestInfoSvcs::
          RequestInfoManager::ImplementationException& e)
    {
      std::cerr <<
        "Caught RequestInfoManager::ImplementationException: " <<
        e.description << std::endl;
    }
    catch(const AdServer::RequestInfoSvcs::
          RequestInfoManager::NotReady& e)
    {
      std::cerr << "Caught RequestInfoManager::NotReady." << std::endl;
    }
    catch(const CORBA::SystemException& e)
    {
      std::cerr << "Caught CORBA::SystemException: " << e << std::endl;
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    }
    return 1;
  } // print
  else
  {
    if (command != "help")
    {
      std::cerr
        << "Unknown command '" << command << "'" << std::endl;
      return 1;
    }
    std::cout  << USAGE << std::endl;
  }
  return 0;
}

int
main(int argc, char** argv)
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

  return app->main(argc, argv);
}
