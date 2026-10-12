#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <eh/Exception.hpp>
#include <ChannelSvcs/DictionaryProvider/DictionaryProvider.grpc.pb.h>

namespace AdServer::ChannelSvcs
{
  class DictionaryProviderClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    using Response = adserver::channel_svcs::dictionary_provider::GetLexemesResponse;

    explicit DictionaryProviderClient(
      const std::vector<std::string>& endpoints,
      std::chrono::milliseconds timeout = std::chrono::seconds(10));

    Response get_lexemes(const char* language, const std::vector<std::string>& words) const;
    bool empty() const noexcept;

  private:
    using Stub = adserver::channel_svcs::dictionary_provider::DictionaryProvider::Stub;
    std::vector<std::unique_ptr<Stub>> stubs_;
    std::vector<std::string> endpoints_;
    std::chrono::milliseconds timeout_;
    mutable std::atomic<std::size_t> next_{0};
  };
}
