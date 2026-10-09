#pragma once

#include <memory>
#include <string>

#include <Generics/Time.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>

#include "BindRequestProcessor.hpp"

namespace AdServer::UserInfoSvcs
{
  class BindRequestChunk final:
    public BindRequestProcessor,
    public ReferenceCounting::AtomicImpl
  {
  public:
    BindRequestChunk(
      std::shared_ptr<ProfilingCommons::RocksDBProfileMapProcessor> processor,
      const String::SubString& path,
      const Generics::Time& expire_time);

    Commons::StartableAwaitable<void> co_add_bind_request(
      const String::SubString& id,
      const BindRequest& bind_request,
      const Generics::Time& now) override;

    void add_bind_request(
      const String::SubString& id,
      const BindRequest& bind_request,
      const Generics::Time& now) override;

    Commons::StartableAwaitable<BindRequest>
    co_get_bind_request(const String::SubString& id, const Generics::Time& now) override;

    BindRequest get_bind_request(const String::SubString& id, const Generics::Time& now) override;

  protected:
    ~BindRequestChunk() noexcept override;

  private:
    using RocksDBMap = ProfilingCommons::RocksDBBatchingProfileMap<std::string>;

    const Generics::Time expire_time_;
    std::unique_ptr<RocksDBMap> map_;
  };

  using BindRequestChunk_var = ReferenceCounting::SmartPtr<BindRequestChunk>;
}
