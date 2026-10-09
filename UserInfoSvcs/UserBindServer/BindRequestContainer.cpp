#include "BindRequestContainer.hpp"

#include <Commons/UserInfoManip.hpp>

namespace AdServer::UserInfoSvcs
{
  BindRequestContainer::BindRequestContainer(
    std::shared_ptr<ProfilingCommons::RocksDBProfileMapProcessor> processor,
    unsigned long common_chunks_number,
    const ChunkPathMap& chunk_folders,
    const char* file_prefix,
    const Generics::Time& expire_time)
    : common_chunks_number_(common_chunks_number),
      processor_(std::move(processor))
  {
    if (!processor_ || common_chunks_number_ == 0 || expire_time <= Generics::Time::ZERO)
    {
      throw Exception("BindRequestContainer: invalid chunk count or expiration time");
    }

    chunks_.resize(common_chunks_number_);
    for (const auto& [id, folder] : chunk_folders)
    {
      if (id >= common_chunks_number_)
      {
        throw Exception("BindRequestContainer: chunk index is out of range");
      }

      const std::string path = folder + "/" + file_prefix + ".rocksdb";
      chunks_[id] = new BindRequestChunk(processor_, path, expire_time);
    }
  }

  BindRequestContainer::~BindRequestContainer() noexcept = default;

  Commons::StartableAwaitable<BindRequestProcessor::BindRequest>
  BindRequestContainer::co_get_bind_request(const String::SubString& id, const Generics::Time& now)
  {
    const auto chunk = get_chunk_(id);
    co_return co_await chunk->co_get_bind_request(id, now);
  }

  BindRequestProcessor::BindRequest
  BindRequestContainer::get_bind_request(const String::SubString& id, const Generics::Time& now)
  {
    return Commons::sync_wait(co_get_bind_request(id, now));
  }

  Commons::StartableAwaitable<void> BindRequestContainer::co_add_bind_request(
    const String::SubString& id,
    const BindRequest& bind_request,
    const Generics::Time& now)
  {
    const auto chunk = get_chunk_(id);
    co_await chunk->co_add_bind_request(id, bind_request, now);
  }

  void BindRequestContainer::add_bind_request(
    const String::SubString& id,
    const BindRequest& bind_request,
    const Generics::Time& now)
  {
    Commons::sync_wait(co_add_bind_request(id, bind_request, now));
  }

  ProfilingCommons::RocksDBProfileMapProcessor::Stats
  BindRequestContainer::rocksdb_stats() const noexcept
  {
    return processor_->stats();
  }

  BindRequestChunk_var BindRequestContainer::get_chunk_(const String::SubString& id) const
  {
    auto chunk = chunks_[Commons::external_id_distribution_hash(id) % common_chunks_number_];
    if (!chunk)
    {
      throw ChunkNotFound("BindRequestContainer: chunk is not hosted here");
    }

    return chunk;
  }
}
