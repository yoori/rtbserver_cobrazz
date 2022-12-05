#ifndef AD_SERVER_LOG_PROCESSING_WEBWISE_DISCOVER_ITEM_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_WEBWISE_DISCOVER_ITEM_STAT_HPP

#include <iosfwd>
#include <sstream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"


namespace AdServer {
namespace LogProcessing {

// WebwiseDiscoverItemStat v1.1 - AdServer v1.12-2.2

class WebwiseDiscoverItemStatInnerKey_V_1_1
{
  typedef Aux_::StringIoWrapper StringT;

public:
  WebwiseDiscoverItemStatInnerKey_V_1_1()
  :
    wdtag_id_(),
    item_id_(),
    xslt_(),
    position_(),
    random_(),
    test_(),
    hash_()
  {
  }

  WebwiseDiscoverItemStatInnerKey_V_1_1(
    unsigned long wdtag_id,
    const std::string &item_id,
    const std::string &xslt,
    unsigned long position,
    bool random,
    bool test
  )
  :
    wdtag_id_(wdtag_id),
    item_id_(item_id),
    xslt_(xslt),
    position_(position),
    random_(random),
    test_(test),
    hash_()
  {
    calc_hash();
  }

  bool operator==(const WebwiseDiscoverItemStatInnerKey_V_1_1 &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return wdtag_id_ == rhs.wdtag_id_ &&
      item_id_ == rhs.item_id_ &&
      xslt_ == rhs.xslt_ &&
      position_ == rhs.position_ &&
      random_ == rhs.random_ &&
      test_ == rhs.test_;
  }

  unsigned long wdtag_id() const
  {
    return wdtag_id_;
  }

  const StringT& item_id() const
  {
    return item_id_;
  }

  const StringT& xslt() const
  {
    return xslt_;
  }

  unsigned long position() const
  {
    return position_;
  }

  bool random() const
  {
    return random_;
  }

  bool test() const
  {
    return test_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend std::istream& operator>>(std::istream &is,
    WebwiseDiscoverItemStatInnerKey_V_1_1 &key) /*throw(eh::Exception)*/;
  friend std::ostream& operator<<(std::ostream &os,
    const WebwiseDiscoverItemStatInnerKey_V_1_1 &key) /*throw(eh::Exception)*/;

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, wdtag_id_);
    hash_add(hasher, position_);
    hash_add(hasher, random_);
    hash_add(hasher, test_);
    hash_add(hasher, xslt_);
    hash_add(hasher, item_id_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (item_id_.empty())
    {
      throw ConstraintViolation("WebwiseDiscoverItemStatInnerKey_V_1_1::"
        "invariant(): item_id_ must be non-empty");
    }
    if (xslt_.empty())
    {
      throw ConstraintViolation("WebwiseDiscoverItemStatInnerKey_V_1_1::"
        "invariant(): xslt_ must be non-empty");
    }
  }

  unsigned long wdtag_id_;
  StringT item_id_;
  StringT xslt_;
  unsigned long position_;
  bool random_;
  bool test_;
  size_t hash_;
};

class WebwiseDiscoverItemStatInnerKey
{
  typedef Aux_::StringIoWrapper StringT;

public:
  WebwiseDiscoverItemStatInnerKey()
  :
    wdtag_id_(),
    item_id_(),
    xslt_(),
    position_(),
    random_(),
    test_(),
    user_status_(),
    hash_()
  {
  }

  WebwiseDiscoverItemStatInnerKey(
    unsigned long wdtag_id,
    const std::string &item_id,
    const std::string &xslt,
    unsigned long position,
    bool random,
    bool test,
    char user_status
  )
  :
    wdtag_id_(wdtag_id),
    item_id_(item_id),
    position_(position),
    random_(random),
    test_(test),
    user_status_(user_status),
    hash_()
  {
    trim(xslt_, xslt, MAX_XSLT_LENGTH_);
    calc_hash();
  }

  WebwiseDiscoverItemStatInnerKey(
    const WebwiseDiscoverItemStatInnerKey_V_1_1 &key
  )
  :
    wdtag_id_(key.wdtag_id()),
    item_id_(key.item_id()),
    position_(key.position()),
    random_(key.random()),
    test_(key.test()),
    user_status_('U'),
    hash_()
  {
    trim(xslt_, key.xslt(), MAX_XSLT_LENGTH_);
    calc_hash();
  }

  bool operator==(const WebwiseDiscoverItemStatInnerKey &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return wdtag_id_ == rhs.wdtag_id_ &&
      item_id_ == rhs.item_id_ &&
      xslt_ == rhs.xslt_ &&
      position_ == rhs.position_ &&
      random_ == rhs.random_ &&
      test_ == rhs.test_ &&
      user_status_ == rhs.user_status_;
  }

  unsigned long wdtag_id() const
  {
    return wdtag_id_;
  }

  const StringT& item_id() const
  {
    return item_id_;
  }

  const StringT& xslt() const
  {
    return xslt_;
  }

  unsigned long position() const
  {
    return position_;
  }

  bool random() const
  {
    return random_;
  }

  bool test() const
  {
    return test_;
  }

  char user_status() const
  {
    return user_status_;
  }

  size_t hash() const
  {
    return hash_;
  }

  static std::size_t max_xslt_length()
  {
    return MAX_XSLT_LENGTH_;
  }

  friend
  std::istream&
  operator>>(std::istream &is, WebwiseDiscoverItemStatInnerKey &key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream &os, const WebwiseDiscoverItemStatInnerKey &key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, wdtag_id_);
    hash_add(hasher, position_);
    hash_add(hasher, random_);
    hash_add(hasher, test_);
    hash_add(hasher, user_status_);
    hash_add(hasher, xslt_);
    hash_add(hasher, item_id_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (item_id_.empty())
    {
      throw ConstraintViolation("WebwiseDiscoverItemStatInnerKey::"
        "invariant(): item_id_ must be non-empty");
    }
    if (xslt_.empty())
    {
      throw ConstraintViolation("WebwiseDiscoverItemStatInnerKey::"
        "invariant(): xslt_ must be non-empty");
    }
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "WebwiseDiscoverItemStatInnerKey::invariant(): user_status_ "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  static const std::size_t MAX_XSLT_LENGTH_ = 2000;

  unsigned long wdtag_id_;
  StringT item_id_;
  StringT xslt_;
  unsigned long position_;
  bool random_;
  bool test_;
  char user_status_;
  size_t hash_;
};

class WebwiseDiscoverItemStatInnerData
{
public:
  WebwiseDiscoverItemStatInnerData()
  :
    imps_(),
    clicks_()
  {
  }

  WebwiseDiscoverItemStatInnerData(
    unsigned long imps,
    unsigned long clicks
  )
  :
    imps_(imps),
    clicks_(clicks)
  {
  }

  bool operator==(const WebwiseDiscoverItemStatInnerData &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return imps_ == rhs.imps_ && clicks_ == rhs.clicks_;
  }

  WebwiseDiscoverItemStatInnerData&
  operator+=(const WebwiseDiscoverItemStatInnerData &rhs)
  {
    imps_ += rhs.imps_;
    clicks_ += rhs.clicks_;
    return *this;
  }

  unsigned long imps() const
  {
    return imps_;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  friend std::istream& operator>>(std::istream &is,
    WebwiseDiscoverItemStatInnerData &data) /*throw(eh::Exception)*/;
  friend std::ostream& operator<<(std::ostream &os,
    const WebwiseDiscoverItemStatInnerData &data) /*throw(eh::Exception)*/;

private:
  unsigned long imps_;
  unsigned long clicks_;
};

struct WebwiseDiscoverItemStatKey
{
  WebwiseDiscoverItemStatKey(): sdate_(), colo_id_(), hash_() {}

  WebwiseDiscoverItemStatKey(
    const DayHourTimestamp &sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash();
  }

  bool operator==(const WebwiseDiscoverItemStatKey &rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return sdate_ == rhs.sdate_ && colo_id_ == rhs.colo_id_;
  }

public:
  const DayHourTimestamp& sdate() const
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

  friend std::istream& operator>>(std::istream &is,
    WebwiseDiscoverItemStatKey &key);
  friend std::ostream& operator<<(std::ostream &os,
    const WebwiseDiscoverItemStatKey &key) /*throw(eh::Exception)*/;

private:
  void calc_hash()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayHourTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};


typedef WebwiseDiscoverItemStatInnerData
  WebwiseDiscoverItemStatInnerData_V_1_1;

typedef StatCollector<
          WebwiseDiscoverItemStatInnerKey_V_1_1,
          WebwiseDiscoverItemStatInnerData_V_1_1
        > WebwiseDiscoverItemStatInnerCollector_V_1_1;

typedef WebwiseDiscoverItemStatInnerCollector_V_1_1
  WebwiseDiscoverItemStatData_V_1_1;

typedef WebwiseDiscoverItemStatKey WebwiseDiscoverItemStatKey_V_1_1;

typedef StatCollector<
          WebwiseDiscoverItemStatKey_V_1_1,
          WebwiseDiscoverItemStatData_V_1_1
        > WebwiseDiscoverItemStatCollector_V_1_1;

typedef StatCollector<
          WebwiseDiscoverItemStatInnerKey,
          WebwiseDiscoverItemStatInnerData
        > WebwiseDiscoverItemStatInnerCollector;

typedef WebwiseDiscoverItemStatInnerCollector WebwiseDiscoverItemStatData;

typedef StatCollector<WebwiseDiscoverItemStatKey, WebwiseDiscoverItemStatData>
  WebwiseDiscoverItemStatCollector;

struct WebwiseDiscoverItemStatTraits : LogDefaultTraits<WebwiseDiscoverItemStatCollector>
{
  template <typename Functor>
  static void
  for_each_old(Functor& f) /*throw(eh::Exception)*/
  {
    f.template operator ()<WebwiseDiscoverItemStatCollector_V_1_1>("1.1");
  }
};


} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_WEBWISE_DISCOVER_ITEM_STAT_HPP */

