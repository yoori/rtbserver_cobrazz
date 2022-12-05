#ifndef AD_SERVER_LOG_PROCESSING_EXPRESSION_PERFORMANCE_HPP
#define AD_SERVER_LOG_PROCESSING_EXPRESSION_PERFORMANCE_HPP


#include <iosfwd>
#include <istream>
#include <ostream>
#include <Generics/Time.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include "LogCommons.hpp"
#include "StatCollector.hpp"

namespace AdServer {
namespace LogProcessing {

class ExpressionPerformanceInnerKey
{
public:
  ExpressionPerformanceInnerKey()
  :
    cc_id_(),
    expression_(),
    hash_()
  {
  }

  ExpressionPerformanceInnerKey(
    unsigned long cc_id,
    const std::string& expression
  )
  :
    cc_id_(cc_id),
    expression_(expression),
    hash_()
  {
    calc_hash_();
  }

  bool operator==(const ExpressionPerformanceInnerKey& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return cc_id_ == rhs.cc_id_ && expression_ == rhs.expression_;
  }

  unsigned long cc_id() const
  {
    return cc_id_;
  }

  const std::string& expression() const
  {
    return expression_;
  }

  size_t hash() const
  {
    return hash_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ExpressionPerformanceInnerKey& key);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ExpressionPerformanceInnerKey& key)
    /*throw(eh::Exception)*/;

private:
  void calc_hash_()
  {
    Generics::Murmur64Hash hasher(hash_);
    hash_add(hasher, cc_id_);
    hash_add(hasher, expression_);
  }

  void invariant() const /*throw(eh::Exception)*/
  {
    if (expression_.empty())
    {
      throw ConstraintViolation("ExpressionPerformanceInnerKey::"
        "invariant(): expression_ must be non-empty");
    }
  }

  unsigned long cc_id_;
  std::string expression_;
  size_t hash_;
};

class ExpressionPerformanceInnerData
{
public:
  ExpressionPerformanceInnerData()
  :
    imps_verified_(),
    clicks_(),
    actions_()
  {
  }

  ExpressionPerformanceInnerData(
    unsigned long imps_verified,
    unsigned long clicks,
    unsigned long actions
  )
  :
    imps_verified_(imps_verified),
    clicks_(clicks),
    actions_(actions)
  {
  }

  bool operator==(const ExpressionPerformanceInnerData& rhs) const
  {
    if (&rhs == this)
    {
      return true;
    }
    return imps_verified_ == rhs.imps_verified_ &&
      clicks_ == rhs.clicks_ &&
      actions_ == rhs.actions_;
  }

  ExpressionPerformanceInnerData&
  operator+=(const ExpressionPerformanceInnerData& rhs)
  {
    imps_verified_ += rhs.imps_verified_;
    clicks_ += rhs.clicks_;
    actions_ += rhs.actions_;
    return *this;
  }

  unsigned long imps_verified() const
  {
    return imps_verified_;
  }

  unsigned long clicks() const
  {
    return clicks_;
  }

  unsigned long actions() const
  {
    return actions_;
  }

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is,
    ExpressionPerformanceInnerData& data);

  friend
  std::ostream&
  operator<<(std::ostream& os, const ExpressionPerformanceInnerData& data);

private:
  unsigned long imps_verified_;
  unsigned long clicks_;
  unsigned long actions_;
};

struct ExpressionPerformanceKey
{
  ExpressionPerformanceKey(): sdate_(), colo_id_(), hash_() {}

  ExpressionPerformanceKey(
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

  bool operator==(const ExpressionPerformanceKey& rhs) const
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

  friend
  std::istream&
  operator>>(std::istream& is, ExpressionPerformanceKey& key);

  friend
  std::ostream& operator<<(std::ostream& os,
    const ExpressionPerformanceKey& key)
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

typedef StatCollector<
          ExpressionPerformanceInnerKey,
          ExpressionPerformanceInnerData,
          false,
          true
        > ExpressionPerformanceInnerCollector;

typedef ExpressionPerformanceInnerCollector ExpressionPerformanceData;

typedef StatCollector<
          ExpressionPerformanceKey,
          ExpressionPerformanceData
        > ExpressionPerformanceCollector;

typedef LogDefaultTraits<ExpressionPerformanceCollector>
  ExpressionPerformanceTraits;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_EXPRESSION_PERFORMANCE_HPP */

