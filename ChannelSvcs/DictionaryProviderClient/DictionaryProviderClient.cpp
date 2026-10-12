#include "DictionaryProviderClient.hpp"
#include <grpcpp/grpcpp.h>

namespace AdServer::ChannelSvcs
{
  DictionaryProviderClient::DictionaryProviderClient(
    const std::vector<std::string>& endpoints, std::chrono::milliseconds timeout)
    : endpoints_(endpoints), timeout_(timeout)
  {
    if (timeout.count() <= 0)
    {
      throw Exception("DictionaryProvider timeout must be positive");
    }

    for (const auto& endpoint : endpoints)
    {
      if (endpoint.empty() || endpoint.find("corbaloc:") == 0 || endpoint.find("IOR:") == 0)
      {
        throw Exception("DictionaryProvider requires a gRPC endpoint");
      }

      grpc::ChannelArguments args;
      args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
      args.SetMaxReceiveMessageSize(-1);
      args.SetMaxSendMessageSize(-1);
      stubs_.push_back(adserver::channel_svcs::dictionary_provider::DictionaryProvider::NewStub(
        grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), args)));
    }
  }

  bool DictionaryProviderClient::empty() const noexcept
  {
    return stubs_.empty();
  }

  DictionaryProviderClient::Response
  DictionaryProviderClient::get_lexemes(const char* language, const std::vector<std::string>& words)
    const
  {
    if (empty())
    {
      throw Exception("No DictionaryProvider endpoints configured");
    }

    adserver::channel_svcs::dictionary_provider::GetLexemesRequest request;
    request.set_language(language);
    for (const auto& word : words)
    {
      request.add_words(word);
    }

    const auto deadline = std::chrono::system_clock::now() + timeout_;
    const auto start = next_.fetch_add(1, std::memory_order_relaxed) % stubs_.size();
    std::string error;
    for (std::size_t attempt = 0; attempt < stubs_.size(); ++attempt)
    {
      const auto index = (start + attempt) % stubs_.size();
      grpc::ClientContext context;
      context.set_deadline(deadline);
      Response response;
      const auto status = stubs_[index]->get_lexemes(&context, request, &response);
      if (status.ok())
      {
        if (static_cast<std::size_t>(response.lexemes_size()) != words.size())
        {
          throw Exception("Invalid DictionaryProvider response: lexeme count differs from words");
        }

        return response;
      }

      error = "DictionaryProvider " + endpoints_[index] + " gRPC " +
        std::to_string(status.error_code()) + ": " + status.error_message();
      if (std::chrono::system_clock::now() >= deadline)
      {
        break;
      }
    }

    throw Exception(error);
  }
}
