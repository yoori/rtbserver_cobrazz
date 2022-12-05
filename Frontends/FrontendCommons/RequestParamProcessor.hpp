#ifndef FRONTENDCOMMONS_REQUESTPARAMPROCESSOR_HPP
#define FRONTENDCOMMONS_REQUESTPARAMPROCESSOR_HPP

#include <string>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <String/AsciiStringManip.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>

#include <Commons/Containers.hpp>
#include <Commons/UserInfoManip.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include "UserIdController.hpp"

#include "Cookies.hpp"
#include "Location.hpp"

namespace FrontendCommons
{
  const long MAX_TIME_SEC = 0x7FFFFFFF;  // to avoid an overflow when pack_time/unpack_time used

  inline
  std::string
  normalize_ifa(const String::SubString& idfa);

  template<typename RequestInfoType>
  struct RequestParamProcessor:
    public ReferenceCounting::AtomicImpl
  {
    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const = 0;

  protected:
    virtual
    ~RequestParamProcessor() noexcept {}
  };

  template<typename RequestInfoType>
  struct ExtRequestParamProcessor:
    public ReferenceCounting::AtomicImpl
  {
    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& name,
      const String::SubString& value) const = 0;

  protected:
    virtual
    ~ExtRequestParamProcessor() noexcept {}
  };

  template<typename RequestInfoType>
  class StringParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    StringParamProcessor(
      std::string RequestInfoType::* field,
      unsigned long max_len = 0,
      bool lower = false,
      bool truncate = false,
      bool mime_decode = false);

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    std::string RequestInfoType::* field_;
    unsigned long max_len_;
    bool lower_;
    bool truncate_;
    bool mime_decode_;
  };

  template <typename RequestInfoType, typename CharCategoryType>
  class StringCheckParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    StringCheckParamProcessor(
      std::string RequestInfoType::* field,
      const CharCategoryType& allowed_symbols,
      unsigned long max_len = 0,
      bool lower = false,
      bool truncate = false);

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    std::string RequestInfoType::* field_;
    const CharCategoryType& allowed_symbols_;
    unsigned long max_len_;
    bool lower_;
    bool truncate_;
  };

  template<typename RequestInfoType, typename BoolType = bool>
  class BoolParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    BoolParamProcessor(BoolType RequestInfoType::* field)
      : field_(field)
    {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const
    {
      if(value.size() == 1 && (
           *value.begin() == '0' || *value.begin() == '1'))
      {
        request_info.*field_ = (*value.begin() == '1');
      }
    }

  private:
    BoolType RequestInfoType::* field_;
  };

  template<
    typename RequestInfoType,
    typename NumberContainerType,
    typename NumberType = NumberContainerType>
  class NumberParamProcessor: public RequestParamProcessor<RequestInfoType>
  {
  public:
    NumberParamProcessor(NumberContainerType RequestInfoType::* field);

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    NumberContainerType RequestInfoType::* field_;
  };

  template<
    typename RequestInfoType,
    typename NumberContainerType,
    typename NumberType = NumberContainerType>
  class DecimalParamProcessor: public RequestParamProcessor<RequestInfoType>
  {
  public:
    DecimalParamProcessor(
      NumberContainerType RequestInfoType::* field,
      bool strict = true);

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    NumberContainerType RequestInfoType::* field_;
    const bool strict_;
  };

  template<typename RequestInfoType>
  class StringEqualParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    StringEqualParamProcessor(
      bool RequestInfoType::* field,
      const String::SubString& true_value);

    virtual ~StringEqualParamProcessor() noexcept {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    bool RequestInfoType::* field_;
    String::SubString true_value_;
  };

  template<typename RequestInfoType>
  class UrlParamProcessor: public RequestParamProcessor<RequestInfoType>
  {
  public:
    static const unsigned long MAX_URL_LENGTH = 8 * 1024;

    UrlParamProcessor(
      std::string RequestInfoType::* field,
      unsigned long max_len = MAX_URL_LENGTH,
      unsigned long view_flags = HTTP::HTTPAddress::VW_FULL);

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    std::string RequestInfoType::* field_;
    const unsigned long max_len_;
    const unsigned long view_flags_;
  };

  template<typename RequestInfoType>
  class HTTPAddressParamProcessor: public RequestParamProcessor<RequestInfoType>
  {
  public:
    static const unsigned long MAX_URL_LENGTH = 8 * 1024;

    HTTPAddressParamProcessor(
      HTTP::HTTPAddress RequestInfoType::* field,
      unsigned long max_len = MAX_URL_LENGTH);

    virtual void
    process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    HTTP::HTTPAddress RequestInfoType::* field_;
    const unsigned long max_len_;
  };

  template<typename RequestInfoType>
  class TimeParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    TimeParamProcessor(
      Generics::Time RequestInfoType::* field,
      const Generics::Time& min = Generics::Time::ZERO);

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    Generics::Time RequestInfoType::* field_;
    Generics::Time min_;
  };

  template<typename RequestInfoType>
  class UnixTimeParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    UnixTimeParamProcessor(
      Generics::Time RequestInfoType::* field,
      const Generics::Time& min = Generics::Time::ZERO);

    virtual void
    process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    Generics::Time RequestInfoType::* field_;
    Generics::Time min_;
  };

  template<typename RequestInfoType>
  class RequestIdParamProcessor: public RequestParamProcessor<RequestInfoType>
  {
  public:
    RequestIdParamProcessor(AdServer::Commons::RequestId RequestInfoType::* field);

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    AdServer::Commons::RequestId RequestInfoType::* field_;
  };

  template<
    typename RequestInfoType,
    typename ContainerType,
    typename SepCategoryType,
    typename ElementConverterType>
  class ContainerParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    ContainerParamProcessor(
      ContainerType RequestInfoType::* field,
      const ElementConverterType& converter = ElementConverterType());

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    ContainerType RequestInfoType::* field_;
    ElementConverterType converter_;
  };

  struct IntConverter
  {
    template<typename NumberType>
    bool operator()(const String::SubString& value, NumberType& res) const
    {
      return String::StringManip::str_to_int(value, res);
    }
  };

  struct StringConverter
  {
    bool operator()(const String::SubString& value, std::string& res) const
    {
      res = value.str();
      return true;
    }
  };

  struct RequestIdConverter
  {
    bool operator()(
      const String::SubString& value,
      AdServer::Commons::RequestId& res) const
    {
      if(value.length() < AdServer::Commons::RequestId::encoded_size()) // hex representation
      {
        Generics::ArrayByte decoded_value;
        int decoded_value_len = String::StringManip::hex_decode(
          value, decoded_value, true);
        decoded_value_len = std::min(decoded_value_len, 8);
        AdServer::Commons::RequestId result_request_id; // filled by zeros
        std::copy(
          std::reverse_iterator<const unsigned char*>(
            decoded_value.get() + decoded_value_len),
          std::reverse_iterator<const unsigned char*>(decoded_value.get()),
          std::reverse_iterator<AdServer::Commons::RequestId::iterator>(
            result_request_id.begin() + result_request_id.size()));
        res = result_request_id;
      }
      else
      {
        res = AdServer::Commons::RequestId(value);
      }

      return true;
    }
  };

  template<
    typename RequestInfoType,
    typename ContainerType,
    typename SepCategoryType>
  class NumberContainerParamProcessor:
    public ContainerParamProcessor<
      RequestInfoType, ContainerType, SepCategoryType, IntConverter>
  {
  public:
    NumberContainerParamProcessor(
      ContainerType RequestInfoType::* field)
      : ContainerParamProcessor<
          RequestInfoType, ContainerType, SepCategoryType, IntConverter>(
            field, IntConverter())
    {}
  };

  template<
    typename RequestInfoType,
    typename ContainerType,
    typename SepCategoryType>
  class RequestIdContainerParamProcessor:
    public ContainerParamProcessor<
      RequestInfoType, ContainerType, SepCategoryType, RequestIdConverter>
  {
  public:
    RequestIdContainerParamProcessor(
      ContainerType RequestInfoType::* field)
      : ContainerParamProcessor<
          RequestInfoType, ContainerType, SepCategoryType, RequestIdConverter>(
            field, RequestIdConverter())
    {}
  };

  template<typename RequestInfoType>
  class OptOutParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    OptOutParamProcessor(
      AdServer::CampaignSvcs::UserStatus RequestInfoType::* field)
      : field_(field)
    {}

    virtual void
    process(
      RequestInfoType& request_info,
      const String::SubString& value) const
    {
      if(value == FrontendCommons::Cookies::OPTOUT_TRUE_VALUE)
      {
        request_info.user_status = AdServer::CampaignSvcs::US_OPTOUT;
      }
    }

  private:
    AdServer::CampaignSvcs::UserStatus RequestInfoType::* field_;
  };

  template<typename RequestInfoType>
  class LocationNameParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    LocationNameParamProcessor(
      Location_var RequestInfoType::* field)
      : field_(field)
    {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  protected:
    Location_var RequestInfoType::* field_;
  };

  template<typename RequestInfoType, typename FieldType = std::string>
  class LocationCountryParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    LocationCountryParamProcessor(
        FieldType RequestInfoType::* field)
      : field_(field)
    {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  protected:
    FieldType RequestInfoType::* field_;
  };

  template<typename RequestInfoType>
  class LocationCoordParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    LocationCoordParamProcessor(
      CoordLocation_var RequestInfoType::* field)
      : field_(field)
    {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  protected:
    CoordLocation_var RequestInfoType::* field_;
  };

  template<typename RequestInfoType>
  class TestRequestParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const
    {
      request_info.log_as_test = (value[0] != '0');
      request_info.test_request = (value[0] == '1');
    }
  };

  template<typename RequestInfoType>
  class UuidParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    UuidParamProcessor(
      Generics::Uuid RequestInfoType::* field)
      : field_(field)
    {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const
    {
      if(!value.empty())
      {
        try
        {
          request_info.*field_ = Generics::Uuid(value);
        }
        catch(const eh::Exception&)
        {}
      }
    }

  protected:
    Generics::Uuid RequestInfoType::* field_;
  };

  template<typename RequestInfoType>
  class SignedUuidParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    SignedUuidParamProcessor(
      Generics::Uuid RequestInfoType::* field,
      std::string RequestInfoType::* signed_user_id_field,
      AdServer::UserIdController* user_id_controller,
      AdServer::UserIdController::KeyType user_id_type,
      bool allow_rewrite = true)
      : user_id_controller_(ReferenceCounting::add_ref(user_id_controller)),
        user_id_type_(user_id_type),
        allow_rewrite_(allow_rewrite),
        field_(field),
        signed_user_id_field_(signed_user_id_field)
    {}

    SignedUuidParamProcessor(
      Generics::Uuid RequestInfoType::* field,
      AdServer::UserIdController* user_id_controller,
      AdServer::UserIdController::KeyType user_id_type,
      bool allow_rewrite = true)
      : user_id_controller_(ReferenceCounting::add_ref(user_id_controller)),
        user_id_type_(user_id_type),
        allow_rewrite_(allow_rewrite),
        field_(field),
        signed_user_id_field_(nullptr)
    {}

    virtual void
    process(
      RequestInfoType& request_info,
      const String::SubString& value) const
      noexcept
    {
      try
      {
        if(value != AdServer::Commons::PROBE_USER_ID.to_string() &&
           ((request_info.*field_).is_null() || allow_rewrite_))
        {
          Generics::SignedUuid uid = 
            user_id_controller_->verify(value, user_id_type_);

          if (!uid.uuid().is_null())
          {
            request_info.*field_ = uid.uuid();
            if(signed_user_id_field_ != nullptr)
            {
              request_info.*signed_user_id_field_ = uid.str();
            }
          }
        }
      }
      catch(const eh::Exception&)
      {}
    }

  private:
    AdServer::UserIdController_var user_id_controller_;
    const AdServer::UserIdController::KeyType user_id_type_;
    const bool allow_rewrite_;
    Generics::Uuid RequestInfoType::* field_;
    std::string RequestInfoType::* signed_user_id_field_;
  };

  template<typename RequestInfoType>
  class DataParamProcessor:
    public RequestParamProcessor<RequestInfoType>
  {
  public:
    static const char AMP = '&';
    static const char EQ = '=';

    typedef ReferenceCounting::SmartPtr<
      RequestParamProcessor<RequestInfoType> >
      RequestInfoParamProcessor_var;

    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestInfoParamProcessor_var>
      ParamProcessorMap;

    DataParamProcessor(
      const ParamProcessorMap& processors,
      ExtRequestParamProcessor<RequestInfoType>* alt_processor = 0)
      noexcept
      : processors_(processors),
        alt_processor_(ReferenceCounting::add_ref(alt_processor))
    {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const
    {
      std::string decoded;

      try
      {
        String::StringManip::base64mod_decode(decoded, value);
      }
      catch(const String::StringManip::InvalidFormatException&)
      {
        return;
      }

      std::string::const_iterator start_name = decoded.begin();
      std::string::const_iterator end_name = decoded.begin();
      bool replace_double_amp = false;
      bool found_eq = false;
      for(std::string::const_iterator it = decoded.begin();
          it <= decoded.end(); ++it)
      {
        if(it == decoded.end() || *it == AMP)
        {
          if(it + 1 < decoded.end() && *(it + 1) == AMP)
          {
            replace_double_amp = true;
            ++it;// skip next amp
          }
          else if(start_name < end_name)
          {
            std::string purify_value;
            String::SubString param_name(&*start_name, &*end_name);
            String::SubString param_value(&*(end_name + 1), it - end_name - 1);
            if(replace_double_amp)
            {
              String::StringManip::replace(
                param_value,
                purify_value,
                String::SubString("&&", 2),
                String::SubString("&", 1));
              param_value = purify_value;
            }

            typename ParamProcessorMap::const_iterator param_it =
              processors_.find(param_name);
            if(param_it != processors_.end())
            {
              param_it->second->process(request_info, param_value); 
            }
            else if(alt_processor_)
            {
              alt_processor_->process(request_info, param_name, param_value);
            }

            replace_double_amp = false;
            found_eq = false;
            start_name = it + 1;
            end_name = start_name;
          }
        }
        else if(!found_eq && *it == EQ)
        {
          end_name = it;
          found_eq = true;
        }
      }
    }

  private:
    const ParamProcessorMap& processors_;
    ReferenceCounting::SmartPtr<
      ExtRequestParamProcessor<RequestInfoType> > alt_processor_;
  };
}

#include "RequestParamProcessor.tpp"

#endif /*FRONTENDCOMMONS_REQUESTPARAMPROCESSOR_HPP*/
