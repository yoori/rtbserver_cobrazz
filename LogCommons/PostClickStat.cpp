#include "PostClickStat.hpp"

#include <utility>
#include <Commons/ExternalMetrics.hpp>

#include "BufferWriter.hpp"

namespace AdServer::LogProcessing
{
  template <> const char* PostClickStatTraits::B::base_name_ = "PostClickStat";
  template <> const char* PostClickStatTraits::B::signature_ = "PostClickStat";
  template <> const char* PostClickStatTraits::B::current_version_ = "2.0";

  PostClickStatTraits::CollectorType
  PostClickStatTraits::convert_collector(const PostClickStatCollector_V_1_0& old_collector)
  {
    CollectorType collector;
    for (const auto& [key, old_data] : old_collector)
    {
      CollectorType::DataT data;
      for (const auto& [old_key, value] : old_data)
      {
        data.add(old_key.current_key(), value);
      }
      collector.add(key, data);
    }
    return collector;
  }

  PostClickStatTraits::CollectorType
  PostClickStatTraits::convert_collector(const PostClickStatCollector_V_1_1& old_collector)
  {
    CollectorType collector;
    for (const auto& [key, old_data] : old_collector)
    {
      CollectorType::DataT data;
      for (const auto& [old_key, value] : old_data)
      {
        data.add(old_key.current_key(), value);
      }
      collector.add(key, data);
    }
    return collector;
  }

  PostClickStatInnerKey::PostClickStatInnerKey() noexcept = default;

  PostClickStatInnerKey::PostClickStatInnerKey(
    CreativeStatInnerKey creative_key,
    std::uint64_t external_metrics_connection_type_id)
    : creative_key_(std::move(creative_key)),
      external_metrics_connection_type_id_(external_metrics_connection_type_id)
  {
    calc_hash_();
  }

  bool
  PostClickStatInnerKey::operator==(const PostClickStatInnerKey& rhs) const noexcept
  {
    return creative_key_ == rhs.creative_key_ &&
      external_metrics_connection_type_id_ == rhs.external_metrics_connection_type_id_;
  }

  const CreativeStatInnerKey&
  PostClickStatInnerKey::creative_key() const noexcept
  {
    return creative_key_;
  }

  std::uint64_t
  PostClickStatInnerKey::external_metrics_connection_type_id() const noexcept
  {
    return external_metrics_connection_type_id_;
  }

  std::size_t
  PostClickStatInnerKey::hash() const noexcept
  {
    return hash_;
  }

  void
  PostClickStatInnerKey::invariant() const
  {
    creative_key_.invariant();
    if (external_metrics_connection_type_id_ > Commons::MAX_METRICS_CONNECTION_TYPE)
    {
      throw ConstraintViolation("Invalid external metrics connection type");
    }
  }

  void
  PostClickStatInnerKey::calc_hash_() noexcept
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, creative_key_.hash());
    hash_add(hasher, external_metrics_connection_type_id_);
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerKey& key)
  {
    is >> key.creative_key_;
    is >> key.external_metrics_connection_type_id_;
    key.invariant();
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const PostClickStatInnerKey& key)
  {
    key.invariant();
    return out << key.creative_key_ << '\t' << key.external_metrics_connection_type_id_;
  }

  PostClickStatInnerKey PostClickStatInnerKey_V_1_1::current_key() const
  {
    return PostClickStatInnerKey(creative_key(), external_metrics_connection_type_id() ?
      Commons::YANDEX_METRIKA : Commons::INTERNAL_METRICS);
  }

  void PostClickStatInnerKey_V_1_1::invariant() const
  {
    creative_key().invariant();
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerKey_V_1_1& key)
  {
    CreativeStatInnerKey creative;
    std::uint64_t connection_id = 0;
    is >> creative >> connection_id;
    static_cast<PostClickStatInnerKey&>(key) = PostClickStatInnerKey(creative, connection_id);
    key.invariant();
    return is;
  }

  bool PostClickStatInnerKey_V_1_0::operator==(const PostClickStatInnerKey_V_1_0& rhs)
    const noexcept
  {
    return PostClickStatInnerKey::operator==(rhs) && source_date_ == rhs.source_date_;
  }

  std::size_t PostClickStatInnerKey_V_1_0::hash() const noexcept
  {
    return hash_;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerKey_V_1_0& key)
  {
    is >> static_cast<PostClickStatInnerKey_V_1_1&>(key);
    is >> key.source_date_;
    Generics::Murmur64Hash hasher(key.hash_);
    hash_add(hasher, key.PostClickStatInnerKey::hash());
    hash_add(hasher, key.source_date_.time().tv_sec);
    return is;
  }

  PostClickStatInnerData::PostClickStatInnerData() noexcept
    : visits_(0),
      visits_with_bounce_(0),
      visits_robot_(0),
      visits_robot_available_(0),
      session_time_sum_(0),
      page_views_(0),
      new_user_visits_(0),
      reporting_visits_(0)
  {}

  PostClickStatInnerData::PostClickStatInnerData(
    std::uint64_t visits,
    std::uint64_t visits_with_bounce,
    std::uint64_t visits_robot,
    std::uint64_t visits_robot_available,
    std::uint64_t session_time_sum,
    std::uint64_t page_views,
    std::uint64_t new_user_visits,
    std::uint64_t reporting_visits) noexcept
    : visits_(visits),
      visits_with_bounce_(visits_with_bounce),
      visits_robot_(visits_robot),
      visits_robot_available_(visits_robot_available),
      session_time_sum_(session_time_sum),
      page_views_(page_views),
      new_user_visits_(new_user_visits),
      reporting_visits_(reporting_visits)
  {}

  bool
  PostClickStatInnerData::operator==(const PostClickStatInnerData& rhs) const noexcept
  {
    return visits_ == rhs.visits_ &&
      visits_with_bounce_ == rhs.visits_with_bounce_ &&
      visits_robot_ == rhs.visits_robot_ &&
      visits_robot_available_ == rhs.visits_robot_available_ &&
      session_time_sum_ == rhs.session_time_sum_ &&
      page_views_ == rhs.page_views_ &&
      new_user_visits_ == rhs.new_user_visits_ &&
      reporting_visits_ == rhs.reporting_visits_;
  }

  PostClickStatInnerData&
  PostClickStatInnerData::operator+=(const PostClickStatInnerData& rhs) noexcept
  {
    visits_ += rhs.visits_;
    visits_with_bounce_ += rhs.visits_with_bounce_;
    visits_robot_ += rhs.visits_robot_;
    visits_robot_available_ += rhs.visits_robot_available_;
    session_time_sum_ += rhs.session_time_sum_;
    page_views_ += rhs.page_views_;
    new_user_visits_ += rhs.new_user_visits_;
    reporting_visits_ += rhs.reporting_visits_;
    return *this;
  }

  std::uint64_t PostClickStatInnerData::visits() const noexcept
  {
    return visits_;
  }

  std::uint64_t PostClickStatInnerData::visits_with_bounce() const noexcept
  {
    return visits_with_bounce_;
  }

  std::uint64_t PostClickStatInnerData::visits_robot() const noexcept
  {
    return visits_robot_;
  }

  std::uint64_t PostClickStatInnerData::visits_robot_available() const noexcept
  {
    return visits_robot_available_;
  }

  std::uint64_t PostClickStatInnerData::session_time_sum() const noexcept
  {
    return session_time_sum_;
  }

  std::uint64_t PostClickStatInnerData::page_views() const noexcept
  {
    return page_views_;
  }

  std::uint64_t PostClickStatInnerData::new_user_visits() const noexcept
  {
    return new_user_visits_;
  }

  std::uint64_t PostClickStatInnerData::reporting_visits() const noexcept
  {
    return reporting_visits_;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerData& data)
  {
    is >> data.visits_;
    is >> data.visits_with_bounce_;
    is >> data.visits_robot_;
    is >> data.visits_robot_available_;
    is >> data.session_time_sum_;
    is >> data.page_views_;
    is >> data.new_user_visits_;
    is >> data.reporting_visits_;
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const PostClickStatInnerData& data)
  {
    out << data.visits_ << '\t';
    out << data.visits_with_bounce_ << '\t';
    out << data.visits_robot_ << '\t';
    out << data.visits_robot_available_ << '\t';
    out << data.session_time_sum_ << '\t';
    out << data.page_views_ << '\t';
    out << data.new_user_visits_ << '\t';
    out << data.reporting_visits_;
    return out;
  }
}
