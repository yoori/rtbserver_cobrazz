#include "PostClickConvStat.hpp"

#include <utility>

#include "BufferWriter.hpp"

namespace AdServer::LogProcessing
{
  template <> const char* PostClickConvStatTraits::B::base_name_ = "PostClickConvStat";
  template <> const char* PostClickConvStatTraits::B::signature_ = "PostClickConvStat";
  template <> const char* PostClickConvStatTraits::B::current_version_ = "2.0";

  PostClickConvStatTraits::CollectorType
  PostClickConvStatTraits::convert_collector(const PostClickConvStatCollector_V_1_0& old_collector)
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

  bool PostClickConvStatInnerKey_V_1_0::operator==(const PostClickConvStatInnerKey_V_1_0& rhs)
    const noexcept
  {
    return PostClickStatInnerKey_V_1_1::operator==(rhs) && conversion_name_ == rhs.conversion_name_;
  }

  PostClickConvStatInnerKey PostClickConvStatInnerKey_V_1_0::current_key() const
  {
    const auto key = PostClickStatInnerKey_V_1_1::current_key();
    return PostClickConvStatInnerKey(
      creative_key(), key.external_metrics_connection_type_id(), conversion_name_);
  }

  std::size_t PostClickConvStatInnerKey_V_1_0::hash() const noexcept
  {
    return hash_;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickConvStatInnerKey_V_1_0& key)
  {
    is >> static_cast<PostClickStatInnerKey_V_1_1&>(key) >> key.conversion_name_;
    key.current_key().invariant();
    Generics::Murmur64Hash hasher(key.hash_);
    hash_add(hasher, key.PostClickStatInnerKey_V_1_1::hash());
    hash_add(hasher, key.conversion_name_);
    return is;
  }

  PostClickConvStatInnerKey::PostClickConvStatInnerKey() noexcept = default;

  PostClickConvStatInnerKey::PostClickConvStatInnerKey(
    CreativeStatInnerKey creative_key, std::uint64_t connection_type_id,
    const std::string& conversion_name)
    : PostClickStatInnerKey(std::move(creative_key), connection_type_id),
      conversion_name_(conversion_name)
  {
    calc_hash_();
  }

  bool PostClickConvStatInnerKey::operator==(const PostClickConvStatInnerKey& rhs) const noexcept
  {
    return PostClickStatInnerKey::operator==(rhs) && conversion_name_ == rhs.conversion_name_;
  }

  const std::string& PostClickConvStatInnerKey::conversion_name() const noexcept
  {
    return conversion_name_;
  }

  std::size_t PostClickConvStatInnerKey::hash() const noexcept
  {
    return hash_;
  }

  void PostClickConvStatInnerKey::invariant() const
  {
    PostClickStatInnerKey::invariant();
    if (conversion_name_.empty() || conversion_name_.find(':') != std::string::npos)
    {
      throw ConstraintViolation("Invalid conversion name in PostClickConvStat");
    }
  }

  void PostClickConvStatInnerKey::calc_hash_() noexcept
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, PostClickStatInnerKey::hash());
    hash_add(hasher, conversion_name_);
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickConvStatInnerKey& key)
  {
    is >> static_cast<PostClickStatInnerKey&>(key);
    is >> key.conversion_name_;
    key.invariant();
    key.calc_hash_();
    return is;
  }

  BufferWriter& operator<<(BufferWriter& out, const PostClickConvStatInnerKey& key)
  {
    key.invariant();
    return out << static_cast<const PostClickStatInnerKey&>(key) << '\t' << key.conversion_name_;
  }

  PostClickConvStatInnerData::PostClickConvStatInnerData() noexcept = default;

  PostClickConvStatInnerData::PostClickConvStatInnerData(std::uint64_t visits, std::uint64_t goals)
    noexcept
    : visits_(visits), goals_(goals)
  {}

  bool PostClickConvStatInnerData::operator==(const PostClickConvStatInnerData& rhs) const noexcept
  {
    return visits_ == rhs.visits_ && goals_ == rhs.goals_;
  }

  PostClickConvStatInnerData&
  PostClickConvStatInnerData::operator+=(const PostClickConvStatInnerData& rhs) noexcept
  {
    visits_ += rhs.visits_;
    goals_ += rhs.goals_;
    return *this;
  }

  std::uint64_t PostClickConvStatInnerData::visits() const noexcept
  {
    return visits_;
  }

  std::uint64_t PostClickConvStatInnerData::goals() const noexcept
  {
    return goals_;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PostClickConvStatInnerData& data)
  {
    is >> data.visits_;
    is >> data.goals_;
    return is;
  }

  BufferWriter& operator<<(BufferWriter& out, const PostClickConvStatInnerData& data)
  {
    return out << data.visits_ << '\t' << data.goals_;
  }
}
