#include <Frontends/FrontendCommons/Cookies.hpp>
#include "RequestInfoFiller.hpp"

namespace
{
  namespace Aspect
  {
    const char PASS_PIXEL_FRONTEND[] = "PassbackPixel::Frontend";
  }

  namespace Request
  {
    namespace Cookies
    {
      const String::SubString OPTOUT("OPTED_OUT");
      const String::SubString OPTOUT_TRUE_VALUE("YES");
    }

    namespace Parameters
    {
      const String::SubString COLO_ID("colo");
      const String::SubString TAG_ID("tid");
      const String::SubString COUNTRY("country");
      const String::SubString LOCATION_NAME("loc.name");

      /* debug params */
      const String::SubString DEBUG_TIME("debug-time");
      const String::SubString IP_ADDRESS("debug.ip");
    }

    namespace Headers
    {
      const String::SubString IP_ADDRESS(".remotehost");
    }
  }
}

namespace AdServer
{
namespace PassbackPixel
{
  namespace
  {
    template<typename RequestInfoType>
    class UuidParamProcessor:
      public FrontendCommons::RequestParamProcessor<RequestInfoType>
    {
    public:
      virtual void
      process(
        RequestInfoType& request_info,
        const String::SubString& /*value*/) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        // user id defined
        request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
      }

    protected:
      virtual ~UuidParamProcessor() noexcept {}
    };
  }

  /** RequestInfoFiller */
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    unsigned long colo_id,
    const char* geo_ip_path)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      colo_id_(colo_id)
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
          Aspect::PASS_PIXEL_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    // track request
    track_cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID,
      PassbackTrackParamProcessor_var(
        new UuidParamProcessor<PassbackTrackInfo>())));
    track_cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::OPTOUT,
      PassbackTrackParamProcessor_var(
        new FrontendCommons::OptOutParamProcessor<PassbackTrackInfo>(
          &PassbackTrackInfo::user_status))));

    track_header_processors_.insert(std::make_pair(
      Request::Headers::IP_ADDRESS,
      new FrontendCommons::StringParamProcessor<PassbackTrackInfo>(
        &PassbackTrackInfo::ip)));

    track_param_processors_.insert(std::make_pair(
      Request::Parameters::COLO_ID,
      PassbackTrackParamProcessor_var(
        new FrontendCommons::NumberParamProcessor<PassbackTrackInfo, unsigned long>(
          &PassbackTrackInfo::colo_id))));
    track_param_processors_.insert(std::make_pair(
      Request::Parameters::TAG_ID,
      PassbackTrackParamProcessor_var(
        new FrontendCommons::NumberParamProcessor<PassbackTrackInfo, unsigned long>(
          &PassbackTrackInfo::tag_id))));
    track_param_processors_.insert(std::make_pair(
      Request::Parameters::COUNTRY,
      PassbackTrackParamProcessor_var(
        new FrontendCommons::StringParamProcessor<PassbackTrackInfo>(
          &PassbackTrackInfo::country,
          2, // max length
          true // to lower
          ))));
    track_param_processors_.insert(std::make_pair(
      Request::Parameters::LOCATION_NAME,
      PassbackTrackParamProcessor_var(
        new FrontendCommons::LocationCountryParamProcessor<PassbackTrackInfo>(
          &PassbackTrackInfo::country))));
    track_param_processors_.insert(std::make_pair(
      Request::Parameters::DEBUG_TIME,
      PassbackTrackParamProcessor_var(
        new FrontendCommons::TimeParamProcessor<PassbackTrackInfo>(
          &PassbackTrackInfo::time, Generics::Time::ONE_DAY))));
    track_param_processors_.insert(std::make_pair(
      Request::Parameters::IP_ADDRESS,
      new FrontendCommons::StringParamProcessor<PassbackTrackInfo>(
        &PassbackTrackInfo::ip)));
  }

  void
  RequestInfoFiller::fill_track(
    PassbackTrackInfo& passback_track_info,
    const HTTP::SubHeaderList& headers,
    const HTTP::ParamList& params) const
    /*throw(InvalidParamException, ForbiddenException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    /* fill ad request parameters */
    passback_track_info.user_status = AdServer::CampaignSvcs::US_UNDEFINED;
    passback_track_info.colo_id = colo_id_;

    try
    {
      for (HTTP::SubHeaderList::const_iterator it = headers.begin();
        it != headers.end(); ++it)
      {
        std::string header_name = it->name.str();
        String::AsciiStringManip::to_lower(header_name);

        PassbackTrackProcessorMap::const_iterator param_it =
          track_header_processors_.find(header_name);

        if(param_it != track_header_processors_.end())
        {
          param_it->second->process(passback_track_info, it->value);
        }
      } /* headers processing */

      for(HTTP::ParamList::const_iterator it = params.begin();
          it != params.end(); ++it)
      {
        PassbackTrackProcessorMap::const_iterator param_it =
          track_param_processors_.find(it->name);

        if(param_it != track_param_processors_.end())
        {
          param_it->second->process(passback_track_info, it->value);
        }
      } /* url parameters processing */

      HTTP::CookieList cookies;

      try
      {
        cookies.load_from_headers(headers);
      }
      catch(HTTP::CookieList::InvalidArgument&)
      {
        throw InvalidParamException("");
      }

      for(HTTP::CookieList::const_iterator it = cookies.begin();
          it != cookies.end(); ++it)
      {
        PassbackTrackProcessorMap::const_iterator param_it =
          track_cookie_processors_.find(it->name);

        if(param_it != track_cookie_processors_.end())
        {
          param_it->second->process(passback_track_info, it->value);
        }
      } /* cookies processing */

      if(passback_track_info.time == Generics::Time::ZERO)
      {
        passback_track_info.time = Generics::Time::get_time_of_day();
      }

      if(passback_track_info.country.empty() &&
         !passback_track_info.ip.empty() &&
         ip_map_.get())
      {
        try
        {
          GeoIPMapping::IPMapCity2::CityLocation loc;

          if(ip_map_->city_location_by_addr(
               passback_track_info.ip.c_str(),
               loc,
               false))
          {
            passback_track_info.country = loc.country_code.str();
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
} /*PassbackPixel*/
} /*AdServer*/
