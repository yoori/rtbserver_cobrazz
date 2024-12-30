#ifndef AD_SERVER_LOG_PROCESSING_LOG_COMMONS_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_COMMONS_HPP

#include <optional>

#include <iosfwd>
#include <memory>
#include <map>
#include <list>
#include <string>
#include <vector>
#include <forward_list>
#include <cmath>
#include <ctime>

#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <String/AsciiStringManip.hpp>
#include <Generics/Hash.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Logger/Logger.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Containers.hpp>
#include <Commons/AtomicInt.hpp>

#include <LogCommons/ArchiveOfstream.hpp>
#include <LogCommons/CollectorBundleTypeDefs.hpp>

namespace AdServer {
namespace LogProcessing {

DECLARE_EXCEPTION(InvalidArgValue, eh::DescriptiveException);
DECLARE_EXCEPTION(ConstraintViolation, eh::DescriptiveException);
DECLARE_EXCEPTION(InvalidLogFileNameFormat, eh::DescriptiveException);

extern const Generics::Time ONE_DAY;
extern const Generics::Time ONE_MONTH;

struct LogFileNameInfo
{
  enum LOG_FILE_NAME_FORMAT
  {
    LFNF_INVALID = 0, LFNF_CSV, LFNF_BASIC, LFNF_EXTENDED, LFNF_V_2_6,
    LFNF_CURRENT = LFNF_V_2_6
  };

  LogFileNameInfo() noexcept
    : format(), timestamp(), random(), distrib_count(),
      distrib_index(), processed_lines_count()
  {}

  explicit
  LogFileNameInfo(const std::string& log_base_name) noexcept
  :
    format(LFNF_CURRENT),
    base_name(log_base_name),
    timestamp(Generics::Time::get_time_of_day()),
    random(),
    distrib_count(1),
    distrib_index(),
    processed_lines_count()
  {}

  bool
  invalid() const noexcept
  {
    return format == LFNF_INVALID;
  }

  const char*
  str_format() const noexcept
  {
    static const char* const CONV_TABLE[] = {
      "LFNF_INVALID", "LFNF_CSV", "LFNF_BASIC", "LFNF_EXTENDED", "LFNF_V_2_6"
    };
    return static_cast<std::size_t>(int(format)) <
      sizeof(CONV_TABLE) / sizeof(CONV_TABLE[0]) ?
        CONV_TABLE[format] : "<OUT OF RANGE>";
  }

  LOG_FILE_NAME_FORMAT format;
  std::string base_name;
  Generics::Time timestamp;
  unsigned long random;
  unsigned long distrib_count;
  unsigned long distrib_index;
  unsigned long processed_lines_count;
};

namespace Aux_ {

inline bool
check_null(std::istream& is, char null_marker) /*throw(eh::Exception)*/;

//
// StringIO traits
//
struct EscapeChar
{
  static const char ESC_CHAR;
};

struct ConvertSpaces
{
  static const char WRITE_RESERVED_CHAR_TABLE[128][2];
};

struct ConvertSpacesSeparators
{
  static const char WRITE_RESERVED_CHAR_TABLE[128][2];
};

//
//  TimeFacet class traits
//

struct DayTimeFacetTraits
{
  static time_t
  round(const Generics::Time &time) noexcept
  {
    return time.tv_sec / Generics::Time::ONE_DAY.tv_sec *
      Generics::Time::ONE_DAY.tv_sec;
  }

  static const char* format() noexcept { return "%Y-%m-%d"; }
};

struct DayHourTimeFacetTraits
{
  static time_t
  round(const Generics::Time &time) noexcept
  {
    return time.tv_sec / Generics::Time::ONE_HOUR.tv_sec *
      Generics::Time::ONE_HOUR.tv_sec;
  }

  static const char* format() noexcept { return "%Y-%m-%d:%H"; }
};

struct SecondsTimeFacetTraits
{
  static time_t
  round(const Generics::Time &time) noexcept
  {
    return time.tv_sec;
  }

  static const char* format() noexcept { return "%Y-%m-%d_%H:%M:%S"; }
};

//
//  OptionalValue class traits
//

template <class T_>
struct DefaultOptionalValueTypeTraits
{
  static const char PRESENT_MARKER = '@';

  static const char NULL_MARKER = '-';

  static bool
  is_empty(const T_& /* value */) noexcept
  {
    return false;
  }

  static bool
  check_null(std::istream& is) /*throw(eh::Exception)*/
  {
    return Aux_::check_null(is, NULL_MARKER);
  }

  static void clear(T_ &value)
  {
    value = T_();
  }

  static std::istream& read_value(std::istream &is, T_ &value)
  {
    if (is.peek() == PRESENT_MARKER)
    {
      is.get();
    }
    return is >> value;
  }

  static std::ostream& write_value(std::ostream &os, const T_ &value)
  {
    return os << PRESENT_MARKER << value;
  }
};

template <typename T_>
struct ClearableOptionalValueTraits :
  public DefaultOptionalValueTypeTraits<T_>
{
  static bool
  is_empty(const T_& value)
  {
    return value.empty();
  }

  static void clear(T_ &value)
  {
    value.clear();
  }
};

//
// Archieves classes traits
//
struct NoInvariants
{
  template <typename Data>
  static void
  invariant(const Data&) noexcept
  {}
};

struct OwnInvariants
{
  template <typename Data>
  static void
  invariant(const Data& data) /*throw(ConstraintViolation)*/
  {
    data.invariant();
  }
};

//
// read_sequence() modificators
//

struct SpacesAtEnd
{
  bool
  operator ()(char ch) const noexcept
  {
    return String::AsciiStringManip::SPACE.is_owned(ch);
  }
};

template <const char END_MARKER = '\t'>
struct EndMarkerOrSpacesAtEnd
{
  bool
  operator ()(char ch) const noexcept
  {
    return ch == END_MARKER || SpacesAtEnd()(ch);
  }
};

} // namespace Aux_


template <typename Category>
class FixedBufStream;

typedef AdServer::Commons::UserId UserId;
typedef AdServer::Commons::RequestId RequestId;
typedef Generics::SimpleDecimal<uint64_t, 18, 8> FixedNumber;

typedef std::deque<unsigned long> NumberArray;

typedef std::list<unsigned long> NumberList;

typedef std::list<std::string> StringList;

typedef std::list<FixedNumber> FixedNumberList;

typedef std::multimap<Generics::Time, std::string> LogSortingMap;

typedef std::pair<std::string, std::string> StringPair;

struct ToUpper
{
  const std::string&
  operator ()(const std::string& str) /*throw(eh::Exception)*/
  {
    up_str_.clear();
    up_str_.reserve(str.size());
    std::transform(str.begin(), str.end(),
      std::back_insert_iterator<std::string>(up_str_),
      static_cast<char(*)(char)>(String::AsciiStringManip::to_upper));
    return up_str_;
  }

private:
  std::string up_str_;
};

template <typename Convertor, const char SEPARATOR = '\t'>
struct StringIO : public std::string
{
  StringIO()
  {}
  StringIO(const char* c_str): std::string(c_str)
  {}
  StringIO(const std::string& str): std::string(str)
  {}
};

template <typename Convertor, const char SEPARATOR>
std::istream&
operator >>(std::istream& is, StringIO<Convertor, SEPARATOR>& str)
  /*throw(eh::Exception)*/;

template <typename Convertor, const char SEPARATOR>
std::ostream&
operator <<(std::ostream& os, const StringIO<Convertor, SEPARATOR>& str)
  /*throw(eh::Exception)*/;

namespace Aux_
{
  typedef StringIO<Aux_::ConvertSpaces> StringIoWrapper;
}
typedef StringIO<Aux_::ConvertSpaces> SpacesString;
typedef StringIO<Aux_::ConvertSpacesSeparators> SpacesMarksString;

template <class PRECISION_TRAITS_> class TimeFacet
{
public:
  TimeFacet() noexcept : timestamp_(0) {}

  TimeFacet(const Generics::Time &timestamp) noexcept
    : timestamp_(PRECISION_TRAITS_::round(timestamp))
  {}

  bool
  operator ==(const TimeFacet& rhs) const noexcept
  {
    return timestamp_ == rhs.timestamp_;
  }

  bool
  operator !=(const TimeFacet& rhs) const noexcept
  {
    return !(*this == rhs);
  }

  bool
  operator <(const TimeFacet& right) const noexcept
  {
    return timestamp_ < right.timestamp_;
  }

  bool
  is_zero() const noexcept
  {
    return timestamp_ == 0;
  }

  Generics::Time
  time() const noexcept
  {
    return Generics::Time(timestamp_);
  }

  time_t
  hash() const noexcept
  {
    return timestamp_;
  }

  template <typename Hash>
  void
  hash_add(Hash& hash) const noexcept
  {
    hash.add(&timestamp_, sizeof(timestamp_));
  }

private:
  time_t timestamp_;
};

typedef TimeFacet<Aux_::DayTimeFacetTraits> DayTimestamp;
typedef TimeFacet<Aux_::DayHourTimeFacetTraits> DayHourTimestamp;
typedef TimeFacet<Aux_::SecondsTimeFacetTraits> SecondsTimestamp;

template <size_t PRECISION_ = 1> class FixedPrecisionDouble
{
public:
  FixedPrecisionDouble(double value = 0.) noexcept
    : value_(round_(value))
  {
  }

  FixedPrecisionDouble<PRECISION_>&
  operator +=(const FixedPrecisionDouble<PRECISION_>& right) noexcept
  {
    value_ = round_(value_ + right.value_);
    return *this;
  }

  double
  value() const noexcept
  {
    return value_;
  }

  template <typename Hash>
  void
  hash_add(Hash& hasher) noexcept
  {
    hasher.add(&value_, sizeof(value_));
  }

  bool
  operator ==(const FixedPrecisionDouble<PRECISION_>& right) const noexcept
  {
    return value_ == right.value_;
  }

  static size_t
  precision() noexcept
  {
    return PRECISION_;
  }

private:
  template <size_t Precision>
  friend std::istream&
  operator >>(std::istream& is, FixedPrecisionDouble<Precision>& value)
    /*throw(eh::Exception)*/;

  template <class Cat, size_t Precision>
  friend
  FixedBufStream<Cat>&
  operator>>(FixedBufStream<Cat>& is, FixedPrecisionDouble<Precision>& value)
    /*throw(eh::Exception)*/;

  template <size_t Precision>
  friend std::ostream&
  operator <<(std::ostream& os, const FixedPrecisionDouble<Precision>& value)
    /*throw(eh::Exception)*/;

  struct Format
  {
    Format();
    char format[16];
  };

  static const Format PRINT_;
  static const double N_;

  inline static double
  round_(double value) noexcept
  {
    return round(value * N_) / N_;
  }

  double value_;
};

template <typename Object>
class EmptyHolder
{
public:
  static const char NULL_MARKER = '-';

  EmptyHolder() noexcept
  {}

  EmptyHolder(const Object& object) : object_(object)
  {}

  Object&
  get() noexcept
  {
    return object_;
  }

  const Object&
  get() const noexcept
  {
    return object_;
  }
private:
  Object object_;
};

template <class UUID_>
struct UuidIoWrapper: public UUID_
{
  UuidIoWrapper(): UUID_() {}

  UuidIoWrapper(const UUID_& uuid): UUID_(uuid) {}

  static const char NULL_MARKER = '-';
};

template <class T_>
std::istream& operator>>(std::istream& is, UuidIoWrapper<T_>& value);

template <class T_>
std::ostream& operator<<(std::ostream& os, const UuidIoWrapper<T_>& value);

typedef UuidIoWrapper<UserId> UserIdIoWrapper;
typedef UuidIoWrapper<RequestId> RequestIdIoWrapper;

using Generics::hash_add;

namespace Internal
{

template<typename ObjectType>
class Optional
{
public:
  Optional(): defined_(false), val_() {}

  Optional(const Optional<ObjectType>& val)
    : defined_(val.defined_),
      val_(val.val_)
  {}

  Optional(Optional<ObjectType>&& val)
    : defined_(val.defined_),
      val_(std::move(val.val_))
  {}

  template<typename...Args>
  explicit Optional(
    bool defined,
    Args&&... data)
    : defined_(defined),
      val_(std::forward<Args>(data)...)
  {}

  explicit Optional(const ObjectType& val)
    : defined_(true),
      val_(val)
  {}

  Optional(const ObjectType* val)
    : defined_(val),
      val_(val ? *val : ObjectType())
  {}

  const ObjectType* operator->() const noexcept
  {
    return &val_;
  }

  ObjectType* operator->() noexcept
  {
    return &val_;
  }

  const ObjectType& operator*() const noexcept
  {
    return val_;
  }

  ObjectType& operator*() noexcept
  {
    return val_;
  }

  bool present() const noexcept
  {
    return defined_;
  }

  bool has_value() const noexcept
  {
    return defined_;
  }

  void set(const ObjectType& val)
  {
    val_ = val;
    defined_ = true;
  }

  Optional& operator=(const ObjectType& val)
  {
    set(val);
    return *this;
  }

  Optional& operator=(const Optional<ObjectType>& val)
  {
    if(val.present())
    {
      set(*val);
    }
    else
    {
      clear();
    }

    return *this;
  }

  template<typename RightType>
  Optional& operator=(const Optional<RightType>& val)
  {
    if(val.present())
    {
      set(*val);
    }
    else
    {
      clear();
    }

    return *this;
  }

  template<typename RightType>
  bool operator==(const Optional<RightType>& right) const
  {
    return &right == this || (present() == right.present() &&
                              (!present() || **this == *right));
  }

  template <typename RightType>
  bool operator !=(const Optional<RightType>& right) const
  {
    return !(*this == right);
  }

  void clear()
  {
    defined_ = false;
    val_ = ObjectType();
  }

protected:
  template <typename CompatibleType>
  Optional(const CompatibleType& val, bool defined)
    : defined_(defined), val_(defined ? ObjectType(val) : ObjectType())
  {}

  void present_(bool new_state) noexcept
  {
    defined_ = new_state;
  }

private:
  bool defined_;

  ObjectType val_;
};

} // namespace Internal

template <
  class T_,
  class VALUE_TYPE_TRAITS_ = Aux_::DefaultOptionalValueTypeTraits<T_>>
class OptionalValue : public Internal::Optional<T_>
{
  typedef Internal::Optional<T_> BaseType;
  typedef OptionalValue<T_, VALUE_TYPE_TRAITS_> Self_;

public:
  using BaseType::present;

  typedef T_ ValueT;

  OptionalValue()
  {}

  template <class VALUE_COMPATIBLE_TYPE_>
  OptionalValue(const VALUE_COMPATIBLE_TYPE_ &value)
    : BaseType(value, true)
  {
    this->present_(!VALUE_TYPE_TRAITS_::is_empty(**this));
  }

  template <typename TYPE_, typename TRAITS_>
  OptionalValue(const OptionalValue<TYPE_, TRAITS_> &value)
    : BaseType(*value, value.present())
  {}

  template <typename TYPE_>
  OptionalValue(const std::optional<TYPE_>& value)
    : BaseType(value.value_or(T_()), value.has_value())
  {
    this->present_(!VALUE_TYPE_TRAITS_::is_empty(**this));
  }

  const T_& get() const noexcept
  {
    return **this;
  }

  T_& get() noexcept
  {
    return **this;
  }

  void clear()
  {
    if (present())
    {
      VALUE_TYPE_TRAITS_::clear(get());
    }
  }

  Self_& operator+=(const Self_ &rhs)
  {
    if (rhs.present())
    {
      if (present())
      {
        get() += rhs.get();
      }
      else
      {
        *this = rhs;
      }
    }
    return *this;
  }

  void reset()
  {
    clear();
    BaseType::present_(false);
  }

  template <typename Hash>
  void hash_add(Hash& hash) const noexcept
  {
    if (present())
    {
      AdServer::LogProcessing::hash_add(hash, get());
    }
  }

  template <typename U, typename OVTraits>
  friend std::istream&
  operator >>(std::istream& is, OptionalValue<U, OVTraits>& ov);

  template <typename U, typename OVTraits>
  friend std::ostream&
  operator <<(std::ostream& os, const OptionalValue<U, OVTraits>& ov);

  template <typename Category, typename U, typename OVTraits>
  friend FixedBufStream<Category>&
  operator >>(FixedBufStream<Category>&, OptionalValue<U, OVTraits>&);
};

typedef OptionalValue<unsigned long> OptionalUlong;
typedef OptionalValue<FixedNumber> OptionalFixedNumber;
typedef OptionalValue<DayTimestamp> OptionalDayTimestamp;
typedef OptionalValue<DayHourTimestamp> OptionalDayHourTimestamp;
typedef OptionalValue<SecondsTimestamp> OptionalSecondsTimestamp;

typedef OptionalValue<std::string,
  Aux_::ClearableOptionalValueTraits<std::string> >
    OptionalString;

typedef OptionalValue<SpacesString,
  Aux_::ClearableOptionalValueTraits<SpacesString> >
    StringIoWrapperOptional;

std::istream& read_until_eol(std::istream &is, std::string &str, bool revert_eol = true);

std::istream& read_until_tab(std::istream &is, std::string &str);

std::istream& read_char(std::istream &is, char ch);

std::istream& read_eol(std::istream &is);

std::istream& read_tab(std::istream &is);

typedef const String::AsciiStringManip::Char1Category<'\t'> TabCategory;
typedef const String::AsciiStringManip::Char1Category<','>  CommaCategory;
typedef const String::AsciiStringManip::Char1Category<':'>  SemiCategory;
typedef const String::AsciiStringManip::Char1Category<'/'>  SlashCategory;
typedef const String::AsciiStringManip::Char1Category<'\n'> EolCategory;
typedef const String::AsciiStringManip::Char1Category<'='> EqlCategory;

template <typename InvariantsChecker = Aux_::OwnInvariants,
  const char SEPARATOR = '\t'>
class InputArchive;

template <typename InvariantsChecker = Aux_::OwnInvariants,
  const char SEPARATOR = '\t'>
class OutputArchive;

typedef InputArchive<Aux_::OwnInvariants, '\t'> TabInputArchive;
typedef OutputArchive<Aux_::OwnInvariants, '\t'> TabOutputArchive;
typedef InputArchive<Aux_::NoInvariants, '\t'> SimpleTabInputArchive;
typedef OutputArchive<Aux_::NoInvariants, '\t'> SimpleTabOutputArchive;

const unsigned long USER_ID_DISTRIBUTION_MOD = 1000;

unsigned long
user_id_distribution_hash(const AdServer::Commons::UserId& user_id);

unsigned long
request_distribution_hash(
  const AdServer::Commons::RequestId& request_id,
  const std::optional<unsigned long>& user_id_distrib_hash
);

unsigned long
request_distribution_hash(
  const AdServer::Commons::RequestId& request_id,
  const AdServer::Commons::UserId& user_id
);

struct TraceLevel
{
  enum
  {
    LOW = Logging::Logger::TRACE,
    MIDDLE,
    HIGH
  };
};

template <typename LogTraits>
class CollectorBundle;

template <typename LogTraits>
class GenericLogHeader;

template <
  class LOG_TYPE_TRAITS_,
  class SaveStrategy,
  bool no_db_mode = false
>
class GenericLogSaverImpl;

template <
  class LOG_TYPE_TRAITS_,
  class SaveStrategy,
  class DistributeStrategy,
  bool no_db_mode = false
>
class DistribLogSaverImpl;

template <class LOG_TYPE_TRAITS_>
struct DefaultSaveStrategy;

template <class LOG_TYPE_TRAITS_>
struct PackedSaveStrategy;

template <class LOG_TYPE_TRAITS_>
struct DefaultDistributeStrategy;

template <class LOG_TYPE_TRAITS_>
struct PackedDistributeStrategy;

namespace Detail
{
  template <
    typename CollectorType,
    bool is_nested
  >
  struct LoadPolicySelector;

  template <
    typename LogTraits,
    bool is_nested = LogTraits::is_nested
  >
  struct BundleLoadPolicySelector;

  template <typename CollectorType, typename Action>
  struct CollectorLoad;
  template <typename LogTraits, typename Action>
  struct BundleLoad;

  struct SimpleLoad;
  struct NestedPackedLoad;
  struct SimpleSafeLoad;
  struct NestedPackedSafeLoad;

  template <typename CollectorType>
  struct LoadPolicySelector<CollectorType, false>
  {
    typedef CollectorLoad<CollectorType, SimpleLoad> LoadType;
  };

  template <typename CollectorType>
  struct LoadPolicySelector<CollectorType, true>
  {
    typedef CollectorLoad<CollectorType, NestedPackedLoad> LoadType;
  };

  template <typename LogTraits>
  struct BundleLoadPolicySelector<LogTraits, false>
  {
    typedef BundleLoad<LogTraits, SimpleSafeLoad> LoadType;
  };

  template <typename LogTraits>
  struct BundleLoadPolicySelector<LogTraits, true>
  {
    typedef BundleLoad<LogTraits, NestedPackedSafeLoad> LoadType;
  };

  template <
    class LOG_TRAITS_,
    bool no_db_mode = false,
    bool is_nested = LOG_TRAITS_::is_nested
  >
  struct SaverImplSelector;

  template <class L_T_>
  struct SaverImplSelector<L_T_, false, false>
  {
    typedef GenericLogSaverImpl<L_T_, DefaultSaveStrategy<L_T_>, false> Type;
  };

  template <class L_T_>
  struct SaverImplSelector<L_T_, false, true>
  {
    typedef GenericLogSaverImpl<L_T_, PackedSaveStrategy<L_T_>, false> Type;
  };

  template <class L_T_>
  struct SaverImplSelector<L_T_, true, false>
  {
    typedef GenericLogSaverImpl<L_T_, DefaultSaveStrategy<L_T_>, true> Type;
  };

  template <class L_T_>
  struct SaverImplSelector<L_T_, true, true>
  {
    typedef GenericLogSaverImpl<L_T_, PackedSaveStrategy<L_T_>, true> Type;
  };

  template <
    class LOG_TRAITS_,
    bool no_db_mode = false,
    bool is_nested = LOG_TRAITS_::is_nested
  >
  struct DistribSaverImplSelector;

  template <class L_T_>
  struct DistribSaverImplSelector<L_T_, false, false>
  {
    typedef DistribLogSaverImpl<
      L_T_,
      DefaultSaveStrategy<L_T_>,
      DefaultDistributeStrategy<L_T_>,
      false> Type;
  };

  template <class L_T_>
  struct DistribSaverImplSelector<L_T_, false, true>
  {
    typedef DistribLogSaverImpl<
      L_T_,
      PackedSaveStrategy<L_T_>,
      PackedDistributeStrategy<L_T_>,
      false> Type;
  };

  template <class L_T_>
  struct DistribSaverImplSelector<L_T_, true, false>
  {
    typedef DistribLogSaverImpl<
      L_T_,
      DefaultSaveStrategy<L_T_>,
      DefaultDistributeStrategy<L_T_>,
      true> Type;
  };

  template <class L_T_>
  struct DistribSaverImplSelector<L_T_, true, true>
  {
    typedef DistribLogSaverImpl<
      L_T_,
      PackedSaveStrategy<L_T_>,
      PackedDistributeStrategy<L_T_>,
      true> Type;
  };
}

template <
  typename LogTraits,
  typename LoadPolicy = typename
    Detail::LoadPolicySelector<typename LogTraits::CollectorType, LogTraits::is_nested>::LoadType>
class GenericLogLoaderImpl;

template <typename LogTraits>
class GenericLogIoHelperImpl;

template <typename CollectorType>
class CollectorFilter;

template <
  typename Collector,
  const bool IS_NESTED = true,
  const bool IS_SUMMABLE = true,
  const std::size_t TYPE_TAG = 0
>
struct LogDefaultPrimaryTraits
{
  static const bool is_nested = IS_NESTED;

  static const bool is_summable = IS_SUMMABLE;

  typedef Collector CollectorType;

  typedef LogDefaultPrimaryTraits<Collector, IS_NESTED, IS_SUMMABLE, TYPE_TAG>
    BaseTraits;

  typedef CollectorBundle<BaseTraits> CollectorBundleType;

  typedef ReferenceCounting::SmartPtr<CollectorBundleType>
    CollectorBundlePtrType;
};

template <
  typename Collector,
  const bool IS_NESTED = true,
  const bool IS_SUMMABLE = true,
  const std::size_t TYPE_TAG = 0
>
struct LogDefaultTraitsBase:
  public LogDefaultPrimaryTraits<Collector, IS_NESTED, IS_SUMMABLE, TYPE_TAG>
{
private:
  typedef LogDefaultPrimaryTraits<Collector, IS_NESTED, IS_SUMMABLE, TYPE_TAG>
    PrimaryTraits_;

public:
  typedef typename PrimaryTraits_::CollectorType CollectorType;

  typedef CollectorFilter<CollectorType> CollectorFilterType;

  typedef GenericLogHeader<LogDefaultTraitsBase> HeaderType;

  template <typename Functor>
  static void
  for_each_old(Functor&) noexcept
  {}

  static const char*
  log_base_name() noexcept
  {
    return base_name_;
  }

  static const char*
  signature() noexcept
  {
    return signature_;
  }

  static const char*
  current_version() noexcept
  {
    return current_version_;
  }

  static
  const std::string&
  snmp_friendly_name() /*throw(eh::Exception)*/
  {
    static const SnmpBaseName NAME(log_base_name());
    return NAME;
  }

private:
  class SnmpBaseName : public std::string
  {
  public:
    SnmpBaseName(const char* log_base_name) /*throw(eh::Exception)*/
      : std::string(log_base_name)
    {
      std::string::iterator it = begin();
      if (std::isupper(*it))
      {
        std::string::iterator prev = it++;
        do
        {
          *prev = String::AsciiStringManip::to_lower(*prev);
          ++prev;
        }
        while (++it != end() && std::isupper(*it));
      }
    }
  };

  static const char* base_name_;
  static const char* signature_;
  static const char* current_version_;
};

template <
  typename Collector,
  const bool IS_NESTED = true,
  const bool IS_SUMMABLE = true,
  const std::size_t TYPE_TAG = 0
>
struct LogDefaultTraits:
  public LogDefaultTraitsBase<Collector, IS_NESTED, IS_SUMMABLE, TYPE_TAG>
{
  typedef LogDefaultTraitsBase<Collector, IS_NESTED, IS_SUMMABLE, TYPE_TAG>
    BaseTraits;

private:
  struct TypedefHelper_: public BaseTraits
  {
    typedef GenericLogLoaderImpl<BaseTraits,
      typename Detail::LoadPolicySelector<typename BaseTraits::CollectorType, BaseTraits::is_nested>::LoadType>
        LoaderType;

    typedef typename Detail::SaverImplSelector<BaseTraits>::Type SaverType;
  };

public:
  typedef typename TypedefHelper_::LoaderType LoaderType;
  typedef typename TypedefHelper_::SaverType SaverType;

  typedef GenericLogIoHelperImpl<TypedefHelper_> IoHelperType;

  typedef BaseTraits B; // Short name - useful when defining base_name_, etc
};

std::string
make_log_file_name(
  const LogFileNameInfo &info,
  const std::string &out_dir_name
);

std::string
restore_log_file_name(
  const LogFileNameInfo& info,
  const std::string& out_dir_name)
  /*throw(InvalidArgValue, eh::Exception)*/;

/*
 * make_log_file_name_pair() returns a pair of strings,
 *   first elem  - ordinary file name,
 *   second elem - its temporary counterpart
 */
StringPair
make_log_file_name_pair(
  const LogFileNameInfo &info,
  const std::string &out_dir_name
);

void
parse_log_file_name(const std::string& log_file_name, LogFileNameInfo& info)
  /*throw(InvalidLogFileNameFormat)*/;

void
search_for_files(
  const std::string &dir_name,
  const std::string &log_type,
  LogSortingMap &log_map,
  std::size_t log_map_inc_size_limit = -1,
  Logging::Logger *logger = 0
);

template <class OStream, class SEQ_>
OStream&
output_sequence(OStream &os, const SEQ_ &seq, const char *sep = ",");

void
parse_string_list(
  const String::SubString& str,
  StringList& list,
  char sep = ',',
  const char* empty = "-"
);

std::string&
trim(std::string& str, std::size_t max_length);

std::string&
trim(std::string& str, const std::string& src, std::size_t max_length)
  /*throw(eh::Exception)*/;

bool
is_valid_user_status(char ch) noexcept;

class FileStore
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  FileStore(
    const std::string &root_dir = ".",
    const std::string &sub_dir = ".",
    bool use_session_name = true
  )
    /*throw(Exception, eh::Exception)*/;

  void
  store(const std::string &file_path) /*throw(Exception, eh::Exception)*/
  {
    store_(file_path, use_session_name_ ? new_session_name_() : "");
  }

  template <class FILE_SEQ_>
  void
  store(const FILE_SEQ_ &files) /*throw(Exception, eh::Exception)*/
  {
    const std::string& session_name =
      use_session_name_ ? new_session_name_() : "";
    for (typename FILE_SEQ_::const_iterator it = files.begin();
      it != files.end(); ++it)
    {
      store_((*it)->full_path(), session_name);
    }
  }

  template <class FILE_SEQ_>
  void
  store(
    const FILE_SEQ_ &files,
    const Generics::Time &timestamp)
    /*throw(Exception, eh::Exception)*/
  {
    for (typename FILE_SEQ_::const_iterator it = files.begin();
      it != files.end(); ++it)
    {
      store_((*it)->full_path(), timestamp);
    }
  }

  void
  store(
    const std::string &file_path,
    std::size_t processed_lines_count)
    /*throw(Exception, eh::Exception)*/;

private:
  static void
  rename_(const char* src, const char* dst) /*throw(Exception)*/;

  static void
  make_dir_(const std::string &dir_name)
    /*throw(Exception, eh::Exception)*/;

  static void
  make_dir_layout_(
    const std::string &store_dir,
    const std::string &store_sub_dir
  )
    /*throw(Exception, eh::Exception)*/;

  static std::string
  new_session_name_();

  void
  store_(
    const std::string &file_path,
    const std::string &session_name = "",
    const std::string &new_file_name = ""
  )
    /*throw(Exception, eh::Exception)*/;

  void
  store_(
    const std::string &file_path,
    const Generics::Time &timestamp
  )
    /*throw(Exception, eh::Exception)*/;

  std::string
  extract_file_name_(const std::string& file_path) const
    /*throw(Exception, eh::Exception)*/;

private:
  std::string dir_name_;
  const bool use_session_name_;
};

class LogLoader: public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  virtual
  void
  load(std::istream& is, const CollectorBundleFileGuard_var& file_handle =
    CollectorBundleFileGuard_var())
    /*throw(Exception)*/ = 0;

protected:
  virtual ~LogLoader() noexcept {}
};

typedef ReferenceCounting::SmartPtr<LogLoader> LogLoader_var;

class LogSaver: public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(OperationDeferred, Exception);

protected:
  virtual
  ~LogSaver() noexcept {}
};

typedef ReferenceCounting::SmartPtr<LogSaver> LogSaver_var;

class SimpleLogSaver: public LogSaver
{
public:
  virtual void save() = 0;

protected:
  virtual
  ~SimpleLogSaver() noexcept {}
};

typedef ReferenceCounting::SmartPtr<SimpleLogSaver> SimpleLogSaver_var;

template <class LOG_TYPE_TRAITS_> class LogIoProxy
{
  typedef typename LOG_TYPE_TRAITS_::CollectorType CollectorT;

  typedef typename LOG_TYPE_TRAITS_::SaverType SaverT;
  typedef ReferenceCounting::SmartPtr<SaverT> SaverPtrT;

  typedef typename LOG_TYPE_TRAITS_::LoaderType LoaderT;
  typedef ReferenceCounting::SmartPtr<LoaderT> LoaderPtrT;

public:
  static void save(
    CollectorT& collector,
    const std::string& path,
    const std::optional<ArchiveParams>& archive_params)
    /*throw(eh::Exception)*/
  {
    SaverPtrT(new SaverT(collector, path, archive_params))->save();
  }

  static void
  save(
    CollectorT& collector,
    const std::string& path,
    const std::optional<ArchiveParams>& archive_params,
    unsigned long distrib_count
  )
    /*throw(eh::Exception)*/
  {
    typedef typename Detail::DistribSaverImplSelector<LOG_TYPE_TRAITS_, false>::Type SaverT;
    typedef ReferenceCounting::SmartPtr<SaverT> SaverPtrT;

    SaverPtrT(new SaverT(collector, path, archive_params, distrib_count))->save();
  }

  static void load(CollectorT &collector, std::istream &is)
    /*throw(eh::Exception)*/
  {
    LoaderPtrT(new LoaderT(collector))->load(is);
  }
};

class LogIoHelper
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  virtual void
  load(std::istream& is) = 0;

  virtual void
  save(
    const std::string& path,
    const std::optional<ArchiveParams>& archive_params) = 0;

  virtual
  ~LogIoHelper() noexcept
  {}
};

template <class T_, class OPTIONAL_VALUE_TRAITS_>
inline bool
operator<(
  const OptionalValue<T_, OPTIONAL_VALUE_TRAITS_> &ov1,
  const OptionalValue<T_, OPTIONAL_VALUE_TRAITS_> &ov2
)
{
  return ov2.present() ?
    (ov1.present() ? ov1.get() < ov2.get() : true) : false;
}

class HitsFilter: public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  HitsFilter(
    unsigned char min_count,
    const char* storage_file_prefix,
    unsigned long table_size
  )
    /*throw(Exception, eh::Exception)*/;

  // Not thread-safe, needs external sync using Locker
  unsigned long
  check(
    const DayTimestamp& date,
    unsigned long hash,
    unsigned long add_count
  )
    /*throw(Exception, eh::Exception)*/;

  // Thread-safe
  void
  clear_expired(const DayTimestamp& exp_date)
    /*throw(Exception, eh::Exception)*/;

protected:
  virtual
  ~HitsFilter() noexcept;

  typedef Sync::Policy::PosixThread SyncPolicy_;
  typedef SyncPolicy_::Mutex Lock_;
  typedef SyncPolicy_::WriteGuard Guard_;

public:
  class Locker: public ReferenceCounting::AtomicImpl
  {
  public:
    Locker(Lock_& lock): lock_(lock) { lock_.lock(); }

  private:
    virtual ~Locker() noexcept { lock_.unlock(); }

    Locker(const Locker&);
    Locker& operator=(const Locker&);

    Lock_& lock_;
  };

  typedef ReferenceCounting::SmartPtr<Locker> Locker_var;

  Locker_var create_locker() const { return new Locker(lock_); }

private:
  void
  load_() /*throw(Exception, eh::Exception)*/;

  void
  save_() /*throw(Exception, eh::Exception)*/;

  void
  make_file_name_(const DayTimestamp& date, std::string& file_name)
    /*throw(Exception, eh::Exception)*/;

  typedef std::vector<unsigned char> ByteVector_;
  typedef std::map<DayTimestamp, ByteVector_> CounterMap_;

  static const char CURRENT_FILE_VERSION_[];
  static const char EOF_MARKER_[];

  static const std::size_t FILE_VERSION_LEN_;
  static const std::size_t TABLE_SIZE_LEN_;

  const unsigned char MIN_COUNT_;
  const unsigned long TABLE_SIZE_;

  std::string storage_dir_;
  std::string file_prefix_;

  CounterMap_ counters_;

  mutable Lock_ lock_;
};

typedef ReferenceCounting::SmartPtr<HitsFilter> HitsFilter_var;

template <typename CollectorType>
struct CollectorFilter : public ReferenceCounting::AtomicImpl
{
  typedef ReferenceCounting::SmartPtr<CollectorFilter<CollectorType> > SmartPtr;

  virtual
  void
  filter(CollectorType&)
  {}
protected:
  virtual
  ~CollectorFilter() noexcept
  {}
};

template <typename CollectorType>
class CollectorHitsFilter : public CollectorFilter<CollectorType>
{
public:
  typedef ReferenceCounting::SmartPtr<CollectorHitsFilter<CollectorType> >
    SmartPtr;

  explicit
  CollectorHitsFilter(HitsFilter_var& hits_filter) :
    hits_filter_(hits_filter)
  {}

  virtual void
  filter(CollectorType& collector)
  {
    for (typename CollectorType::iterator it = collector.begin();
      it != collector.end(); )
    {
      {
        HitsFilter::Locker_var locker = hits_filter_->create_locker();
        for (typename CollectorType::DataT::iterator data_it = it->second.begin();
          data_it != it->second.end(); )
        {
          unsigned long hits = hits_filter_->check(it->first.sdate(),
            data_it->first.distrib_hash(), data_it->second.hits());
          if (hits)
          {
            (data_it++)->second.set_hits(hits);
          }
          else
          {
            it->second.erase(data_it++);
          }
        }
      }
      if (it->second.empty())
      {
        collector.erase(it++);
      }
      else
      {
        ++it;
      }
    }
  }

private:
  virtual
  ~CollectorHitsFilter() noexcept
  {}

  HitsFilter_var hits_filter_;
};

inline
void
HitsFilter::make_file_name_(const DayTimestamp& date, std::string& file_name)
  /*throw(Exception, eh::Exception)*/
{
  file_name = storage_dir_;
  file_name += '/';
  file_name += file_prefix_;
  file_name += '.';
  file_name += date.time().gm_f();
}

class SafeSequenceGenerator
{
public:
  SafeSequenceGenerator(unsigned int min_value, unsigned int max_value);

  unsigned int
  get();

  virtual ~SafeSequenceGenerator() noexcept = default;

private:
  const unsigned int min_value_;
  const unsigned int diff_;
  Algs::AtomicInt value_;
};

} // namespace LogProcessing
} // namespace AdServer

#include <LogCommons/LogCommons.ipp>

#endif // AD_SERVER_LOG_PROCESSING_LOG_COMMONS_HPP
