#ifndef BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP

// STD
#include <memory>

// THIS
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>
#include <LogCommons/StatCollector.hpp>
#include "HelpCollector.hpp"

namespace AdServer
{
namespace LogProcessing
{

namespace Predictor = PredictorSvcs::BidCostPredictor;

class CtrKey final
{
public:
  using Key = typename Predictor::HelpCollector::Key;
  using TagId = typename Key::TagId;
  using Url = typename Key::Url;
  using Url_var = typename Key::Url_var;
  using Hash = std::size_t;

public:
  explicit CtrKey()
  : tag_id_(0),
    url_(std::make_shared<Url>())
  {
  }

  explicit CtrKey(
    const TagId& tag_id,
    const Url& url)
    : tag_id_(tag_id),
      url_(std::make_shared<Url>(url))
  {
    calc_hash();
  }

  explicit CtrKey(
    const TagId& tag_id,
    const Url_var& url)
    : tag_id_(tag_id),
      url_(url)
  {
    calc_hash();
  }

  CtrKey(const CtrKey&) = default;
  CtrKey(CtrKey&&) = default;
  CtrKey& operator=(const CtrKey&) = default;
  CtrKey& operator=(CtrKey&&) = default;

  bool operator==(const CtrKey& rht) const
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

  void invariant() const noexcept
  {
  }

  Hash hash() const noexcept
  {
    return hash_;
  }

  template <class ARCHIVE_>
  void serialize(ARCHIVE_& ar)
  {
    (ar
     & tag_id_)
    ^ *url_;
  }

  friend FixedBufStream<TabCategory>& operator>>(
    FixedBufStream<TabCategory>& is,
    CtrKey& key);

  friend std::ostream& operator<<(
    std::ostream& os,
    const CtrKey& key);

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

class CtrData final
{
public:
  using Imps = long;
  using Clicks = long;

public:
  CtrData() = default;

  explicit CtrData(
    const Clicks& clicks,
    const Imps& imps)
    : clicks_(clicks),
      imps_(imps)
  {
  }

  ~CtrData() = default;

  CtrData(const CtrData&) = default;
  CtrData(CtrData&&) = default;
  CtrData& operator=(const CtrData&) = default;
  CtrData& operator=(CtrData&&) = default;

  Imps imps() const noexcept
  {
    return imps_;
  }

  Clicks clicks() const noexcept
  {
    return clicks_;
  }

  CtrData& operator+=(const CtrData& rhs)
  {
    clicks_ += rhs.clicks_;
    imps_ += rhs.imps_;
    return *this;
  }

  bool is_null() const noexcept
  {
    return imps_ == 0 && clicks_ == 0;
  }

  template <class ARCHIVE_>
  void serialize(ARCHIVE_& ar)
  {
    (ar
     & clicks_)
    ^ imps_;
  }

  friend FixedBufStream<TabCategory>& operator>>(
    FixedBufStream<TabCategory>& is,
    CtrData& data);

  friend std::ostream& operator<<(
    std::ostream& os,
    const CtrData& data);

private:
  Clicks clicks_ = 0;

  Imps imps_ = 0;
};

using CtrCollector = StatCollector<CtrKey, CtrData, true, true>;
using CtrTraits = LogDefaultTraits<CtrCollector, false>;

} // namespace LogProcessing
} // namespace AdServer

#endif //BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP
