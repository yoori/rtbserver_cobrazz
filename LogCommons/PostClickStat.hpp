#pragma once

#include <cstdint>

#include "CreativeStat.hpp"
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer::LogProcessing
{
  class PostClickStatInnerKey
  {
  public:
    PostClickStatInnerKey() noexcept;

    PostClickStatInnerKey(
      CreativeStatInnerKey creative_key,
      std::uint64_t yandex_ref_id,
      const DayTimestamp& yandex_event_date);

    bool
    operator==(const PostClickStatInnerKey& rhs) const noexcept;

    const CreativeStatInnerKey& creative_key() const noexcept;
    std::uint64_t yandex_ref_id() const noexcept;
    const DayTimestamp& yandex_event_date() const noexcept;
    std::size_t hash() const noexcept;
    void invariant() const;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerKey& key);

    friend BufferWriter&
    operator<<(BufferWriter& out, const PostClickStatInnerKey& key);

  private:
    void calc_hash_() noexcept;

    CreativeStatInnerKey creative_key_;
    std::uint64_t yandex_ref_id_ = 0;
    DayTimestamp yandex_event_date_;
    std::size_t hash_ = 0;
  };

  class PostClickStatInnerData
  {
  public:
    PostClickStatInnerData() noexcept;

    PostClickStatInnerData(
      std::uint64_t visits,
      std::uint64_t visits_with_bounce,
      std::uint64_t session_time_sum,
      std::uint64_t page_views,
      std::uint64_t new_user_visits,
      std::uint64_t reporting_visits) noexcept;

    bool
    operator==(const PostClickStatInnerData& rhs) const noexcept;

    PostClickStatInnerData&
    operator+=(const PostClickStatInnerData& rhs) noexcept;

    std::uint64_t visits() const noexcept;
    std::uint64_t visits_with_bounce() const noexcept;
    std::uint64_t session_time_sum() const noexcept;
    std::uint64_t page_views() const noexcept;
    std::uint64_t new_user_visits() const noexcept;
    std::uint64_t reporting_visits() const noexcept;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerData& data);

    friend BufferWriter&
    operator<<(BufferWriter& out, const PostClickStatInnerData& data);

  private:
    std::uint64_t visits_;
    std::uint64_t visits_with_bounce_;
    std::uint64_t session_time_sum_;
    std::uint64_t page_views_;
    std::uint64_t new_user_visits_;
    std::uint64_t reporting_visits_;
  };

  using PostClickStatInnerCollector = StatCollector<
    PostClickStatInnerKey,
    PostClickStatInnerData,
    false,
    true>;
  using PostClickStatCollector = StatCollector<CreativeStatKey, PostClickStatInnerCollector>;

  struct PostClickStatTraits: LogDefaultTraits<PostClickStatCollector>
  {};
}
