#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <ChannelSvcs/ChannelClient/ChannelCorbaClient.hpp>
#include <ChannelSvcs/ChannelClient/ChannelDistributedGrpcClient.hpp>

namespace
{
  using namespace AdServer::ChannelSvcs;
  namespace Proto = adserver::channel_svcs::channel_server;

  struct CallResult
  {
    grpc::Status status;
    Proto::MatchResponse match_response;
    Proto::GetCcgTraitsResponse traits_response;
    bool done = false;
  };

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

  void
  wait_for_call(CallResult& result, std::mutex& lock, std::condition_variable& cv)
  {
    std::unique_lock<std::mutex> guard(lock);
    cv.wait(guard, [&result] { return result.done; });
    if (!result.status.ok())
    {
      Stream::Error ostr;
      ostr << "grpc call failed: code=" <<
        static_cast<int>(result.status.error_code()) <<
        ", message=" << result.status.error_message();
      throw std::runtime_error(ostr.str().str());
    }
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
        "Usage: ChannelAdmin2 --endpoints controller-host:port[,controller-host:port] "
        "match|ccg_traits [--url URL] [--pwords WORDS] [--swords WORDS] "
        "[--uid UID] [--status S] [--ids 1,2]\n";
      return 1;
    }

    Logging::Logger_var logger =
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor =
      std::make_shared<AdServer::Grpc::GrpcExecutor>(1);
    grpc_executor->activate_object();

    ChannelDistributedGrpcClient_var client =
      new ChannelDistributedGrpcClient(
        split_endpoints(*endpoints),
        AdServer::Grpc::BatchingOptions(),
        grpc_executor);
    client->activate_object();

    std::mutex lock;
    std::condition_variable cv;
    CallResult result;

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
      client->match(
        request,
        [&](
          const grpc::Status& status,
          const Proto::MatchResponse& response)
        {
          std::lock_guard<std::mutex> guard(lock);
          result.status = status;
          result.match_response = response;
          result.done = true;
          cv.notify_one();
        });
      wait_for_call(result, lock, cv);
      ChannelServerBase::MatchResult_var corba_result =
        AdServer::ChannelSvcs::GrpcAlgs::make_match_result(result.match_response);
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
      client->get_ccg_traits(
        request,
        [&](
          const grpc::Status& status,
          const Proto::GetCcgTraitsResponse& response)
        {
          std::lock_guard<std::mutex> guard(lock);
          result.status = status;
          result.traits_response = response;
          result.done = true;
          cv.notify_one();
        });
      wait_for_call(result, lock, cv);
      ChannelServerBase::CCGKeywordSeq_var corba_result =
        AdServer::ChannelSvcs::GrpcAlgs::make_ccg_traits_result(
          result.traits_response);
      print_traits_result(*corba_result);
    }
    else
    {
      throw std::runtime_error("unsupported ChannelAdmin2 command");
    }

    client->deactivate_object();
    client->wait_object();
    grpc_executor->deactivate_object();
    grpc_executor->wait_object();
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
