#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <grpcpp/grpcpp.h>
#include <Logger/StreamLogger.hpp>
#include <Commons/ScopeGuard.hpp>
#include <ChannelSvcs/DictionaryProviderClient/DictionaryProviderClient.hpp>
#include <ChannelSvcs/DictionaryProvider/DictionaryProviderImpl.hpp>
#include <ChannelSvcs/ChannelServer/DictionaryMatcher.hpp>

namespace
{
  namespace pb = adserver::channel_svcs::dictionary_provider;
  using namespace AdServer::ChannelSvcs;
  using namespace std::chrono_literals;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  template<class Function>
  void require_error(Function function, const char* fragment)
  {
    try
    {
      function();
    }
    catch (const DictionaryProviderClient::Exception& error)
    {
      require(std::string(error.what()).find(fragment) != std::string::npos, error.what());
      return;
    }

    throw std::runtime_error("Expected DictionaryProvider error");
  }

  struct Server
  {
    explicit Server(grpc::Service* service);
    ~Server();
    std::unique_ptr<grpc::Server> server;
    std::string endpoint;
  };

  Server::Server(grpc::Service* service)
  {
    grpc::ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(-1);
    builder.SetMaxSendMessageSize(-1);
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(service);
    server = builder.BuildAndStart();
    require(server && port, "Failed to start test gRPC server");
    endpoint = "127.0.0.1:" + std::to_string(port);
  }

  Server::~Server()
  {
    server->Shutdown();
    server->Wait();
  }

  struct BadService : pb::DictionaryProvider::Service
  {
    grpc::Status get_lexemes(
      grpc::ServerContext*, const pb::GetLexemesRequest* request,
      pb::GetLexemesResponse*) override;
  };

  grpc::Status BadService::get_lexemes(
    grpc::ServerContext*, const pb::GetLexemesRequest* request, pb::GetLexemesResponse*)
  {
    if (request->language() == "slow")
    {
      std::this_thread::sleep_for(200ms);
    }

    if (request->language() == "failure")
    {
      return {grpc::StatusCode::INTERNAL, "test failure"};
    }

    return grpc::Status::OK;
  }

  void check_words(DictionaryProviderClient& client)
  {
    const auto response = client.get_lexemes("en", {"cats", "absent", "cat", "cats"});
    require(response.lexemes_size() == 4, "Result size/order lost");
    require(response.lexemes(0).forms_size() == 2, "Missing English forms");
    require(response.lexemes(0).forms(0) == "cat", "Unexpected first form");
    require(response.lexemes(0).forms(1) == "cats", "Unexpected second form");
    require(response.lexemes(1).forms_size() == 0, "Unknown word must be empty");
    require(response.lexemes(2).forms(0) == "cat", "Request order lost");
    require(response.lexemes(3).forms(1) == "cats", "Duplicate word lost");
    require(client.get_lexemes("unknown", {"cat"}).lexemes(0).forms_size() == 0,
      "Unknown language must be empty");
    require(client.get_lexemes("ru", {"кот"}).lexemes(0).forms_size() == 2,
      "Unicode forms lost");
    require(client.get_lexemes("", {"кот", "cat"}).lexemes(1).forms_size() == 2,
      "Empty language must search all dictionaries");
    require(client.get_lexemes("", {}).lexemes_size() == 0, "Empty query failed");
  }

  void test(const char* filename)
  {
    auto config = xsd::AdServer::Configuration::AdConfiguration(filename);
    Logging::Logger_var logger = new Logging::OStream::Logger(Logging::OStream::Config(std::cerr));
    DictionaryProviderImpl_var service =
      new DictionaryProviderImpl(logger, &config->DictionaryProviderConfig());
    auto stop_service = AdServer::Commons::make_scope_guard([&service]()
    {
      service->deactivate_object();
      service->wait_object();
    });
    Server server(service.in());
    DictionaryProviderClient client({server.endpoint}, 10s);
    require_error([&]() { client.get_lexemes("", {}); }, "not ready");
    DictionaryMatcher matcher({server.endpoint}, logger);
    require(!matcher.ready(), "Matcher must wait for loading");
    service->activate_object();
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!matcher.ready())
    {
      require(std::chrono::steady_clock::now() < deadline, "Dictionary load timeout");
      std::this_thread::sleep_for(10ms);
    }

    check_words(client);
    std::cout << "PASS: dictionary readiness and word lookup" << std::endl;
    DictionaryMatcher::LexemeCache cache;
    cache.emplace(Generics::StringHashAdapter("cats"), Lexeme_var());
    cache.emplace(Generics::StringHashAdapter("absent"), Lexeme_var());
    matcher.get_lexemes("en", cache);
    const auto& lexeme = cache.at(Generics::StringHashAdapter("cats"));
    require(lexeme && lexeme->forms.size() == 2, "Matcher failed to cache forms");
    require(lexeme->forms[1].text() == "cats", "Matcher form storage is invalid");
    require(!cache.at(Generics::StringHashAdapter("absent")), "Unknown word must not be cached");
    DictionaryMatcher disabled({}, logger);
    require(disabled.ready(), "Unconfigured dictionary must be optional");

    std::vector<std::future<void>> requests;
    for (int i = 0; i < 8; ++i)
    {
      requests.emplace_back(std::async(std::launch::async, [&]() { check_words(client); }));
    }

    for (auto& request : requests)
    {
      request.get();
    }

    std::cout << "PASS: matcher cache and concurrent requests" << std::endl;
    const std::vector<std::string> words(350000, "cats");
    const auto large = client.get_lexemes("en", words);
    require(large.ByteSizeLong() > 4 * 1024 * 1024, "Large response test is too small");
    require(large.lexemes_size() == static_cast<int>(words.size()), "Large response truncated");
    std::cout << "PASS: response larger than 4 MiB" << std::endl;
    DictionaryProviderClient fallback({"127.0.0.1:1", server.endpoint}, 2s);
    check_words(fallback);
    require_error([]() { DictionaryProviderClient invalid({"corbaloc:iiop:host:1/Dictionary"}); },
      "gRPC endpoint");
    DictionaryProviderClient unavailable({"127.0.0.1:1"}, 100ms);
    require_error([&]() { unavailable.get_lexemes("", {}); }, "gRPC 14");

    BadService bad_service;
    Server bad_server(&bad_service);
    DictionaryProviderClient bad_client({bad_server.endpoint}, 50ms);
    require_error([&]() { bad_client.get_lexemes("en", {"cat"}); }, "lexeme count");
    require_error([&]() { bad_client.get_lexemes("failure", {}); }, "test failure");
    require_error([&]() { bad_client.get_lexemes("slow", {}); }, "gRPC 4");
    std::cout << "PASS: readiness, forms, matcher, concurrency, large reply, failover, errors\n";
  }
}

int main(int argc, char** argv)
{
  try
  {
    require(argc == 2 || argc == 3, "Usage: DictionaryProviderGrpcTest config | --remote endpoint");
    if (argc == 3)
    {
      DictionaryProviderClient client({argv[2]}, std::chrono::seconds(2));
      check_words(client);
    }
    else
    {
      test(argv[1]);
    }
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }

  return 0;
}
