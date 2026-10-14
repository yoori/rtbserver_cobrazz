#pragma once

#include <string>
#include <deque>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <Generics/CompositeActiveObject.hpp>
#include <Commons/Coro/StartableAwaitable.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>

namespace AdServer::RequestInfoSvcs
{
  class UrlInfoStorage final: public Generics::RefCountableCompositeActiveObject
  {
  public:
    UrlInfoStorage(const std::string& path, unsigned long workers, unsigned long batch_size);

    Commons::StartableAwaitable<std::string> get_url_profile(std::string url) const;
    Commons::StartableAwaitable<void> set_url_profile(std::string url, std::string text) const;
    Commons::StartableAwaitable<bool> compare_exchange_url_profile(
      std::string url, std::string expected, std::string text) const;
    Commons::StartableAwaitable<std::string> get_url_description(std::string url) const;
    Commons::StartableAwaitable<void> set_url_description(std::string url, std::string text) const;
    Commons::StartableAwaitable<bool> compare_exchange_url_description(
      std::string url, std::string expected, std::string text) const;

  protected:
    ~UrlInfoStorage() noexcept override;

  private:
    struct WriteGuard
    {
      const UrlInfoStorage& owner;
      const std::string& url;
      ~WriteGuard();
    };

    void acquire_write_(const std::string& url, std::function<void()> ready) const;
    void release_write_(const std::string& url) const;

    mutable std::mutex writes_mutex_;
    mutable std::unordered_map<std::string, std::deque<std::function<void()>>> writes_;
    using ProfileMap = ProfilingCommons::RocksDBBatchingProfileMap<std::string>;
    Commons::StartableAwaitable<std::string> get_(ProfileMap& map, std::string url) const;
    Commons::StartableAwaitable<void> set_(
      ProfileMap& map, std::string url, std::string text) const;
    Commons::StartableAwaitable<bool> compare_exchange_(
      ProfileMap& map, std::string url, std::string expected, std::string text) const;
    ReferenceCounting::SmartPtr<ProfileMap> profiles_;
    ReferenceCounting::SmartPtr<ProfileMap> descriptions_;
  };

  using UrlInfoStorage_var = ReferenceCounting::SmartPtr<UrlInfoStorage>;
}
