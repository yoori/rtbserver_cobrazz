#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <Generics/ActiveObject.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/UserInfoManip.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <UserInfoSvcs/UserInfoClient/UserInfoDistributedGrpcClient.hpp>
#include <UserInfoSvcs/UserInfoCommons/ChannelMatcher.hpp>
#include <UserInfoSvcs/UserInfoCommons/UserFreqCapProfile.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerGrpc.grpc.pb.h>
#include <UserInfoSvcs/UserInfoController/UserInfoControllerGrpc.grpc.pb.h>

namespace
{
  namespace uim = adserver::user_info_svcs::user_info_manager;
  namespace uic = adserver::user_info_svcs::user_info_controller;

  const char TIME_FORMAT[] = "%d-%m-%Y:%H-%M-%S.%q";

  const char USAGE[] =
    "\nUsage: \nUserInfoAdmin <command> <command arguments>\n\n"
    "UserInfoAdmin match --uid=<user id> --page-channels=<ids> "
      "-r[--reference=]<user_info_controller_grpc_endpoint>|"
      "<user_info_manager_grpc_endpoint> --time=<DD-MM-YYYY:HH-MM-SS>\n\n"
    "UserInfoAdmin [print|print-freq-cap|print-base|print-add|print-history] "
      "--uid=<user id> [-e|--expand] [-a|--align] [-p|--plain] "
      "-r[--reference=]<grpc endpoint>\n\n"
    "UserInfoAdmin remove --uid=<user id> -r[--reference=]<grpc endpoint>\n\n"
    "UserInfoAdmin delete-old-profiles -r[--reference=]<grpc endpoint> "
      "--sync --time=<HH-MM> --portion=<number>\n\n"
    "UserInfoAdmin get-config-timestamp -r[--reference=]<grpc endpoint>\n\n"
    "UserInfoAdmin generate --keys-dir=<path to key directory> --persistent\n";

  const unsigned long PRINT_BASE = 0x01;
  const unsigned long PRINT_ADD = 0x02;
  const unsigned long PRINT_HISTORY = 0x04;
  const unsigned long PRINT_ALL = static_cast<unsigned long>(-1);
  const std::chrono::seconds RPC_TIMEOUT(10);

  struct ChannelMatch
  {
    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  bool operator<(const ChannelMatch& left, const ChannelMatch& right) noexcept
  {
    return left.channel_id < right.channel_id ||
      (left.channel_id == right.channel_id &&
        left.channel_trigger_id < right.channel_trigger_id);
  }

  std::string bytes_from_user_id_(const AdServer::Commons::UserId& user_id)
  {
    return GrpcAlgs::pack_user_id(user_id);
  }

  std::string bytes_from_time_(const Generics::Time& time)
  {
    return GrpcAlgs::pack_time(time);
  }

  Generics::Time time_from_bytes_(const std::string& bytes)
  {
    return GrpcAlgs::unpack_time(bytes);
  }

  void print_plain_(
    std::ostream& ostr,
    const void* buf,
    unsigned long size,
    const char* prefix)
    noexcept
  {
    ostr << prefix;
    for (unsigned long i = 0; i < size; ++i)
    {
      ostr << "0x" << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<int>(*(static_cast<const unsigned char*>(buf) + i)) <<
        " ";
      if (i && (i + 1) % 16 == 0)
      {
        ostr << std::endl << prefix;
      }
    }
    ostr << std::endl << std::dec << std::setw(0);
  }

  template<typename T>
  void tokenize_channels_(
    std::set<T>& matched_channel_ids,
    const String::SubString& matched_channels)
  {
    String::StringManip::Splitter<
      const String::AsciiStringManip::Char2Category<',', ' '> >
      tokenizer(matched_channels);

    String::SubString channel;
    while (tokenizer.get_token(channel))
    {
      int value = 0;
      String::StringManip::str_to_int(channel, value);
      matched_channel_ids.insert(static_cast<unsigned long>(value));
    }
  }

  void tokenize_channels_(
    std::set<ChannelMatch>& matched_channel_ids,
    const String::SubString& matched_channels)
  {
    String::StringManip::Splitter<
      const String::AsciiStringManip::Char2Category<',', ' '> >
      tokenizer(matched_channels);

    String::SubString channel;
    while (tokenizer.get_token(channel))
    {
      int value = 0;
      String::StringManip::str_to_int(channel, value);
      matched_channel_ids.insert(ChannelMatch{
        static_cast<unsigned long>(value),
        static_cast<unsigned long>(value)});
    }
  }

  bool is_user_info_controller_(const std::string& reference)
  {
    auto stub = uic::UserInfoControllerGrpc::NewStub(
      grpc::CreateChannel(reference, grpc::InsecureChannelCredentials()));

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + RPC_TIMEOUT);
    uic::GetSessionDescriptionRequest request;
    uic::GetSessionDescriptionResponse response;
    const auto status = stub->get_session_description(
      &context,
      request,
      &response);

    return status.ok() && !response.user_info_managers().empty();
  }

  struct UserInfoClientHolder
  {
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner;
    std::shared_ptr<Generics::ActiveObject> active_object;
    std::shared_ptr<AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient>
      client;
  };

  UserInfoClientHolder create_user_info_client_(const std::string& reference)
  {
    UserInfoClientHolder result;
    result.grpc_executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    result.grpc_executor->activate_object();
    Logging::Logger_var logger =
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
    result.coalesce_runner =
      std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
        new Logging::ActiveObjectCallbackImpl(logger, "UserInfoAdmin", "gRPC"),
        std::make_shared<boost::asio::io_service>(),
        1);
    result.coalesce_runner->activate_object();

    if (is_user_info_controller_(reference))
    {
      auto client =
        std::make_shared<AdServer::UserInfoSvcs::UserInfoDistributedGrpcClient>(
          std::vector<std::string>{reference},
          AdServer::Grpc::BatchingOptions(),
          result.grpc_executor,
          logger,
          result.coalesce_runner);
      result.client = client;
      result.active_object = client;
    }
    else
    {
      auto client = std::make_shared<
        AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncBatchingClient>(
          reference,
          result.grpc_executor,
          result.coalesce_runner,
          AdServer::Grpc::BatchingOptions());
      result.client = client;
      result.active_object = client;
    }

    result.active_object->activate_object();
    return result;
  }

  void shutdown_user_info_client_(UserInfoClientHolder& holder) noexcept
  {
    try
    {
      if (holder.active_object)
      {
        holder.active_object->deactivate_object();
        holder.active_object->wait_object();
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
    catch (...)
    {}
  }

  void print_profile_(
    AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient* client,
    const AdServer::Commons::UserId& user_id,
    bool print_plain,
    bool print_expand,
    bool print_align,
    unsigned long print_kind)
  {
    uim::GetUserProfileRequest request;
    request.set_user_id(bytes_from_user_id_(user_id));
    request.set_temporary(false);
    request.mutable_profile_request()->set_base_profile(
      (print_kind & PRINT_BASE) != 0);
    request.mutable_profile_request()->set_add_profile(
      (print_kind & PRINT_ADD) != 0);
    request.mutable_profile_request()->set_history_profile(
      (print_kind & PRINT_HISTORY) != 0);
    request.mutable_profile_request()->set_freq_cap_profile(
      print_kind == 0);

    const auto response =
      AdServer::Grpc::sync_call<uim::GetUserProfileResponse>(
        [&](auto callback)
        {
          client->get_user_profile(request, std::move(callback));
        });

    if ((print_kind & PRINT_BASE) != 0)
    {
      const auto& profile = response.user_profile().base_user_profile();
      std::cout << "Base profile:" << std::endl;
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), " ");
      }
      AdServer::UserInfoSvcs::ChannelsMatcher::print(
        profile.data(),
        profile.size(),
        std::cout,
        print_expand,
        print_align);
    }

    if ((print_kind & PRINT_ADD) != 0)
    {
      const auto& profile = response.user_profile().add_user_profile();
      std::cout << "Additional profile:" << std::endl;
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), " ");
      }
      AdServer::UserInfoSvcs::ChannelsMatcher::print(
        profile.data(),
        profile.size(),
        std::cout,
        print_expand,
        print_align);
    }

    if ((print_kind & PRINT_HISTORY) != 0)
    {
      const auto& profile = response.user_profile().history_user_profile();
      std::cout << "History profile:" << std::endl;
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), " ");
      }
      AdServer::UserInfoSvcs::ChannelsMatcher::history_print(
        profile.data(),
        profile.size(),
        std::cout,
        print_align);
    }

    if (print_kind == 0)
    {
      const auto& profile = response.user_profile().freq_cap();
      std::cout << "FreqCap profile:" << std::endl;
      if (print_plain)
      {
        print_plain_(std::cout, profile.data(), profile.size(), " ");
      }
      Generics::ConstSmartMemBuf_var mb(
        new Generics::ConstSmartMemBuf(profile.data(), profile.size()));
      AdServer::UserInfoSvcs::UserFreqCapProfile freq_cap_profile(mb);
      freq_cap_profile.print(std::cout, 0);
    }
  }

  void match_(
    AdServer::UserInfoSvcs::UserInfoManagerGrpcAsyncClient* client,
    const AdServer::Commons::UserId& user_id,
    const String::SubString& opt_time,
    const String::SubString& matched_page_channels,
    const String::SubString& matched_search_channels,
    const String::SubString& matched_url_channels,
    const String::SubString& matched_persistent_channels)
  {
    uim::MatchRequest request;
    request.mutable_user_info()->set_user_id(bytes_from_user_id_(user_id));
    request.mutable_user_info()->set_temporary(false);
    request.mutable_user_info()->set_last_colo_id(-1);
    request.mutable_user_info()->set_current_colo_id(1);
    request.mutable_user_info()->set_time(
      Generics::Time(opt_time, "%d-%m-%Y:%H-%M-%S").tv_sec);

    std::set<ChannelMatch> page_channels;
    std::set<ChannelMatch> search_channels;
    std::set<ChannelMatch> url_channels;
    std::set<unsigned long> persistent_channels;
    tokenize_channels_(page_channels, matched_page_channels);
    tokenize_channels_(search_channels, matched_search_channels);
    tokenize_channels_(url_channels, matched_url_channels);
    tokenize_channels_(persistent_channels, matched_persistent_channels);

    for (const auto& channel : page_channels)
    {
      auto* out = request.mutable_match_params()->add_page_channel_ids();
      out->set_channel_id(channel.channel_id);
      out->set_channel_trigger_id(channel.channel_trigger_id);
    }
    for (const auto& channel : search_channels)
    {
      auto* out = request.mutable_match_params()->add_search_channel_ids();
      out->set_channel_id(channel.channel_id);
      out->set_channel_trigger_id(channel.channel_trigger_id);
    }
    for (const auto& channel : url_channels)
    {
      auto* out = request.mutable_match_params()->add_url_channel_ids();
      out->set_channel_id(channel.channel_id);
      out->set_channel_trigger_id(channel.channel_trigger_id);
    }
    for (const auto channel_id : persistent_channels)
    {
      request.mutable_match_params()->add_persistent_channel_ids(channel_id);
    }
    request.mutable_match_params()->set_publishers_optin_timeout(
      bytes_from_time_(Generics::Time::ZERO));

    const auto response =
      AdServer::Grpc::sync_call<uim::MatchResponse>(
        [&](auto callback)
        {
          client->match(request, std::move(callback));
        });

    std::cout << "Output Channels: ";
    for (int i = 0; i < response.match_result().channels_size(); ++i)
    {
      std::cout << (i ? "," : "") <<
        response.match_result().channels(i).channel_id();
    }
    std::cout << std::endl;
  }
}

int main(int argc, char** argv)
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::CheckOption opt_plain;
  Generics::AppUtils::CheckOption opt_expand;
  Generics::AppUtils::CheckOption opt_align;
  Generics::AppUtils::CheckOption opt_persistent;
  Generics::AppUtils::CheckOption opt_sync;
  Generics::AppUtils::Option<std::string> opt_user_id;
  Generics::AppUtils::Option<std::string> opt_reference;
  Generics::AppUtils::Option<std::string> opt_page_channels("");
  Generics::AppUtils::Option<std::string> opt_search_channels("");
  Generics::AppUtils::Option<std::string> opt_url_channels("");
  Generics::AppUtils::Option<std::string> opt_persistent_channels("");
  Generics::AppUtils::Option<std::string> opt_keys_directory;
  Generics::AppUtils::Option<std::string> opt_time;
  Generics::AppUtils::Option<unsigned long> opt_portion;
  Generics::AppUtils::Args args(-1);

  args.add(Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"), opt_help);
  args.add(Generics::AppUtils::equal_name("plain") ||
    Generics::AppUtils::short_name("p"), opt_plain);
  args.add(Generics::AppUtils::equal_name("expand") ||
    Generics::AppUtils::short_name("e"), opt_expand);
  args.add(Generics::AppUtils::equal_name("align") ||
    Generics::AppUtils::short_name("a"), opt_align);
  args.add(Generics::AppUtils::equal_name("user-id") ||
    Generics::AppUtils::equal_name("uid") ||
    Generics::AppUtils::short_name("u"), opt_user_id);
  args.add(Generics::AppUtils::equal_name("reference") ||
    Generics::AppUtils::short_name("r"), opt_reference);
  args.add(Generics::AppUtils::equal_name("page-channels"), opt_page_channels);
  args.add(Generics::AppUtils::equal_name("search-channels"), opt_search_channels);
  args.add(Generics::AppUtils::equal_name("url-channels"), opt_url_channels);
  args.add(Generics::AppUtils::equal_name("persistent-channels"),
    opt_persistent_channels);
  args.add(Generics::AppUtils::equal_name("keys-dir"), opt_keys_directory);
  args.add(Generics::AppUtils::equal_name("persistent"), opt_persistent);
  args.add(Generics::AppUtils::equal_name("time"), opt_time);
  args.add(Generics::AppUtils::equal_name("sync"), opt_sync);
  args.add(Generics::AppUtils::equal_name("portion"), opt_portion);

  try
  {
    args.parse(argc - 1, argv + 1);
    const auto& commands = args.commands();
    if (commands.empty() || opt_help.enabled() || *commands.begin() == "help")
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    const std::string command = *commands.begin();
    if (command == "generate" || command == "sign")
    {
      std::string keys_dir;
      if (opt_keys_directory.installed())
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
      if (command == "generate")
      {
        std::cout << "'" << generator.generate().str() << "'" << std::endl;
      }
      else
      {
        std::cout << "'" << generator.sign(
          AdServer::Commons::UserId(opt_user_id->c_str())).str() << "'" <<
          std::endl;
      }
      return 0;
    }

    if (!opt_reference.installed())
    {
      throw std::runtime_error("reference to gRPC manager or controller is required");
    }

    UserInfoClientHolder client_holder = create_user_info_client_(*opt_reference);
    auto client_guard = std::unique_ptr<
      UserInfoClientHolder,
      void(*)(UserInfoClientHolder*)>(
        &client_holder,
        [](UserInfoClientHolder* holder)
        {
          shutdown_user_info_client_(*holder);
        });
    auto* client = client_holder.client.get();

    if (command == "get-config-timestamp")
    {
      uim::GetMasterStampRequest request;
      const auto response =
        AdServer::Grpc::sync_call<uim::GetMasterStampResponse>(
          [&](auto callback)
          {
            client->get_master_stamp(request, std::move(callback));
          });
      std::cout << "Config timestamp : " <<
        time_from_bytes_(response.master_stamp()).get_gm_time().format(TIME_FORMAT) <<
        std::endl;
      return 0;
    }

    if (command == "delete-old-profiles")
    {
      uim::ClearExpiredRequest request;
      request.set_sync(opt_sync.enabled());
      request.set_cleanup_time(bytes_from_time_(
        opt_time.installed() ? Generics::Time(*opt_time, "%H-%M") :
          Generics::Time(-1)));
      request.set_portion(opt_portion.installed() ? *opt_portion : -1);
      AdServer::Grpc::sync_call<uim::ClearExpiredResponse>(
        [&](auto callback)
        {
          client->clear_expired(request, std::move(callback));
        });
      return 0;
    }

    if (!opt_user_id.installed())
    {
      throw std::runtime_error("user-id is required");
    }

    const AdServer::Commons::UserId user_id(opt_user_id->c_str());
    if (command == "print")
    {
      print_profile_(client, user_id, opt_plain.enabled(), opt_expand.enabled(),
        opt_align.enabled(), PRINT_ALL);
    }
    else if (command == "print-base")
    {
      print_profile_(client, user_id, opt_plain.enabled(), opt_expand.enabled(),
        opt_align.enabled(), PRINT_BASE);
    }
    else if (command == "print-add")
    {
      print_profile_(client, user_id, opt_plain.enabled(), opt_expand.enabled(),
        opt_align.enabled(), PRINT_ADD);
    }
    else if (command == "print-history")
    {
      print_profile_(client, user_id, opt_plain.enabled(), opt_expand.enabled(),
        opt_align.enabled(), PRINT_HISTORY);
    }
    else if (command == "print-freq-cap")
    {
      print_profile_(client, user_id, opt_plain.enabled(), false, false, 0);
    }
    else if (command == "match")
    {
      match_(client, user_id, *opt_time, *opt_page_channels,
        *opt_search_channels, *opt_url_channels, *opt_persistent_channels);
    }
    else if (command == "remove")
    {
      uim::RemoveUserProfileRequest request;
      request.set_user_id(bytes_from_user_id_(user_id));
      AdServer::Grpc::sync_call<uim::RemoveUserProfileResponse>(
        [&](auto callback)
        {
          client->remove_user_profile(request, std::move(callback));
        });
    }
    else
    {
      std::cerr << "Unknown command '" << command << "'. See help for more info." <<
        std::endl;
      return 1;
    }
  }
  catch (const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
