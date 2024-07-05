#include <algorithm>
#include <functional>
#include <math.h>
#include <String/AsciiStringManip.hpp>
#include <String/UTF8Case.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>
#include <Generics/Rand.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/CorbaAlgs.hpp>

#include <Frontends/ProfilingServer/DMPProfilingInfo.hpp>

#include "RequestInfoFiller.hpp"
#include <Generics/Hash.hpp>

namespace
{
  const std::string AES_SOURCE_MARKER("bl2");
}

namespace AdServer
{
namespace Profiling
{
  namespace
  {
    template<typename Cont>
    void
    copy_channels(
      const Cont& src,
      AdServer::ChannelSvcs::ChannelServerBase::ChannelAtomSeq& dst)
      /*throw(eh::Exception)*/
    {
      dst.length(src.size());
      std::size_t i = 0;

      for(auto it = src.begin(); it != src.end(); ++it, ++i)
      {
        dst[i].id = (*it).channel_id();
        dst[i].trigger_channel_id = (*it).channel_trigger_id();
      }
    }

    template<typename Cont>
    void
    copy_channels(
      const Cont& src,
      AdServer::ChannelSvcs::ChannelServerBase::ContentChannelAtomSeq& dst)
      /*throw(eh::Exception)*/
    {
      dst.length(src.size());
      std::size_t i = 0;

      for(auto it = src.begin(); it != src.end(); ++it, ++i)
      {
        dst[i].id = (*it).channel_id();
        dst[i].weight = (*it).weight();
      }
    }

    std::string
    norm_coord(
      const AdServer::CampaignSvcs::CoordDecimal& coord,
      int precision) noexcept
    {
      AdServer::CampaignSvcs::CoordDecimal coord_floor = coord;
      coord_floor.floor(precision);
      std::string coord_str = coord_floor.str();

      std::size_t dot_pos = coord_str.find('.');
      if(dot_pos != std::string::npos)
      {
        int addition_size = precision - (coord_str.size() - dot_pos);
        if(addition_size > 0)
        {
          coord_str += std::string(
            static_cast<std::size_t>(addition_size), '0');
        }
      }
      else if(precision > 0)
      {
        coord_str += '.';
        coord_str += std::string(
          static_cast<std::size_t>(precision), '0');
      }

      for(auto it = coord_str.begin(); it != coord_str.end(); ++it)
      {
        if(*it == '.')
        {
          *it = 'd';
          break;
        }
      }

      return coord_str;
    }

    std::string
    generate_coord_keyword(
      const AdServer::CampaignSvcs::CoordDecimal& latitude,
      const AdServer::CampaignSvcs::CoordDecimal& longitude,
      int lat_precision,
      int long_precision,
      const String::SubString& precision_str) noexcept
    {
      std::string lat_part = norm_coord(latitude, lat_precision);
      std::string long_part = norm_coord(longitude, long_precision);
      return std::string("poadgeop") + precision_str.str() + "x" +
        lat_part + "x" +
        long_part;
    }

    String::AsciiStringManip::CharCategory SEP_PERIOD_CATEGORY(".");

    struct GeoPrecision
    {
      GeoPrecision(
        unsigned long precision_val)
        : precision(precision_val)
      {
        std::ostringstream ostr;
        ostr << "p" << precision;
        precision_str = ostr.str();
      }

      unsigned long precision;
      std::string precision_str;
    };

    const GeoPrecision GET_PRECISIONS[] = {
      GeoPrecision(1),
      GeoPrecision(2),
      GeoPrecision(3)
    };
  }

  // RequestInfoFiller
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    unsigned long colo_id,
    const ExternalUserIdSet& skip_external_ids,
    const String::SubString& bind_url_suffix,
    bool debug_on)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      colo_id_(colo_id),
      skip_external_ids_(skip_external_ids),
      bind_url_suffix_(bind_url_suffix.str()),
      debug_on_(debug_on)
  {}

  typedef const String::AsciiStringManip::Char2Category<'/', '?'>
    UrlParamMarkersCategory;

  namespace
  {
    UrlParamMarkersCategory URL_PARAM_MARKERS_SYMBOLS;
  }

  void
  RequestInfoFiller::fill_by_dmp_profiling_info(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    RequestInfo& request_info,
    const DMPProfilingInfoReader& dmp_profiling_info,
    const Generics::Time& now)
    noexcept
  {
    // === Debug request info ===

    std::string url(dmp_profiling_info.url());
    String::AsciiStringManip::to_lower(url);

    String::SubString url_substr(url);

    bool debug_by_request_context =
      debug_on_ ||
      ((URL_PARAM_MARKERS_SYMBOLS.find_owned(url_substr.begin(), url_substr.end()) != url_substr.end()) && // ignore requests without params (dns, ...)
      (
      // aid= trusted sites
      ((::strstr(url.c_str(), ".tappx.com") != 0 || ::strstr(url.c_str(), ".lkqd.net") != 0) && ::strstr(url.c_str(), "&aid=") != 0) ||
      // key= trusted sites
      (::strstr(url.c_str(), "adspirit") != 0 && ::strstr(url.c_str(), "&key=") != 0) ||
      ::strstr(url.c_str(), "did=") != 0 ||
      ::strstr(url.c_str(), "ifa") != 0 ||
      ::strstr(url.c_str(), "idfa") != 0 ||
      ::strstr(url.c_str(), "device_id") != 0 ||
      ::strstr(url.c_str(), "gaid") != 0 ||
      ::strstr(url.c_str(), "google_aid") != 0 ||
      ::strstr(url.c_str(), "udid=") != 0 ||
      ::strstr(url.c_str(), "aaid") != 0 ||
      ::strstr(url.c_str(), "adid") != 0 ||
      ::strstr(url.c_str(), "advid") != 0 ||
      ::strstr(url.c_str(), "am-uid") != 0 ||
      ::strstr(url.c_str(), "advertisingid") != 0 ||
      ::strstr(url.c_str(), "advark") != 0 ||
      ::strstr(url.c_str(), "source=buzzoola") != 0 ||
      ::strstr(url.c_str(), "/buzzoola") != 0 ||
      ::strstr(url.c_str(), "videonow") != 0 ||
      ::strstr(url.c_str(), "gpmd") != 0 ||
      ::strstr(url.c_str(), "relap") != 0 ||
      ::strstr(url.c_str(), "bidswitch") != 0 ||
      ::strstr(url.c_str(), "otm") != 0 ||
      ::strstr(url.c_str(), "nativeroll") != 0 ||
      ::strstr(url.c_str(), "appodeal") != 0 ||
      ::strstr(url.c_str(), "adcolony") != 0 ||
      ::strstr(url.c_str(), "inneractive") != 0 ||
      ::strstr(url.c_str(), "inner-active") != 0 ||
      ::strstr(url.c_str(), "republer") != 0 ||
      ::strstr(url.c_str(), "media-today") != 0 ||
      ::strstr(url.c_str(), "mediatoday") != 0 ||
      ::strstr(dmp_profiling_info.url(), "CAESE") != 0 || // google
      ::strstr(url.c_str(), "ssp_user_id") != 0 ||
      ::strstr(url.c_str(), "sspuserid") != 0 ||
      ::strstr(url.c_str(), "phone") != 0 ||
      ::strstr(url.c_str(), "userbind") != 0 ||
      ::strstr(url.c_str(), "%40") != 0 ||
      ::strstr(url.c_str(), "@") != 0 ||
      ::strstr(url.c_str(), "rocket10") != 0 ||
      ::strstr(url.c_str(), "=between") != 0 ||
      ::strstr(url.c_str(), "=marketgid") != 0 ||
      ::strstr(url.c_str(), "cm.marketgid.com") != 0 ||
      ::strstr(url.c_str(), "=sape") != 0 ||
      ::strstr(url.c_str(), "aff_sub1=14") != 0 ||
      ::strstr(url.c_str(), "aff_sub1=15") != 0 ||
      ::strstr(url.c_str(), "aff_sub2=15") != 0 ||
      ::strstr(url.c_str(), "ga1.2.") != 0 ||
      ::strstr(url.c_str(), "mybee") != 0 ||
      ::strstr(url.c_str(), ".sync.upravel.com") != 0 ||
      ::strstr(url.c_str(), "kadam") != 0 ||
      (::strstr(url.c_str(), "adlmerge.com") != 0 && ::strstr(url.c_str(), "ssp_id=") != 0) ||
      (::strstr(url.c_str(), "rtb.com.ru") != 0 && ::strstr(url.c_str(), "sspKey=") != 0) ||
      (::strstr(url.c_str(), "ssp.adriver.ru") != 0 && ::strstr(url.c_str(), "ssp_id=") != 0) ||
      (::strstr(url.c_str(), "cm.marketgid.com") != 0 && ::strstr(url.c_str(), "dsp=") != 0) ||
      (::strstr(url.c_str(), "exchange.nativeads.com") != 0 && ::strstr(url.c_str(), "dsp=") != 0) ||
      (::strstr(url.c_str(), "sync.adkernel.com") != 0 && ::strstr(url.c_str(), "dsp=") != 0) ||
      (::strstr(url.c_str(), "x.fidelity-media.com") != 0 && ::strstr(url.c_str(), "dsp=") != 0) ||
      ::strstr(url.c_str(), "inappreqad") != 0 ||
      ::strstr(url.c_str(), "bidsw") != 0 ||
      ::strstr(url.c_str(), "ptoide.com/pnp/v1/notifications/") != 0 ||
      ::strstr(url.c_str(), "api.pubnative.net") != 0 ||
      ::strstr(url.c_str(), "avazunativeads.com") != 0 ||
      ::strstr(url.c_str(), "nexage.com/exchange/") != 0 ||
      ::strstr(url.c_str(), "advid=") != 0 ||
      ::strstr(url.c_str(), "rdid=") != 0 ||
      ::strstr(url.c_str(), "goid=") != 0 ||
      ::strstr(url.c_str(), "adid=") != 0 ||
      ::strstr(url.c_str(), "aafaid=") != 0 ||
      ::strstr(url.c_str(), "a.bitmango.com") != 0 ||
      ::strstr(url.c_str(), "pw.foreca.com") != 0 ||
      ::strstr(url.c_str(), "andadvid=") != 0 ||
      ::strstr(url.c_str(), "iosadvid=") != 0 ||
      ::strstr(url.c_str(), "google_adv_id=") != 0 ||
      ::strstr(url.c_str(), "google_adid=") != 0 ||
      ::strstr(url.c_str(), "googleadid") != 0 ||
      ::strstr(url.c_str(), "androidid=") != 0 ||
      ::strstr(url.c_str(), "android_id=") != 0 ||
      ::strstr(url.c_str(), "googleaid=") != 0
      ));

    if(debug_by_request_context)
    {
      Stream::Error ostr;
      ostr << /*"Request:" << std::endl <<
        "  version = " << dmp_profiling_info.version() << std::endl <<
        "  time = " << dmp_profiling_info.time() << std::endl <<
        "  source = " << dmp_profiling_info.source() << std::endl <<
        */
        "  external_user_id = " << dmp_profiling_info.external_user_id() << std::endl <<
        //"  bind_user_ids = " << dmp_profiling_info.bind_user_ids() << std::endl <<
        "  url = " << dmp_profiling_info.url() << std::endl
        /* <<
        "  keywords = " << dmp_profiling_info.keywords() << std::endl <<
        "  longitude = " << dmp_profiling_info.longitude() << std::endl <<
        "  latitude = " << dmp_profiling_info.latitude() << std::endl*/
        ;
      std::cerr << ostr.str() << std::endl;
    }

    // === Debug bind user ids ===

    if(dmp_profiling_info.bind_user_ids()[0])
    {
      Stream::Error ostr;
      ostr << "Request:" << std::endl <<
        "  bind_user_ids = " << dmp_profiling_info.bind_user_ids() << std::endl;
      std::cerr << ostr.str() << std::endl;
    }

    // fill request info
    std::string original_bind_user_id = dmp_profiling_info.bind_user_ids();
    std::optional<HTTP::BrowserAddress> referer;

    if(dmp_profiling_info.url()[0])
    {
      try
      {
        if(dmp_profiling_info.url()[0] == '/')
        {
          // can be https link, example /ib.isappcloud.com:443
          // try init without /
          try
          {
            referer = HTTP::BrowserAddress(String::SubString(dmp_profiling_info.url() + 1));
          }
          catch(const eh::Exception&)
          {}

          if(!referer)
          {
            // got uri without domain
            std::string use_url("dummy");
            use_url += dmp_profiling_info.url();
            referer = HTTP::BrowserAddress(use_url);
          }

          request_params.common_info.referer << referer->url();
        }
        else
        {
          referer = HTTP::BrowserAddress(String::SubString(dmp_profiling_info.url()));
          request_params.common_info.referer << referer->url();
        }

        if(!request_info.keywords.empty())
        {
          request_info.keywords += '\n';
        }
        std::string clickstream_kw("poadclickstream");
        clickstream_kw += '\n';
        request_info.keywords += clickstream_kw;
      }
      catch(const eh::Exception&)
      {
        // ignore invalid referer value
      }
    }

    // unpack bind key from referer if it have required suffix (bind_url_suffix_)
    if(original_bind_user_id.empty())
    {
      const String::SubString host = referer ?
        referer->host() : String::SubString();

      if(bind_url_suffix_.size() < host.size() &&
        host.compare(
          host.size() - bind_url_suffix_.size(),
          bind_url_suffix_.size(),
          bind_url_suffix_) == 0)
      {
        // host have next format: <id block>.sdc.beeline.ru
        // where <id block> have next format: x[a-z0-9]+([.]x[a-z0-9]+)*
        //
        String::SubString::SizeType id_block_end_pos = 0;

        while((id_block_end_pos = host.find('.', id_block_end_pos)) !=
          String::SubString::NPOS)
        {
          if(id_block_end_pos + 1 >= host.size() ||
            host[id_block_end_pos + 1] != 'x')
          {
            // skip part after first domain without x prefix (query too)
            break;
          }
          else
          {
            id_block_end_pos += 1;
          }
        }

        String::SubString id_block = id_block_end_pos != String::SubString::NPOS ?
          String::SubString(host.begin(), host.begin() + id_block_end_pos) :
          host;

        original_bind_user_id = id_block.str();
      }
    }

    request_info.keywords = dmp_profiling_info.keywords();

    std::string request_source(dmp_profiling_info.source());

    {
      auto sep_pos = request_source.find('/');
      if(sep_pos != std::string::npos)
      {
        request_source.resize(sep_pos);
      }
    }

    std::string prof_add("poad");
    prof_add += request_source;
    prof_add += "prof";

    if(!request_source.empty())
    {
      prof_add += '\n';
      prof_add += "poadprof";
    }

    if(!request_info.keywords.empty())
    {
      request_info.keywords += '\n';
      std::string bln_segment_kw("poadblnsegment");
      bln_segment_kw += '\n'; 
      request_info.keywords += bln_segment_kw;
    }

    request_info.keywords += prof_add;

    if(dmp_profiling_info.external_user_id()[0])
    {
      // split user ids
      split_external_user_id_(
        request_info.bind_user_ids,
        request_info.cohort2,
        String::SubString(dmp_profiling_info.external_user_id()),
        request_source,
        AES_SOURCE_MARKER);

      /*
      request_info.bind_user_ids.push_back(
        request_source + "/" + dmp_profiling_info.external_user_id());
      */
    }

    if(!original_bind_user_id.empty() && original_bind_user_id[0] == 'r')
    {
      request_info.bind_request_id = original_bind_user_id;
    }
    else
    {
      AdServer::Commons::dns_decode_external_user_ids(
        request_info.bind_user_ids,
        original_bind_user_id);
    }

    if(!request_info.bind_user_ids.empty())
    {
      std::string bind_source_kw;

      auto it = request_info.bind_user_ids.begin();
      if(it != request_info.bind_user_ids.end())
      {
        for(++it; it != request_info.bind_user_ids.end(); ++it)
        {
          auto sep_pos = it->find('/');
          if(sep_pos != std::string::npos)
          {
            bind_source_kw.assign(*it, 0, sep_pos);
            break;
          }
        }
      }

      if (!original_bind_user_id.empty())
      {
        std::string bind_add("poad");
        bind_add += bind_source_kw;
        bind_add += "bind";

        if(!bind_source_kw.empty())
        {
          bind_add += '\n';
          bind_add += "poadbind";
        }
        
        if(!request_info.keywords.empty())
        {
          request_info.keywords += '\n';
        }

        request_info.keywords += bind_add;
      }
    }

    if(dmp_profiling_info.longitude() != 0 || dmp_profiling_info.latitude() != 0)
    {
      try
      {
        const AdServer::CampaignSvcs::CoordDecimal COORD_FACTOR(
          false,
          10000000,
          0);

        FrontendCommons::CoordLocation_var coord_location =
          new FrontendCommons::CoordLocation();
        coord_location->longitude = AdServer::CampaignSvcs::CoordDecimal::div(
          AdServer::CampaignSvcs::CoordDecimal(
            dmp_profiling_info.longitude() < 0, std::abs(dmp_profiling_info.longitude()), 0),
          COORD_FACTOR);
        coord_location->latitude = AdServer::CampaignSvcs::CoordDecimal::div(
          AdServer::CampaignSvcs::CoordDecimal(
            dmp_profiling_info.latitude() < 0, std::abs(dmp_profiling_info.latitude()), 0),
          COORD_FACTOR);
        coord_location->accuracy = AdServer::CampaignSvcs::CoordDecimal(
          false,
          5000,
          0); // 5km

        request_info.coord_location.swap(coord_location);
      }
      catch(const eh::Exception&)
      {}

      if(request_info.coord_location.in())
      {
        for(unsigned long precision_i = 0;
          precision_i < sizeof(GET_PRECISIONS) / sizeof(GET_PRECISIONS[0]);
          ++precision_i)
        {
          const std::string keyword = generate_coord_keyword(
            request_info.coord_location->latitude,
            request_info.coord_location->longitude,
            GET_PRECISIONS[precision_i].precision,
            GET_PRECISIONS[precision_i].precision,
            GET_PRECISIONS[precision_i].precision_str);

          if(!request_info.keywords.empty())
          {
            request_info.keywords += '\n';
          }

          request_info.keywords += keyword;
        }
      }
    }

    // TODO : fill search

    // fill request_params
    request_params.disable_fraud_detection = false;
    request_params.common_info.log_as_test = false;
    request_params.need_debug_info = false;
    request_params.only_display_ad = false;
    request_params.profiling_type = AdServer::CampaignSvcs::PT_PROFILING_INFO;
    request_params.context_info.enabled_notice = false;
    request_params.context_info.profile_referer = false;
    // request_params.common_info.signed_user_id; // TO CHECK

    request_params.common_info.user_status = AdServer::CampaignSvcs::US_UNDEFINED;
    request_params.common_info.user_id = CorbaAlgs::pack_user_id(AdServer::Commons::UserId());

    request_params.common_info.track_user_id = request_params.common_info.user_id;
    request_params.common_info.test_request = false;
    request_params.common_info.colo_id = colo_id_;

    request_params.common_info.time = CorbaAlgs::pack_time(now);
    //request_params.context_info.time = CorbaAlgs::pack_time(Generics::Time(dmp_profiling_info.time()));
    request_params.common_info.set_cookie = false;
  }

  void
  RequestInfoFiller::split_external_user_id_(
    AdServer::Commons::ExternalUserIdArray& bind_user_ids,
    std::string& cohort2,
    const String::SubString& external_user_id,
    const String::SubString& primary_source,
    const String::SubString& secondary_source)
    noexcept
  {
    String::SubString::SizeType id_block_begin_pos = 0;
    String::SubString::SizeType id_block_end_pos = 0;
    unsigned int block_index = 0;

    while((id_block_end_pos = external_user_id.find(';', id_block_begin_pos)) !=
      String::SubString::NPOS)
    {
      String::SubString block(
        external_user_id.begin() + id_block_begin_pos,
        external_user_id.begin() + id_block_end_pos);

      if(!block.empty())
      {
        if(block.find('/') == String::SubString::NPOS)
        {
          if(block_index == 0 || block.size() < 50) // temporary fix for second 64 symbol id
          {
            bind_user_ids.push_back(
              (block_index == 0 ? primary_source : secondary_source).str() + "/" + block.str());

            if(block_index != 0 && cohort2.empty())
            {
              cohort2 = block.str();
            }
          }
        }
        else
        {
          bind_user_ids.push_back(block.str());
        }
      }

      id_block_begin_pos = id_block_end_pos + 1;
      ++block_index;
    }

    String::SubString block = external_user_id.substr(id_block_begin_pos);

    if(!block.empty())
    {
      if(block.find('/') == String::SubString::NPOS)
      {
        bind_user_ids.push_back(
          (block_index == 0 ? primary_source : secondary_source).str() + "/" + block.str());
      }
      else
      {
        bind_user_ids.push_back(block.str());
      }
    }
  }
}
}
