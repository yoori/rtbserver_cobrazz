/// LogCommons.ipp

#ifndef AD_SERVER_LOG_PROCESSING_LOG_COMMONS_IPP
#define AD_SERVER_LOG_PROCESSING_LOG_COMMONS_IPP

#include <LogCommons/LogCommons.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer {
namespace LogProcessing {

namespace Aux_ {

  extern const String::AsciiStringManip::CharCategory VALID_USER_STATUSES;

inline bool
check_null(std::istream& is, char null_marker) /*throw(eh::Exception)*/
{
  if (is.peek() == null_marker)
  {
    is.get();
    // look for is_space behind null marker
    typedef std::ctype<char> CType;
    const CType& ctype = std::use_facet<CType>(is.getloc());
    if (ctype.is(std::ctype_base::space, is.peek()) || is.eof())
    {
      return true;
    }
    else
    {
      is.unget();
    }
  }
  return false;
}

struct DefaultSequenceAdapter
{
  template <class T>
  const T
  operator()(const T& value) const noexcept
  {
    return value;
  }
};

struct FixedNumberSequenceAdapter
{
  const std::string
  operator()(const FixedNumber& value) const
  {
    return value.str();
  }
};

struct CollectorBundleFileGuardSequenceAdapter
{
  const std::string
  operator()(const CollectorBundleFileGuard_var& value) const
  {
    return value->full_path();
  }
};

template <class T>
struct SequenceAdapterSelector
{
  typedef DefaultSequenceAdapter Type;
};

template <>
struct SequenceAdapterSelector<FixedNumber>
{
  typedef FixedNumberSequenceAdapter Type;
};

template <>
struct SequenceAdapterSelector<CollectorBundleFileGuard_var>
{
  typedef CollectorBundleFileGuardSequenceAdapter Type;
};

template <class OStream, class SEQ_, class SequenceAdapter>
OStream&
output_sequence(OStream &os, const SEQ_ &seq, const char *sep, SequenceAdapter sequence_adapter)
{
  typename SEQ_::const_iterator cit(seq.begin());
  if (cit != seq.end())
  {
    os << *cit;
    for (++cit; cit != seq.end(); ++cit)
    {
      os << sep << sequence_adapter(*cit);
    }
  }
  return os;
}

} //namespace Aux_

template <typename Convertor, const char SEPARATOR>
inline std::istream&
operator >>(std::istream& is, StringIO<Convertor, SEPARATOR>& str)
  /*throw(eh::Exception)*/
{
  typedef StringIO<Convertor, SEPARATOR> String;
  typedef typename String::size_type SizeType;
  typedef typename std::istream::traits_type TraitsType;
  typedef typename std::istream::int_type IntType;
  std::istream::sentry ok(is, true);
  if (ok)
  {
    std::ios_base::iostate iostate(std::ios_base::goodbit);
    char buf[128];
    SizeType len = 0;
    SizeType extracted = 0;
    IntType ch = 0;
    try
    {
      typedef std::ctype<char> CType;
      const CType& ctype = std::use_facet<CType>(is.getloc());

      str.erase();
      const std::streamsize WIDTH = is.width();
      const SizeType N = WIDTH > 0 ?
        static_cast<SizeType>(WIDTH) : str.max_size();

      const IntType eof = TraitsType::eof();
      std::basic_streambuf<char>* sb = is.rdbuf();
      ch = sb->sgetc();

      while (extracted < N && !TraitsType::eq_int_type(ch, eof) &&
        !ctype.is(std::ctype_base::space, TraitsType::to_char_type(ch)) &&
        TraitsType::to_char_type(ch) != SEPARATOR )
      {
        if (len == sizeof(buf))
        {
          str.append(buf, sizeof(buf));
          len = 0;
        }
        if (TraitsType::eq_int_type(ch,
          TraitsType::to_int_type(Aux_::EscapeChar::ESC_CHAR)))
        {
          unsigned result = 0;
          bool stop;
          for (std::size_t i = 0; i < 2; ++i)
          {
            ch = sb->snextc();
            stop = TraitsType::eq_int_type(ch, eof) ||
              ctype.is(std::ctype_base::space, TraitsType::to_char_type(ch)) ||
              TraitsType::to_char_type(ch) == SEPARATOR;
            if (stop)
            {
              iostate |= std::ios_base::failbit;
              break;
            }
            unsigned digit = ch - '0';
            if (digit > 9)
            {
              digit -= 7;
              if (digit < 10 || digit > 15)
              {
                result *= 16;
                iostate |= std::ios_base::failbit;
                break;
              }
            }
            result = result * 16 + digit;
          }
          if (stop)
          {
            break;
          }
          ch = result;
        }
        buf[len++] = TraitsType::to_char_type(ch);
        ++extracted;
        ch = sb->snextc();
      }
      str.append(buf, len);
      if (TraitsType::eq_int_type(ch, eof))
      {
        iostate |= std::ios_base::eofbit;
      }
      is.width(0);
    }
    catch (...)
    {
      is.setstate(std::ios_base::badbit);
      if (is.exceptions() & std::ios_base::badbit)
      {
        throw;
      }
    }
    if (!extracted && TraitsType::to_char_type(ch) != SEPARATOR)
    {
      iostate |= std::ios_base::failbit;
    }
    if (iostate)
    {
      is.setstate(iostate);
    }
  }
  return is;
}

template <typename Convertor, const char SEPARATOR>
inline std::ostream&
operator <<(std::ostream& os, const StringIO<Convertor, SEPARATOR>& str)
  /*throw(eh::Exception)*/
{
  using namespace std;
  ostream::sentry ok(os);
  if (ok)
  {
    // rem: need handle width() and do padding to be like std::string
    const char* ptr = str.data();
    const char* const END = ptr + str.size();
    try
    {
      for (; ptr != END; ++ptr)
      {
        if (static_cast<unsigned char>(*ptr) < 128)
        {
          const char* reserved = Convertor::WRITE_RESERVED_CHAR_TABLE[
            static_cast<unsigned char>(*ptr)];
          if (*reserved)
          {
            if (ostream::traits_type::eq_int_type(
              os.rdbuf()->sputc(Aux_::EscapeChar::ESC_CHAR),
              ostream::traits_type::eof()))
            {
              return os.setstate(ios_base::badbit), os;
            }
            if (os.rdbuf()->sputn(reserved, 2) != 2)
            {
              return os.setstate(ios_base::badbit), os;
            }
            continue;
          }
        }
        if (ostream::traits_type::eq_int_type(os.rdbuf()->sputc(*ptr),
          ostream::traits_type::eof()))
        {
          return os.setstate(ios_base::badbit), os;
        }
      }
    }
    catch (...)
    {
      os.setstate(ios_base::badbit);
      if (os.exceptions() & ios_base::badbit)
      {
        throw;
      }
    }
  }
  return os;
}

template <class T_>
inline
std::istream&
operator>>(std::istream& is, UuidIoWrapper<T_>& value)
{
  if (Aux_::check_null(is, UuidIoWrapper<T_>::NULL_MARKER))
  {
    T_().swap(value);
  }
  else
  {
    is >> static_cast<T_&>(value);
  }
  return is;
}

template <class T_>
inline
std::ostream&
operator<<(std::ostream& os, const UuidIoWrapper<T_>& value)
{
  if (value.is_null())
  {
    os << UuidIoWrapper<T_>::NULL_MARKER;
  }
  else
  {
    os << static_cast<const T_&>(value);
  }
  return os;
}

template <class PRECISION_TRAITS_>
inline std::ostream&
operator<<(std::ostream &os, const TimeFacet<PRECISION_TRAITS_> &time_facet)
  /*throw(eh::Exception)*/
{
  return os <<
    time_facet.time().get_gm_time().format(PRECISION_TRAITS_::format());
}

template <class PRECISION_TRAITS_>
inline std::istream&
operator>>(std::istream &is, TimeFacet<PRECISION_TRAITS_> &time_facet)
  /*throw(eh::Exception)*/
{
  std::string str;
  if (is >> str)
  {
    Generics::Time time;
    try
    {
      time.set(str, PRECISION_TRAITS_::format());
      time_facet = time;
    }
    catch (...)
    {
      is.setstate(std::ios::failbit);
    }
  }
  return is;
}


template <size_t PRECISION_>
inline std::istream&
operator >>(std::istream& is, FixedPrecisionDouble<PRECISION_>& value)
  /*throw(eh::Exception)*/
{
  is >> value.value_;
  value.value_ = value.round_(value.value_);
  return is;
}

template <size_t PRECISION_>
inline std::ostream&
operator <<(std::ostream& os, const FixedPrecisionDouble<PRECISION_>& value)
  /*throw(eh::Exception)*/
{
  const size_t LEN = 64;
  char buf[LEN];
  int wrote =
    std::snprintf(buf, LEN,
      FixedPrecisionDouble<PRECISION_>::PRINT_.format, value.value_);
  if (wrote < 0)
  {
    os.setstate(std::ios::failbit);
    return os;
  }
  return os.write(buf, wrote);
}

inline
std::ostream&
operator<<(std::ostream& os, const CampaignSvcs::AuctionType& auction_type)
  /*throw(eh::Exception)*/
{
  os << CampaignSvcs::put_auction_type(auction_type);
  return os;
}

template <class CATEGORY_>
FixedBufStream<CATEGORY_>&
operator>>(FixedBufStream<CATEGORY_> &is, char &ch)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    ch = token[0];
  }
  return is;
}

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, Generics::Uuid& out)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    out = Generics::Uuid(token);
  }
  return is;
}

template <typename Category, class T_>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, UuidIoWrapper<T_>& value)
{
  const String::SubString token = is.read_token();

  if (is.good())
  {
    if (token.length() == 1 && token[0] == UuidIoWrapper<T_>::NULL_MARKER)
    {
      T_().swap(value);
    }
    else
    {
      try
      {
        static_cast<T_&>(value) = Generics::Uuid(token);
      }
      catch (const eh::Exception&)
      {
        is.setstate(std::ios_base::failbit);
      }
    }
  }
  return is;
}

template <typename Category>
FixedBufStream<Category>&
operator>>(FixedBufStream<Category>& is, std::string& out)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    token.assign_to(out);
  }
  return is;
}

template <typename Category>
class FixedBufStream
{
public:

  FixedBufStream(const String::SubString& str) /*throw(eh::Exception)*/
    :
      splitter_(str),
      iostate_(std::ios_base::goodbit)
  {}

  String::SubString
  read_token()
  {
    if (last_token_.present())
    {
      return pop_token();
    }
    String::SubString result;
    splitter_.get_token(result);
    if (result.empty())
    {
      iostate_ |= std::ios_base::eofbit;
    }
    return result;
  }

  bool
  good() const noexcept
  {
    return iostate_ == 0;
  }

  bool
  bad() const noexcept
  {
    return (iostate_ & std::ios_base::badbit) != 0;
  }

  bool
  eof() const noexcept
  {
    return (iostate_ & std::ios_base::eofbit) != 0;
  }

  bool
  fail() const noexcept
  {
    return (iostate_ & (std::ios_base::failbit | std::ios_base::badbit)) != 0;
  }

  operator void*() const { return reinterpret_cast<void*>(!fail()); }

  bool operator!() const { return fail(); }

  std::ios_base::iostate
  rdstate() const noexcept
  {
    return iostate_;
  }

  void
  setstate(const std::ios_base::iostate& state) noexcept
  {
    iostate_ |= state;
  }

  template <typename SomeCategory>
  void
  take_fails(const FixedBufStream<SomeCategory>& stream) noexcept
  {
    iostate_ |=
      stream.rdstate() & (std::ios_base::badbit | std::ios_base::failbit);
  }

  template <typename Char, typename Traits>
  void
  transfer_state(std::basic_ios<Char, Traits>& stream) const noexcept
  {
    std::ios_base::iostate state(iostate_);
    state &= ~std::ios_base::eofbit;
    stream.setstate(state);
  }

  /**
   * Save information for type managed by OptionalValue
   * The type knowns how it should be parsed:
   * inplace - through this stream, or
   * on copy - must be loaded from copy stream
   */
  void
  push_back(const String::SubString& str)
  {
    last_token_ = str;
  }

  String::SubString
  pop_token()
  {
    String::SubString result = last_token_.get();
    last_token_ = OptionalValue<String::SubString>();
    return result;
  }

protected:
  String::StringManip::Splitter<Category> splitter_;
  /// goodbit, badbit, eofbit, failbit reading indicator
  std::ios_base::iostate iostate_;
  OptionalValue<String::SubString> last_token_;
};

template <typename InvariantsChecker = Aux_::OwnInvariants, typename Category = TabCategory>
class TokenizerInputArchive
{
public:
  TokenizerInputArchive(FixedBufStream<Category>& stream) noexcept
    : fixed_buf_stream_(stream)
  {}

  template <typename Field>
  TokenizerInputArchive&
  operator &(Field& field)
  {
    fixed_buf_stream_ >> field;
    return *this;
  }

  template <typename Field>
  TokenizerInputArchive&
  operator ^(Field& field)
  {
    return operator &(field);
  }

  template <typename Data>
  TokenizerInputArchive&
  operator >>(Data& data)
    /*throw(ConstraintViolation, eh::Exception)*/
  {
    data.serialize(*this);
    InvariantsChecker::invariant(data);
    return *this;
  }
private:
  FixedBufStream<Category>& fixed_buf_stream_;
};

template <typename InvariantsChecker,/* = Aux_::OwnInvariants,*/
  const char SEPARATOR/* = '\t'*/>
class InputArchive
{
public:
  InputArchive(std::istream& istr) noexcept
    : istr_(istr)
  {}

  template <typename Field>
  InputArchive&
  operator &(Field& field)
  {
    istr_ >> field;
    read_char(istr_, SEPARATOR);
    return *this;
  }

  template <typename Field>
  InputArchive&
  operator ^(Field& field)
  {
    istr_ >> field;
    return *this;
  }

  template <typename Data>
  InputArchive&
  operator >>(Data& data)
    /*throw(ConstraintViolation, eh::Exception)*/
  {
    data.serialize(*this);
    if (istr_)
    {
      InvariantsChecker::invariant(data);
    }
    return *this;
  }
private:
  std::istream& istr_;
};

template <typename InvariantsChecker,/* = Aux_::OwnInvariants*/
  const char SEPARATOR/* = '\t'*/>
class OutputArchive
{
public:
  OutputArchive(std::ostream& ostr) noexcept
    : ostr_(ostr)
  {}

  template <typename Field>
  OutputArchive&
  operator &(const Field& field)
  {
    ostr_ << field << SEPARATOR;
    return *this;
  }

  template <typename Field>
  OutputArchive&
  operator ^(const Field& field)
  {
    ostr_ << field;
    return *this;
  }

  template <typename Data>
  OutputArchive&
  operator <<(const Data& data)
    /*throw(ConstraintViolation, eh::Exception)*/
  {
    InvariantsChecker::invariant(data);
    const_cast<Data&>(data).serialize(*this);
    return *this;
  }
private:
  std::ostream& ostr_;
};

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, bool& value)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    if (token == "1")
    {
      value = true;
    }
    else if (token == "0")
    {
      value = false;
    }
    else
    {
      is.setstate(std::ios_base::failbit);
    }
  }
  return is;
}

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, FixedNumber& value)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    try
    {
      value = FixedNumber(token);
    }
    catch (const eh::Exception&)
    {
      is.setstate(std::ios_base::failbit);
    }
  }
  return is;
}

template <
  typename Category,
  typename DecBase,
  const unsigned DecTotal,
  const unsigned DecFraction
>
inline
FixedBufStream<Category>&
operator>>(
  FixedBufStream<Category>& is,
  Generics::SimpleDecimal<DecBase, DecTotal, DecFraction>& value
)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    try
    {
      value = Generics::SimpleDecimal<DecBase, DecTotal, DecFraction>(token);
    }
    catch (const eh::Exception&)
    {
      is.setstate(std::ios_base::failbit);
    }
  }
  return is;
}

template <class CAT_, size_t PRECISION_>
inline
FixedBufStream<CAT_>&
operator>>(FixedBufStream<CAT_>& is, FixedPrecisionDouble<PRECISION_>& value)
  /*throw(eh::Exception)*/
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    try
    {
      char cstr[token.size() + 1];
      std::copy(token.begin(), token.end(), cstr);
      cstr[token.size()] = 0;
      errno = 0;
      value = std::strtod(cstr, nullptr);
      if (errno)
      {
        is.setstate(std::ios_base::failbit);
      }
      else
      {
        value.value_ = value.round_(value.value_);
      }
    }
    catch (const eh::Exception&)
    {
      is.setstate(std::ios_base::failbit);
    }
  }
  return is;
}

template <class CAT_, class OBJECT_>
inline
FixedBufStream<CAT_>&
operator>>(FixedBufStream<CAT_>& is, EmptyHolder<OBJECT_>& object)
  /*throw(eh::Exception)*/
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    if (*token.data() == EmptyHolder<OBJECT_>::NULL_MARKER)
    {
      object.get().clear();
    }
    else
    {
      is.push_back(token);
      is >> object.get();
    }
  }
  return is;
}


template <typename Category, typename TimeTraits>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, TimeFacet<TimeTraits>& value)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    try
    {
      value = Generics::Time(token, TimeTraits::format());
    }
    catch (const eh::Exception&)
    {
      is.setstate(std::ios_base::failbit);
    }
  }
  return is;
}

template <typename Category, typename Convertor, const char SEPARATOR>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, StringIO<Convertor, SEPARATOR>& value)
  /*throw(eh::Exception)*/
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    if (!token.size())
    {
      is.setstate(std::ios_base::eofbit);
      return is;
    }
    std::string str;
    str.reserve(token.size());
    for (const char* ptr = token.data(); ptr != token.end(); ++ptr)
    {
      char ch = *ptr;
      if (ch == Aux_::EscapeChar::ESC_CHAR)
      {
        unsigned result = 0;
        for (std::size_t i = 0; i < 2; ++i)
        {
          ch = *++ptr;
          if (ptr == token.end())
          {
            is.setstate(std::ios_base::failbit);
            return is;
          }
          unsigned digit = ch - '0';
          if (digit > 9)
          {
            digit -= 7;
            if (digit < 10 || digit > 15)
            {
              is.setstate(std::ios_base::failbit);
              return is;
            }
          }
          result = result * 16 + digit;
        }
        ch = result;
      }
      str.push_back(ch);
    }
    str.swap(value);
  }
  return is;
}

template <typename Category, typename T, typename OptionalValueTraits>
FixedBufStream<Category>&
operator >>(
  FixedBufStream<Category>& is,
  OptionalValue<T, OptionalValueTraits>& ov)
  /*throw(eh::Exception)*/
{
  if (is.good())
  {
    const String::SubString token = is.read_token();
    if (token.empty())
    {
      is.setstate(std::ios_base::badbit);
      return is;
    }
    if (token[0] == OptionalValueTraits::NULL_MARKER)
    {
      if (token.length() > 1)
      {
        is.setstate(std::ios_base::badbit);
      }
      else
      {
        ov.clear();
        ov.present_(false);
      }
      return is;
    }
    if (token[0] == OptionalValueTraits::PRESENT_MARKER)
    {
      if (token.length() == 1)
      {
        is.setstate(std::ios_base::badbit);
        return is;
      }
      is.push_back(token.substr(1)); // skip PRESENT_MARKER
    }
    else
    {
      is.push_back(token);
    }
    if (is >> ov.get())
    {
      ov.present_(!OptionalValueTraits::is_empty(ov.get()));
    }
  }
  return is;
}

template <typename Category, typename ValueType>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, std::list<ValueType>& values)
  /*throw(eh::Exception)*/
{
  const String::SubString token = is.read_token();

  if (is.good())
  {
    if (token == "-")
    {
      values.clear();
      return is;
    }
    std::list<ValueType> container;
    FixedBufStream<CommaCategory> list_stream(token);
    while (true)
    {
      ValueType elem = ValueType();
      list_stream >> elem;
      if (!list_stream.good())
      {
        break;
      }
      container.push_back(elem);
    }
    is.take_fails(list_stream);
    if (is.good())
    {
      values.swap(container);
    }
  }
  return is;
}

template <typename Category, typename ValueType>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, std::deque<ValueType>& values)
  /*throw(eh::Exception)*/
{
  const String::SubString token = is.read_token();

  if (is.good())
  {
    if (token == "-")
    {
      values.clear();
      return is;
    }
    std::deque<ValueType> container;
    FixedBufStream<CommaCategory> list_stream(token);
    while (true)
    {
      ValueType elem = ValueType();
      list_stream >> elem;
      if (!list_stream.good())
      {
        break;
      }
      container.push_back(elem);
    }
    is.take_fails(list_stream);
    if (is.good())
    {
      values.swap(container);
    }
  }
  return is;
}

template <typename Category, typename IntType>
inline
FixedBufStream<Category>&
read_integer(FixedBufStream<Category>& is, IntType& value)
{
  const String::SubString token = is.read_token();
  if (is.good())
  {
    if (!String::StringManip::str_to_int(token, value))
    {
      is.setstate(std::ios_base::failbit);
    }
  }
  return is;
}

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, short& value)
{
  return read_integer(is, value);
}

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, unsigned short& value)
{
  return read_integer(is, value);
}

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, int& value)
{
  return read_integer(is, value);
}

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, unsigned int& value)
{
  return read_integer(is, value);
}

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, long& value)
{
  return read_integer(is, value);
}

template <typename Category>
FixedBufStream<Category>&
operator >>(FixedBufStream<Category>& is, unsigned long& value)
{
  return read_integer(is, value);
}

inline
std::istream&
read_until_eol(std::istream &is, std::string &str, bool revert_eol)
{
  std::getline(is, str);
  if (!is.eof() && revert_eol)
  {
    is.putback('\n');
  }
  return is;
}

inline
std::istream& read_until_tab(std::istream &is, std::string &str)
{
  std::getline(is, str, '\t');
  if (!is.eof())
  {
    is.putback('\t');
  }
  return is;
}

inline
std::istream& read_char(std::istream &is, char ch)
{
  char read_ch;
  is.get(read_ch);
  if (!is.fail() && read_ch != ch)
  {
    is.putback(read_ch);
    is.setstate(std::ios::failbit);
  }
  return is;
}

inline
std::istream& read_eol(std::istream &is)
{
  return read_char(is, '\n');
}

inline
std::istream& read_tab(std::istream &is)
{
  return read_char(is, '\t');
}

template <typename Object>
inline std::istream&
operator >>(
  std::istream& is,
  EmptyHolder<Object>& object)
  /*throw(eh::Exception)*/
{
  if (is.good())
  {
    if (Aux_::check_null(is, EmptyHolder<Object>::NULL_MARKER))
    {
      object.get().clear();
    }
    else
    {
      is >> object.get();
    }
  }
  return is;
}

template <typename Object>
inline std::ostream&
operator <<(
  std::ostream& os,
  const EmptyHolder<Object>& object)
  /*throw(eh::Exception)*/
{
  if (object.get().empty())
  {
    return os << EmptyHolder<Object>::NULL_MARKER;
  }
  return os << object.get();
}

template <class OStream, class SEQ_>
OStream&
output_sequence(OStream &os, const SEQ_ &seq, const char *sep)
{
  return Aux_::output_sequence(os, seq, sep,
    typename Aux_::SequenceAdapterSelector<typename SEQ_::value_type>::Type());
}

template <class Container, typename EndValidator>
std::istream&
read_sequence(std::istream& is, Container& sequence, char separator,
  const EndValidator& check_end = Aux_::SpacesAtEnd())
  /*throw(eh::Exception)*/
{
  Container load_sequence;
  typename Container::iterator end = load_sequence.end();
  for (typename Container::value_type value; is >> value; is.get())
  {
    load_sequence.insert(end, value);
    if (is.eof())
    {
      break;
    }
    char ch = is.peek();
    if (ch == separator)
    {
      continue;
    }
    if (!check_end(ch))
    {
      is.setstate(std::ios_base::failbit);
    }
    break;
  }
  sequence.swap(load_sequence);
  return is;
}

template <typename ValueType>
inline std::istream&
operator >>(std::istream& is, std::list<ValueType>& values)
  /*throw(eh::Exception)*/
{
  if (is.good())
  {
    if (Aux_::check_null(is, '-'))
    {
      values.clear();
    }
    else
    {
      read_sequence(is, values, ',', Aux_::SpacesAtEnd());
    }
  }
  return is;
}

template <typename ValueType>
inline std::ostream&
operator <<(std::ostream& os, const std::list<ValueType>& values)
  /*throw(eh::Exception)*/
{
  return values.empty() ? os << '-' : output_sequence(os, values);
}

template <typename ValueType>
inline std::ostream&
operator <<(std::ostream& os, const std::deque<ValueType>& values)
  /*throw(eh::Exception)*/
{
  return values.empty() ? os << '-' : output_sequence(os, values);
}

template <class U_, class OPTIONAL_VALUE_TRAITS_>
inline
std::istream&
operator>>(
  std::istream &is,
  OptionalValue<U_, OPTIONAL_VALUE_TRAITS_> &ov
)
  /*throw(eh::Exception)*/
{
  if (is.peek() == OPTIONAL_VALUE_TRAITS_::NULL_MARKER)
  {
    is.get();
    ov.clear();
    ov.present_(false);
  }
  else if (OPTIONAL_VALUE_TRAITS_::read_value(is, ov.get()))
  {
    ov.present_(!OPTIONAL_VALUE_TRAITS_::is_empty(ov.get()));
  }
  return is;
}

template <class U_, class OPTIONAL_VALUE_TRAITS_>
inline
std::ostream&
operator<<(
  std::ostream &os,
  const OptionalValue<U_, OPTIONAL_VALUE_TRAITS_> &ov
)
  /*throw(eh::Exception)*/
{
  if (ov.present() && !OPTIONAL_VALUE_TRAITS_::is_empty(ov.get()))
  {
    return OPTIONAL_VALUE_TRAITS_::write_value(os, ov.get());
  }
  return os << OPTIONAL_VALUE_TRAITS_::NULL_MARKER;
}

inline
std::string&
trim(std::string& str, std::size_t max_length)
{
  if (str.length() > max_length)
  {
    str.resize(max_length);
    str[str.length() - 1] = '.';
    str[str.length() - 2] = '.';
    str[str.length() - 3] = '.';
  }
  return str;
}

inline
std::string&
trim(std::string& str, const std::string& src, std::size_t max_length)
  /*throw(eh::Exception)*/
{
  if (src.length() > max_length)
  {
    str.assign(src.data(), max_length - 3);
    str.append("...", 3);
  }
  else
  {
    str.assign(src);
  }
  return str;
}

inline
bool
is_valid_user_status(char ch) noexcept
{
  return Aux_::VALID_USER_STATUSES.is_owned(ch);
}

inline
unsigned long
user_id_distribution_hash(const AdServer::Commons::UserId& user_id)
{
  return AdServer::Commons::uuid_distribution_hash(user_id) %
    USER_ID_DISTRIBUTION_MOD;
}

inline
unsigned long
request_distribution_hash(
  const AdServer::Commons::RequestId& request_id,
  const std::optional<unsigned long>& user_id_distrib_hash
)
{
  return user_id_distrib_hash ? *user_id_distrib_hash :
    AdServer::Commons::uuid_distribution_hash(request_id);
}

inline
unsigned long
request_distribution_hash(
  const AdServer::Commons::RequestId& request_id,
  const AdServer::Commons::UserId& user_id
)
{
  return user_id.is_null() ?
    AdServer::Commons::uuid_distribution_hash(request_id) :
    user_id_distribution_hash(user_id);
}

inline
SafeSequenceGenerator::SafeSequenceGenerator(unsigned int min_value, unsigned int max_value)
  : min_value_(min_value),
    diff_(max_value - min_value + 1),
    value_(0)
{
  assert(min_value < max_value);
}

inline
unsigned int
SafeSequenceGenerator::get()
{
  unsigned int value = static_cast<unsigned int>(value_.exchange_and_add(1));
  return (value % diff_) + min_value_;
}

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_LOG_COMMONS_IPP */

