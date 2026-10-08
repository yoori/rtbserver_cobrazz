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
      std::uint64_t external_metrics_connection_type_id);

    bool
    operator==(const PostClickStatInnerKey& rhs) const noexcept;

    const CreativeStatInnerKey& creative_key() const noexcept;
    std::uint64_t external_metrics_connection_type_id() const noexcept;
    std::size_t hash() const noexcept;
    void invariant() const;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerKey& key);

    friend BufferWriter&
    operator<<(BufferWriter& out, const PostClickStatInnerKey& key);

  private:
    void calc_hash_() noexcept;

    CreativeStatInnerKey creative_key_;
    std::uint64_t external_metrics_connection_type_id_ = 0;
    std::size_t hash_ = 0;
  };

  // Keep old connection IDs distinct until conversion sums their type-level keys.
  class PostClickStatInnerKey_V_1_1: public PostClickStatInnerKey
  {
  public:
    PostClickStatInnerKey current_key() const;
    void invariant() const;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerKey_V_1_1& key);
  };

  class PostClickStatInnerKey_V_1_0: public PostClickStatInnerKey_V_1_1
  {
  public:
    bool operator==(const PostClickStatInnerKey_V_1_0& rhs) const noexcept;
    std::size_t hash() const noexcept;

    friend FixedBufStream<TabCategory>&
    operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerKey_V_1_0& key);

  private:
    DayTimestamp source_date_;
    std::size_t hash_ = 0;
  };

  class PostClickStatInnerData
  {
  public:
    PostClickStatInnerData() noexcept;

    PostClickStatInnerData(
      std::uint64_t visits,
      std::uint64_t visits_with_bounce,
      std::uint64_t visits_robot,
      std::uint64_t visits_robot_available,
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
    std::uint64_t visits_robot() const noexcept;
    std::uint64_t visits_robot_available() const noexcept;
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
    std::uint64_t visits_robot_;
    std::uint64_t visits_robot_available_;
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

  using PostClickStatInnerCollector_V_1_1 =
    StatCollector<PostClickStatInnerKey_V_1_1, PostClickStatInnerData, false, true>;
  using PostClickStatCollector_V_1_1 =
    StatCollector<CreativeStatKey, PostClickStatInnerCollector_V_1_1>;

  using PostClickStatInnerCollector_V_1_0 =
    StatCollector<PostClickStatInnerKey_V_1_0, PostClickStatInnerData, false, true>;
  using PostClickStatCollector_V_1_0 =
    StatCollector<CreativeStatKey, PostClickStatInnerCollector_V_1_0>;

  struct PostClickStatTraits: LogDefaultTraits<PostClickStatCollector>
  {
    static CollectorType convert_collector(const PostClickStatCollector_V_1_0& old_collector);
    static CollectorType convert_collector(const PostClickStatCollector_V_1_1& old_collector);

    template <class Functor>
    static void for_each_old(Functor& functor);
  };

  template <class Functor>
  void PostClickStatTraits::for_each_old(Functor& functor)
  {
    functor.template support<PostClickStatCollector_V_1_0, true>("1.0");
    functor.template support<PostClickStatCollector_V_1_1, true>("1.1");
  }
}
