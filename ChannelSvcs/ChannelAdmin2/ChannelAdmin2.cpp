#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <String/StringManip.hpp>

#include <Commons/CorbaAlgs.hpp>
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
      grpc::CreateChannel(reference, grpc::InsecureChannelCredentials()));

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
    ChannelServerGrpcAsyncClient_var client;
    Generics::ActiveObject* active_object = nullptr;
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

    if (references.size() > 1 || is_channel_controller(references.front()))
    {
      ChannelDistributedGrpcClient_var client =
        new ChannelDistributedGrpcClient(
          references,
          AdServer::Grpc::BatchingOptions(),
          result.grpc_executor);
      result.client = client;
      result.active_object = client;
    }
    else
    {
      ReferenceCounting::SmartPtr<ChannelServerGrpcAsyncBatchingClient> client =
        new ChannelServerGrpcAsyncBatchingClient(
          references.front(),
          result.grpc_executor,
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
    }
    catch (...)
    {}
  }

  void
  print_match_result(const ChannelServerBase::MatchResult& result)
  {
    std::cout << "page_channels=" <<
      result.matched_channels.page_channels.length() << '\n';
    std::cout << "search_channels=" <<
      result.matched_channels.search_channels.length() << '\n';
    std::cout << "url_channels=" <<
      result.matched_channels.url_channels.length() << '\n';
    std::cout << "url_keyword_channels=" <<
      result.matched_channels.url_keyword_channels.length() << '\n';
    std::cout << "uid_channels=" <<
      result.matched_channels.uid_channels.length() << '\n';
    std::cout << "content_channels=" <<
      result.content_channels.length() << '\n';
    std::cout << "no_adv=" << result.no_adv << '\n';
    std::cout << "no_track=" << result.no_track << '\n';
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
    Generics::AppUtils::StringOption pwords("");
    Generics::AppUtils::StringOption swords("");
    Generics::AppUtils::StringOption uid("");
    Generics::AppUtils::StringOption status("A");
    Generics::AppUtils::StringOption ids("");

    args.add(
      Generics::AppUtils::equal_name("endpoints") ||
      Generics::AppUtils::short_name("h"),
      endpoints);
    args.add(Generics::AppUtils::equal_name("url"), url);
    args.add(Generics::AppUtils::equal_name("pwords"), pwords);
    args.add(Generics::AppUtils::equal_name("swords"), swords);
    args.add(Generics::AppUtils::equal_name("uid"), uid);
    args.add(Generics::AppUtils::equal_name("status"), status);
    args.add(Generics::AppUtils::equal_name("ids"), ids);
    args.parse(argc - 1, argv + 1);

    const auto& commands = args.commands();
    if (commands.empty() || !endpoints.installed())
    {
      std::cerr <<
        "Usage: ChannelAdmin2 --endpoints controller-host:port|server-host:port "
        "match|ccg_traits [--url URL] [--pwords WORDS] [--swords WORDS] "
        "[--uid UID] [--status S] [--ids 1,2]\n";
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
    auto* client = holder.client.in();

    if (commands.front() == "match")
    {
      ChannelServerBase::MatchQuery query;
      query.request_id << String::SubString("ChannelAdmin2");
      query.first_url << *url;
      query.pwords << *pwords;
      query.swords << *swords;
      query.uid = uid->empty() ?
        CorbaAlgs::pack_user_id(Generics::Uuid()) :
        CorbaAlgs::pack_user_id(Generics::Uuid(*uid, true));
      query.statuses[0] = status->empty() ? '\0' : (*status)[0];
      query.statuses[1] = status->size() > 1 ? (*status)[1] : '\0';
      query.non_strict_word_match = false;
      query.non_strict_url_match = false;
      query.return_negative = false;
      query.simplify_page = true;
      query.fill_content = true;

      Proto::MatchRequest request;
      AdServer::ChannelSvcs::GrpcAlgs::make_match_request(query, request);
      const auto response =
        AdServer::Grpc::sync_call<Proto::MatchResponse>(
          [&](auto callback)
          {
            client->match(request, std::move(callback));
          });
      ChannelServerBase::MatchResult_var corba_result =
        AdServer::ChannelSvcs::GrpcAlgs::make_match_result(response);
      print_match_result(*corba_result);
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
