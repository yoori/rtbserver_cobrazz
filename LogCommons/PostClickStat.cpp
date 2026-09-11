#include "PostClickStat.hpp"

#include "BufferWriter.hpp"

namespace AdServer::LogProcessing
{
  template <> const char* PostClickStatTraits::B::base_name_ = "PostClickStat";
  template <> const char* PostClickStatTraits::B::signature_ = "PostClickStat";
  template <> const char* PostClickStatTraits::B::current_version_ = "1.0";

  PostClickStatInnerKey::PostClickStatInnerKey() noexcept = default;

  PostClickStatInnerKey::PostClickStatInnerKey(
    CreativeStatInnerKey creative_key,
    std::uint64_t yandex_ref_id,
    const DayTimestamp& yandex_event_date)
    : creative_key_(std::move(creative_key)),
      yandex_ref_id_(yandex_ref_id),
      yandex_event_date_(yandex_event_date)
  {
    calc_hash_();
  }

  bool
  PostClickStatInnerKey::operator==(const PostClickStatInnerKey& rhs) const noexcept
  {
    return creative_key_ == rhs.creative_key_ &&
      yandex_ref_id_ == rhs.yandex_ref_id_ &&
      yandex_event_date_ == rhs.yandex_event_date_;
  }

  const CreativeStatInnerKey&
  PostClickStatInnerKey::creative_key() const noexcept
  {
    return creative_key_;
  }

  std::uint64_t
  PostClickStatInnerKey::yandex_ref_id() const noexcept
  {
    return yandex_ref_id_;
  }

  const DayTimestamp&
  PostClickStatInnerKey::yandex_event_date() const noexcept
  {
    return yandex_event_date_;
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
  }

  void
  PostClickStatInnerKey::calc_hash_() noexcept
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, creative_key_.hash());
    hash_add(hasher, yandex_ref_id_);
    hash_add(hasher, yandex_event_date_.time().tv_sec);
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickStatInnerKey& key)
  {
    is >> key.creative_key_;
    is >> key.yandex_ref_id_;
    is >> key.yandex_event_date_;
    key.invariant();
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const PostClickStatInnerKey& key)
  {
    key.invariant();
    return out << key.creative_key_ << '\t' << key.yandex_ref_id_ << '\t' <<
      key.yandex_event_date_;
  }

  PostClickStatInnerData::PostClickStatInnerData() noexcept
    : visits_(0),
      visits_with_bounce_(0),
      session_time_sum_(0),
      page_views_(0),
      new_user_visits_(0),
      reporting_visits_(0)
  {}

  PostClickStatInnerData::PostClickStatInnerData(
    std::uint64_t visits,
    std::uint64_t visits_with_bounce,
    std::uint64_t session_time_sum,
    std::uint64_t page_views,
    std::uint64_t new_user_visits,
    std::uint64_t reporting_visits) noexcept
    : visits_(visits),
      visits_with_bounce_(visits_with_bounce),
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
    out << data.session_time_sum_ << '\t';
    out << data.page_views_ << '\t';
    out << data.new_user_visits_ << '\t';
    out << data.reporting_visits_;
    return out;
  }
}
