#pragma once

#include <string>
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

  protected:
    ~UrlInfoStorage() noexcept override;

  private:
    using ProfileMap = ProfilingCommons::RocksDBBatchingProfileMap<std::string>;
    ReferenceCounting::SmartPtr<ProfileMap> profiles_;
  };

  using UrlInfoStorage_var = ReferenceCounting::SmartPtr<UrlInfoStorage>;
}
