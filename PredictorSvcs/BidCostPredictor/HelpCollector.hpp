#ifndef BIDCOSTPREDICTOR_HELPCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_HELPCOLLECTOR_HPP

// STD
#include <memory>

// THIS
#include <Generics/Hash.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class HelpInnerKey final
{
  using FixedNumber = LogProcessing::FixedNumber;
  using Cost = FixedNumber;
  using Hash = std::size_t;

public:
  explicit HelpInnerKey()
    : cost_(FixedNumber::ZERO)
  {
  }

  explicit HelpInnerKey(const Cost& cost)
    : cost_(cost)
  {
    calc_hash();
  }

  HelpInnerKey(const HelpInnerKey&) = default;
  HelpInnerKey(HelpInnerKey&&) = default;
  HelpInnerKey& operator=(const HelpInnerKey&) = default;
  HelpInnerKey& operator=(HelpInnerKey&&) = default;

  bool operator==(const HelpInnerKey& rht) const
  {
    if (&rht == this)
      return true;

    return cost_ == rht.cost_;
  }

  bool operator<(const HelpInnerKey& rht) const
  {
    return cost_ < rht.cost_;
  }

  const Cost& cost() const
  {
    return cost_;
  }

  Hash hash() const
  {
    return hash_;
  };

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    Generics::hash_add(hasher, cost_);
  }

private:
  Cost cost_;

  Hash hash_ = 0;
};

using HelpInnerData = LogProcessing::BidCostStatInnerData;

class HelpKey final
{
public:
  using TagId = unsigned long;
  using Url = std::string;
  using Url_var = std::shared_ptr<Url>;
  using Hash = std::size_t;

public:
  explicit HelpKey()
  : tag_id_(0),
    url_(std::make_shared<std::string>())
  {
  }

  explicit HelpKey(
    const TagId& tag_id,
    const Url& url)
    : tag_id_(tag_id),
      url_(std::make_shared<std::string>(url))
  {
    calc_hash();
  }

  explicit HelpKey(
    const TagId& tag_id,
    const Url_var& url)
    : tag_id_(tag_id),
      url_(url)
  {
    calc_hash();
  }

  HelpKey(const HelpKey&) = default;
  HelpKey(HelpKey&&) = default;
  HelpKey& operator=(const HelpKey&) = default;
  HelpKey& operator=(HelpKey&&) = default;

  bool operator==(const HelpKey& rht) const noexcept
  {
    if (&rht == this)
      return true;

    return tag_id_ == rht.tag_id_
      && *url_ == *rht.url_;
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

  Hash hash() const noexcept
  {
    return hash_;
  }

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    Generics::hash_add(hasher, tag_id_);
    Generics::hash_add(hasher, *url_);
  }

private:
  TagId tag_id_ = 0;

  Url_var url_;

  Hash hash_ = 0;
};

constexpr bool is_help_collector_map = false;

using HelpInnerCollector =
  LogProcessing::StatCollector<HelpInnerKey, HelpInnerData, true, true, true, is_help_collector_map>;

using HelpData = HelpInnerCollector;

using HelpCollector = LogProcessing::StatCollector<HelpKey, HelpData, true, false>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_HELPCOLLECTOR_HPP
