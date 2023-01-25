#ifndef BIDCOSTPREDICTOR_BIDCOSTCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_BIDCOSTCOLLECTOR_HPP

// THIS
#include <LogCommons/StatCollector.hpp>
#include "Types.hpp"

namespace AdServer
{
namespace LogProcessing
{

namespace Predictor = PredictorSvcs::BidCostPredictor;

class BidCostKey final
{
public:
  using TagId = Predictor::Types::TagId;
  using Url = Predictor::Types::Url;
  using Url_var = Predictor::Types::Url_var;
  using WinRate = Predictor::Types::WinRate;

public:
  explicit BidCostKey()
  : tag_id_(0),
    url_(std::make_shared<Url>()),
    win_rate_(FixedNumber::ZERO)
  {
  }

  explicit BidCostKey(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate)
    : tag_id_(tag_id),
      url_(std::make_shared<Url>(url)),
      win_rate_(win_rate)
  {
    calc_hash();
  }

  explicit BidCostKey(
    const TagId& tag_id,
    const Url_var& url,
    const WinRate& win_rate)
    : tag_id_(tag_id),
      url_(url),
      win_rate_(win_rate)
  {
    calc_hash();
  }

  BidCostKey(const BidCostKey&) = default;
  BidCostKey(BidCostKey&&) = default;
  BidCostKey& operator=(const BidCostKey&) = default;
  BidCostKey& operator=(BidCostKey&&) = default;

  bool operator==(const BidCostKey& rht) const
  {
    if (&rht == this)
      return true;

    return tag_id_ == rht.tag_id_
      && *url_ == *rht.url_
      && win_rate_ == rht.win_rate_;
  }

  TagId tag_id() const noexcept
  {
    return tag_id_;
  }

  const Url& url() const noexcept
  {
    return *url_;
  }

  const Url_var& url_var() const noexcept
  {
    return url_;
  }

  const WinRate& win_rate() const noexcept
  {
    return win_rate_;
  }

  void invariant() const noexcept
  {
  }

  std::size_t hash() const noexcept
  {
    return hash_;
  }

  template <class ARCHIVE_>
  void serialize(ARCHIVE_& ar)
  {
    (ar
     & tag_id_
     & *url_)
    ^ win_rate_;
  }

  friend FixedBufStream<TabCategory>& operator>>(
    FixedBufStream<TabCategory>& is,
    BidCostKey& key);

  friend std::ostream& operator<<(
    std::ostream& os,
    const BidCostKey& key);

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    Generics::hash_add(hasher, tag_id_);
    Generics::hash_add(hasher, *url_);
    Generics::hash_add(hasher, win_rate_);
  }

private:
  TagId tag_id_ = 0;

  Url_var url_;

  WinRate win_rate_;

  std::size_t hash_ = 0;
};

class BidCostData final
{
  using Cost = FixedNumber;
public:
  explicit BidCostData()
  : cost_(Cost::ZERO),
    max_cost_(Cost::ZERO)
  {
  }

  explicit BidCostData(
    const Cost& cost,
    const Cost& max_cost)
    : cost_(cost),
      max_cost_(max_cost)
  {
  }

  BidCostData(const BidCostData&) = default;
  BidCostData(BidCostData&&) = default;
  BidCostData& operator=(const BidCostData&) = default;
  BidCostData& operator=(BidCostData&&) = default;

  const Cost& cost() const noexcept
  {
    return cost_;
  }

  const Cost& max_cost() const noexcept
  {
    return max_cost_;
  }

  BidCostData& operator+=(const BidCostData& rhs)
  {
    cost_ += rhs.cost_;
    max_cost_ += rhs.max_cost_;
    return *this;
  }

  bool is_null() const noexcept
  {
    return cost_.is_zero() && max_cost_.is_zero();
  }

  template <class ARCHIVE_>
  void serialize(ARCHIVE_& ar)
  {
    (ar
     & cost_)
    ^ max_cost_;
  }

  friend FixedBufStream<TabCategory>& operator>>(
    FixedBufStream<TabCategory>& is,
    BidCostData& data);

  friend std::ostream& operator<<(
    std::ostream& os,
    const BidCostData& data);

private:
  Cost cost_;

  Cost max_cost_;
};

using BidCostCollector = StatCollector<BidCostKey, BidCostData, true, true>;
using BidCostTraits = LogDefaultTraits<BidCostCollector, false>;

} // namespace LogProcessing
} // namespace AdServer

#endif //BIDCOSTPREDICTOR_BIDCOSTCOLLECTOR_HPP
