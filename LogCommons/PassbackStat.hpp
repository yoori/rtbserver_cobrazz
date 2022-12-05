#ifndef AD_SERVER_LOG_PROCESSING_PASSBACK_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_PASSBACK_STAT_HPP


#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class PassbackStatInnerKey_V_2_4
{
  typedef StringIoWrapperOptional OptionalStringT;

public:
  PassbackStatInnerKey_V_2_4()
  :
    user_status_(),
    country_code_(),
    tag_id_(),
    hash_()
  {
  }

  PassbackStatInnerKey_V_2_4(
    char user_status,
    const std::string& country_code,
    unsigned long tag_id
  )
  :
    user_status_(user_status),
    country_code_(country_code),
    tag_id_(tag_id),
    hash_()
  {
    calc_hash_();
    country_code_.get().resize(2);
  }

  bool operator==(const PassbackStatInnerKey_V_2_4& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return user_status_ == rhs.user_status_ &&
      country_code_ == rhs.country_code_ &&
      tag_id_ == rhs.tag_id_;
  }

  char user_status() const
  {
    return user_status_;
  }

  const std::string& country_code() const
  {
    return country_code_.get();
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PassbackStatInnerKey_V_2_4& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const PassbackStatInnerKey_V_2_4& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    hash_add(hasher, country_code_.get());
    hash_add(hasher, user_status_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "PassbackStatInnerKey_V_2_4::invariant(): user_status_ "
         << "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  char user_status_;
  OptionalStringT country_code_;
  unsigned long tag_id_;
  size_t hash_;
};

class PassbackStatInnerKey
{
  typedef StringIoWrapperOptional OptionalStringT;

public:
  PassbackStatInnerKey()
  :
    user_status_(),
    country_code_(),
    tag_id_(),
    size_id_(),
    hash_()
  {
  }

  PassbackStatInnerKey(
    char user_status,
    const std::string& country_code,
    unsigned long tag_id,
    const OptionalUlong& size_id
  )
  :
    user_status_(user_status),
    country_code_(country_code),
    tag_id_(tag_id),
    size_id_(size_id),
    hash_()
  {
    country_code_.get().resize(2);
    calc_hash_();
  }

  PassbackStatInnerKey(
    const PassbackStatInnerKey_V_2_4& key
  )
  :
    user_status_(key.user_status()),
    country_code_(key.country_code()),
    tag_id_(key.tag_id()),
    size_id_(),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const PassbackStatInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return user_status_ == rhs.user_status_ &&
      country_code_ == rhs.country_code_ &&
      tag_id_ == rhs.tag_id_ &&
      size_id_ == rhs.size_id_;
  }

  char user_status() const
  {
    return user_status_;
  }

  const std::string& country_code() const
  {
    return country_code_.get();
  }

  unsigned long tag_id() const
  {
    return tag_id_;
  }

  const OptionalUlong& size_id() const
  {
    return size_id_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PassbackStatInnerKey& key)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const PassbackStatInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, tag_id_);
    size_id_.hash_add(hasher);
    hash_add(hasher, country_code_.get());
    hash_add(hasher, user_status_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (!is_valid_user_status(user_status_))
    {
      Stream::Error es;
      es << "PassbackStatInnerKey::invariant(): user_status_ "
         << "has invalid value '" << user_status_ << '\'';
      throw ConstraintViolation(es);
    }
  }

  char user_status_;
  OptionalStringT country_code_;
  unsigned long tag_id_;
  OptionalUlong size_id_;
  size_t hash_;
};

class PassbackStatInnerData
{
public:
  PassbackStatInnerData()
  :
    requests_()
  {
  }

  explicit
  PassbackStatInnerData(
    unsigned long requests
  )
  :
    requests_(requests)
  {
  }

  bool operator==(const PassbackStatInnerData& rhs) const
  {
    return requests_ == rhs.requests_;
  }

  PassbackStatInnerData&
  operator+=(const PassbackStatInnerData& rhs)
  {
    requests_ += rhs.requests_;
    return *this;
  }

  unsigned long requests() const
  {
    return requests_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, PassbackStatInnerData& data)
    /*throw(eh::Exception)*/;

  friend
  std::ostream&
  operator<<(std::ostream& os, const PassbackStatInnerData& data)
    /*throw(eh::Exception)*/;

private:
  unsigned long requests_;
};

struct PassbackStatKey
{
  PassbackStatKey(): sdate_(), colo_id_(), hash_() {}

  PassbackStatKey(
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

  bool operator==(const PassbackStatKey& rhs) const
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
  operator>>(std::istream& is, PassbackStatKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const PassbackStatKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    sdate_.hash_add(hasher);
    hash_add(hasher, colo_id_);
  }

  DayHourTimestamp sdate_;
  unsigned long colo_id_;
  size_t hash_;
};


typedef PassbackStatInnerData PassbackStatInnerData_V_2_4;

typedef StatCollector<
          PassbackStatInnerKey_V_2_4, PassbackStatInnerData_V_2_4, false, true
        > PassbackStatInnerCollector_V_2_4;

typedef PassbackStatInnerCollector_V_2_4 PassbackStatData_V_2_4;

typedef PassbackStatKey PassbackStatKey_V_2_4;

typedef StatCollector<PassbackStatKey_V_2_4, PassbackStatData_V_2_4>
  PassbackStatCollector_V_2_4;

typedef StatCollector<PassbackStatInnerKey, PassbackStatInnerData, false, true>
  PassbackStatInnerCollector;

typedef PassbackStatInnerCollector PassbackStatData;

typedef StatCollector<PassbackStatKey, PassbackStatData>
  PassbackStatCollector;

struct PassbackStatTraits: LogDefaultTraits<PassbackStatCollector>
{
  template <class FUNCTOR_>
  static
  void
  for_each_old(FUNCTOR_& f) /*throw(eh::Exception)*/
  {
    // V2.4 may be packed
    f.template operator()<PassbackStatCollector_V_2_4, true>("2.4");
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_PASSBACK_STAT_HPP */

