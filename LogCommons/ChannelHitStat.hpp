#ifndef AD_SERVER_LOG_PROCESSING_CHANNEL_HIT_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CHANNEL_HIT_STAT_HPP


#include <iosfwd>
#include <istream>
#include <ostream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class ChannelHitStatInnerKey
{
public:
  explicit
  ChannelHitStatInnerKey(
    unsigned long channel_id = 0
  )
  :
    channel_id_(channel_id)
  {
  }

  bool operator==(const ChannelHitStatInnerKey& rhs) const
  {
    return channel_id_ == rhs.channel_id_;
  }

  unsigned long channel_id() const
  {
    return channel_id_;
  }

  size_t hash() const
  {
    return channel_id_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelHitStatInnerKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelHitStatInnerKey& key);

private:
  unsigned long channel_id_;
};

class ChannelHitStatInnerData_V_1_0
{
public:
  ChannelHitStatInnerData_V_1_0()
  :
    hits_(),
    hits_urls_(),
    hits_kws_(),
    hits_search_kws_(),
    hits_repeat_kws_()
  {
  }

  ChannelHitStatInnerData_V_1_0(
    unsigned long hits,
    unsigned long hits_urls,
    unsigned long hits_kws,
    unsigned long hits_search_kws,
    unsigned long hits_repeat_kws
  )
  :
    hits_(hits),
    hits_urls_(hits_urls),
    hits_kws_(hits_kws),
    hits_search_kws_(hits_search_kws),
    hits_repeat_kws_(hits_repeat_kws)
  {
  }

  bool operator==(const ChannelHitStatInnerData_V_1_0& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return hits_ == rhs.hits_ &&
      hits_urls_ == rhs.hits_urls_ &&
      hits_kws_ == rhs.hits_kws_ &&
      hits_search_kws_ == rhs.hits_search_kws_ &&
      hits_repeat_kws_ == rhs.hits_repeat_kws_;
  }

  ChannelHitStatInnerData_V_1_0&
  operator+=(const ChannelHitStatInnerData_V_1_0& rhs)
  {
    hits_ += rhs.hits_;
    hits_urls_ += rhs.hits_urls_;
    hits_kws_ += rhs.hits_kws_;
    hits_search_kws_ += rhs.hits_search_kws_;
    hits_repeat_kws_ += rhs.hits_repeat_kws_;
    return *this;
  }

  unsigned long hits() const
  {
    return hits_;
  }

  unsigned long hits_urls() const
  {
    return hits_urls_;
  }

  unsigned long hits_kws() const
  {
    return hits_kws_;
  }

  unsigned long hits_search_kws() const
  {
    return hits_search_kws_;
  }

  unsigned long hits_repeat_kws() const
  {
    return hits_repeat_kws_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ChannelHitStatInnerData_V_1_0& data);

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelHitStatInnerData_V_1_0& data);

private:
  unsigned long hits_;
  unsigned long hits_urls_;
  unsigned long hits_kws_;
  unsigned long hits_search_kws_;
  unsigned long hits_repeat_kws_;
};

class ChannelHitStatInnerData
{
public:
  ChannelHitStatInnerData()
  :
    hits_(),
    hits_urls_(),
    hits_kws_(),
    hits_search_kws_(),
    hits_url_kws_()
  {
  }

  ChannelHitStatInnerData(
    unsigned long hits,
    unsigned long hits_urls,
    unsigned long hits_kws,
    unsigned long hits_search_kws,
    unsigned long hits_url_kws
  )
  :
    hits_(hits),
    hits_urls_(hits_urls),
    hits_kws_(hits_kws),
    hits_search_kws_(hits_search_kws),
    hits_url_kws_(hits_url_kws)
  {
  }

  ChannelHitStatInnerData(
    const ChannelHitStatInnerData_V_1_0& data
  )
  :
    hits_(data.hits()),
    hits_urls_(data.hits_urls()),
    hits_kws_(data.hits_kws()),
    hits_search_kws_(data.hits_search_kws()),
    hits_url_kws_()
  {
  }

  bool operator==(const ChannelHitStatInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return hits_ == rhs.hits_ &&
      hits_urls_ == rhs.hits_urls_ &&
      hits_kws_ == rhs.hits_kws_ &&
      hits_search_kws_ == rhs.hits_search_kws_ &&
      hits_url_kws_ == rhs.hits_url_kws_;
  }

  ChannelHitStatInnerData&
  operator+=(const ChannelHitStatInnerData& rhs)
  {
    hits_ += rhs.hits_;
    hits_urls_ += rhs.hits_urls_;
    hits_kws_ += rhs.hits_kws_;
    hits_search_kws_ += rhs.hits_search_kws_;
    hits_url_kws_ += rhs.hits_url_kws_;
    return *this;
  }

  unsigned long hits() const
  {
    return hits_;
  }

  unsigned long hits_urls() const
  {
    return hits_urls_;
  }

  unsigned long hits_kws() const
  {
    return hits_kws_;
  }

  unsigned long hits_search_kws() const
  {
    return hits_search_kws_;
  }

  unsigned long hits_url_kws() const
  {
    return hits_url_kws_;
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ChannelHitStatInnerData& data);

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelHitStatInnerData& data);

private:
  unsigned long hits_;
  unsigned long hits_urls_;
  unsigned long hits_kws_;
  unsigned long hits_search_kws_;
  unsigned long hits_url_kws_;
};

struct ChannelHitStatKey
{
  ChannelHitStatKey(): sdate_(), colo_id_(), hash_() {}

  ChannelHitStatKey(
    const DayTimestamp& sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ChannelHitStatKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
  }

  const DayTimestamp& sdate() const
  {
    return sdate_;
  }

  unsigned long colo_id() const
  {
    return colo_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend std::istream&
  operator>>(std::istream& is, ChannelHitStatKey& key);

  friend std::ostream&
  operator<<(std::ostream& os, const ChannelHitStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef ChannelHitStatInnerKey ChannelHitStatInnerKey_V_1_0;

typedef StatCollector<
          ChannelHitStatInnerKey_V_1_0,
          ChannelHitStatInnerData_V_1_0,
          false,
          true
        > ChannelHitStatInnerCollector_V_1_0;

typedef ChannelHitStatKey ChannelHitStatKey_V_1_0;
typedef ChannelHitStatInnerCollector_V_1_0 ChannelHitStatData_V_1_0;

typedef StatCollector<ChannelHitStatKey_V_1_0, ChannelHitStatData_V_1_0>
  ChannelHitStatCollector_V_1_0;

typedef StatCollector<
          ChannelHitStatInnerKey, ChannelHitStatInnerData, false, true
        > ChannelHitStatInnerCollector;

typedef ChannelHitStatInnerCollector ChannelHitStatData;

typedef StatCollector<ChannelHitStatKey, ChannelHitStatData>
  ChannelHitStatCollector;

struct ChannelHitStatTraits: LogDefaultTraits<ChannelHitStatCollector>
{
  template <class FUNCTOR_>
  static
  void
  for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    // V1.0 may be packed
    f.template operator()<ChannelHitStatCollector_V_1_0, true>("1.0");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CHANNEL_HIT_STAT_HPP */

