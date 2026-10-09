#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <ProfilingCommons/ProfileMap/RocksDBProfileMapProcessor.hpp>

#include "BindRequestChunk.hpp"

namespace AdServer::UserInfoSvcs
{
  class BindRequestContainer final:
    public BindRequestProcessor,
    public ReferenceCounting::AtomicImpl
  {
  public:
    using ChunkPathMap = std::map<unsigned long, std::string>;

    BindRequestContainer(
      std::shared_ptr<ProfilingCommons::RocksDBProfileMapProcessor> processor,
      unsigned long common_chunks_number,
      const ChunkPathMap& chunk_folders,
      const char* file_prefix,
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

    ProfilingCommons::RocksDBProfileMapProcessor::Stats rocksdb_stats() const noexcept;

  protected:
    ~BindRequestContainer() noexcept override;

  private:
    BindRequestChunk_var get_chunk_(const String::SubString& id) const;

    const unsigned long common_chunks_number_;
    std::shared_ptr<ProfilingCommons::RocksDBProfileMapProcessor> processor_;
    std::vector<BindRequestChunk_var> chunks_;
  };

  using BindRequestContainer_var = ReferenceCounting::SmartPtr<BindRequestContainer>;
}
