#ifndef RTBSERVER_CTRHELPCOLLECTOR_HPP
#define RTBSERVER_CTRHELPCOLLECTOR_HPP

// BOOST
#include <boost/functional/hash.hpp>

// STD
#include <memory>
#include <unordered_map>

// THIS
#include "Types.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace DetailCtrHelpCollector
{

class Key final
{
public:
  using TagId = Types::TagId;
  using Url = Types::Url;
  using UrlPtr = Types::UrlPtr;
  using CcId = Types::CcId;
  using Hash = std::size_t;

public:
  explicit Key(
    const TagId& tag_id,
    const UrlPtr& url,
    const CcId& cc_id)
    : tag_id_(tag_id),
      url_(url),
      cc_id_(cc_id)
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
    {
      return true;
    }

    return tag_id_ == rht.tag_id_
      && *url_ == *rht.url_
      && cc_id_ == rht.cc_id_;
  }

  TagId tag_id() const noexcept
  {
    return tag_id_;
  }

  const Url& url() const noexcept
  {
    return *url_;
  }

  const UrlPtr& url_ptr() const noexcept
  {
    return url_;
  }

  CcId cc_id() const noexcept
  {
    return cc_id_;
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
    boost::hash_combine(hash_, cc_id_);
  }

private:
  TagId tag_id_;

  UrlPtr url_;

  CcId cc_id_;

  Hash hash_ = 0;
};

} // namespace DetailCtrHelpCollector

class CtrHelpCollector final
{
public:
  using Imps = Types::Imps;
  using Clicks = Types::Clicks;
  using Key = DetailCtrHelpCollector::Key;

  struct Data final
  {
    Data() = default;

    bool operator==(const Data& rht) const noexcept = default;

    Imps total_imps = 0;
    Clicks total_clicks = 0;
  };

  using Map = std::unordered_map<
    Key,
    Data,
    Generics::HashFunForHashAdapter<Key>>;
  using MapPtr = std::shared_ptr<Map>;
  using ConstIterator = typename Map::const_iterator;

public:
  CtrHelpCollector(
    const Imps max_imps = 100000,
    const std::size_t bucket_count = 1000000)
    : map_(std::make_shared<Map>(bucket_count)),
      max_imps_(max_imps)
  {
  }

  ~CtrHelpCollector() = default;

  CtrHelpCollector(const CtrHelpCollector&) = default;
  CtrHelpCollector(CtrHelpCollector&&) = default;
  CtrHelpCollector& operator=(const CtrHelpCollector&) = default;
  CtrHelpCollector& operator=(CtrHelpCollector&&) = default;

  CtrHelpCollector& add(
    const Key& key,
    const Imps imps,
    const Clicks clicks)
  {
    auto& [total_imps, total_clicks] = (*map_)[key];
    if (total_imps <= max_imps_)
    {
      total_imps += imps;
      total_clicks += clicks;
    }

    return *this;
  }

  bool empty() const noexcept
  {
    return map_->empty();
  }

  ConstIterator begin() const noexcept
  {
    return map_->begin();
  }

  ConstIterator end() const noexcept
  {
    return map_->end();
  }

  bool operator==(const CtrHelpCollector& other) const noexcept
  {
    if (this == &other)
    {
      return true;
    }

    return *map_ == *other.map_;
  }

private:
  MapPtr map_;

  Imps max_imps_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //RTBSERVER_CTRHELPCOLLECTOR_HPP
