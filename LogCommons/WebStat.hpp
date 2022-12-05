#ifndef AD_SERVER_LOG_PROCESSING_WEB_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_WEB_STAT_HPP

#include <cctype>

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer {
namespace LogProcessing {

class WebStatInnerKey_V_3_1
{
  typedef StringIoWrapperOptional OptionalStringT;

public:
  typedef OptionalValue<unsigned long> OptionalUlong;

  WebStatInnerKey_V_3_1() noexcept
  :
    ct_(),
    curct_(),
    browser_(),
    os_(),
    web_operation_id_(),
    result_(),
    user_status_(),
    test_(),
    tag_id_(),
    cc_id_(),
    hash_()
  {
  }

  WebStatInnerKey_V_3_1(
    const std::string& ct,
    const std::string& curct,
    const std::string& browser,
    const std::string& os,
    unsigned long web_operation_id,
    char result,
    char user_status,
    bool test,
    const OptionalUlong& tag_id,
    const OptionalUlong& cc_id
  )
    /*throw(eh::Exception)*/
  :
    ct_(ct),
    curct_(curct),
    browser_(browser),
    os_(os),
    web_operation_id_(web_operation_id),
    result_(result),
    user_status_(user_status),
    test_(test),
    tag_id_(tag_id),
    cc_id_(cc_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const WebStatInnerKey_V_3_1& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return ct_ == rhs.ct_ &&
      curct_ == rhs.curct_ &&
      browser_ == rhs.browser_ &&
      os_ == rhs.os_ &&
      web_operation_id_ == rhs.web_operation_id_ &&
      result_ == rhs.result_ &&
      user_status_ == rhs.user_status_ &&
      test_ == rhs.test_ &&
      tag_id_ == rhs.tag_id_ &&
      cc_id_ == rhs.cc_id_;
  }

  const std::string& ct() const noexcept
  {
    return ct_.get();
  }

  const std::string& curct() const noexcept
  {
    return curct_.get();
  }

  const std::string& browser() const noexcept
  {
    return browser_.get();
  }

  const std::string& os() const noexcept
  {
    return os_.get();
  }

  unsigned long
  web_operation_id() const noexcept
  {
    return web_operation_id_;
  }

  char result() const noexcept
  {
    return result_;
  }

  char user_status() const noexcept
  {
    return user_status_;
  }

  bool test() const noexcept
  {
    return test_;
  }

  const OptionalUlong& tag_id() const noexcept
  {
    return tag_id_;
  }

  const OptionalUlong& cc_id() const noexcept
  {
    return cc_id_;
  }

  size_t hash() const noexcept
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, WebStatInnerKey_V_3_1& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, ct_.get());
    hash_add(hasher, curct_.get());
    hash_add(hasher, browser_.get());
    hash_add(hasher, os_.get());
    hash_add(hasher, web_operation_id_);
    hash_add(hasher, result_);
    hash_add(hasher, user_status_);
    hash_add(hasher, test_);
    tag_id_.hash_add(hasher);
    cc_id_.hash_add(hasher);
  }

  void invariant_() const /*throw(ConstraintViolation)*/
  {
    if (!std::isgraph(result_))
    {
      Stream::Error es;
      es << "WebStatInnerKey_V_3_1::invariant_(): result_ "
        "has invalid value (ASCII code: " << short(result_) << ")";
      throw ConstraintViolation(es);
    }
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "WebStatInnerKey_V_3_1::invariant_(): user_status_ "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  OptionalStringT ct_;
  OptionalStringT curct_;
  OptionalStringT browser_;
  OptionalStringT os_;
  unsigned long web_operation_id_;
  char result_;
  char user_status_;
  bool test_;
  OptionalUlong tag_id_;
  OptionalUlong cc_id_;
  size_t hash_;
};

class WebStatInnerKey
{
  typedef StringIoWrapperOptional OptionalStringT;

public:
  typedef OptionalValue<unsigned long> OptionalUlong;

  WebStatInnerKey() noexcept
  :
    ct_(),
    curct_(),
    browser_(),
    os_(),
    web_operation_id_(),
    result_(),
    user_status_(),
    test_(),
    tag_id_(),
    cc_id_(),
    hash_()
  {
  }

  WebStatInnerKey(
    const std::string& ct,
    const std::string& curct,
    const std::string& browser,
    const std::string& os,
    const std::string& source,
    unsigned long web_operation_id,
    char result,
    char user_status,
    bool test,
    const OptionalUlong& tag_id,
    const OptionalUlong& cc_id
  )
    /*throw(eh::Exception)*/
  :
    ct_(ct),
    curct_(curct),
    browser_(browser),
    os_(os),
    source_(source),
    web_operation_id_(web_operation_id),
    result_(result),
    user_status_(user_status),
    test_(test),
    tag_id_(tag_id),
    cc_id_(cc_id),
    hash_()
  {
    calc_hash_();
  }

  WebStatInnerKey(
    const WebStatInnerKey_V_3_1& key
  )
    /*throw(eh::Exception)*/
  :
    ct_(key.ct()),
    curct_(key.curct()),
    browser_(key.browser()),
    os_(key.os()),
    source_(),
    web_operation_id_(key.web_operation_id()),
    result_(key.result()),
    user_status_(key.user_status()),
    test_(key.test()),
    tag_id_(key.tag_id()),
    cc_id_(key.cc_id()),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const WebStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return ct_ == rhs.ct_ &&
      curct_ == rhs.curct_ &&
      browser_ == rhs.browser_ &&
      os_ == rhs.os_ &&
      source_ == rhs.source_ &&
      web_operation_id_ == rhs.web_operation_id_ &&
      result_ == rhs.result_ &&
      user_status_ == rhs.user_status_ &&
      test_ == rhs.test_ &&
      tag_id_ == rhs.tag_id_ &&
      cc_id_ == rhs.cc_id_;
  }

  const std::string& ct() const noexcept
  {
    return ct_.get();
  }

  const std::string& curct() const noexcept
  {
    return curct_.get();
  }

  const std::string& browser() const noexcept
  {
    return browser_.get();
  }

  const std::string& os() const noexcept
  {
    return os_.get();
  }

  const std::string& source() const noexcept
  {
    return source_.get();
  }

  unsigned long
  web_operation_id() const noexcept
  {
    return web_operation_id_;
  }

  char result() const noexcept
  {
    return result_;
  }

  char user_status() const noexcept
  {
    return user_status_;
  }

  bool test() const noexcept
  {
    return test_;
  }

  const OptionalUlong& tag_id() const noexcept
  {
    return tag_id_;
  }

  const OptionalUlong& cc_id() const noexcept
  {
    return cc_id_;
  }

  size_t hash() const noexcept
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, WebStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const WebStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, ct_.get());
    hash_add(hasher, curct_.get());
    hash_add(hasher, browser_.get());
    hash_add(hasher, os_.get());
    hash_add(hasher, source_.get());
    hash_add(hasher, web_operation_id_);
    hash_add(hasher, result_);
    hash_add(hasher, user_status_);
    hash_add(hasher, test_);
    tag_id_.hash_add(hasher);
    cc_id_.hash_add(hasher);
  }

  void invariant_() const /*throw(ConstraintViolation)*/
  {
    if (!std::isgraph(result_))
    {
      Stream::Error es;
      es << "WebStatInnerKey::invariant_(): result_ "
        "has invalid value (ASCII code: " << short(result_) << ")";
      throw ConstraintViolation(es);
    }
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "WebStatInnerKey::invariant_(): user_status_ "
        "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  OptionalStringT ct_;
  OptionalStringT curct_;
  OptionalStringT browser_;
  OptionalStringT os_;
  OptionalStringT source_;
  unsigned long web_operation_id_;
  char result_;
  char user_status_;
  bool test_;
  OptionalUlong tag_id_;
  OptionalUlong cc_id_;
  size_t hash_;
};

class WebStatInnerData
{
public:
  explicit
  WebStatInnerData(unsigned long count = 0) noexcept
  :
    count_(count)
  {
  }

  bool operator==(const WebStatInnerData& rhs) const noexcept
  {
    return count_ == rhs.count_;
  }

  WebStatInnerData&
  operator+=(const WebStatInnerData& rhs)
    noexcept
  {
    count_ += rhs.count_;
    return *this;
  }

  unsigned long count() const noexcept
  {
    return count_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, WebStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const WebStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long count_;
};

struct WebStatKey
{
  WebStatKey(): sdate_(), colo_id_(), hash_() {}

  WebStatKey(
    const DayHourTimestamp& sdate,
    unsigned long colo_id
  )
  :
    sdate_(sdate),
    colo_id_(colo_id),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const WebStatKey& rhs) const
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

  friend
  std::istream&
  operator>>(std::istream& is, WebStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const WebStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  void invariant_() const /*throw(ConstraintViolation)*/
  {
    if (!colo_id_)
    {
      throw ConstraintViolation("WebStatKey::invariant_(): "
        "colo_id_ must be > 0");
    }
  }

  DayHourTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};

typedef WebStatInnerData WebStatInnerData_V_3_1;

typedef WebStatKey WebStatKey_V_3_1;

typedef StatCollector<
          WebStatKey_V_3_1,
          StatCollector<
            WebStatInnerKey_V_3_1, WebStatInnerData_V_3_1, false, true
          >
        > WebStatCollector_V_3_1;

typedef StatCollector<
          WebStatKey,
          StatCollector<WebStatInnerKey, WebStatInnerData, false, true>
        > WebStatCollector;

struct WebStatTraits: LogDefaultTraits<WebStatCollector>
{
  template <class FUNCTOR_>
  static void for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    f.template operator()<WebStatCollector_V_3_1, true>("3.1");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_WEB_STAT_HPP */

