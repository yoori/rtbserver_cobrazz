#ifndef BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP

// STD
#include <memory>

// UNIXCOMMONS
#include <eh/Exception.hpp>

// THIS
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>
#include <LogCommons/StatCollector.hpp>
#include "Types.hpp"

namespace AdServer::LogProcessing
{

namespace Predictor = PredictorSvcs::BidCostPredictor;

class CtrKey final
{
public:
  using TagId = Predictor::Types::TagId;
  using Url = Predictor::Types::Url;
  using UrlPtr = Predictor::Types::UrlPtr;
  using CreativeCategoryId = Predictor::Types::CreativeCategoryId;
  using Hash = std::size_t;

public:
  explicit CtrKey()
  : tag_id_(0),
    url_(std::make_shared<Url>()),
    creative_category_id_(0)
  {
  }

  explicit CtrKey(
    const TagId& tag_id,
    const Url& url,
    const CreativeCategoryId& creative_category_id)
    : tag_id_(tag_id),
      url_(std::make_shared<Url>(url)),
      creative_category_id_(creative_category_id)
  {
    calc_hash();
  }

  explicit CtrKey(
    const TagId& tag_id,
    const UrlPtr& url,
    const CreativeCategoryId& creative_category_id)
    : tag_id_(tag_id),
      url_(url),
      creative_category_id_(creative_category_id)
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
    {
      return true;
    }

    return tag_id_ == rht.tag_id_
      && *url_ == *rht.url_
      && creative_category_id_ == rht.creative_category_id_;
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

  CreativeCategoryId creative_category_id() const noexcept
  {
    return creative_category_id_;
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
    (ar & tag_id_ & *url_) ^ creative_category_id_;
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
    Generics::hash_add(hasher, creative_category_id_);
  }

private:
  TagId tag_id_ = 0;

  UrlPtr url_;

  CreativeCategoryId creative_category_id_ = 0;

  Hash hash_ = 0;
};

class CtrData final
{
public:
  using Ctr = LogProcessing::FixedNumber;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  CtrData() = default;

  explicit CtrData(const Ctr& ctr)
    : ctr_(ctr)
  {
  }

  ~CtrData() = default;

  CtrData(const CtrData&) = default;
  CtrData(CtrData&&) = default;
  CtrData& operator=(const CtrData&) = default;
  CtrData& operator=(CtrData&&) = default;

  const Ctr& ctr() const noexcept
  {
    return ctr_;
  }

  bool is_null() const noexcept
  {
    return ctr_.is_zero();
  }

  CtrData& operator+=(const CtrData& /*other*/)
  {
    Stream::Error ostr;
    ostr << FNS
         << " : Reason: Logic error";
    throw Exception(ostr);
  }

  template <class ARCHIVE_>
  void serialize(ARCHIVE_& ar)
  {
    ar ^ ctr_;
  }

  friend FixedBufStream<TabCategory>& operator>>(
    FixedBufStream<TabCategory>& is,
    CtrData& data);

  friend std::ostream& operator<<(
    std::ostream& os,
    const CtrData& data);

private:
  Ctr ctr_ = Ctr::ZERO;
};

using CtrCollector = StatCollector<CtrKey, CtrData, true, true>;
using CtrTraits = LogDefaultTraits<CtrCollector, false>;

} // namespace AdServer::LogProcessing

#endif //BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP
