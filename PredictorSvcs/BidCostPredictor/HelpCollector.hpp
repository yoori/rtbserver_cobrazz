#ifndef BIDCOSTPREDICTOR_HELPCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_HELPCOLLECTOR_HPP

// STD
#include <memory>

// BOOST
#include <boost/container/flat_map.hpp>
#include <boost/functional/hash.hpp>

// THIS
#include <Generics/GnuHashTable.hpp>
#include <LogCommons/BidCostStat.hpp>
#include <LogCommons/LogCommons.hpp>
#include "Types.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

namespace detail_help_collector
{

class Key final
{
public:
  using TagId = Types::TagId;
  using Url = Types::Url;
  using Url_var = Types::Url_var;
  using Hash = std::size_t;

public:
  explicit Key(
    const TagId& tag_id,
    const Url_var& url)
    : tag_id_(tag_id),
      url_(url)
  {
    calc_hash();
  }

  Key(const Key&) = default;
  Key(Key&&) = default;
  Key& operator=(const Key&) = default;
  Key& operator=(Key&&) = default;

  bool operator==(const Key& rht) const noexcept
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
    boost::hash_combine(hash_, tag_id_);
    boost::hash_combine(hash_, *url_);
  }

private:
  TagId tag_id_ = 0;

  Url_var url_;

  Hash hash_ = 0;
};

} // namespace detail_help_collector

class HelpCollector final
{
public:
  using Cost = Types::Cost;
  using InnerKey = Cost;
  using InnerData = LogProcessing::BidCostStatInnerData;
  using Imps = Types::Imps;
  using Clicks = Types::Clicks;

  class InnerCollector final
    : protected boost::container::flat_map<InnerKey, InnerData>
  {
  public:
    using FlatMap = boost::container::flat_map<InnerKey, InnerData>;
    using const_iterator = typename FlatMap::const_iterator;
    using const_reverse_iterator = typename FlatMap::const_reverse_iterator;
    using size_type = typename FlatMap::size_type;
    using Imps = typename HelpCollector::Imps;
    using Clicks = typename HelpCollector::Clicks;

  public:
    explicit InnerCollector(const Imps max_imps)
      : max_imps_(max_imps)
    {
    }

    ~InnerCollector() = default;

    InnerCollector(const InnerCollector&) = default;

    InnerCollector(InnerCollector&&) = default;

    InnerCollector& operator=(const InnerCollector&) = default;

    InnerCollector& operator=(InnerCollector&&) = default;

    Clicks total_clicks() const noexcept
    {
      return total_clicks_;
    }

    Imps total_imps() const noexcept
    {
      return total_imps_;
    }

    void reserve(const size_type cnt)
    {
      FlatMap::reserve(cnt);
    }

    bool empty() const noexcept
    {
      return FlatMap::empty();
    }

    size_type size() const noexcept
    {
      return FlatMap::size();
    }

    const_iterator begin() const noexcept
    {
      return FlatMap::begin();
    }

    const_iterator end() const noexcept
    {
      return FlatMap::end();
    }

    const_reverse_iterator rbegin() const noexcept
    {
      return FlatMap::rbegin();
    }

    const_reverse_iterator rend() const noexcept
    {
      return FlatMap::rend();
    }

    const_iterator nth(const size_type index) const noexcept
    {
      return FlatMap::nth(index);
    }

    InnerCollector& add(
      const InnerKey& key,
      const InnerData& data)
    {
      auto result = FlatMap::try_emplace(key, data);
      if (!result.second)
      {
        result.first->second += data;
      }

      if (total_imps_ <= max_imps_)
      {
        total_imps_ += data.imps();
        total_clicks_ += data.clicks();
      }

      return *this;
    }

    InnerCollector& operator+=(const InnerCollector& collector)
    {
      for (const auto& [key, data] : collector)
      {
        add(key, data);
      }

      return *this;
    }

    bool operator==(const InnerCollector& other) const
    {
      return static_cast<const FlatMap&>(*this) == static_cast<const FlatMap&>(other);
    }

    bool operator!=(const InnerCollector& other) const
    {
      return static_cast<const FlatMap&>(*this) != static_cast<const FlatMap&>(other);
    }

  private:
    const Imps max_imps_ = 0;

    Imps total_imps_ = 0;

    Clicks total_clicks_ = 0;
  };

  using Key = detail_help_collector::Key;
  using Data = InnerCollector;
  using Data_var = std::shared_ptr<Data>;
  using Map =
    std::unordered_map<
      Key,
      Data_var,
      Generics::HashFunForHashAdapter<Key>
    >;
  using Map_var = std::shared_ptr<Map>;
  using iterator = typename Map::iterator;
  using const_iterator = typename Map::const_iterator;

public:
  explicit HelpCollector(
    const Imps max_imps,
    const std::size_t bucket_count = 1,
    const std::size_t inner_collector_capacity = 1)
    : map_(std::make_shared<Map>(bucket_count)),
      max_imps_(max_imps),
      inner_collector_capacity_(inner_collector_capacity)
  {
  }

  ~HelpCollector() = default;

  HelpCollector(const HelpCollector&) = default;

  HelpCollector(HelpCollector&&) = default;

  HelpCollector& operator=(const HelpCollector&) = default;

  HelpCollector& operator=(HelpCollector&&) = default;

  std::size_t size() const
  {
    return map_->size();
  }

  bool empty() const noexcept
  {
    return map_->empty();
  }

  iterator begin() noexcept
  {
    return map_->begin();
  }

  const_iterator begin() const noexcept
  {
    return map_->cbegin();
  }

  iterator end() noexcept
  {
    return map_->end();
  }

  const_iterator end() const noexcept
  {
    return map_->cend();
  }

  iterator find(const Key& key)
  {
    return map_->find(key);
  }

  const_iterator find(const Key& key) const
  {
    return map_->find(key);
  }

  InnerCollector& find_or_insert(const Key& key)
  {
    auto it = map_->find(key);
    if (it == map_->end())
    {
      auto inner_collector = std::make_shared<InnerCollector>(max_imps_);
      inner_collector->reserve(inner_collector_capacity_);
      it = map_->try_emplace(key, std::move(inner_collector)).first;
    }

    return *it->second;
  }

  void erase(iterator it)
  {
    map_->erase(it);
  }

  HelpCollector& add(const Key& key, const Data& data)
  {
    auto it = map_->find(key);
    if (it == map_->end())
    {
      auto inner_collector = std::make_shared<InnerCollector>(max_imps_);
      inner_collector->reserve(inner_collector_capacity_);
      it = map_->try_emplace(key, std::move(inner_collector)).first;
    }
    *it->second += data;
    return *this;
  }

  bool operator==(const HelpCollector& other) const
  {
    if (this == &other)
      return true;

    if (map_->size() != other.map_->size())
      return false;

    auto it = map_->begin();
    auto it_end = map_->end();
    for (; it != it_end; ++it)
    {
      const auto& key = it->first;
      auto it_other = other.map_->find(key);
      if (it_other == other.map_->end())
        return false;

      if (*it->second != *it_other->second)
        return false;
    }

    return true;
  }

private:
  Map_var map_;

  Imps max_imps_ = 0;

  std::size_t inner_collector_capacity_ = 0;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_HELPCOLLECTOR_HPP
