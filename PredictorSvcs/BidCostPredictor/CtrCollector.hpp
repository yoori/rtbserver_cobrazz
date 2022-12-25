#ifndef BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP
#define BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP

// STD
#include <memory>

// THIS
#include <eh/Exception.hpp>
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
    ostr << __PRETTY_FUNCTION__
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

} // namespace LogProcessing
} // namespace AdServer

#endif //BIDCOSTPREDICTOR_CTRCOLLECTOR_HPP
