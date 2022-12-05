#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Commons/DecimalUtils.hpp>

namespace FrontendCommons
{
  template<typename RequestInfoType>
  StringParamProcessor<RequestInfoType>::StringParamProcessor(
    std::string RequestInfoType::* field,
    unsigned long max_len,
    bool lower,
    bool truncate,
    bool mime_decode)
    : field_(field),
      max_len_(max_len),
      lower_(lower),
      truncate_(truncate),
      mime_decode_(mime_decode)
  {}

  template<typename RequestInfoType>
  void
  StringParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    std::string dst;
    String::SubString in_value;
    if(mime_decode_)
    {
      String::StringManip::mime_url_decode(value, dst);
      in_value = dst; 
    }
    else
    {
      in_value = value;
    }
    if(max_len_ == 0 || in_value.size() <= max_len_)
    {
      in_value.assign_to(request_info.*field_);
      if(lower_)
      {
        String::AsciiStringManip::to_lower(request_info.*field_);
      }
    }
    else if (truncate_)
    {
      in_value.substr(0, max_len_).assign_to(request_info.*field_);
      if(lower_)
      {
        String::AsciiStringManip::to_lower(request_info.*field_);
      }
    }
  }

  template<typename RequestInfoType, typename CharCategoryType>
  StringCheckParamProcessor<RequestInfoType, CharCategoryType>::
   StringCheckParamProcessor(
    std::string RequestInfoType::* field,
    const CharCategoryType& allowed_symbols,
    unsigned long max_len,
    bool lower,
    bool truncate)
    : field_(field),
      allowed_symbols_(allowed_symbols),
      max_len_(max_len),
      lower_(lower),
      truncate_(truncate)
  {}

  template<typename RequestInfoType, typename CharCategoryType>
  void
  StringCheckParamProcessor<RequestInfoType, CharCategoryType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    if(max_len_ == 0 || value.size() <= max_len_)
    {
      const char* const END = value.end();
      if (allowed_symbols_.find_nonowned(value.begin(), END) == END)
      {
        value.assign_to(request_info.*field_);
        if(lower_)
        {
          String::AsciiStringManip::to_lower(request_info.*field_);
        }
      }
    }
    else if (truncate_)
    {
      const char* const END = value.begin() + max_len_;
      if (allowed_symbols_.find_nonowned(value.begin(), END) == END)
      {
        value.substr(0, max_len_).assign_to(request_info.*field_);
        if(lower_)
        {
          String::AsciiStringManip::to_lower(request_info.*field_);
        }
      }
    }
  }

  template<
    typename RequestInfoType,
    typename NumberContainerType,
    typename NumberType>
  NumberParamProcessor<RequestInfoType, NumberContainerType, NumberType>::
  NumberParamProcessor(NumberContainerType RequestInfoType::* field)
    : field_(field)
  {}

  template<
    typename RequestInfoType,
    typename NumberContainerType,
    typename NumberType>
  void
  NumberParamProcessor<RequestInfoType, NumberContainerType, NumberType>::
  process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    NumberType num_val;
    if(String::StringManip::str_to_int(value, num_val))
    {
      request_info.*field_ = num_val;
    }
  }

  template<
    typename RequestInfoType,
    typename NumberContainerType,
    typename NumberType>
  DecimalParamProcessor<RequestInfoType, NumberContainerType, NumberType>::
  DecimalParamProcessor(
    NumberContainerType RequestInfoType::* field,
    bool strict)
    : field_(field),
      strict_(strict)
  {}

  template<
    typename RequestInfoType,
    typename NumberContainerType,
    typename NumberType>
  void
  DecimalParamProcessor<RequestInfoType, NumberContainerType, NumberType>::
  process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    try
    {
      if(strict_)
      {
        request_info.*field_ = NumberType(value);
      }
      else if(!value.empty())
      {
        request_info.*field_ = AdServer::Commons::extract_decimal<NumberType>(
          value);
      }
    }
    catch(const typename NumberType::Overflow&)
    {
      Stream::Error ostr;
      ostr << "Float parameter overflow on '" << value << "'";
      throw HTTPExceptions::InvalidParamException(ostr);
    }
    catch(const typename NumberType::NotNumber&)
    {
      Stream::Error ostr;
      ostr << "Invalid float parameter value '" << value << "'";
      throw HTTPExceptions::InvalidParamException(ostr);
    }
  }

  template<typename RequestInfoType>
  StringEqualParamProcessor<RequestInfoType>::StringEqualParamProcessor(
    bool RequestInfoType::* field,
    const String::SubString& true_value)
    : field_(field),
      true_value_(true_value)
  {}

  template<typename RequestInfoType>
  void
  StringEqualParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    request_info.*field_ = (true_value_ == value);
  }

  template<typename RequestInfoType>
  UrlParamProcessor<RequestInfoType>::UrlParamProcessor(
    std::string RequestInfoType::* field,
    unsigned long max_len,
    unsigned long view_flags)
    : field_(field),
      max_len_(max_len),
      view_flags_(view_flags)
  {}

  template<typename RequestInfoType>
  void
  UrlParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    if(!value.empty())
    {
      if(value.size() > max_len_)
      {
        Stream::Error ostr;
        ostr << "Value length(" << value.size() << ") exceed";
        throw HTTPExceptions::InvalidParamException(ostr);
      }
      try
      {
        HTTP::BrowserAddress addr(value);
        addr.get_view(view_flags_, request_info.*field_);
      }
      catch(...)
      {}
    }
  }

  template<typename RequestInfoType>
  HTTPAddressParamProcessor<RequestInfoType>::HTTPAddressParamProcessor(
    HTTP::HTTPAddress RequestInfoType::* field,
    unsigned long max_len)
    : field_(field),
      max_len_(max_len)
  {}

  template<typename RequestInfoType>
  void
  HTTPAddressParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    if(!value.empty())
    {
      if(value.size() > max_len_)
      {
        Stream::Error ostr;
        ostr << "Value length(" << value.size() << ") exceed";
        throw HTTPExceptions::InvalidParamException(ostr);
      }
      try
      {
        request_info.*field_ = HTTP::BrowserAddress(value);
      }
      catch(...)
      {}
    }
  }

  // TimeParamProcessor
  template<typename RequestInfoType>
  TimeParamProcessor<RequestInfoType>::TimeParamProcessor(
    Generics::Time RequestInfoType::* field,
    const Generics::Time& min)
    : field_(field),
      min_(min)
  {}

  template<typename RequestInfoType>
  void
  TimeParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    try
    {
      Generics::Time time(value,
        value.size() > 2 && value[2] == '-' ?
        "%d-%m-%Y:%H-%M-%S" : "%Y-%m-%d %H:%M:%S");
      if(time >= min_ && time.tv_sec <= MAX_TIME_SEC)
      {
        request_info.*field_ = time;
      }
    }
    catch(const eh::Exception& ex)
    {}
  }

  // UnixTimeParamProcessor
  template<typename RequestInfoType>
  UnixTimeParamProcessor<RequestInfoType>::UnixTimeParamProcessor(
    Generics::Time RequestInfoType::* field,
    const Generics::Time& min)
    : field_(field),
      min_(min)
  {}

  template<typename RequestInfoType>
  void
  UnixTimeParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    try
    {
      unsigned long unix_time;
      if(String::StringManip::str_to_int(value, unix_time))
      {
        Generics::Time time(unix_time);
        if(time >= min_ && time.tv_sec <= MAX_TIME_SEC)
        {
          request_info.*field_ = time;
        }
      }
    }
    catch(const eh::Exception& ex)
    {}
  }

  // RequestIdParamProcessor
  template<typename RequestInfoType>
  RequestIdParamProcessor<RequestInfoType>::RequestIdParamProcessor(
    AdServer::Commons::RequestId RequestInfoType::* field)
    : field_(field)
  {}

  template<typename RequestInfoType>
  void
  RequestIdParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    try
    {
      RequestIdConverter conv;
      conv(value, request_info.*field_);
    }
    catch(const eh::Exception& ex)
    {}
  }

  template<typename RequestInfoType,
    typename ContainerType,
    typename SepCategoryType,
    typename ElementConverterType>
  ContainerParamProcessor<
    RequestInfoType, ContainerType, SepCategoryType, ElementConverterType>::
  ContainerParamProcessor(
    ContainerType RequestInfoType::* field,
    const ElementConverterType& converter)
    : field_(field),
      converter_(converter)
  {}

  template<typename RequestInfoType,
    typename ContainerType,
    typename SepCategoryType,
    typename ElementConverterType>
  void
  ContainerParamProcessor<
    RequestInfoType, ContainerType, SepCategoryType, ElementConverterType>::
  process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    String::StringManip::Splitter<SepCategoryType> tokenizer(
      value);
    String::SubString token;
    while(tokenizer.get_token(token))
    {
      typename ContainerType::value_type val;
      if(converter_(token, val))
      {
        (request_info.*field_).insert((request_info.*field_).end(), val);
      }
    }
  }

  template<typename RequestInfoType>
  void
  LocationNameParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    Location_var location = Location::parse(value);
    if(location.in())
    {
      request_info.*field_ = location;
    }
  }

  template<typename RequestInfoType, typename FieldType>
  void
  LocationCountryParamProcessor<RequestInfoType, FieldType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    std::string country;
    String::SubString::SizeType country_end;

    if (Location::extract_country(value, country, country_end, true))
    {
      request_info.*field_ = country;
    }
  }

  template<typename RequestInfoType>
  void
  LocationCoordParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    CoordLocation_var coord_location = CoordLocation::parse(value);

    if(coord_location.in())
    {
      request_info.*field_ = coord_location;
    }
  }

  inline
  std::string
  normalize_ifa(const String::SubString& idfa)
  {
    std::string res = idfa.str();
    String::AsciiStringManip::to_lower(res);

    std::string norm_res;
    norm_res.reserve(32);
    String::StringManip::Splitter<String::AsciiStringManip::SepMinus> tokenizer(res);
    String::SubString token;
    while(tokenizer.get_token(token))
    {
      norm_res += token.str();
    }

    if(norm_res.size() == 32 &&
      String::AsciiStringManip::HEX_NUMBER.find_nonowned(
        norm_res.data(), norm_res.data() + norm_res.size()) == norm_res.data() + norm_res.size())
    {
      std::string minus_norm_res;
      minus_norm_res.reserve(36);
      minus_norm_res += norm_res.substr(0, 8);
      minus_norm_res += '-';
      minus_norm_res += norm_res.substr(8, 4);
      minus_norm_res += '-';
      minus_norm_res += norm_res.substr(12, 4);
      minus_norm_res += '-';
      minus_norm_res += norm_res.substr(16, 4);
      minus_norm_res += '-';
      minus_norm_res += norm_res.substr(20, 12);

      return minus_norm_res;
    }

    return std::string();
  }
}
