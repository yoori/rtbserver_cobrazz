#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <Generics/HashTableAdapters.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>

namespace AdServer::ProfilingCommons
{
  class RocksDBProfileMapCache final
  {
  public:
    struct OperationState final
    {
      // Assigned when a write becomes visible in the cache, before queue admission.
      std::uint64_t write_revision = 0;
      // Identifies the cached profile for which a TTL touch was scheduled.
      std::uint64_t touch_revision = 0;
      // Prevents a RocksDB read result from replacing a concurrent cached write.
      bool read_miss = false;
    };

    enum class ResultType
    {
      DISABLED,
      MISS,
      PROFILE,
      NOT_FOUND
    };

    struct LookupResult final
    {
      ResultType type = ResultType::DISABLED;
      Generics::ConstSmartMemBuf_var profile;
      std::uint64_t touch_revision = 0;
      bool touch = false;
    };

    struct Stats final
    {
      std::uint64_t size = 0;
      std::uint64_t entries = 0;
      std::uint64_t hits = 0;
      std::uint64_t misses = 0;
      std::uint64_t evictions = 0;
    };

    RocksDBProfileMapCache(std::size_t limit, unsigned long portions_count);

    ~RocksDBProfileMapCache() noexcept;

    LookupResult lookup(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const Generics::Time& expire_time,
      OperationState& state,
      bool allow_touch);

    void publish(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const Generics::ConstSmartMemBuf_var& profile,
      bool remove,
      OperationState& state);

    void cancel(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state) noexcept;

    bool requires_write(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state,
      bool touch) noexcept;

    void complete(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state,
      bool touch) noexcept;

    void fail(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state,
      bool touch) noexcept;

    std::uint64_t fill(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state,
      Generics::ConstSmartMemBuf_var profile,
      const Generics::Time& write_time,
      bool schedule_touch) noexcept;

    void cancel_touch(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      std::uint64_t touch_revision) noexcept;

    void remove_map(std::uint64_t map_id) noexcept;

    Stats stats() const noexcept;

  private:
    class Portion;

    Portion& portion_(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key) noexcept;

  private:
    const std::size_t limit_;
    std::vector<std::unique_ptr<Portion>> portions_;
    std::atomic<std::uint64_t> size_{0};
    std::atomic<std::uint64_t> entries_{0};
    std::atomic<std::uint64_t> hits_{0};
    std::atomic<std::uint64_t> misses_{0};
    std::atomic<std::uint64_t> evictions_{0};
  };
}
