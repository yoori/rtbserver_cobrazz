#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <Generics/ActiveObject.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Language/BLogic/NormalizeTrigger.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>

#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <ChannelSvcs/ChannelClient/ChannelCorbaClient.hpp>
#include <ChannelSvcs/ChannelClient/ChannelDistributedGrpcClient.hpp>
#include <ChannelControllerGrpc.grpc.pb.h>
#include <ChannelServerGrpc.grpc-client.hpp>

namespace
{
  using namespace AdServer::ChannelSvcs;
  namespace Proto = adserver::channel_svcs::channel_server;
  namespace Controller = adserver::channel_svcs::channel_controller;

  const std::chrono::seconds RPC_TIMEOUT(10);

  bool
  is_channel_controller(const std::string& reference)
  {
    auto stub = Controller::ChannelControllerGrpc::NewStub(
      AdServer::Grpc::create_channel(
        reference,
        grpc::InsecureChannelCredentials()));

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + RPC_TIMEOUT);
    Controller::GetSessionDescriptionRequest request;
    Controller::GetSessionDescriptionResponse response;
    const auto status = stub->get_session_description(
      &context,
      request,
      &response);

    return status.ok() && !response.channel_server_groups().empty();
  }

  std::vector<std::string>
  split_endpoints(const std::string& endpoints)
  {
    std::vector<std::string> result;
    String::StringManip::SplitComma tokenizer(endpoints);
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      result.emplace_back(token.str());
    }
    return result;
  }

  struct ClientHolder
  {
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner;
    std::shared_ptr<ChannelServerGrpcAsyncClient> client;
    std::shared_ptr<Generics::ActiveObject> active_object;
  };

  ClientHolder
  create_client(const std::vector<std::string>& references)
  {
    if (references.empty())
    {
      throw std::runtime_error("empty ChannelAdmin2 endpoint list");
    }

    ClientHolder result;
    result.grpc_executor = std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    result.grpc_executor->activate_object();
    Logging::Logger_var logger =
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
    result.coalesce_runner =
      std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
        new Logging::ActiveObjectCallbackImpl(logger, "ChannelAdmin2", "gRPC"),
        std::make_shared<boost::asio::io_service>(),
        1);
    result.coalesce_runner->activate_object();

    if (references.size() > 1 || is_channel_controller(references.front()))
    {
      auto client = std::make_shared<ChannelDistributedGrpcClient>(
        ChannelDistributedGrpcClient::ChannelControllerRefs{references},
        AdServer::Grpc::BatchingOptions(),
        result.grpc_executor,
        result.coalesce_runner);
      result.client = client;
      result.active_object = client;
    }
    else
    {
      auto client = std::make_shared<ChannelServerGrpcAsyncBatchingClient>(
        references.front(),
        result.grpc_executor,
        result.coalesce_runner,
        AdServer::Grpc::BatchingOptions());
      result.client = client;
      result.active_object = client;
    }

    result.active_object->activate_object();
    return result;
  }

  void
  shutdown_client(ClientHolder& holder) noexcept
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

  void
  print_channel_atoms(
    const char* name,
    const google::protobuf::RepeatedPtrField<Proto::ChannelAtom>& channels)
  {
    std::cout << name << '=';
    for (const auto& channel : channels)
    {
      std::cout << ' ' << channel.id();
      if (channel.trigger_channel_id())
      {
        std::cout << "::" << channel.trigger_channel_id();
      }
    }
    std::cout << '\n';
  }

  void
  print_content_channels(
    const google::protobuf::RepeatedPtrField<Proto::ContentChannelAtom>& channels)
  {
    std::cout << "content_channel_ids=";
    for (const auto& channel : channels)
    {
      std::cout << ' ' << channel.id() << ':' << channel.weight();
    }
    std::cout << '\n';
  }

  void
  print_match_result(const Proto::MatchResponse& result)
  {
    const auto& matched_channels = result.matched_channels();
    std::cout << "hostname=" << result.hostname() << '\n';
    std::cout << "page_channels=" <<
      matched_channels.page_channels_size() << '\n';
    std::cout << "search_channels=" <<
      matched_channels.search_channels_size() << '\n';
    std::cout << "url_channels=" <<
      matched_channels.url_channels_size() << '\n';
    std::cout << "url_keyword_channels=" <<
      matched_channels.url_keyword_channels_size() << '\n';
    std::cout << "uid_channels=" <<
      matched_channels.uid_channels_size() << '\n';
    std::cout << "content_channels=" <<
      result.content_channels_size() << '\n';
    print_channel_atoms("page_channel_ids", matched_channels.page_channels());
    print_channel_atoms("search_channel_ids", matched_channels.search_channels());
    print_channel_atoms("url_channel_ids", matched_channels.url_channels());
    print_channel_atoms(
      "url_keyword_channel_ids",
      matched_channels.url_keyword_channels());
    print_content_channels(result.content_channels());
    std::cout << "no_adv=" << result.no_adv() << '\n';
    std::cout << "no_track=" << result.no_track() << '\n';
  }

  void
  print_match_request(const Proto::MatchRequest& request)
  {
    std::cout << "request.first_url=" << request.first_url() << '\n';
    std::cout << "request.first_url_words=" << request.first_url_words() << '\n';
    std::cout << "request.pwords=" << request.pwords() << '\n';
    std::cout << "request.swords=" << request.swords() << '\n';
    std::cout << "request.statuses=" << request.statuses() << '\n';
  }

  void
  print_traits_result(const ChannelServerBase::CCGKeywordSeq& result)
  {
    for (CORBA::ULong i = 0; i < result.length(); ++i)
    {
      std::cout <<
        result[i].ccg_keyword_id << '\t' <<
        result[i].ccg_id << '\t' <<
        result[i].channel_id << '\t' <<
        result[i].click_url.in() << '\t' <<
        result[i].original_keyword.in() << '\n';
    }
  }
}

int
main(int argc, char** argv)
{
  try
  {
    Generics::AppUtils::Args args(-1);
    Generics::AppUtils::StringOption endpoints;
    Generics::AppUtils::StringOption url("");
    Generics::AppUtils::StringOption url_words("");
    Generics::AppUtils::StringOption pwords("");
    Generics::AppUtils::StringOption swords("");
    Generics::AppUtils::StringOption uid("");
    Generics::AppUtils::StringOption status("A");
    Generics::AppUtils::StringOption ids("");
    Generics::AppUtils::CheckOption normalize_url;
    Generics::AppUtils::CheckOption normalize_url_words;
    Generics::AppUtils::CheckOption show_request;

    args.add(
      Generics::AppUtils::equal_name("endpoints") ||
      Generics::AppUtils::short_name("h"),
      endpoints);
    args.add(Generics::AppUtils::equal_name("url"), url);
    args.add(Generics::AppUtils::equal_name("url-words"), url_words);
    args.add(Generics::AppUtils::equal_name("pwords"), pwords);
    args.add(Generics::AppUtils::equal_name("swords"), swords);
    args.add(Generics::AppUtils::equal_name("uid"), uid);
    args.add(Generics::AppUtils::equal_name("status"), status);
    args.add(Generics::AppUtils::equal_name("ids"), ids);
    args.add(Generics::AppUtils::equal_name("normalize-url"), normalize_url);
    args.add(
      Generics::AppUtils::equal_name("normalize-url-words"),
      normalize_url_words);
    args.add(Generics::AppUtils::equal_name("show-request"), show_request);
    args.parse(argc - 1, argv + 1);

    const auto& commands = args.commands();
    if (commands.empty() || !endpoints.installed())
    {
      std::cerr <<
        "Usage: ChannelAdmin2 --endpoints controller-host:port|server-host:port "
        "match|ccg_traits [--url URL] [--url-words WORDS] [--pwords WORDS] "
        "[--swords WORDS] [--uid UID] [--status S] [--ids 1,2] "
        "[--normalize-url] [--normalize-url-words] [--show-request]\n";
      return 1;
    }

    const auto endpoint_list = split_endpoints(*endpoints);
    ClientHolder holder = create_client(endpoint_list);
    auto holder_guard = std::unique_ptr<ClientHolder, void(*)(ClientHolder*)>(
      &holder,
      [](ClientHolder* value)
      {
        shutdown_client(*value);
      });
    auto* client = holder.client.get();

    if (commands.front() == "match")
    {
      Proto::MatchRequest request;
      std::string first_url = *url;
      if (normalize_url.enabled() && !first_url.empty())
      {
        HTTP::BrowserAddress addr(first_url);
        addr.get_view(HTTP::HTTPAddress::VW_FULL, first_url);
      }

      std::string first_url_words = *url_words;
      if (normalize_url_words.enabled() && first_url_words.empty())
      {
        std::string kw_from_http = HTTP::keywords_from_http_address(first_url);
        Language::Trigger::normalize_phrase(kw_from_http, first_url_words, 0);
      }

      request.set_request_id("ChannelAdmin2");
      request.set_first_url(first_url);
      request.set_first_url_words(first_url_words);
      request.set_pwords(*pwords);
      request.set_swords(*swords);
      request.set_uid(::GrpcAlgs::pack_user_id(
        uid->empty() ? Generics::Uuid() : Generics::Uuid(*uid, true)));
      request.set_statuses(status->data(), status->size());
      request.set_non_strict_word_match(false);
      request.set_non_strict_url_match(false);
      request.set_return_negative(false);
      request.set_simplify_page(true);
      request.set_fill_content(true);
      if (show_request.enabled())
      {
        print_match_request(request);
      }
      const auto result =
        AdServer::ChannelSvcs::GrpcAlgs::channel_match(*client, request);
      print_match_result(result);
    }
    else if (commands.front() == "ccg_traits")
    {
      ChannelIdSeq query;
      std::vector<unsigned long> values;
      String::StringManip::SplitComma tokenizer(*ids);
      String::SubString token;
      while (tokenizer.get_token(token))
      {
        unsigned long id = 0;
        String::StringManip::str_to_int(token, id);
        values.push_back(id);
      }
      query.length(values.size());
      std::copy(values.begin(), values.end(), query.get_buffer());

      Proto::GetCcgTraitsRequest request;
      AdServer::ChannelSvcs::GrpcAlgs::make_get_ccg_traits_request(query, request);
      const auto response =
        AdServer::Grpc::sync_call<Proto::GetCcgTraitsResponse>(
          [&](auto callback)
          {
            client->get_ccg_traits(request, std::move(callback));
          });
      ChannelServerBase::CCGKeywordSeq_var corba_result =
        AdServer::ChannelSvcs::GrpcAlgs::make_ccg_traits_result(response);
      print_traits_result(*corba_result);
    }
    else
    {
      throw std::runtime_error("unsupported ChannelAdmin2 command");
    }

    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
