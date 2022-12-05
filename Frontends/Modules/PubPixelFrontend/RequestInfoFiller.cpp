#include <iostream>
#include "RequestInfoFiller.hpp"

namespace
{
  namespace Aspect
  {
    const char PUBPIXEL_FRONTEND[] = "PubPixel::Frontend";
  }

  namespace Request
  {
    namespace Parameters
    {
      const String::SubString COUNTRY("country");
      const String::SubString USER_STATUS("us");
      const String::SubString IP_ADDRESS("debug.ip");
      const String::SubString LOCATION_NAME("loc.name");
      const String::SubString PUBLISHER_ACCOUNT_IDS("aid");
    }

    namespace Headers
    {
      const String::SubString IP_ADDRESS(".remotehost");
      const String::SubString PUBLISHER_ACCOUNT_IDS("aid");
    }
  }
}

namespace AdServer
{
namespace PubPixel
{
  namespace
  {
    class UserStatusParamProcessor:
      public RequestParamProcessor
    {
    public:
      virtual void
      process(
        RequestInfo& request_info,
        const String::SubString& value) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        // user id defined
        if(value.compare("in") == 0)
        {
          request_info.user_status =
            AdServer::CampaignSvcs::US_OPTIN;
        }
        else if(value.compare("out") == 0)
        {
          request_info.user_status =
            AdServer::CampaignSvcs::US_OPTOUT;
        }
        else
        {
          throw RequestInfoFiller::InvalidParamException("");
        }
      }

    protected:
      virtual ~UserStatusParamProcessor() noexcept {}
    };
  }

  class CountryParamProcessor: public RequestParamProcessor
  {
  public:
    virtual void process(
      RequestInfo& request_info,
      const String::SubString& value) const
    {
      if(value.size() == 2 || value.empty())
      {
        request_info.country = value.str();
      }
    }
  };

  /** RequestInfoFiller */
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    const char* geo_ip_path)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger))
  {
    static const char* FUN = "RequestInfoFiller::RequestInfoFiller()";

    if(geo_ip_path)
    {
      try
      {
        ip_map_ = IPMapPtr(new GeoIPMapping::IPMapCity2(geo_ip_path));
      }
      catch (const GeoIPMapping::IPMap::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": GeoIPMapping::IPMap::Exception caught: " << e.what();

        logger->log(ostr.str(),
          Logging::Logger::CRITICAL,
          Aspect::PUBPIXEL_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    // redirect request
    param_processors_.insert(std::make_pair(
      Request::Parameters::USER_STATUS,
      RequestParamProcessor_var(new UserStatusParamProcessor())));
    param_processors_.insert(std::make_pair(
      Request::Parameters::COUNTRY,
      RequestParamProcessor_var(new CountryParamProcessor())));
    param_processors_.insert(std::make_pair(
      Request::Parameters::IP_ADDRESS,
      RequestParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::peer_ip))));
    param_processors_.insert(std::make_pair(
      Request::Parameters::LOCATION_NAME,
      RequestParamProcessor_var(
        new FrontendCommons::LocationCountryParamProcessor<
          RequestInfo, AdServer::Commons::Optional<std::string> >(
            &RequestInfo::country))));
    param_processors_.insert(std::make_pair(
      Request::Parameters::PUBLISHER_ACCOUNT_IDS,
      RequestParamProcessor_var(
        new FrontendCommons::NumberContainerParamProcessor<
          RequestInfo,
          RequestInfo::PublisherAccountIDs,
          String::AsciiStringManip::SepComma>(
            &RequestInfo::publisher_account_ids))));

    header_processors_.insert(std::make_pair(
      Request::Headers::IP_ADDRESS,
      RequestParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::peer_ip))));
  }

  void
  RequestInfoFiller::fill(
    RequestInfo& request_info,
    const HTTP::SubHeaderList& headers,
    const HTTP::ParamList& params) const
    /*throw(InvalidParamException, ForbiddenException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    try
    {
      // headers processing
      for (HTTP::SubHeaderList::const_iterator it = headers.begin();
        it != headers.end(); ++it)
      {
        std::string header_name = it->name.str();
        String::AsciiStringManip::to_lower(header_name);

        RequestParamProcessorMap::const_iterator param_it =
          header_processors_.find(header_name);

        if(param_it != header_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      }

      // url parameters processing
      for(HTTP::ParamList::const_iterator it = params.begin();
          it != params.end(); ++it)
      {
        RequestParamProcessorMap::const_iterator param_it =
          param_processors_.find(it->name);

        if(param_it != param_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      }

      if(!request_info.country.present() && ip_map_.get())
      {
        try
        {
          GeoIPMapping::IPMapCity2::CityLocation geo_location;

          if(ip_map_->city_location_by_addr(
               request_info.peer_ip.c_str(),
               geo_location,
               false))
          {
            request_info.country = geo_location.country_code.str();
          }
        }
        catch(const eh::Exception&)
        {}
      }
    }
    catch(const InvalidParamException&)
    {
      throw;
    }
    catch(const ForbiddenException&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't fill request info. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }
} /*PubPixel*/
} /*AdServer*/
