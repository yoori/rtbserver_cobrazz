#ifndef AD_SERVER_LOG_PROCESSING_CSV_UTILS_HPP
#define AD_SERVER_LOG_PROCESSING_CSV_UTILS_HPP


#include <stdint.h>
#include <string>
#include <String/StringManip.hpp>
#include <LogCommons/LogCommons.hpp>
#include <Commons/StringHolder.hpp>

namespace AdServer {
namespace LogProcessing {

const String::AsciiStringManip::CharCategory
UNDISPLAYABLE("\x01-\x1F%");

const String::AsciiStringManip::CharCategory
NON_ASCII_AND_UNDISPLAYABLE("\x01-\x1F\x7F-\xFF%");

inline
void
undisplayable_mime_encode(
  std::string& dest,
  const String::SubString& src,
  const String::AsciiStringManip::CharCategory& RESTRICTED_CHARS =
    NON_ASCII_AND_UNDISPLAYABLE)
  /*throw(eh::Exception)*/
{
  dest.clear();
  String::SubString::ConstPointer begin = src.begin();
  String::SubString::ConstPointer end = src.end();
  String::SubString::ConstPointer ptr =
    RESTRICTED_CHARS.find_owned(begin, end);
  if (ptr != end)
  {
    dest.reserve((end - ptr) * 3 + ptr - begin);
    do
    {
      dest.append(begin, ptr);
      uint8_t ch = static_cast<uint8_t>(*ptr);
      char buf[] = { '%',
        String::AsciiStringManip::HEX_DIGITS[(ch >> 4) & 0x0F],
        String::AsciiStringManip::HEX_DIGITS[ch & 0x0F] };
      dest.append(buf, sizeof(buf));
      begin = ptr;
      ++begin;
      ptr = RESTRICTED_CHARS.find_owned(begin, end);
    }
    while (ptr != end);
    dest.append(begin, end - begin);
  }
  else
  {
    dest.assign(begin, end - begin);
  }
}

inline std::string
hex_request_id(
  const AdServer::Commons::RequestId& request_id)
{
  // convert last uint64 of uuid to hex
  return String::StringManip::hex_encode(
    request_id.end() - 8, 8, true);
}

struct URLEncoder // this isn't URL encode !!!
{
  static inline void
  encode(std::string& encoded, const std::string& str) /*throw(eh::Exception)*/
  {
    undisplayable_mime_encode(encoded, str);
  }
};

struct UTF8Encoder
{
  static inline void
  encode(std::string& encoded, const std::string& str) /*throw(eh::Exception)*/
  {
    if (String::UTF8Handler::is_correct_utf8_string(str.c_str()) == 0)
    {
      undisplayable_mime_encode(encoded, str, UNDISPLAYABLE);
    }
    else
    {
      undisplayable_mime_encode(encoded, str);
    }
  }
};

template <typename Encoder = URLEncoder>
struct MimeCoder
{
  const std::string&
  operator()(const std::string& str) /*throw(eh::Exception)*/
  {
    Encoder::encode(encoded_str_, str);
    return encoded_str_;
  }

  const std::string&
  operator()(const std::string& str, std::size_t max_length)
    /*throw(eh::Exception)*/
  {
    Encoder::encode(encoded_str_, str);
    trim(encoded_str_, max_length);
    return encoded_str_;
  }

private:
  std::string encoded_str_;
};


template <class OPTIONAL_>
inline std::ostream&
write_optional_value_as_csv(
  std::ostream& os,
  const OPTIONAL_& opt,
  const char *not_present_marker = 0
)
{
  return opt.present() ? os << opt.get() :
    (not_present_marker ? os << not_present_marker : os);
}

inline std::ostream&
write_date_as_csv(std::ostream& os, const DayTimestamp& date)
  /*throw(eh::Exception)*/
{
  return os << date.time().gm_f();
}

inline std::ostream&
write_date_as_csv(std::ostream& os, const Generics::Time& date)
  /*throw(eh::Exception)*/
{
  return os << date.gm_f();
}

template <typename Timestamp>
std::ostream&
write_date_as_csv(std::ostream& os, const Timestamp& date)
  /*throw(eh::Exception)*/
{
  return os << date.time().gm_ft();
}

// Writes specified zero_value if date.is_zero() is true
template <class TIMESTAMP_>
std::ostream&
write_date_as_csv_2(
  std::ostream& os,
  const TIMESTAMP_& date,
  const char* zero_value = "-infinity"
)
  /*throw(eh::Exception)*/
{
  return date.is_zero() ? os << zero_value : write_date_as_csv(os, date);
}

template <class OPTIONAL_DATE_>
inline std::ostream&
write_optional_date_as_csv(std::ostream& os, const OPTIONAL_DATE_& opt_date)
{
  return opt_date.present() ? write_date_as_csv(os, opt_date.get()) : os;
}

template <class OPTIONAL_DATE_>
inline std::ostream&
write_optional_or_null_date_as_csv(
  std::ostream& os,
  const OPTIONAL_DATE_& opt_date,
  const typename OPTIONAL_DATE_::ValueT& null_date =
    typename OPTIONAL_DATE_::ValueT()
)
{
  write_date_as_csv(os, opt_date.present() ? opt_date.get() : null_date);
  return os;
}

// Writes specified null_value if opt_date.present() is false
// or opt_date.get().is_zero() is true
template <class OPTIONAL_DATE_>
inline std::ostream&
write_optional_or_null_date_as_csv_2(
  std::ostream& os,
  const OPTIONAL_DATE_& opt_date,
  const char* null_value = "-infinity"
)
{
  return opt_date.present() && !opt_date.get().is_zero() ?
    write_date_as_csv(os, opt_date.get()) : os << null_value;
}

inline std::ostream&
write_not_empty_string_as_csv(std::ostream& os, const std::string& str)
{
  std::string csv_encoded_str;
  using String::StringManip::csv_encode;
  csv_encode(str.c_str(), csv_encoded_str);
  os.write(csv_encoded_str.data(), csv_encoded_str.size());
  return os;
}

inline std::ostream&
write_string_as_csv(
  std::ostream& os,
  const std::string& str,
  const char *emptiness_marker = "\"\""
)
{
  return str.empty() ? os << emptiness_marker :
    write_not_empty_string_as_csv(os, str);
}

inline std::ostream&
write_string_as_csv(
  std::ostream& os,
  const Commons::StringHolder* str,
  const char *emptiness_marker = "\"\""
)
{
  return (!str || str->str().empty()) ? os << emptiness_marker :
    write_not_empty_string_as_csv(os, str->str());
}

template <class OPTIONAL_STRING_>
inline std::ostream&
write_optional_string_as_csv(
  std::ostream& os,
  const OPTIONAL_STRING_& opt_str,
  const char *not_present_marker = 0
)
{
  return opt_str.present() ? write_not_empty_string_as_csv(os, opt_str.get()) :
    (not_present_marker ? os << not_present_marker : os);
}

template <class OPTIONAL_STRING_>
inline std::ostream&
write_optional_string_upper_as_csv(
  std::ostream& os,
  const OPTIONAL_STRING_& opt_str,
  const char *not_present_marker = 0
)
{
  return opt_str.present() ?
    write_not_empty_string_as_csv(os, ToUpper()(opt_str.get())) :
    (not_present_marker ? os << not_present_marker : os);
}

template <typename T>
inline std::ostream&
write_list_as_csv(
  std::ostream& os,
  const T& list,
  const char* separator = "|",
  const char* empty_marker = 0
)
{
  if (list.empty())
  {
    return empty_marker ? os << empty_marker : os;
  }
  os << separator;
  output_sequence(os, list, separator);
  os << separator;
  return os;
}

inline char bool_to_char(bool value)
{
  return value ? 'Y' : 'N';
}

struct UuidIoCsvWrapper: public Generics::Uuid
{
  UuidIoCsvWrapper(): Generics::Uuid() {}

  UuidIoCsvWrapper(const Generics::Uuid& uuid): Generics::Uuid(uuid) {}
};

inline
std::ostream&
operator <<(std::ostream& ostr, const UuidIoCsvWrapper& uuid) noexcept
{
  if (!uuid.is_null())
  {
    ostr << static_cast<const Generics::Uuid&>(uuid);
  }
  return ostr;
}


} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CSV_UTILS_HPP */

