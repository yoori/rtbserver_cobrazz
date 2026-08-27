#include <malloc.h>
#include <list>
#include <iterator>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

#include <String/StringManip.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/ArrayAutoPtr.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>

#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>
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
  namespace Proto = adserver::request_info_svcs::request_info_manager;

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
    "  -r, --ref : RequestInfoManager gRPC endpoint.\n"
    "  -P, --host_and_port : RequestInfoManager gRPC endpoint.\n"
    "  -i, --rid, --request-id : request id.\n"
    "  -u, --uid, --user-id : user id.\n"
    "  --plain : print plain representation.\n"
    "  -s, --sync : clear profiles synchroniously (for clear-expired command).\n"
    "NOTES:\n"
    "  if value of uid or rid start with symbol '-' escape it with \\"
    "\n"
    "Sample:\n"
    "  RequestInfoAdmin print-request '\\-lgNQrDiQtOF50DRxzfdtA..'"
    " -r xen.ocslab.com:12116\n";

  std::string
  normalize_endpoint(std::string endpoint)
  {
    constexpr std::string_view corbaloc_prefix = "corbaloc::";
    constexpr std::string_view iiop_prefix = "corbaloc:iiop:";
    if (endpoint.compare(0, corbaloc_prefix.size(), corbaloc_prefix) == 0)
    {
      endpoint.erase(0, corbaloc_prefix.size());
    }
    else if (endpoint.compare(0, iiop_prefix.size(), iiop_prefix) == 0)
    {
      endpoint.erase(0, iiop_prefix.size());
    }

    const std::size_t object_key_pos = endpoint.find('/');
    if (object_key_pos != std::string::npos)
    {
      endpoint.resize(object_key_pos);
    }
    return endpoint;
  }

  template<typename Response, typename Start>
  std::optional<Response>
  sync_call(Start&& start) noexcept
  {
    try
    {
      grpc::Status error;
      auto response = AdServer::Grpc::sync_call<Response>(
        std::forward<Start>(start),
        [&error](const grpc::Status& status)
        {
          error = status;
        });

      if (error.ok())
      {
        return response;
      }

      std::cerr << "RequestInfoManager gRPC call failed: code=" <<
        error.error_code() << ", message=" << error.error_message() << '\n';
    }
    catch(const std::exception& ex)
    {
      std::cerr << "RequestInfoManager gRPC call failed: " << ex.what() << '\n';
    }

    return std::nullopt;
  }

  template<typename Call>
  bool
  get_profile(Call&& call, const char* id, Proto::ProfileResponse& response)
  {
    Proto::ProfileRequest request;
    request.set_id(id);
    auto result = sync_call<Proto::ProfileResponse>(
      [&call, &request](auto callback)
      {
        call(request, std::move(callback));
      });
    if (!result)
    {
      return false;
    }

    response = std::move(*result);
    return true;
  }

  struct ClientHolder
  {
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner;
    std::shared_ptr<AdServer::RequestInfoSvcs::RequestInfoManagerGrpcAsyncBatchingClient> client;
  };

  ClientHolder
  create_client(const std::string& endpoint)
  {
    ClientHolder result;
    result.grpc_executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    result.grpc_executor->activate_object();

    Logging::Logger_var logger = new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
    result.coalesce_runner =
      std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
        new Logging::ActiveObjectCallbackImpl(logger, "RequestInfoAdmin", "gRPC"),
        std::make_shared<boost::asio::io_service>(),
        1);
    result.coalesce_runner->activate_object();

    AdServer::Grpc::BatchingOptions batching_options;
    batching_options.max_batch_delay = Generics::Time::ZERO;
    result.client = std::make_shared<AdServer::RequestInfoSvcs::
      RequestInfoManagerGrpcAsyncBatchingClient>(
        endpoint,
        result.grpc_executor,
        result.coalesce_runner,
        batching_options);
    result.client->activate_object();
    return result;
  }

  void
  shutdown_client(ClientHolder& holder) noexcept
  {
    try
    {
      if (holder.client)
      {
        holder.client->deactivate_object();
        holder.client->wait_object();
      }

      if (holder.grpc_executor)
      {
        holder.grpc_executor->deactivate_object();
        holder.grpc_executor->wait_object();
      }

      if (holder.coalesce_runner)
      {
        holder.coalesce_runner->deactivate_object();
        holder.coalesce_runner->wait_object();
      }
    }
    catch(...)
    {}
  }
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

    if (uuid[0] == '\\')
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
    for (UserFraudProtectionProfileReader::requests_Container::const_iterator it = motions.begin();
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
  process_action(ActionType, const Generics::Time&, const AdServer::Commons::RequestId&)
    noexcept
  {}

  virtual void
  process_custom_action(const AdServer::Commons::RequestId&, const AdvCustomActionInfo&)
    /*throw(Exception)*/
  {}

  virtual void
  process_impression_post_action(const AdServer::Commons::RequestId&, const RequestPostActionInfo&)
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

  for (unsigned long i = 0; i < size; ++i)
  {
    ostr << "0x" << (int)*((const unsigned char*)buf + i) << " ";
    if (i && (i + 1) % 16 == 0)
    {
      ostr << std::endl << prefix;
    }
  }

  ostr << std::dec << std::setw(0);
}

int
Application_::print(Client& request_info_manager, const char* request_id_str, bool print_plain)
  noexcept
{
  using namespace AdServer::RequestInfoSvcs;

  try
  {
    AdServer::Commons::RequestId request_id(request_id_str);

    Proto::ProfileResponse request_profile;
    if (!get_profile(
         [&request_info_manager](const Proto::ProfileRequest& request, auto callback)
         {
           request_info_manager.get_profile(request, std::move(callback));
         },
         request_id_str,
         request_profile))
    {
      return 1;
    }

    if (request_profile.found())
    {
      const std::string& profile = request_profile.profile();

      // print plain
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), "  ");

        std::cout << std::endl;
      }

      // print content
      RequestInfoProfileReader request_reader(profile.data(), profile.size());

      AdServer::RequestInfoSvcs::print_request_info_profile(std::cout, request_reader);
    }
    else
    {
      std::cout << "Profile for '" << request_id_str << "' not found." << std::endl;
    }
    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_campaign_reach(
  Client& request_info_manager,
  const char* user_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    Proto::ProfileResponse user_campaign_reach_profile;
    if (!get_profile(
         [&request_info_manager](const Proto::ProfileRequest& request, auto callback)
         {
           request_info_manager.get_user_campaign_reach_profile(request, std::move(callback));
         },
         user_id_str,
         user_campaign_reach_profile))
    {
      return 1;
    }

    if (user_campaign_reach_profile.found())
    {
      const std::string& profile = user_campaign_reach_profile.profile();

      // print plain
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), "  ");

        std::cout << std::endl;
      }

      AdServer::RequestInfoSvcs::UserCampaignReachProfileReader
        user_campaign_reach_reader(profile.data(), profile.size());

      AdServer::RequestInfoSvcs::print_user_campaign_reach_profile(
        std::cout, user_campaign_reach_reader);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
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
  if (print_plain)
  {
    print_plain_(std::cout, buf, buf_size, "  ");

    std::cout << std::endl;
  }

  AdServer::RequestInfoSvcs::UserActionProfileReader
    user_action_reader(buf, buf_size);

  AdServer::RequestInfoSvcs::print_user_action_profile(std::cout, user_action_reader, align);
}

int
Application_::print_user_action(
  Client& request_info_manager,
  const char* user_id_str,
  bool print_plain,
  bool align)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    Proto::ProfileResponse user_action_profile;
    if (!get_profile(
         [&request_info_manager](const Proto::ProfileRequest& request, auto callback)
         {
           request_info_manager.get_user_action_profile(request, std::move(callback));
         },
         user_id_str,
         user_action_profile))
    {
      return 1;
    }

    if (user_action_profile.found())
    {
      const std::string& profile = user_action_profile.profile();
      print_user_action_buf_(profile.data(), profile.size(), print_plain, align);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_fraud_protection(
  Client& request_info_manager,
  const char* user_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    Proto::ProfileResponse user_fraud_profile;
    if (!get_profile(
         [&request_info_manager](const Proto::ProfileRequest& request, auto callback)
         {
           request_info_manager.get_user_fraud_protection_profile(request, std::move(callback));
         },
         user_id_str,
         user_fraud_profile))
    {
      return 1;
    }

    if (user_fraud_profile.found())
    {
      const std::string& profile = user_fraud_profile.profile();

      // print plain
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), "  ");

        std::cout << std::endl;
      }

      AdServer::RequestInfoSvcs::UserFraudProtectionProfileReader
        user_fraud_reader(profile.data(), profile.size());

      unsigned long columns =
        sizeof(USER_FRAUD_TABLE_COLUMNS) /
        sizeof(USER_FRAUD_TABLE_COLUMNS[0]);

      Table table(columns);

      for (unsigned long i = 0; i < columns; i++)
      {
        table.column(i, USER_FRAUD_TABLE_COLUMNS[i]);
      }

      Table::Row row(table.columns());

      row.add_field(Generics::Time(user_fraud_reader.fraud_time()).gm_ft());

      std::string space_align(table.value_align(), ' ');

      row.add_field(print_motion_list(user_fraud_reader.requests(), space_align.c_str()));
      row.add_field(print_motion_list(user_fraud_reader.rollback_requests(), space_align.c_str()));
      row.add_field(print_motion_list(user_fraud_reader.clicks(), space_align.c_str()));

      table.add_row(row);
      table.dump(std::cout);
    }
    else
    {
      std::cout << "Profile not found." << std::endl;
    }
    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_site_reach(
  Client& request_info_manager,
  const char* user_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    Proto::ProfileResponse user_site_reach_profile;
    if (!get_profile(
         [&request_info_manager](const Proto::ProfileRequest& request, auto callback)
         {
           request_info_manager.get_user_site_reach_profile(request, std::move(callback));
         },
         user_id_str,
         user_site_reach_profile))
    {
      return 1;
    }

    if (user_site_reach_profile.found())
    {
      const std::string& profile = user_site_reach_profile.profile();

      // print plain
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), "  ");

        std::cout << std::endl;
      }

      AdServer::RequestInfoSvcs::UserSiteReachProfileReader
        user_site_reach_reader(profile.data(), profile.size());

      unsigned long columns =
        sizeof(USER_SITE_REACH_TABLE_COLUMNS) /
        sizeof(USER_SITE_REACH_TABLE_COLUMNS[0]);

      Table table(columns);

      for (unsigned long i = 0; i < columns; i++)
      {
        table.column(i, USER_SITE_REACH_TABLE_COLUMNS[i]);
      }

      Table::Row row(table.columns());

      std::string space_align(table.value_align(), ' ');

      std::ostringstream ostr;
      print_appearance_list(ostr, user_site_reach_reader.appearance_list(), space_align.c_str());
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
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_user_tag_request_group(
  Client& request_info_manager,
  const char* user_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    Proto::ProfileResponse user_tag_request_group_profile;
    if (!get_profile(
         [&request_info_manager](const Proto::ProfileRequest& request, auto callback)
         {
           request_info_manager.get_user_tag_request_group_profile(request, std::move(callback));
         },
         user_id_str,
         user_tag_request_group_profile))
    {
      return 1;
    }

    if (user_tag_request_group_profile.found())
    {
      const std::string& profile = user_tag_request_group_profile.profile();

      // print plain
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), "  ");

        std::cout << std::endl;
      }

      unsigned long columns =
        sizeof(USER_TAG_REQUEST_GROUP_TABLE_COLUMNS) /
        sizeof(USER_TAG_REQUEST_GROUP_TABLE_COLUMNS[0]);

      Table table(columns);

      for (unsigned long i = 0; i < columns; i++)
      {
        table.column(i, USER_TAG_REQUEST_GROUP_TABLE_COLUMNS[i]);
      }

      AdServer::RequestInfoSvcs::UserTagRequestMergeProfileReader
        user_tag_request_group_reader(profile.data(), profile.size());

      for (UserTagRequestMergeProfileReader::tag_groups_Container::const_iterator tg_it =
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
  bool align)
  noexcept
{
  try
  {
    AdServer::Commons::UserId user_id(user_id_str);

    Logging::Logger_var logger(new Logging::Null::Logger);
    NullProcessor_var act_processor(new NullProcessor());

    UserActionInfoContainer_var act_container(
      new UserActionInfoContainer(
        logger,
        act_processor,
        file,
        Generics::Time::ZERO, // action_ignore_time
        Generics::Time(1000000))); // expire time (sec)

    Generics::ConstSmartMemBuf_var buf = act_container->get_profile(user_id);

    if (buf.in())
    {
      print_user_action_buf_(buf->membuf().data(), buf->membuf().size(), print_plain, align);

      if (debug_plain)
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
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }
  return 1;
}

int
Application_::print_passback(
  Client& request_info_manager,
  const char* request_id_str,
  bool print_plain)
  noexcept
{
  try
  {
    AdServer::Commons::RequestId request_id(request_id_str);

    Proto::ProfileResponse passback_profile;
    if (!get_profile(
         [&request_info_manager](const Proto::ProfileRequest& request, auto callback)
         {
           request_info_manager.get_passback_profile(request, std::move(callback));
         },
         request_id_str,
         passback_profile))
    {
      return 1;
    }

    if (passback_profile.found())
    {
      const std::string& profile = passback_profile.profile();

      // print plain
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), "  ");

        std::cout << std::endl;
      }

      // print content
      AdServer::RequestInfoSvcs::PassbackInfoReader
        passback_reader(profile.data(), profile.size());

      print_passback_profile(std::cout, passback_reader);
    }
    else
    {
      std::cout << "Profile for '" << request_id_str << "' not found." << std::endl;
    }
    return 0;
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
  if (!option.installed())
  {
    if (!value.empty())
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

  args.add(Generics::AppUtils::equal_name("help") || Generics::AppUtils::short_name("h"), opt_help);

  args.add(
    Generics::AppUtils::equal_name("host_and_port") ||
    Generics::AppUtils::short_name("P"),
    opt_endpoint);

  args.add(Generics::AppUtils::equal_name("ref") || Generics::AppUtils::short_name("r"), opt_ref);

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

  args.add(Generics::AppUtils::short_name("f"), opt_file);

  args.add(Generics::AppUtils::equal_name("plain"), opt_print_plain);

  args.add(Generics::AppUtils::equal_name("align"), opt_align);

  args.add(Generics::AppUtils::equal_name("plain-debug"), opt_debug_plain);

  args.add(Generics::AppUtils::equal_name("sync") || Generics::AppUtils::short_name("s"), opt_sync);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if (commands.empty() || opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  std::string command = *commands.begin();
  std::string first_option;
  if (commands.size() > 1)
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
    if (command == "print-action" && opt_file.installed())
    {
      check_option_("user-id", opt_user_id, first_option);
      return print_user_action_from_file(
        opt_file->c_str(),
        remove_uuid_escape(opt_user_id->c_str()).c_str(),
        opt_print_plain.enabled(),
        opt_debug_plain.enabled(),
        opt_align.enabled());
    }

    try
    {
      if (!opt_ref.installed())
      {
        check_option_("host_and_port", opt_endpoint);
        opt_ref.set_value(*opt_endpoint);
      }

      const std::string endpoint = normalize_endpoint(*opt_ref);
      ClientHolder client_holder = create_client(endpoint);
      auto client_guard = std::unique_ptr<ClientHolder, void(*)(ClientHolder*)>(
        &client_holder,
        [](ClientHolder* holder)
        {
          shutdown_client(*holder);
        });
      auto* request_info_manager = client_holder.client.get();

      if (command == "print-request")
      {
        check_option_("request-id", opt_request_id, first_option);

        return print(
          *request_info_manager,
          remove_uuid_escape(opt_request_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if (command == "print-reach")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_campaign_reach(
          *request_info_manager,
          remove_uuid_escape(opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if (command == "print-action")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_action(
          *request_info_manager,
          remove_uuid_escape(opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled(),
          opt_align.enabled());
      }
      else if (command == "print-fraud")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_fraud_protection(
          *request_info_manager,
          remove_uuid_escape(opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if (command == "print-passback")
      {
        check_option_("request-id", opt_request_id, first_option);

        return print_passback(
          *request_info_manager,
          remove_uuid_escape(opt_request_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if (command == "print-site-reach")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_site_reach(
          *request_info_manager,
          remove_uuid_escape(opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if (command == "print-tag-request-groups")
      {
        check_option_("user-id", opt_user_id, first_option);

        return print_user_tag_request_group(
          *request_info_manager,
          remove_uuid_escape(opt_user_id->c_str()).c_str(),
          opt_print_plain.enabled());
      }
      else if (command == "clear-expired")
      {
        Proto::ClearExpiredRequest request;
        request.set_synchronous(opt_sync.enabled());
        const auto response = sync_call<Proto::ClearExpiredResponse>(
          [request_info_manager, &request](auto callback)
          {
            request_info_manager->clear_expired(request, std::move(callback));
          });
        return response ? 0 : 1;
      }
      return 1;
    }
    catch(const InvalidParam& e)
    {
      std::cerr << e.what() << std::endl;
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
      std::cerr << "Unknown command '" << command << "'" << std::endl;
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
