#ifndef AD_SERVER_LOG_COMMONS_IMP_NOTIFY_HPP
#define AD_SERVER_LOG_COMMONS_IMP_NOTIFY_HPP


#include <iosfwd>
#include <ostream>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

#include <LogCommons/GenericLogIoImpl.hpp>

namespace AdServer {
namespace LogProcessing {

struct ImpNotifyData
{
  SecondsTimestamp time;
  UserId user_id;
  NumberList channels;
  FixedNumber revenue;

  ImpNotifyData(): revenue(FixedNumber::ZERO) {}

  ImpNotifyData(
    const SecondsTimestamp& time_val,
    const UserId& user_id_val,
    const NumberList& channels_val,
    const FixedNumber& revenue_val
  )
  :
    time(time_val),
    user_id(user_id_val),
    channels(channels_val),
    revenue(revenue_val)
  {
  }

  bool operator==(const ImpNotifyData& data) const
  {
    if (this == &data)
    {
      return true;
    }
    return time == data.time &&
      user_id == data.user_id &&
      channels == data.channels &&
      revenue == data.revenue;
  }

  unsigned long distrib_hash() const
  {
    return user_id_distribution_hash(user_id);
  }

  void invariant() const /*throw(ConstraintViolation)*/
  {
    if (user_id.is_null())
    {
      Stream::Error es;
      es << "ImpNotifyData::invariant(): user_id is NULL";
      throw ConstraintViolation(es);
    }
  }

  friend FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ImpNotifyData& data)
    /*throw(eh::Exception)*/;

  friend std::ostream&
  operator<<(std::ostream& os, const ImpNotifyData& data)
    /*throw(eh::Exception)*/;
};

typedef SeqCollector<ImpNotifyData, true> ImpNotifyCollector;

struct ImpNotifyTraits: LogDefaultTraits<ImpNotifyCollector, false, false>
{
  typedef GenericLogIoHelperImpl<ImpNotifyTraits> IoHelperType;
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_COMMONS_IMP_NOTIFY_HPP */

