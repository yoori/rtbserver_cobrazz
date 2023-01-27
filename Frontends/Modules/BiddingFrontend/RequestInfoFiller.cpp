#include <algorithm>
#include <functional>
#include <math.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <String/AsciiStringManip.hpp>
#include <String/UTF8Case.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>
#include <Generics/Rand.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/JsonParamProcessor.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <Language/SegmentorManager/SegmentorManager.hpp>

#include <Frontends/FrontendCommons/OptOutManip.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "KeywordFormatter.hpp"
#include "RequestInfoFiller.hpp"

namespace Aspect
{
  const char BIDDING_FRONTEND[] = "BiddingFrontend";
}

namespace Request
{
  namespace Context
  {
    const String::SubString PUBLISHER_ACCOUNT_ID("aid");
    const String::SubString PUBLISHER_SITE_ID("sid");
    const String::SubString SOURCE_ID("src");
    const String::SubString RANDOM("random");
    const String::SubString FLAG("f");
    const String::SubString FORMAT("format");
    const String::SubString LOCATION_NAME("loc.name");
  }

  namespace Debug
  {
    const String::SubString EXPECTED_CCG("debug.ccg");
    const String::SubString CURRENT_TIME("debug.time");
    const String::SubString ADSLOTS_SIZE("debug.size");
    const String::SubString AD_SLOT("debug.adslot");

    namespace AdSlot
    {
      const String::SubString SIZE("size");
    }
  }

  namespace OpenRtb
  {
    const std::string STABLE_SOURCE("stable");

    const String::SubString REQUEST_ID("id");
    const String::SubString MAX_PROCESSING_TIME("tmax");
    const String::SubString IMP("imp");
    const String::SubString BADV("badv"); // ?
    const String::SubString SITE("site");
    const String::SubString APP("app");
    const String::SubString CURRENCY("cur");
    const String::SubString DEVICE("device");
    const String::SubString USER("user");
    const String::SubString BLOCKED_CATEGORIES("bcat");
    const String::SubString EXT("ext");
    const String::SubString EXT_TEST("is_test");
    const String::SubString EXT_SECURE("secure");
    const String::SubString EXT_UDI("udi");
    const String::SubString EXT_GAID("gaid");
    const String::SubString EXT_IDFA("idfa");
    const String::SubString TEST("test");
    const String::SubString PUBLISHER("publisher");

    const String::SubString IMP_ID("id");
    const String::SubString IMP_MIN_CPM_PRICE("bidfloor");
    const String::SubString IMP_MIN_CPM_PRICE_CURRENCY_CODE("bidfloorcur");
    const String::SubString IMP_SECURE("secure");

    // ext
    const String::SubString IMP_EXT("ext");
    const String::SubString IMP_EXT_TYPE("type");

    // banner
    const String::SubString IMP_BANNER("banner");
    const String::SubString IMP_BANNERS("banners");
    const String::SubString IMP_BANNER_POS("pos");
    const String::SubString IMP_BANNER_BLOCKED_CATEGORIES("battr");
    const String::SubString IMP_BANNER_EXT_BLOCKED_CATEGORIES("bcont");
    const String::SubString IMP_BANNER_EXT_MATCHING_AD("matching_ad_id");
    const String::SubString IMP_BANNER_FORMAT("format");

    const String::SubString IMP_BANNER_WIDTH("w");
    const String::SubString IMP_BANNER_HEIGHT("h");
    const String::SubString IMP_BANNER_EXT("ext");
    const String::SubString IMP_BANNER_EXT_TYPE("type");
    const String::SubString IMP_BANNER_EXT_FORMAT("fmt");
    const String::SubString IMP_BANNER_EXT_OVERLAY("overlay");
    const String::SubString IMP_BANNER_EXT_HPOS("hpos");

    // video
    const String::SubString IMP_VIDEO("video");
    const String::SubString IMP_VIDEO_WIDTH("w");
    const String::SubString IMP_VIDEO_HEIGHT("h");
    const String::SubString IMP_VIDEO_POS("pos");
    const String::SubString IMP_VIDEO_BLOCKED_CATEGORIES("battr");
    const String::SubString IMP_VIDEO_MIMES("mimes");
    const String::SubString IMP_VIDEO_MINDURATION("minduration");
    const String::SubString IMP_VIDEO_MAXDURATION("maxduration");
    const String::SubString IMP_VIDEO_PROTOCOL("protocol");
    const String::SubString IMP_VIDEO_PROTOCOLS("protocols");
    const String::SubString IMP_VIDEO_PLAYBACKMETHOD("playbackmethod");
    const String::SubString IMP_VIDEO_STARTDELAY("startdelay");
    const String::SubString IMP_VIDEO_LINEARITY("linearity");
    const String::SubString IMP_VIDEO_EXT("ext");
    const String::SubString IMP_VIDEO_EXT_ADTYPE("adtype");
    const String::SubString IMP_VIDEO_PLACEMENT("placement"); // v2.5
    const String::SubString IMP_VIDEO_API("api");

    const String::SubString SITE_DOMAIN("domain");
    const String::SubString SITE_PAGE("page");
    const String::SubString SITE_NAME("name");
    const String::SubString SITE_ID("id");
    const String::SubString SITE_EXT("ext");
    const String::SubString SITE_EXT_SECURE("ssl_enabled");
    const String::SubString SITE_EXT_PUID1("puid1");
    const String::SubString SITE_EXT_PUID2("puid2");
    const String::SubString SITE_KEYWORDS("keywords");
    const String::SubString SITE_PAGECAT("pagecat");
    const String::SubString SITE_SECTIONCAT("sectioncat");
    const String::SubString SITE_CAT("cat");
    const String::SubString SITE_SEARCH("search");
    const String::SubString SITE_REF("ref");
    const String::SubString SITE_REFERER("referer");
    const String::SubString SITE_REREFERER("rereferer");

    const String::SubString APP_ID("id");
    const String::SubString APP_NAME("name");
    const String::SubString APP_BUNDLE("bundle");
    const String::SubString APP_DOMAIN("domain");
    const String::SubString APP_STORE_URL("storeurl");
    const String::SubString APP_KEYWORDS("keywords");
    const String::SubString APP_PAGECAT("pagecat");
    const String::SubString APP_SECTIONCAT("sectioncat");
    const String::SubString APP_CAT("cat");

    // content
    const String::SubString CONTENT("content");
    const String::SubString CONTENT_KEYWORDS("keywords");
    const String::SubString CONTENT_TITLE("title");
    const String::SubString CONTENT_SERIES("series");
    const String::SubString CONTENT_SEASON("season");
    const String::SubString CONTENT_CAT("cat");

    // device
    const String::SubString DEVICE_IP("ip");
    const String::SubString DEVICE_IPv6("ipv6");
    const String::SubString DEVICE_USER_AGENT("ua");
    const String::SubString DEVICE_USER_DATA("userdata");
    const String::SubString DEVICE_IFA("ifa");
    const String::SubString DEVICE_DIDMD5("didmd5");
    const String::SubString DEVICE_DIDSHA1("didsha1");
    const String::SubString DEVICE_DPIDMD5("dpidmd5");
    const String::SubString DEVICE_DPISHA1("dpisha1");
    const String::SubString DEVICE_MACSHA1("macsha1");
    const String::SubString DEVICE_MACMD5("macmd5");
    const String::SubString DEVICE_LANGUAGE("language");
    const String::SubString DEVICE_GEO("geo");
    const String::SubString DEVICE_GEO_COUNTRY("country");
    const String::SubString DEVICE_GEO_REGION("region");
    const String::SubString DEVICE_GEO_CITY("city");

    // regs
    const String::SubString REGS("regs");
    const String::SubString REGS_COPPA("coppa");
    
    // pmp
    const String::SubString PMP("pmp");
    const String::SubString PMP_PRIVATE_AUCTION("private_auction");
    const String::SubString PMP_DEALS("deals");
    const String::SubString PMP_DEAL_ID("id");
    const String::SubString PMP_DEAL_CPM_PRICE("bidfloor");
    const String::SubString PMP_DEAL_CURRENCY_CODE("bidfloorcur");
    

    // user
    const String::SubString USER_ID("id");
    const String::SubString BUYER_ID("buyeruid");
    const String::SubString BUYER_ID2("buyerid");
    const String::SubString USER_YOB("yob");
    const String::SubString USER_GENDER("gender");
    const String::SubString USER_KEYWORDS("keywords");

    const String::SubString USER_DATA("data");
    const String::SubString USER_DATA_SEGMENT("segment");
    const String::SubString USER_SEGMENT_ID("id");
    const String::SubString USER_SEGMENT_NAME("name");
    const String::SubString USER_SEGMENT_VALUE("value");

    // user.ext
    const String::SubString USER_EXT("ext"); // user.ext
    const String::SubString USER_EXT_EIDS("eids"); // user.ext.eids
    const String::SubString USER_EXT_EIDS_SOURCE("source"); // user.ext.eids*.source
    const String::SubString USER_EXT_EIDS_UIDS("uids"); // user.ext.eids*.uids -> JsonUserEidUidParamProcessor
    const String::SubString USER_EXT_EIDS_UIDS_ID("id"); // user.ext.eids*.uids*.id
    const String::SubString USER_EXT_EIDS_UIDS_STABLEID("stableid"); // user.ext.eids*.uids*.stableid

    // publisher
    const String::SubString PUBLISHER_NAME("name");
    const String::SubString PUBLISHER_ID("id");
    const String::SubString PUBLISHER_CAT("cat");

    // producer
    const String::SubString PRODUCER("producer");
    const String::SubString PRODUCER_NAME("name");

    // ext
    const String::SubString EXT_CATEGORY("category");

    // ADSC-10919
    const String::SubString DEFAULT_BIDFLOORCUR_CURRENCY("usd");

    const String::SubString VIDEO_PLACEMENT_NONE("none");
    
    const String::SubString VIDEO_PLACEMENTS[] =
    {
      String::SubString("unknown"),
      String::SubString("instream"),
      String::SubString("inbanner"),
      String::SubString("inread"),  // ADSC-10951 for google comatibility
      String::SubString("infeed"),
      String::SubString("interstitial")
    };

    // ADSC-10933
    const String::SubString VIDEO_PLAYBACKMETHODS[] =
    {
      String::SubString("unknown"),
      String::SubString("autoplaysoundon"),
      String::SubString("autoplaysoundoff"),
      String::SubString("clicktoplay"),
      String::SubString("mouseover")
    };

    // ADSC-10918 Native ads

    const String::SubString NATIVE("native");
    const String::SubString NATIVE_REQUEST("request");
    const String::SubString NATIVE_VER("ver");
    const String::SubString NATIVE_PLACEMENT("plcmttype");
    const String::SubString NATIVE_ASSETS("assets");
    const String::SubString NATIVE_ASSET_ID("id");
    const String::SubString NATIVE_ASSET_REQUIRED("required");
    const String::SubString NATIVE_ASSET_TITLE("title");
    const String::SubString NATIVE_ASSET_DATA("data");
    const String::SubString NATIVE_ASSET_IMAGE("img");
    const String::SubString NATIVE_ASSET_VIDEO("video");
    const String::SubString NATIVE_ASSET_TITLE_LEN("len");
    const String::SubString NATIVE_ASSET_DATA_TYPE("type");
    const String::SubString NATIVE_ASSET_DATA_LEN("len");
    const String::SubString NATIVE_ASSET_IMAGE_TYPE("type");
    const String::SubString NATIVE_ASSET_IMAGE_WIDTH("w");
    const String::SubString NATIVE_ASSET_IMAGE_HEIGHT("h");
    const String::SubString NATIVE_ASSET_IMAGE_WMIN("wmin");
    const String::SubString NATIVE_ASSET_IMAGE_HMIN("hmin");
    const String::SubString NATIVE_ASSET_IMAGE_MIMES("mimes");
    const String::SubString NATIVE_ASSET_VIDEO_MIMES("mimes");
    const String::SubString NATIVE_ASSET_VIDEO_PROTOCOLS("protocols");
    const String::SubString NATIVE_ASSET_VIDEO_MIN_DURATION("minduration");
    const String::SubString NATIVE_ASSET_VIDEO_MAX_DURATION("maxduration");

    const String::SubString NATIVE_DATA_ASSET_TOKENS[] =
    {
      String::SubString("ADTITLE"),    // supported - title asset
      String::SubString("SPONSORED"),
      String::SubString("ALTTEXT"),    // supported - Descriptive text associated with the product 
      String::SubString("RATING"),
      String::SubString("LIKES"),
      String::SubString("DOWNLOADS"),
      String::SubString("PRICE"),
      String::SubString("SALEPRICE"),
      String::SubString("PHONE"),
      String::SubString("ADDRESS"),
      String::SubString("DESCRIPTION2"), // supported - Additional descriptive text
      String::SubString("DISPLAY_URL"),  // supported - Display url for the text ad.
      String::SubString("CTA")           // supported - CTA description
    };

    const String::SubString NATIVE_IMAGE_ASSET_TOKENS[] =
    {
      String::SubString(), // unknown type
      String::SubString("ADICON"),
      String::SubString("ADICON"),
      String::SubString("ADIMAGE"),
      String::SubString("ADIMAGE_SQUARE")
    };
    
    const String::SubString NATIVE_PLACEMENTS[] =
    {
      String::SubString("unknown"),
      String::SubString("infeed"),
      String::SubString("inread"),
      String::SubString("outside"),
      String::SubString("recommendation")
    };
  }

  namespace AppNexus
  {
    const String::SubString REQUEST("bid_request");

    const String::SubString REQUEST_DEBUGREQUESTED("debug_requested");
    const String::SubString REQUEST_TEST("test");
    const String::SubString REQUEST_TAGS("tags");
    const String::SubString REQUEST_BIDINFO("bid_info");
    const String::SubString REQUEST_MEMBERS("members");

    const String::SubString REQUEST_TAG_ID("id");
    const String::SubString REQUEST_TAG_SIZES("sizes");
    const String::SubString REQUEST_TAG_AUCTIONID("auction_id_64");
    const String::SubString REQUEST_TAG_MINCPMPRICE("reserve_price");
    const String::SubString REQUEST_TAG_CREATIVEFORMATS("creative_formats");
    const String::SubString REQUEST_TAG_ALLOWEDMEDIATYPES("allowed_media_types");
    const String::SubString REQUEST_TAG_ALLOWEDMEDIASUBTYPES("allowed_media_subtypes");
    const String::SubString REQUEST_TAG_PREDICTED_VIEW_RATE("predicted_view_rate");
    const String::SubString REQUEST_TAG_POS("position");
    const String::SubString REQUEST_TAG_SITE_ID("site_id");
    const String::SubString REQUEST_TAG_INVENTORY_SOURCE_ID("inventory_source_id");

    const String::SubString REQUEST_BIDINFO_USERID("user_id_64");
    const String::SubString REQUEST_BIDINFO_IP("ip_address");
    const String::SubString REQUEST_BIDINFO_USERAGENT("user_agent");
    const String::SubString REQUEST_BIDINFO_URL("url");
    const String::SubString REQUEST_BIDINFO_GENDER("gender");
    const String::SubString REQUEST_BIDINFO_AGE("age");
    const String::SubString REQUEST_BIDINFO_APPID("app_id");
    const String::SubString REQUEST_BIDINFO_SELLING_MEMBER_ID("selling_member_id");
    const String::SubString REQUEST_BIDINFO_NO_FLASH("no_flash");

    const String::SubString REQUEST_MEMBERS_ID("id");

    const String::SubString USER("user");
    const String::SubString USER_AGE("age");
    const String::SubString USER_GENDER("gender");
    const String::SubString INVENTORY_AUDITS("inventory_audits");
    const String::SubString CONTENT_CATEGORIES("content_categories");

  }
}

namespace CreativeCategory
{
  const std::string VPAID1("vpaid1");
  const std::string VPAID2("vpaid2");
}

namespace GooglePlay
{
  const String::SubString STORE_HOST("play.google.com");
  const String::SubString STORE_PATH("/store/apps/details");
  const String::SubString STORE_APP_ID("id");
}

namespace AppStore
{
  const String::SubString STORE_HOST("itunes.apple.com");
}

namespace AdServer
{

namespace Commons
{
  std::ostream&
  operator <<(std::ostream& os, const Optional<long>& ov)
    /*throw(eh::Exception)*/
  {
    return os << (ov.present() ? *ov : 0);
  }
}

namespace Bidding
{
  namespace Google
  {
    const String::SubString APPLICATION("google");
    const String::SubString APPLICATION_VERSION("1");
    const String::SubString APP_FORMAT("html");
    
    // https://storage.googleapis.com/adx-rtb-dictionaries/publisher-excludable-creative-attributes.txt
    // 
    const google::protobuf::int32 UP_EXPANDING[] = 
    {
      13, // ExpandingDirection: ExpandingUp
      17, // ExpandingDirection: ExpandingUpLeft
      18, // ExpandingDirection: ExpandingUpRight
      25  // ExpandingDirection: ExpandingUpOrDown
    };

    const google::protobuf::int32 DOWN_EXPANDING[] =
    {
      14, // ExpandingDirection: ExpandingDown
      19, // ExpandingDirection: ExpandingDownLeft
      20, // ExpandingDirection: ExpandingDownRight
      25  // ExpandingDirection: ExpandingUpOrDown
    };

    const google::protobuf::int32 LEFT_EXPANDING[] =
    {
      15, // ExpandingDirection: ExpandingLeft
      17, // ExpandingDirection: ExpandingUpLeft
      19, // ExpandingDirection: ExpandingDownLeft
      26  // ExpandingDirection: ExpandingLeftOrRight
    };

    const google::protobuf::int32 RIGHT_EXPANDING[] =
    {
      16,// ExpandingDirection: ExpandingRight
      18,// ExpandingDirection: ExpandingUpRight
      20,// ExpandingDirection: ExpandingDownRight
      26 // ExpandingDirection: ExpandingLeftOrRight
    };

    const google::protobuf::int32 CREATIVE_ATTR[] =
    {
      8, // CookieTargeting: IsCookieTargeted
      9 // UserInterestTargeting: IsUserInterestTargeted
    };

    // CreativeType: Html
    const ::google::protobuf::int32 CREATIVETYPE_HTML  = 21;
    // CreativeType: VastVideo
    const ::google::protobuf::int32 CREATIVETYPE_VIDEO = 22;

    // RichMediaCapabilityType: RichMediaCapabilityFlash
    const ::google::protobuf::int32 CREATIVETYPE_FLASH = 34;
    const String::SubString CREATIVETYPE_FLASH_STR("34");
    
    // RichMediaCapabilityType: RichMediaCapabilityNonSSL
    const ::google::protobuf::int32 NON_SECURE = 48;
    
    const AdServer::CampaignSvcs::ExtRevenueDecimal BIDFLOOR_MULTIPLIER =
      AdServer::CampaignSvcs::ExtRevenueDecimal::div(
        AdServer::CampaignSvcs::ExtRevenueDecimal(false, 100, 0),
        AdServer::CampaignSvcs::ExtRevenueDecimal(false, 1000000, 0));


    // ADSC-10919
    const String::SubString VIDEO_PLACEMENTS[] =
    {
      String::SubString("unknown"),
      String::SubString("instream"),
      String::SubString("interstitial"),
      String::SubString("inread"),
      String::SubString("audio"),
      String::SubString("inarticle")
    };
  }

  namespace
  {
    // AppNexus
    const String::SubString APPNEXUS_APP_FORMAT("js");
    const String::SubString APPNEXUS_APPLICATION("appnexus");
    const String::SubString APPNEXUS_APPLICATION_VERSION("1");

    // Openrtb
    const String::SubString OPENRTB_APPLICATION("openrtb");
    const String::SubString OPENRTB_APPLICATION_VERSION("1");
    const String::SubString OPENRTB_APP_FORMAT("html");
    const String::SubString OPENRTB_VAST_APP_FORMAT("vast");
    const String::SubString OPENRTB_NATIVE_APP_FORMAT("native");
    const String::SubString VAST_PROTOCOL_SIZE("vast");

    const std::size_t EXT_TAG_ID_MAX_LENGTH = 50;

    const AdServer::CampaignSvcs::RevenueDecimal OPENRTB_BIDFLOOR_MULTIPLIER(
      false, 100, 0);

    const AdServer::CampaignSvcs::RevenueDecimal YANDEX_BIDFLOOR_MULTIPLIER =
      AdServer::CampaignSvcs::RevenueDecimal::div(
        OPENRTB_BIDFLOOR_MULTIPLIER,
        AdServer::CampaignSvcs::RevenueDecimal(false, 1000, 0));

    const std::string NATIVE_TEXT_SIZE("1x1");
    const std::string POPUP_SIZE("300x300");
    const std::string RM_SIZE("rm");
    const std::string RM_DTO_SIZE("rmdto");
    const std::string RM_RICH_SIZE("rmrich");

    const std::string VIDEO_MIME_MP4("video/mp4");
    const std::string VIDEO_MIME_FLV("video/flv");
    const std::string VIDEO_MIME_MOV("video/mov");
    
    namespace PlatformNames
    {
      const std::string IPAD("ipad");
      const std::string IPHONE("iphone");
      const std::string IOS("ios");
    }

    // Utils

    // std::string case insensitive comparison
    bool char_caseless_compare(
      unsigned char lhs,
      unsigned char rhs)
    {
      return std::tolower(lhs) == std::tolower(rhs);
    }
    
    bool caseless_compare(
      const String::SubString& lhs,
      const String::SubString& rhs)
    {
      if (lhs.size() == rhs.size())
      {
        return std::equal(rhs.begin(), rhs.end(),
          lhs.begin(), char_caseless_compare);
      }
      return false;
    }

    // Get width & height from string size
    // (Appnexus size parsing for the REQ-3939)
    void size_decode(
      const std::string& size,
      unsigned long& width,
      unsigned long& height)
    {
      unsigned long s[] = { 0, 0 };
      int index = 0;
      for (std::string::const_iterator cit = size.begin();
        cit != size.end(); ++cit)
      {
        if (isdigit(*cit))
        {
          s[index] = 10 * s[index] + (*cit - '0');
        }
        else if (*cit == 'x' && index == 0)
        {
          index++;
        }
        else
        {
          return;
        }
      }
      if (index)
      {
        width = s[0];
        height = s[1];
      }
    }

    template <typename Seq, typename Arg>
    bool test_value_in_list(
      const Seq& seq,
      const Arg& value)
    {
      return
        std::find(
          seq.begin(), seq.end(), value) != seq.end();
    }

 
    template <typename T, size_t Count> 
    const T&
    get_value_from_seq(
      const T (&array)[Count],
      size_t index)
    {
      if (index >= Count)
      {
        static_assert(
          Count != 0, "Invalid array size");
        return array[0];
      }
      return array[index];
    }

    // Be careful, the first param should be an ordered sequence
    template <typename Seq, typename Arg>
    bool test_value_in_set(
      const Seq& seq,
      const Arg& value)
    {
      return
        std::binary_search(
          seq.begin(), seq.end(), value);
    }
    
    template <typename Seq, typename Arg, size_t Size>
    bool test_range_in_set(
      const Seq& seq,
      const Arg(&range) [Size])
    {
      for (size_t i = 0; i < Size; ++i)
      {
        if(
          std::binary_search(
            seq.begin(), seq.end(), range[i]))
        {
          return true;
        }
      }
      return false;
    }

    // ADSC-10918 Native ads

    typedef AdServer::Bidding::JsonAdSlotProcessingContext::Native Native;
    typedef Native::DataList NativeDataList;
    typedef Native::Data NativeData;
    typedef Native::ImageList NativeImageList;
    typedef Native::Image NativeImage;
    typedef Native::Video NativeVideo;

    bool find_main_image(
      const NativeImageList& image_assets,
      NativeImage& image)
    {
      auto image_it = std::find_if(
        image_assets.begin(),
        image_assets.end(),
        std::mem_fun_ref(&NativeImage::is_main));
      if (image_it != image_assets.end())
      {
        image = *image_it;
        return true;
      }
      return false;
    }

    void fill_token(
      AdServer::CampaignSvcs::CampaignManager::NativeDataToken&,
      const NativeData&)
    {}

    void fill_token(
      AdServer::CampaignSvcs::CampaignManager::NativeImageToken& res_token,
      const NativeImage& image_asset)
    {
      res_token.width = image_asset.width;
      res_token.height = image_asset.height;
    }
      
    template <typename ResultSeq, typename AssetSeq, size_t DictSize>
    bool
    fill_asset_tokens(
      ResultSeq& res_seq,
      const AssetSeq& assets,
      const String::SubString(&dict)[DictSize],
      long AssetSeq::value_type::* type_field,
      const char* /*token_prefix*/)
    {
      res_seq.length(assets.size());
      size_t i = 0;
      for (auto it = assets.begin(); it != assets.end(); ++it)
      {
        long asset_type = (*it).*type_field;

        if(asset_type < 0 || static_cast<size_t>(asset_type) >= DictSize)
          // unknown assets
        {
          if(it->required)
          {
            return false;
          }
        }
        else
        {
          if(dict[asset_type].empty())
          {
            if(it->required)
            {
              return false;
            }
          }
          else
          {
            res_seq[i].name << dict[asset_type];
            res_seq[i].required = it->required;
            fill_token(res_seq[i], *it);
            i++;
          }
        }
      }

      res_seq.length(i);

      return true;
    }

    // REQ-3939
    // Player VAST size
    void 
    fill_video_size(
      KeywordFormatter& kw_fmt,
      AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot_request,
      unsigned long width,
      unsigned long height)
    {
      std::string oss;
      oss.reserve(35);
      oss += "vastplayer";

      if (width)
      {
        ad_slot_request.video_width = width;

        char buf[15];
        size_t buf_size = String::StringManip::int_to_str(
          width, buf, sizeof(buf));

        String::SubString(buf, buf_size).append_to(oss);
      }
      oss += "x";
      if (height)
      {
        ad_slot_request.video_height = height;

        char buf[15];
        size_t buf_size = String::StringManip::int_to_str(
          height, buf, sizeof(buf));

        String::SubString(buf, buf_size).append_to(oss);
      }

      kw_fmt.add_keyword(oss);
    }
  }

  /*
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
  */

  void
  stringify_ifa(std::string& res, const String::SubString& bin_idfa)
  {
    res.reserve(bin_idfa.size() + 4);
    if(bin_idfa.size() == 16)
    {
      // reformat to XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
      res = String::StringManip::hex_encode(
        reinterpret_cast<const unsigned char*>(bin_idfa.data()), 4, false);
      res += '-';
      res += String::StringManip::hex_encode(
        reinterpret_cast<const unsigned char*>(bin_idfa.data()) + 4, 2, false);
      res += '-';
      res += String::StringManip::hex_encode(
        reinterpret_cast<const unsigned char*>(bin_idfa.data()) + 6, 2, false);
      res += '-';
      res += String::StringManip::hex_encode(
        reinterpret_cast<const unsigned char*>(bin_idfa.data()) + 8, 2, false);
      res += '-';
      res += String::StringManip::hex_encode(
        reinterpret_cast<const unsigned char*>(bin_idfa.data()) + 10, 6, false);
    }
    else
    {
      res = String::StringManip::hex_encode(
        reinterpret_cast<const unsigned char*>(bin_idfa.data()),
        bin_idfa.size(),
        false);
    }  
  }

  void
  json_serialize(std::ostream& ostr, JsonNode* node);

  void
  json_serialize(std::ostream& ostr, const JsonValue& value)
  {
    if(value.getTag() == JSON_TAG_NUMBER)
    {
      double f = value.toNumber();
      double int_part;
      if(fabs(modf(f, &int_part)) < 0.0000000001)
      {
        ostr << static_cast<long long>(int_part + 0.01);
      }
      else
      {
        ostr << f;
      }
    }
    else if(value.getTag() == JSON_TAG_STRING)
    {
      std::string escaped_str =
        String::StringManip::json_escape(String::SubString(value.toString()));
      ostr << "\"" << escaped_str << "\"";
    }
    else if(value.getTag() == JSON_TAG_BOOL)
    {
      ostr << (value.toBool() ? "true" : "false");
    }
    else if(value.getTag() == JSON_TAG_ARRAY)
    {
      ostr << "[";
      for(JsonIterator arr_it = begin(value); arr_it != end(value); ++arr_it)
      {
        if(arr_it != begin(value))
        {
          ostr << ", ";
        }
        json_serialize(ostr, arr_it->value);
      }
      ostr << "]";
    }
    else if(value.getTag() == JSON_TAG_OBJECT)
    {
      ostr << "{";
      for(JsonIterator arr_it = begin(value); arr_it != end(value); ++arr_it)
      {
        if(arr_it != begin(value))
        {
          ostr << ", ";
        }
        json_serialize(ostr, *arr_it);
      }
      ostr << "}";
    }
    else if(value.getTag() == JSON_TAG_NULL)
    {
      ostr << "null";
    }
  }

  void
  json_serialize(std::ostream& ostr, JsonNode* node)
  {
    ostr << "\"" << node->key << "\": ";
    json_serialize(ostr, node->value);
  }

  bool
  decode_google_advertising_id(
    std::string& advertising_id,
    const void* encrypted_advertising_id,
    unsigned long encrypted_advertising_id_size,
    const unsigned char* encryption_key,
    unsigned long encryption_key_size,
    const unsigned char* integrity_key,
    unsigned long integrity_key_size)
  {
    static const int INITIALIZATION_VECTOR_SIZE = 16;
    static const int CIPHER_TEXT_SIZE = 16;
    static const int SIGNATURE_SIZE = 4;
    static const int HASH_OUTPUT_SIZE = 20;
    const unsigned int ALL_LENGTH = INITIALIZATION_VECTOR_SIZE +
      CIPHER_TEXT_SIZE + SIGNATURE_SIZE;

    if(encrypted_advertising_id_size < ALL_LENGTH)
    {
      return false;
    }

    // Compute plaintext.
    const uint8_t* initialization_vector = reinterpret_cast<const uint8_t*>(
      encrypted_advertising_id);
    // len(ciphertext_bytes) = 8 bytes
    const uint8_t* ciphertext_bytes =
      initialization_vector + INITIALIZATION_VECTOR_SIZE;
    // signature = initialization_vector + INITIALIZATION_VECTOR_SIZE(16) + CIPHER_TEXT_SIZE(8)
    // len(signature) = 4 bytes
    const uint8_t* signature = ciphertext_bytes + CIPHER_TEXT_SIZE;

    uint32_t pad_size = HASH_OUTPUT_SIZE;
    uint8_t price_pad[HASH_OUTPUT_SIZE];

    // get price_pad using openssl/hmac.h
    if (!HMAC(
          EVP_sha1(),
          encryption_key,
          encryption_key_size,
          initialization_vector,
          INITIALIZATION_VECTOR_SIZE,
          price_pad,
          &pad_size))
    {
      return false;
    }

    uint8_t plaintext_bytes[CIPHER_TEXT_SIZE];
    for(int32_t i = 0; i < CIPHER_TEXT_SIZE; ++i)
    {
      plaintext_bytes[i] = price_pad[i] ^ ciphertext_bytes[i];
    }

    // Verify integrity bits.
    uint32_t integrity_hash_size = HASH_OUTPUT_SIZE;
    uint8_t integrity_hash[HASH_OUTPUT_SIZE];
    const int32_t INPUT_MESSAGE_SIZE = CIPHER_TEXT_SIZE + INITIALIZATION_VECTOR_SIZE;
    uint8_t input_message[INPUT_MESSAGE_SIZE];

    ::memcpy(input_message, plaintext_bytes, CIPHER_TEXT_SIZE);
    ::memcpy(input_message + CIPHER_TEXT_SIZE,
      initialization_vector,
      INITIALIZATION_VECTOR_SIZE);

    if(!HMAC(
         EVP_sha1(),
         integrity_key,
         integrity_key_size,
         input_message,
         INPUT_MESSAGE_SIZE,
         integrity_hash,
         &integrity_hash_size))
    {
      return false;
    }
    else if(::memcmp(integrity_hash, signature, SIGNATURE_SIZE) == 0)
    {
      stringify_ifa(
        advertising_id,
        String::SubString(reinterpret_cast<char*>(plaintext_bytes), CIPHER_TEXT_SIZE));

      return true;
    }

    // value = ntohllprice_pad ^ ciphertext_bytes)
    return false;
  }

  template<typename ContextType>
  class JsonStringArrayParamProcessor: public JsonParamProcessor<ContextType>
  {
  public:
    JsonStringArrayParamProcessor(
      StringList ContextType::* field)
       : field_(field)
    {}

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() != JSON_TAG_NULL)
      {
        if(value.getTag() != JSON_TAG_ARRAY)
        {
          Stream::Error ostr;
          ostr << "incorrect bcat element type";
          throw RequestInfoFiller::InvalidParamException(ostr);
        }

        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          if(it->value.getTag() == JSON_TAG_STRING || it->value.getTag() == JSON_TAG_NUMBER)
          {
            std::string str;
            it->value.toString(str, true);
            (context.*field_).emplace_back(str);
          }
        }
      }
    }

  protected:
    StringList ContextType::* field_;

  protected:
    virtual
    ~JsonStringArrayParamProcessor() noexcept
    {}
  };

  template<typename ContextType>
  class JsonSerializeParamProcessor:
    public AdServer::Commons::JsonParamProcessor<ContextType>
  {
  public:
    JsonSerializeParamProcessor(
      std::string ContextType::* field)
      : field_(field)
    {}

    virtual void
    process(
      ContextType& context,
      const JsonValue& value) const
    {
      std::ostringstream ostr;
      json_serialize(ostr, value);
      context.*field_ = ostr.str();
    }

  protected:
    virtual ~JsonSerializeParamProcessor() noexcept
    {}

  protected:
    std::string ContextType::* field_;
  };

  // JsonBannerFormatParamProcessor
  class JsonBannerFormatParamProcessor:
    public AdServer::Commons::JsonParamProcessor<JsonAdSlotProcessingContext::Banner>
  {
  public:
    JsonBannerFormatParamProcessor()
    {
      typedef ReferenceCounting::SmartPtr<
        AdServer::Commons::JsonCompositeParamProcessor<BannerFormat> >
        JsonCompositeParamProcessor_var;

      JsonCompositeParamProcessor_var banner_format_processor =
        new AdServer::Commons::JsonCompositeParamProcessor<
          JsonAdSlotProcessingContext::BannerFormat>();

      banner_format_processor->add_processor(
        Request::OpenRtb::IMP_BANNER_WIDTH,
        JsonBannerFormatParamProcessor_var(
          new AdServer::Commons::JsonStringParamProcessor<BannerFormat>(
            &BannerFormat::width)));

      banner_format_processor->add_processor(
        Request::OpenRtb::IMP_BANNER_HEIGHT,
        JsonBannerFormatParamProcessor_var( 
          new AdServer::Commons::JsonStringParamProcessor<BannerFormat>(
            &BannerFormat::height)));

      {
        JsonCompositeParamProcessor_var banner_format_ext_processor =
          new AdServer::Commons::JsonCompositeParamProcessor<BannerFormat>();

        banner_format_ext_processor->add_processor(
          Request::OpenRtb::IMP_BANNER_EXT_TYPE,
          JsonBannerFormatParamProcessor_var(
            new AdServer::Commons::JsonStringParamProcessor<BannerFormat>(
              &BannerFormat::ext_type)));
        banner_format_ext_processor->add_processor(
          Request::OpenRtb::IMP_BANNER_EXT_FORMAT,
          JsonBannerFormatParamProcessor_var(
            new AdServer::Commons::JsonStringParamProcessor<BannerFormat>(
              &BannerFormat::ext_format)));
      }

      banner_format_processor_ = banner_format_processor;
    }

    virtual void
    process(
      JsonAdSlotProcessingContext::Banner& banner,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          banner.formats.push_back(BannerFormat_var(new BannerFormat()));
          banner_format_processor_->process(*banner.formats.back(), it->value);
        }
      }
      else
      {
        banner.formats.push_back(BannerFormat_var(new BannerFormat()));
        banner_format_processor_->process(*banner.formats.back(), value);
      }
    }

  protected:
    typedef JsonAdSlotProcessingContext::BannerFormat BannerFormat;
    typedef JsonAdSlotProcessingContext::BannerFormat_var BannerFormat_var;

    typedef ReferenceCounting::SmartPtr<
      AdServer::Commons::JsonParamProcessor<JsonAdSlotProcessingContext::BannerFormat> >
      JsonBannerFormatParamProcessor_var;

  protected:
    JsonBannerFormatParamProcessor_var banner_format_processor_;
  };

  // JsonBannerParamProcessor
  class JsonBannerParamProcessor: public JsonParamProcessor<JsonAdSlotProcessingContext>
  {
  public:
    JsonBannerParamProcessor()
    {
      typedef ReferenceCounting::SmartPtr<
        AdServer::Commons::JsonCompositeParamProcessor<Banner> >
        JsonCompositeParamProcessor_var;

      JsonCompositeParamProcessor_var banner_processor =
        new AdServer::Commons::JsonCompositeParamProcessor<
          JsonAdSlotProcessingContext::Banner>();

      banner_format_processor_ = JsonBannerParamProcessor_var(
        new JsonBannerFormatParamProcessor());

      banner_processor->add_processor(
        Request::OpenRtb::IMP_BANNER_POS,
        JsonBannerParamProcessor_var( 
          new AdServer::Commons::JsonStringParamProcessor<Banner>(
            &Banner::pos)));

      banner_processor->add_processor(
        Request::OpenRtb::IMP_BANNER_FORMAT,
        banner_format_processor_);

      banner_processor->add_processor(
        Request::OpenRtb::IMP_BANNER_BLOCKED_CATEGORIES,
        JsonBannerParamProcessor_var(
          new AdServer::Commons::JsonStringArrayParamProcessor<
            Banner, StringList>(
              &Banner::exclude_categories)));

      {
        JsonCompositeParamProcessor_var banner_ext_processor =
          new AdServer::Commons::JsonCompositeParamProcessor<Banner>();

        banner_ext_processor->add_processor(
          Request::OpenRtb::IMP_BANNER_EXT_BLOCKED_CATEGORIES,
          JsonBannerParamProcessor_var(
            new AdServer::Commons::JsonStringArrayParamProcessor<
              Banner, StringList>(
                &Banner::exclude_categories)));
        banner_ext_processor->add_processor(
          Request::OpenRtb::IMP_BANNER_EXT_MATCHING_AD,
          JsonBannerParamProcessor_var(
            new AdServer::Commons::JsonArrayParamProcessor<Banner>(
              JsonBannerParamProcessor_var(
                new JsonSerializeParamProcessor<Banner>(
                  &Banner::matching_ad)),
              1U))); // process only first element

        JsonCompositeParamProcessor_var banner_ext_overlay_processor =
          new AdServer::Commons::JsonCompositeParamProcessor<Banner>();

        banner_ext_overlay_processor->add_processor(
          Request::OpenRtb::IMP_BANNER_EXT_HPOS,
          JsonBannerParamProcessor_var(
            new AdServer::Commons::JsonNumberParamProcessor<Banner, unsigned long>(
              &Banner::ext_hpos)));
        banner_ext_processor->add_processor(
          Request::OpenRtb::IMP_BANNER_EXT_OVERLAY,
          banner_ext_overlay_processor);

        banner_processor->add_processor(
          Request::OpenRtb::IMP_BANNER_EXT,
          banner_ext_processor);
      }

      banner_processor_ = banner_processor;
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::
        RequestParams& /*request_params*/,
      JsonAdSlotProcessingContext& ad_slot_context,
      const JsonValue& value) const
    {
      //const Banner* added_banner = 0;

      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          ad_slot_context.banners.push_back(Banner_var(new Banner));
          banner_processor_->process(*ad_slot_context.banners.back(), it->value);
          banner_format_processor_->process(*ad_slot_context.banners.back(), it->value);
          //added_banner = ad_slot_context.banners.back();
        }
      }
      else
      {
        ad_slot_context.banners.push_back(Banner_var(new Banner));
        banner_processor_->process(*ad_slot_context.banners.back(), value);
        banner_format_processor_->process(*ad_slot_context.banners.back(), value);
        //added_banner = ad_slot_context.banners.back();
      }

      /*
      if(added_banner && !added_banner->formats.empty())
      {
        if((ad_slot_context.banners.back()->default_format->width.empty() ||
            ad_slot_context.banners.back()->default_format->height.empty()) &&
          ad_slot_context.banners.back()->default_format->ext_type.empty() &&
          ad_slot_context.banners.back()->default_format->ext_format.empty())
      {
        added_banner->formats.pop_back();
      }
      */
    }

  protected:
    typedef JsonAdSlotProcessingContext::Banner Banner;
    typedef JsonAdSlotProcessingContext::Banner_var Banner_var;

    typedef ReferenceCounting::SmartPtr<
      AdServer::Commons::JsonParamProcessor<Banner> >
      JsonBannerParamProcessor_var;

  protected:
    JsonBannerParamProcessor_var banner_processor_;
    JsonBannerParamProcessor_var banner_format_processor_;
  };

  // JsonPmpParamProcessor
  class JsonPmpParamProcessor: public JsonParamProcessor<JsonAdSlotProcessingContext>
  {
    typedef JsonAdSlotProcessingContext::Deal Deal;
    
    typedef JsonParamProcessor<Deal> JsonDealParamProcessor;

    typedef ReferenceCounting::SmartPtr<JsonDealParamProcessor>
      JsonDealParamProcessor_var;

  public:
    JsonPmpParamProcessor()
    {
      typedef ReferenceCounting::SmartPtr<JsonCompositeParamProcessor<Deal> >
        JsonDealCompositeParamProcessor_var;
      
      JsonDealCompositeParamProcessor_var deal_processor =
        new JsonCompositeParamProcessor<
          Deal>();

      deal_processor->add_processor(
        Request::OpenRtb::PMP_DEAL_ID,
        JsonDealParamProcessor_var(
          new JsonContextStringParamProcessor<Deal>(
            &Deal::id)));

      deal_processor->add_processor(
        Request::OpenRtb::PMP_DEAL_CPM_PRICE,
        JsonDealParamProcessor_var(
          new JsonContextDoubleParamProcessor<
            Deal,
            AdServer::CampaignSvcs::RevenueDecimal>(
              &Deal::cpm_price,
              Generics::DMR_CEIL,
              AdServer::CampaignSvcs::RevenueDecimal::MAXIMUM)));

      deal_processor->add_processor(
        Request::OpenRtb::PMP_DEAL_CURRENCY_CODE,
        JsonDealParamProcessor_var(
          new JsonContextStringParamProcessor<Deal>(
            &Deal::currency_code)));

      deal_processor_ = deal_processor;
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      JsonAdSlotProcessingContext& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          process_deal_(request_params, context, it->value);
        }
      }
      else
      {
        process_deal_(request_params, context, value);
      }
    }

  private:
    void process_deal_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      JsonAdSlotProcessingContext& context,
      const JsonValue& value) const
    {
      Deal deal;
      deal_processor_->process(request_params, deal, value);
      
      context.deals.push_back(deal);
    }

 
  protected:
    JsonDealParamProcessor_var deal_processor_;
  };

  // segment element processor {"value" : "", "id" : "", "name" : ""}
  class JsonUserSegmentParamProcessor: public JsonParamProcessor<JsonProcessingContext>
  {
    typedef JsonProcessingContext::Segment Segment;
    typedef JsonParamProcessor<Segment> JsonSegmentParamProcessor;
    typedef ReferenceCounting::SmartPtr<
      JsonSegmentParamProcessor> JsonSegmentParamProcessor_var;

  public:
    JsonUserSegmentParamProcessor()
    {
      typedef ReferenceCounting::SmartPtr<JsonCompositeParamProcessor<Segment> >
        JsonSegmentCompositeParamProcessor_var;

      JsonSegmentCompositeParamProcessor_var segment_processor =
        new JsonCompositeParamProcessor<Segment>();

      segment_processor->add_processor(
        Request::OpenRtb::USER_SEGMENT_ID,
        JsonSegmentParamProcessor_var(
          new JsonContextStringParamProcessor<Segment>(&Segment::id)));

      segment_processor->add_processor(
        Request::OpenRtb::USER_SEGMENT_NAME,
        JsonSegmentParamProcessor_var(
          new JsonContextStringParamProcessor<Segment>(&Segment::name)));

      segment_processor->add_processor(
        Request::OpenRtb::USER_SEGMENT_VALUE,
        JsonSegmentParamProcessor_var(
          new JsonContextStringParamProcessor<Segment>(&Segment::value)));

      segment_processor_ = segment_processor;
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      JsonProcessingContext& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          process_segment_(request_params, context, it->value);
        }
      }
      else
      {
        process_segment_(request_params, context, value);
      }
    }

  private:
    void process_segment_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      JsonProcessingContext& context,
      const JsonValue& value) const
    {
      Segment segment;
      segment_processor_->process(request_params, segment, value);
      context.segments.emplace_back(std::move(segment));
    }

  protected:
    JsonSegmentParamProcessor_var segment_processor_;
  };

  // Native object processors
  typedef JsonCompositeParamProcessor<JsonAdSlotProcessingContext::Native>
    JsonNativeCompositeParamProcessor;

  typedef ReferenceCounting::SmartPtr<JsonNativeCompositeParamProcessor>
    JsonNativeCompositeParamProcessor_var;

  // Common Asset class
  template <typename Asset>
  class JsonNativeAssetProcessor :
    public JsonParamProcessor<JsonAdSlotProcessingContext::Native>
  {
  protected:

    typedef JsonParamProcessor<Asset> JsonAssetParamProcessor;

    typedef ReferenceCounting::SmartPtr<
      JsonAssetParamProcessor> JsonAssetParamProcessor_var;

    typedef std::vector<Asset> SeqType;

    typedef JsonCompositeParamProcessor<Asset>
      JsonAssetCompositeParamProcessor;

    typedef ReferenceCounting::SmartPtr<JsonAssetCompositeParamProcessor>
      JsonAssetCompositeParamProcessor_var;

    typedef JsonAdSlotProcessingContext::Native Native;

  public:

    JsonNativeAssetProcessor(
      SeqType Native::* seq_field):
      seq_field_(seq_field),
      processor_(new JsonCompositeParamProcessor<Asset>())
    {
      processor_->add_processor(
        Request::OpenRtb::NATIVE_ASSET_ID,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Asset, long>(&Asset::id)));
      
      processor_->add_processor(
        Request::OpenRtb::NATIVE_ASSET_REQUIRED,
        JsonAssetParamProcessor_var(
          new JsonContextBoolParamProcessor<Asset>(
            &Asset::required)));
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      Native& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_OBJECT)
      {
        Asset asset;
        processor_->process(request_params, asset, value);
        (context.*seq_field_).push_back(asset);
      }
    }

  protected:
    SeqType Native::* seq_field_;
    JsonAssetCompositeParamProcessor_var processor_;
  };

  // Title asset processor
  class JsonNativeTitleProcessor :
    public JsonNativeAssetProcessor<JsonAdSlotProcessingContext::Native::Data>
  {
    typedef Native::Data Data;
    
  public:
    JsonNativeTitleProcessor() :
      JsonNativeAssetProcessor<Data>(
        &Native::data_assets)
    {
      JsonAssetCompositeParamProcessor_var title_processor =
        new JsonAssetCompositeParamProcessor();

      title_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_TITLE_LEN,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Data, long>(&Data::len)));

      processor_->add_processor(
        Request::OpenRtb::NATIVE_ASSET_TITLE,
        title_processor);
    }
  };

  // Data asset processor
  
  class JsonNativeDataProcessor :
    public JsonNativeAssetProcessor<JsonAdSlotProcessingContext::Native::Data>
  {
    typedef Native::Data Data;
    
  public:
    JsonNativeDataProcessor() :
      JsonNativeAssetProcessor<Data>(
        &Native::data_assets)
    {
      JsonAssetCompositeParamProcessor_var data_processor =
        new JsonAssetCompositeParamProcessor();

      data_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_DATA_TYPE,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Data, long>(&Data::data_type)));
      
      data_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_DATA_LEN,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Data, long>(&Data::len)));

      processor_->add_processor(
        Request::OpenRtb::NATIVE_ASSET_DATA,
        data_processor);
    }

  };
  
  // Image asset processor
  
  class JsonNativeImageProcessor :
    public JsonNativeAssetProcessor<JsonAdSlotProcessingContext::Native::Image>
  {
    typedef Native::Image Image;

  public:
    JsonNativeImageProcessor()
      : JsonNativeAssetProcessor<Image>(&Native::image_assets)      
    {
      JsonAssetCompositeParamProcessor_var image_processor =
        new JsonAssetCompositeParamProcessor();

      image_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_IMAGE_TYPE,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Image, long>(&Image::image_type)));

      image_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_IMAGE_WMIN,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Image, long>(&Image::width)));

      image_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_IMAGE_WIDTH,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Image, long>(&Image::width)));

      image_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_IMAGE_HMIN,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Image, long>(&Image::height)));

      image_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_IMAGE_HEIGHT,
        JsonAssetParamProcessor_var(
          new JsonContextNumberParamProcessor<
          Image, long>(&Image::height)));

      image_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_IMAGE_MIMES,
        JsonAssetParamProcessor_var(
          new JsonContextStringArrayParamProcessor<
            Image,
            JsonAdSlotProcessingContext::StringSet>(
            &Image::mimes)));
      
      processor_->add_processor(
        Request::OpenRtb::NATIVE_ASSET_IMAGE,
        image_processor);

    }
  };


  // Video asset processor
  
  class JsonNativeVideoProcessor :
    public JsonNativeAssetProcessor<JsonAdSlotProcessingContext::Native::Video>
  {
    typedef Native::Video Video;

  public:
    JsonNativeVideoProcessor() :
      JsonNativeAssetProcessor<Video>(
        &Native::video_assets)   
    {
      JsonAssetCompositeParamProcessor_var video_processor =
        new JsonAssetCompositeParamProcessor();

      video_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_VIDEO_PROTOCOLS,
        JsonAssetParamProcessor_var(
          new JsonContextNumberArrayStateParamProcessor<
            Video,
            JsonAdSlotProcessingContext::ULSetStateHolder>(
            &Video::protocols)));

      video_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_VIDEO_MIMES,
        JsonAssetParamProcessor_var(
          new JsonContextStringArrayParamProcessor<
            Video,
            JsonAdSlotProcessingContext::StringSet>(
            &Video::mimes)));

      video_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_VIDEO_MIN_DURATION,
        JsonAssetParamProcessor_var(
          new JsonContextNumberStateParamProcessor<
            Video,
            JsonAdSlotProcessingContext::ULValueStateHolder>(
            &Video::min_duration)));

      video_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSET_VIDEO_MIN_DURATION,
        JsonAssetParamProcessor_var(
          new JsonContextNumberStateParamProcessor<
            Video,
            JsonAdSlotProcessingContext::ULValueStateHolder>(
            &Video::max_duration)));
      
      processor_->add_processor(
        Request::OpenRtb::NATIVE_ASSET_VIDEO,
        video_processor);
    }
  };

  // Native assets processor
  class JsonAssetProcessor :
    public JsonParamProcessor<JsonAdSlotProcessingContext::Native>
  {
    typedef JsonAdSlotProcessingContext::Native Native;
    typedef JsonParamProcessor<Native> JsonAssetParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonAssetParamProcessor>
      JsonAssetParamProcessor_var;
    
    typedef Generics::GnuHashTable<
        Generics::SubStringHashAdapter,
        JsonAssetParamProcessor_var> JsonAssetParamProcessorMap;

  public:
    JsonAssetProcessor()
    {
      sub_processors_.insert(
        std::make_pair(
          Request::OpenRtb::NATIVE_ASSET_TITLE,
          JsonAssetParamProcessor_var(
            new JsonNativeTitleProcessor())));

      sub_processors_.insert(
        std::make_pair(
          Request::OpenRtb::NATIVE_ASSET_DATA,
          JsonAssetParamProcessor_var(
            new JsonNativeDataProcessor())));

      sub_processors_.insert(
        std::make_pair(
          Request::OpenRtb::NATIVE_ASSET_IMAGE,
          JsonAssetParamProcessor_var(
            new JsonNativeImageProcessor())));

      sub_processors_.insert(
        std::make_pair(
          Request::OpenRtb::NATIVE_ASSET_VIDEO,
          JsonAssetParamProcessor_var(
            new JsonNativeVideoProcessor())));
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::RequestParams&
        request_params,
      JsonAdSlotProcessingContext::Native& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_OBJECT)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          JsonAssetParamProcessorMap::const_iterator processor_it =
            sub_processors_.find(String::SubString(it->key));
          if(processor_it != sub_processors_.end())
          {
            processor_it->second->process(request_params, context, value);
          }
        }
      }
    }

  protected:
    virtual ~JsonAssetProcessor() noexcept = default;

  protected:
    JsonAssetParamProcessorMap sub_processors_;
  };

  // Native object processor
  class JsonNativeProcessor: public JsonParamProcessor<JsonAdSlotProcessingContext>
  {
    typedef JsonAdSlotProcessingContext::Native Native;
    
    typedef JsonParamProcessor<Native> JsonNativeParamProcessor;

    typedef ReferenceCounting::SmartPtr<JsonNativeParamProcessor>
      JsonNativeParamProcessor_var;

    class JsonRequestCompositeParamProcessor: public JsonCompositeParamProcessor<Native>
    {
      virtual void
      process(
        AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
        Native& context,
        const JsonValue& value) const
      {
        if(value.getTag() == JSON_TAG_OBJECT)
        {
          process(request_params, context, value);
        }
        else if(value.getTag() == JSON_TAG_STRING)
        {
          std::string str;
          value.toString(str, true);
          Generics::ArrayAutoPtr<char> str_holder(str.size() + 1);
          ::strcpy(str_holder.get(), str.c_str());

          JsonValue root_value;
          JsonAllocator json_allocator;
          char* parse_end = str_holder.get();
          JsonParseStatus status = json_parse(
            str_holder.get(), &parse_end, &root_value, json_allocator);
          JsonTag root_tag = root_value.getTag();
          if(status == JSON_PARSE_OK && root_tag == JSON_TAG_OBJECT)
          {
            process(request_params, context, root_value);
          }
          else
          {
            throw RequestInfoFiller::InvalidParamException(
              "incorrect native.request tag type");
          }
        }
        else
        {
          throw RequestInfoFiller::InvalidParamException(
            "incorrect native.request tag type");
        }
      }
    };

    typedef JsonCompositeParamProcessor<Native>
      JsonNativeCompositeParamProcessor;

    typedef ReferenceCounting::SmartPtr<JsonNativeCompositeParamProcessor>
      JsonNativeCompositeParamProcessor_var;

  public:
    JsonNativeProcessor()
    {
      JsonNativeCompositeParamProcessor_var processor =
        new JsonNativeCompositeParamProcessor();      

      JsonNativeCompositeParamProcessor_var native_processor =
        new JsonNativeCompositeParamProcessor();

      add_native_processor_(native_processor);

      {
        // MobFox & Yandex case
        JsonNativeCompositeParamProcessor_var mobfox_processor =
          new JsonNativeCompositeParamProcessor();

        add_native_processor_(mobfox_processor);

        native_processor->add_processor(
          Request::OpenRtb::NATIVE,
          mobfox_processor);
      }

      processor->add_processor(
        Request::OpenRtb::NATIVE_REQUEST,
        native_processor);
        
      processor_ = processor;
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::
        RequestParams& request_params,
      JsonAdSlotProcessingContext& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_OBJECT)
      {
        context.native = JsonAdSlotProcessingContext::Native_var(new Native);
        processor_->process(request_params, *context.native, value);
      }
      else
      {
        throw RequestInfoFiller::InvalidParamException(
          "incorrect native tag type");
      }
    }

  private:
    void
    add_native_processor_(
      JsonNativeCompositeParamProcessor* native_processor)
    {
      native_processor->add_processor(
        Request::OpenRtb::NATIVE_VER,
        JsonNativeParamProcessor_var(
          new JsonContextStringParamProcessor<Native>(
            &Native::version)));

      native_processor->add_processor(
        Request::OpenRtb::NATIVE_PLACEMENT,
        JsonNativeParamProcessor_var(
          new JsonContextNumberParamProcessor<Native,
            AdServer::Commons::Optional<long> >(
            &Native::placement)));

      JsonNativeParamProcessor_var asset_processor =
        new JsonAssetProcessor();
        
      native_processor->add_processor(
        Request::OpenRtb::NATIVE_ASSETS,
        JsonNativeParamProcessor_var(
          new JsonArrayParamProcessor<Native>(asset_processor)));
    }

  protected:
    JsonNativeParamProcessor_var processor_;
  };

  // user.ext.eids.uids element processor {"id" : ""}
  class JsonUserEidUidParamProcessor: public JsonParamProcessor<JsonProcessingContext::UserEid>
  {
    typedef JsonParamProcessor<JsonProcessingContext::UserEidUid>
      JsonEidUidParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonEidUidParamProcessor>
      JsonEidUidParamProcessor_var;

  public:
    JsonUserEidUidParamProcessor()
    {
      typedef ReferenceCounting::SmartPtr<
        JsonCompositeParamProcessor<JsonProcessingContext::UserEidUid> >
        JsonUserEidUidCompositeParamProcessor_var;

      JsonUserEidUidCompositeParamProcessor_var user_eid_uid_processor =
        new JsonCompositeParamProcessor<JsonProcessingContext::UserEidUid>();

      user_eid_uid_processor->add_processor(
        Request::OpenRtb::USER_EXT_EIDS_UIDS_STABLEID,
        JsonEidUidParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext::UserEidUid>(
            &JsonProcessingContext::UserEidUid::stable_id)));

      user_eid_uid_processor->add_processor(
        Request::OpenRtb::USER_EXT_EIDS_UIDS_ID,
        JsonEidUidParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext::UserEidUid>(
            &JsonProcessingContext::UserEidUid::id)));

      user_eid_uid_processor_ = user_eid_uid_processor;
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      JsonProcessingContext::UserEid& user_eid,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          process_uid_(request_params, user_eid, it->value);
        }
      }
      else
      {
        process_uid_(request_params, user_eid, value);
      }
    }

  private:
    void process_uid_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      JsonProcessingContext::UserEid& user_eid,
      const JsonValue& value) const
    {
      JsonProcessingContext::UserEidUid user_eid_uid;
      user_eid_uid_processor_->process(request_params, user_eid_uid, value);
      user_eid.uids.emplace_back(std::move(user_eid_uid));
    }

  protected:
    JsonEidUidParamProcessor_var user_eid_uid_processor_;
  };

  // user.ext.eids element processor {"source" : "", "uids" : [{"id" : ""}]}
  class JsonUserEidParamProcessor: public JsonParamProcessor<JsonProcessingContext>
  {
    typedef JsonProcessingContext::UserEid UserEid;
    typedef JsonParamProcessor<UserEid> JsonEidParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonEidParamProcessor>
      JsonEidParamProcessor_var;

  public:
    JsonUserEidParamProcessor()
    {
      typedef ReferenceCounting::SmartPtr<JsonCompositeParamProcessor<UserEid> >
        JsonUserEidCompositeParamProcessor_var;

      JsonUserEidCompositeParamProcessor_var user_eid_processor =
        new JsonCompositeParamProcessor<UserEid>();

      user_eid_processor->add_processor(
        Request::OpenRtb::USER_EXT_EIDS_SOURCE,
        JsonEidParamProcessor_var(
          new JsonContextStringParamProcessor<UserEid>(&UserEid::source)));

      JsonEidParamProcessor_var user_eid_uid_processor = new JsonUserEidUidParamProcessor();

      user_eid_processor->add_processor(
        Request::OpenRtb::USER_EXT_EIDS_UIDS,
        JsonEidParamProcessor_var(
          new JsonArrayParamProcessor<UserEid>(user_eid_uid_processor)));

      user_eid_processor_ = user_eid_processor;
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      JsonProcessingContext& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          process_user_eid_(request_params, context, it->value);
        }
      }
      else
      {
        process_user_eid_(request_params, context, value);
      }
    }

  private:
    void process_user_eid_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      JsonProcessingContext& context,
      const JsonValue& value) const
    {
      UserEid user_eid;
      user_eid_processor_->process(request_params, user_eid, value);
      context.user_eids.emplace_back(std::move(user_eid));
    }

  protected:
    JsonEidParamProcessor_var user_eid_processor_;
  };

  // JsonImpParamProcessor
  class JsonImpParamProcessor: public JsonParamProcessor<JsonProcessingContext>
  {
  public:
    JsonImpParamProcessor()
    {
      // init imp element processor
      JsonAdSlotCompositeParamProcessor_var processor =
        new JsonAdSlotCompositeParamProcessor();

      processor->add_processor(
        Request::OpenRtb::IMP_ID,
        JsonAdSlotParamProcessor_var(
          new JsonContextStringParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::id)));

      processor->add_processor(
        Request::OpenRtb::IMP_MIN_CPM_PRICE,
        JsonAdSlotParamProcessor_var(
          new JsonContextDoubleParamProcessor<
            JsonAdSlotProcessingContext,
            AdServer::CampaignSvcs::RevenueDecimal>(
              &JsonAdSlotProcessingContext::min_cpm_price,
              Generics::DMR_CEIL,
              AdServer::CampaignSvcs::RevenueDecimal::MAXIMUM)));

      processor->add_processor(
        Request::OpenRtb::IMP_MIN_CPM_PRICE_CURRENCY_CODE,
        JsonAdSlotParamProcessor_var(
          new JsonContextStringParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::min_cpm_price_currency_code)));

      processor->add_processor(
        Request::OpenRtb::IMP_SECURE,
        JsonAdSlotParamProcessor_var(
          new JsonContextBoolParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::secure)));

      // init imp::ext::type processor
      JsonAdSlotCompositeParamProcessor_var imp_ext_processor =
        new JsonAdSlotCompositeParamProcessor();

      imp_ext_processor->add_processor(
        Request::OpenRtb::IMP_EXT_TYPE,
        JsonAdSlotParamProcessor_var(
          new JsonContextStringParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::imp_ext_type)));

      processor->add_processor(
        Request::OpenRtb::IMP_EXT,
        imp_ext_processor);

      // banner & banners processor
      JsonAdSlotParamProcessor_var banner_processor = new JsonBannerParamProcessor();

      processor->add_processor(
        Request::OpenRtb::IMP_BANNER,
        JsonAdSlotParamProcessor_var(
          new JsonBannerParamProcessor()));

      processor->add_processor(
        Request::OpenRtb::IMP_BANNERS,
        JsonAdSlotParamProcessor_var(
          new JsonArrayParamProcessor<JsonAdSlotProcessingContext>(
            banner_processor)));

      // init video element
      JsonAdSlotCompositeParamProcessor_var video_processor =
        new JsonAdSlotCompositeParamProcessor(
          &JsonAdSlotProcessingContext::video);

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_WIDTH,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULValueStateHolder>(
              &JsonAdSlotProcessingContext::video_width)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_HEIGHT,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULValueStateHolder>(
              &JsonAdSlotProcessingContext::video_height)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_POS,
        JsonAdSlotParamProcessor_var(
          new JsonContextStringParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::video_pos)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_MIMES,
        JsonAdSlotParamProcessor_var(
          new JsonContextStringArrayParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::StringSet>(
            &JsonAdSlotProcessingContext::video_mimes)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_MINDURATION,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULValueStateHolder>(
            &JsonAdSlotProcessingContext::video_min_duration)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_MAXDURATION,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULValueStateHolder>(
            &JsonAdSlotProcessingContext::video_max_duration)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_PROTOCOL,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberArrayStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULSetStateHolder>(
            &JsonAdSlotProcessingContext::video_protocols)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_PROTOCOLS,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberArrayStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULSetStateHolder>(
            &JsonAdSlotProcessingContext::video_protocols)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_PLAYBACKMETHOD,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberArrayStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULSetStateHolder>(
            &JsonAdSlotProcessingContext::video_playbackmethods)));

     video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_API,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberArrayStateParamProcessor<
            JsonAdSlotProcessingContext,
          JsonAdSlotProcessingContext::ULSetStateHolder>(
            &JsonAdSlotProcessingContext::video_api)));

     video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_STARTDELAY,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULValueStateHolder>(
            &JsonAdSlotProcessingContext::video_start_delay)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_LINEARITY,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberStateParamProcessor<
            JsonAdSlotProcessingContext,
            JsonAdSlotProcessingContext::ULValueStateHolder>(
            &JsonAdSlotProcessingContext::video_linearity)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_PLACEMENT,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberParamProcessor<
            JsonAdSlotProcessingContext,
            AdServer::Commons::Optional<long> >(
            &JsonAdSlotProcessingContext::video_placement)));

      video_processor->add_processor(
        Request::OpenRtb::IMP_VIDEO_BLOCKED_CATEGORIES,
        JsonAdSlotParamProcessor_var(
          new JsonStringArrayParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::video_exclude_categories)));

      {
        JsonAdSlotCompositeParamProcessor_var video_ext_processor =
          new JsonAdSlotCompositeParamProcessor();
        video_ext_processor->add_processor(
          Request::OpenRtb::IMP_VIDEO_EXT_ADTYPE,
          JsonAdSlotParamProcessor_var(
            new JsonContextNumberStateParamProcessor<
              JsonAdSlotProcessingContext,
              JsonAdSlotProcessingContext::ULValueStateHolder>(
              &JsonAdSlotProcessingContext::video_adtype)));
        video_processor->add_processor(
          Request::OpenRtb::IMP_VIDEO_EXT,
          video_ext_processor);
      }

      processor->add_processor(
        Request::OpenRtb::IMP_VIDEO,
        video_processor);

      // Pmp object
      {
        JsonAdSlotCompositeParamProcessor_var pmp_processor =
          new JsonAdSlotCompositeParamProcessor();

        pmp_processor->add_processor(
          Request::OpenRtb::PMP_PRIVATE_AUCTION,
          JsonAdSlotParamProcessor_var(
            new JsonContextNumberParamProcessor<
              JsonAdSlotProcessingContext,
              AdServer::Commons::Optional<long> >(
                &JsonAdSlotProcessingContext::private_auction)));

        JsonAdSlotParamProcessor_var deal_processor =
          new JsonPmpParamProcessor();
        
        pmp_processor->add_processor(
          Request::OpenRtb::PMP_DEALS,
          JsonAdSlotParamProcessor_var(
            new JsonArrayParamProcessor<JsonAdSlotProcessingContext>(
              deal_processor)));
        
        processor->add_processor(
          Request::OpenRtb::PMP,
          pmp_processor);
      }

      // Native object
      {
        processor->add_processor(
          Request::OpenRtb::NATIVE,
          JsonAdSlotParamProcessor_var(
            new JsonNativeProcessor())); 
      }
      
      processor_ = processor;
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::
        RequestParams& request_params,
      JsonProcessingContext& context,
      const JsonValue& value) const
    {
      if(value.getTag() != JSON_TAG_ARRAY)
      {
        Stream::Error ostr;
        ostr << "incorrect imp element type";
        throw RequestInfoFiller::InvalidParamException(ostr);
      }

      for(JsonIterator it = begin(value); it != end(value); ++it)
      {
        JsonAdSlotProcessingContext ad_slot_context;
        processor_->process(request_params, ad_slot_context, it->value);
        context.ad_slots.push_back(ad_slot_context);
      }
    }

  protected:
    typedef JsonParamProcessor<JsonAdSlotProcessingContext>
      JsonAdSlotParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonAdSlotParamProcessor>
      JsonAdSlotParamProcessor_var;
    typedef JsonCompositeParamProcessor<JsonAdSlotProcessingContext>
      JsonAdSlotCompositeParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonAdSlotCompositeParamProcessor>
      JsonAdSlotCompositeParamProcessor_var;

  protected:
    JsonAdSlotParamProcessor_var processor_;
  };

  template <typename Context>
  class InventoryAuditsParamProcessor: public JsonParamProcessor<Context>
  {
  public:
    InventoryAuditsParamProcessor()
    {
      JsonCompositeRequestParamProcessor_var_ inventory_audits_processor =
        new JsonCompositeRequestParamProcessor_();

      inventory_audits_processor->add_processor(
        Request::AppNexus::CONTENT_CATEGORIES,
        JsonRequestParamProcessor_var_(
          new JsonContextNumberArrayParamProcessor<
            Context,
            ULongSet>(
            &Context::inventory_audits_content_categories)));

      processor_ = inventory_audits_processor;
    }

  protected:
    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      Context& context,
      const JsonValue& value) const
    {
      if(value.getTag() != JSON_TAG_ARRAY)
      {
        Stream::Error ostr;
        ostr << "incorrect imp element type";
        throw RequestInfoFiller::InvalidParamException(ostr);
      }

      for(JsonIterator it = begin(value); it != end(value); ++it)
      {
        processor_->process(request_params, context, it->value);
      }
    }

  protected:
    typedef JsonParamProcessor<Context>
      JsonRequestParamProcessor_;
    typedef ReferenceCounting::SmartPtr<JsonRequestParamProcessor_>
      JsonRequestParamProcessor_var_;
    typedef JsonCompositeParamProcessor<Context>
      JsonCompositeRequestParamProcessor_;
    typedef ReferenceCounting::SmartPtr<JsonCompositeRequestParamProcessor_>
      JsonCompositeRequestParamProcessor_var_;

    JsonRequestParamProcessor_var_ processor_;

  protected:
    virtual
    ~InventoryAuditsParamProcessor() noexcept
    {}
  };

  typedef InventoryAuditsParamProcessor<JsonProcessingContext>
    BidInfoInventoryAuditsParamProcessor;

  typedef InventoryAuditsParamProcessor<JsonAdSlotProcessingContext>
    AdSlotInventoryAuditsParamProcessor;

  class AppNexusTagParamProcessor: public JsonParamProcessor<JsonProcessingContext>
  {
  public:
    AppNexusTagParamProcessor()
    {
      // init imp element processor
      JsonAdSlotCompositeParamProcessor_var processor =
        new JsonAdSlotCompositeParamProcessor();

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_ID,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberParamProcessor<JsonAdSlotProcessingContext,
            AdServer::Commons::Optional<long> >(
            &JsonAdSlotProcessingContext::slot_id)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_AUCTIONID,
        JsonAdSlotParamProcessor_var(
          new JsonContextStringParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::id)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_SIZES,
        JsonAdSlotParamProcessor_var(
          new JsonStringArrayParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::appnexus_sizes)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_MINCPMPRICE,
        JsonAdSlotParamProcessor_var(
          new JsonContextDoubleParamProcessor<
            JsonAdSlotProcessingContext,
            AdServer::CampaignSvcs::RevenueDecimal>(
              &JsonAdSlotProcessingContext::min_cpm_price,
              Generics::DMR_CEIL)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_POS,
        JsonAdSlotParamProcessor_var(
          new JsonContextStringParamProcessor<JsonAdSlotProcessingContext>(
            &JsonAdSlotProcessingContext::appnexus_pos)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_SITE_ID,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberParamProcessor<JsonAdSlotProcessingContext,
            AdServer::Commons::Optional<long> >(
            &JsonAdSlotProcessingContext::site_id)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_INVENTORY_SOURCE_ID,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberParamProcessor<JsonAdSlotProcessingContext,
            AdServer::Commons::Optional<long> >(
            &JsonAdSlotProcessingContext::inventory_source_id)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_ALLOWEDMEDIATYPES,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberArrayParamProcessor<JsonAdSlotProcessingContext,
            ULongSet>(
            &JsonAdSlotProcessingContext::allowed_media_types)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_ALLOWEDMEDIASUBTYPES,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberArrayParamProcessor<JsonAdSlotProcessingContext,
            ULongSet>(
            &JsonAdSlotProcessingContext::allowed_media_subtypes)));

      processor->add_processor(
        Request::AppNexus::REQUEST_TAG_PREDICTED_VIEW_RATE,
        JsonAdSlotParamProcessor_var(
          new JsonContextNumberParamProcessor<JsonAdSlotProcessingContext, double>(
            &JsonAdSlotProcessingContext::predicted_view_rate)));

      processor->add_processor(
        Request::AppNexus::INVENTORY_AUDITS,
        JsonAdSlotParamProcessor_var(
          new AdSlotInventoryAuditsParamProcessor()));

      processor_ = processor;
    }

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::
        RequestParams& request_params,
      JsonProcessingContext& context,
      const JsonValue& value) const
    {
      if(value.getTag() != JSON_TAG_ARRAY)
      {
        Stream::Error ostr;
        ostr << "incorrect imp element type";
        throw RequestInfoFiller::InvalidParamException(ostr);
      }

      for(JsonIterator it = begin(value); it != end(value); ++it)
      {
        JsonAdSlotProcessingContext ad_slot_context;
        processor_->process(request_params, ad_slot_context, it->value);
        context.ad_slots.push_back(ad_slot_context);
      }
    }

  protected:
    typedef JsonParamProcessor<JsonAdSlotProcessingContext>
      JsonAdSlotParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonAdSlotParamProcessor>
      JsonAdSlotParamProcessor_var;
    typedef JsonCompositeParamProcessor<JsonAdSlotProcessingContext>
      JsonAdSlotCompositeParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonAdSlotCompositeParamProcessor>
      JsonAdSlotCompositeParamProcessor_var;

  protected:
    JsonAdSlotParamProcessor_var processor_;
  };

  class AppNexusMemberIdParamProcessor:
    public JsonParamProcessor<JsonProcessingContext>
  {
  public:
    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::
      RequestParams& /*request_params*/,
      JsonProcessingContext& context,
      const JsonValue& value) const
    {
      if(value.getTag() != JSON_TAG_NUMBER)
      {
        Stream::Error ostr;
        ostr << "incorrect members id element type";
        throw RequestInfoFiller::InvalidParamException(ostr);
      }

      context.member_ids.insert(value.toNumber());
    }
  };

  // RequestInfoFiller
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    unsigned long colo_id,
    CommonModule* common_module,
    const char* geo_ip_path,
    const char* user_agent_filter_path,
    const ExternalUserIdSet& skip_external_ids,
    bool ip_logging_enabled,
    const char* ip_salt,
    const SourceMap& sources,
    bool enable_profile_referer,
    const AccountTraitsById& account_traits)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      colo_id_(colo_id),
      skip_external_ids_(skip_external_ids),
      common_module_(ReferenceCounting::add_ref(common_module)),
      ip_logging_enabled_(ip_logging_enabled),
      ip_salt_(ip_salt),
      sources_(sources),
      enable_profile_referer_(enable_profile_referer),
      account_traits_(account_traits),
      adxml_request_info_filler_(new AdXmlRequestInfoFiller(this))
  {
    static const char* FUN = "RequestInfoFiller::RequestInfoFiller()";

    source_mapping_.emplace(Generics::SubStringHashAdapter(String::SubString("MegafonID")), "megafon-stableid");

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
          Aspect::BIDDING_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    if(user_agent_filter_path[0])
    {
      user_agent_matcher_.init(user_agent_filter_path);
    }

    init_param_processors_();

    // initialize OpenRTB json processors
    JsonCompositeRequestParamProcessor_var root_processor =
      new JsonCompositeRequestParamProcessor();

    root_processor->add_processor(
      Request::OpenRtb::REQUEST_ID,
      JsonRequestParamProcessor_var(
        new JsonContextStringParamProcessor<JsonProcessingContext>(
          &JsonProcessingContext::request_id)));

    root_processor->add_processor(
      Request::OpenRtb::CURRENCY,
      JsonRequestParamProcessor_var(
        new JsonStringArrayParamProcessor<JsonProcessingContext>(
          &JsonProcessingContext::currencies)));

    root_processor->add_processor(
      Request::OpenRtb::IMP,
      JsonRequestParamProcessor_var(new JsonImpParamProcessor()));

    root_processor->add_processor(
      Request::OpenRtb::BLOCKED_CATEGORIES,
      JsonRequestParamProcessor_var(
        new JsonStringArrayParamProcessor<JsonProcessingContext>(
          &JsonProcessingContext::exclude_categories)));

    root_processor->add_processor(
      Request::OpenRtb::TEST,
      JsonRequestParamProcessor_var(
        new JsonContextBoolParamProcessor<JsonProcessingContext>(
          &JsonProcessingContext::test)));

    {
      JsonCompositeRequestParamProcessor_var site_processor =
        new JsonCompositeRequestParamProcessor(
          &JsonProcessingContext::site);

      site_processor->add_processor(
        Request::OpenRtb::SITE_PAGE,
        JsonRequestParamProcessor_var(
          new JsonContextURLParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_page)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_NAME,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_name)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_DOMAIN,
        JsonRequestParamProcessor_var(
          new JsonContextURLParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_domain)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_ID,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_id)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_SEARCH,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_search)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_REF,
        JsonRequestParamProcessor_var(
          new JsonContextURLParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_ref)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_REFERER,
        JsonRequestParamProcessor_var(
          new JsonContextURLParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_referer)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_REREFERER,
        JsonRequestParamProcessor_var(
          new JsonContextURLParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_rereferer)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_KEYWORDS,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_keywords)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_PAGECAT,
        JsonRequestParamProcessor_var(
          new JsonStringArrayParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_pagecat)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_SECTIONCAT,
        JsonRequestParamProcessor_var(
          new JsonStringArrayParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_sectioncat)));

      site_processor->add_processor(
        Request::OpenRtb::SITE_CAT,
        JsonRequestParamProcessor_var(
          new JsonStringArrayParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_cat)));

      {
        JsonCompositeRequestParamProcessor_var site_ext_processor =
          new JsonCompositeRequestParamProcessor();

        site_ext_processor->add_processor(
          Request::OpenRtb::SITE_EXT_SECURE,
          JsonRequestParamProcessor_var(
            new JsonContextBoolParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::secure)));

        site_ext_processor->add_processor(
          Request::OpenRtb::SITE_EXT_PUID1,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::puid1)));

        site_ext_processor->add_processor(
          Request::OpenRtb::SITE_EXT_PUID2,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::puid2)));

        site_processor->add_processor(
          Request::OpenRtb::SITE_EXT,
          site_ext_processor);
      }

      // content
      {
        JsonCompositeRequestParamProcessor_var site_content_processor =
          new JsonCompositeRequestParamProcessor(
            &JsonProcessingContext::site_content);

        site_content_processor->add_processor(
          Request::OpenRtb::CONTENT_KEYWORDS,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_keywords)));

        site_content_processor->add_processor(
          Request::OpenRtb::CONTENT_TITLE,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_title)));

        site_content_processor->add_processor(
          Request::OpenRtb::CONTENT_SERIES,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_series)));

        site_content_processor->add_processor(
          Request::OpenRtb::CONTENT_SEASON,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_season)));

        site_content_processor->add_processor(
          Request::OpenRtb::CONTENT_CAT,
          JsonRequestParamProcessor_var(
            new JsonStringArrayParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_cat)));

        // producer
        {
          JsonCompositeRequestParamProcessor_var site_content_producer_processor =
            new JsonCompositeRequestParamProcessor(
              &JsonProcessingContext::site_content_producer);

         site_content_producer_processor->add_processor(
          Request::OpenRtb::PRODUCER_NAME,
          JsonRequestParamProcessor_var(
            new JsonContextStringArrayParamProcessor<
              JsonProcessingContext,
              StringList>(
              &JsonProcessingContext::content_producer_name)));

          site_content_processor->add_processor(
            Request::OpenRtb::PRODUCER,
            site_content_producer_processor);
        }

        site_processor->add_processor(
          Request::OpenRtb::CONTENT,
          site_content_processor);
      }

      // publisher
      {
        JsonCompositeRequestParamProcessor_var site_publisher_processor =
          new JsonCompositeRequestParamProcessor(
            &JsonProcessingContext::site_publisher);

        site_publisher_processor->add_processor(
          Request::OpenRtb::PUBLISHER_NAME,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::publisher_name)));

        site_publisher_processor->add_processor(
          Request::OpenRtb::PUBLISHER_ID,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::publisher_id)));

        site_publisher_processor->add_processor(
          Request::OpenRtb::PUBLISHER_CAT,
          JsonRequestParamProcessor_var(
            new JsonStringArrayParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::publisher_cat)));

        site_processor->add_processor(
          Request::OpenRtb::PUBLISHER,
          site_publisher_processor);
      }

      root_processor->add_processor(
        Request::OpenRtb::SITE,
        site_processor);
    }

    {
      JsonCompositeRequestParamProcessor_var app_processor =
        new JsonCompositeRequestParamProcessor(
          &JsonProcessingContext::app);

      app_processor->add_processor(
        Request::OpenRtb::APP_ID,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_id)));

      app_processor->add_processor(
        Request::OpenRtb::APP_NAME,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_name)));

      app_processor->add_processor(
        Request::OpenRtb::APP_BUNDLE,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_bundle)));

      app_processor->add_processor(
        Request::OpenRtb::APP_DOMAIN,
        JsonRequestParamProcessor_var(
          new JsonContextURLParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_domain)));

      app_processor->add_processor(
        Request::OpenRtb::APP_STORE_URL,
        JsonRequestParamProcessor_var(
          new JsonContextURLParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_store_url)));

      app_processor->add_processor(
        Request::OpenRtb::APP_KEYWORDS,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_keywords)));

      app_processor->add_processor(
        Request::OpenRtb::APP_PAGECAT,
        JsonRequestParamProcessor_var(
          new JsonStringArrayParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_pagecat)));

      app_processor->add_processor(
        Request::OpenRtb::APP_SECTIONCAT,
        JsonRequestParamProcessor_var(
          new JsonStringArrayParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_sectioncat)));

      app_processor->add_processor(
        Request::OpenRtb::APP_CAT,
        JsonRequestParamProcessor_var(
          new JsonStringArrayParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_cat)));


      // content
      {
        JsonCompositeRequestParamProcessor_var app_content_processor =
          new JsonCompositeRequestParamProcessor(
            &JsonProcessingContext::app_content);

        app_content_processor->add_processor(
          Request::OpenRtb::CONTENT_KEYWORDS,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_keywords)));

        app_content_processor->add_processor(
          Request::OpenRtb::CONTENT_TITLE,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_title)));

        app_content_processor->add_processor(
          Request::OpenRtb::CONTENT_SERIES,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_series)));

        app_content_processor->add_processor(
          Request::OpenRtb::CONTENT_SEASON,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_season)));

        app_content_processor->add_processor(
          Request::OpenRtb::CONTENT_CAT,
          JsonRequestParamProcessor_var(
            new JsonStringArrayParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::content_cat)));

        // producer
        {
          JsonCompositeRequestParamProcessor_var app_content_producer_processor =
            new JsonCompositeRequestParamProcessor(
              &JsonProcessingContext::app_content_producer);

         app_content_producer_processor->add_processor(
          Request::OpenRtb::PRODUCER_NAME,
          JsonRequestParamProcessor_var(
            new JsonContextStringArrayParamProcessor<
              JsonProcessingContext,
              StringList>(
              &JsonProcessingContext::content_producer_name)));

          app_content_processor->add_processor(
            Request::OpenRtb::PRODUCER,
            app_content_producer_processor);
        }

        app_processor->add_processor(
          Request::OpenRtb::CONTENT,
          app_content_processor);

      }

      // publisher
      {
        JsonCompositeRequestParamProcessor_var app_publisher_processor =
          new JsonCompositeRequestParamProcessor(
            &JsonProcessingContext::app_publisher);

        app_publisher_processor->add_processor(
          Request::OpenRtb::PUBLISHER_NAME,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::publisher_name)));

        app_publisher_processor->add_processor(
          Request::OpenRtb::PUBLISHER_ID,
          JsonRequestParamProcessor_var(
            new JsonContextStringParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::publisher_id)));

        app_publisher_processor->add_processor(
          Request::OpenRtb::PUBLISHER_CAT,
          JsonRequestParamProcessor_var(
            new JsonStringArrayParamProcessor<JsonProcessingContext>(
              &JsonProcessingContext::publisher_cat)));

        app_processor->add_processor(
          Request::OpenRtb::PUBLISHER,
          app_publisher_processor);
      }

      root_processor->add_processor(
        Request::OpenRtb::APP,
        app_processor);
    }

    {
      JsonCompositeRequestParamProcessor_var device_processor =
        new JsonCompositeRequestParamProcessor();

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_IP,
        &JsonProcessingContext::ip);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_IPv6,
        &JsonProcessingContext::ipv6);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_USER_AGENT,
        &JsonProcessingContext::user_agent);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_USER_DATA,
        &JsonProcessingContext::user_id);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_IFA,
        &JsonProcessingContext::ifa);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_DIDMD5,
        &JsonProcessingContext::didmd5);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_DIDSHA1,
        &JsonProcessingContext::didsha1);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_DPIDMD5,
        &JsonProcessingContext::dpidmd5);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_DPISHA1,
        &JsonProcessingContext::dpisha1);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_MACSHA1,
        &JsonProcessingContext::macsha1);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_MACMD5,
        &JsonProcessingContext::macmd5);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_LANGUAGE,
        &JsonProcessingContext::language);

      JsonCompositeRequestParamProcessor_var device_geo_processor =
        new JsonCompositeRequestParamProcessor();

      device_geo_processor->add_processor(
        Request::OpenRtb::DEVICE_GEO_COUNTRY,
        &JsonProcessingContext::ssp_country);

      device_geo_processor->add_processor(
        Request::OpenRtb::DEVICE_GEO_REGION,
        &JsonProcessingContext::ssp_region);

      device_geo_processor->add_processor(
        Request::OpenRtb::DEVICE_GEO_CITY,
        &JsonProcessingContext::ssp_city);

      device_processor->add_processor(
        Request::OpenRtb::DEVICE_GEO,
        device_geo_processor);

      root_processor->add_processor(
        Request::OpenRtb::DEVICE,
        device_processor);
    }

    {
      JsonCompositeRequestParamProcessor_var user_processor =
        new JsonCompositeRequestParamProcessor(&JsonProcessingContext::user);

      user_processor->add_processor(
        Request::OpenRtb::USER_ID,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::external_user_id)));

      user_processor->add_processor(
        Request::OpenRtb::BUYER_ID,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::user_id)));

      user_processor->add_processor(
        Request::OpenRtb::BUYER_ID2,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::user_id)));

      user_processor->add_processor(
        Request::OpenRtb::USER_KEYWORDS,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::user_keywords)));

      user_processor->add_processor(
        Request::OpenRtb::USER_GENDER,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::user_gender)));

      user_processor->add_processor(
        Request::OpenRtb::USER_YOB,
        JsonRequestParamProcessor_var(
          new JsonContextNumberParamProcessor<JsonProcessingContext, unsigned long>(
            &JsonProcessingContext::user_yob)));

      JsonCompositeRequestParamProcessor_var user_data_processor =
        new JsonCompositeRequestParamProcessor(&JsonProcessingContext::user);

      user_data_processor->add_processor(
        Request::OpenRtb::USER_DATA_SEGMENT,
        JsonRequestParamProcessor_var(new JsonUserSegmentParamProcessor()));

      user_processor->add_processor(
        Request::OpenRtb::USER_DATA,
        JsonRequestParamProcessor_var(
          new JsonArrayParamProcessor<JsonProcessingContext>(user_data_processor))
        );

      JsonCompositeRequestParamProcessor_var user_ext_processor =
        new JsonCompositeRequestParamProcessor();

      // user.ext.eids
      JsonRequestParamProcessor_var user_ext_eid_processor =
        new JsonUserEidParamProcessor();

      user_ext_processor->add_processor(
        Request::OpenRtb::USER_EXT_EIDS,
        JsonRequestParamProcessor_var(
          new JsonArrayParamProcessor<JsonProcessingContext>(user_ext_eid_processor))
        );

      user_processor->add_processor(Request::OpenRtb::USER_EXT, user_ext_processor);

      {
        JsonCompositeRequestParamProcessor_var ext_processor =
          new JsonCompositeRequestParamProcessor();

        ext_processor->add_processor(
          Request::OpenRtb::USER_EXT_EIDS,
          JsonRequestParamProcessor_var(
            new JsonArrayParamProcessor<JsonProcessingContext>(
              JsonRequestParamProcessor_var(new JsonUserEidParamProcessor()))));
        
        user_processor->add_processor(Request::OpenRtb::USER_EXT, ext_processor);
      }

      root_processor->add_processor(Request::OpenRtb::USER, user_processor);
    }

    {
      // regs.coppa
      
      JsonCompositeRequestParamProcessor_var regs_processor =
        new JsonCompositeRequestParamProcessor();
      
      regs_processor->add_processor(
        Request::OpenRtb::REGS_COPPA,
        JsonRequestParamProcessor_var(
          new JsonContextBoolParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::regs_coppa)));
      
      root_processor->add_processor(
        Request::OpenRtb::REGS,
        regs_processor);
    }

    {
      JsonCompositeRequestParamProcessor_var ext_processor =
        new JsonCompositeRequestParamProcessor();

      ext_processor->add_processor(
        Request::OpenRtb::EXT_TEST,
        JsonRequestParamProcessor_var(
          new JsonContextBoolParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::test)));
 
      ext_processor->add_processor(
        Request::OpenRtb::EXT_SECURE,
        JsonRequestParamProcessor_var(
          new JsonContextBoolParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::secure)));
      {
        JsonCompositeRequestParamProcessor_var ext_udi_processor =
          new JsonCompositeRequestParamProcessor();

        ext_udi_processor->add_processor(
          Request::OpenRtb::EXT_IDFA,
          &JsonProcessingContext::ifa);
      
        ext_udi_processor->add_processor(
          Request::OpenRtb::EXT_GAID,
          &JsonProcessingContext::ifa);
        
        ext_processor->add_processor(
          Request::OpenRtb::EXT_UDI,
          ext_udi_processor);
      }

      ext_processor->add_processor(
        Request::OpenRtb::EXT_CATEGORY,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::required_category)));

      root_processor->add_processor(
        Request::OpenRtb::EXT,
        ext_processor);
    }

    json_root_processor_ = root_processor;

    init_appnexus_processors_();
  }

  AdXmlRequestInfoFiller*
  RequestInfoFiller::adxml_request_info_filler() noexcept
  {
    return adxml_request_info_filler_.get();
  }

  bool
  RequestInfoFiller::parse_debug_size_param_(
    DebugAdSlotSizeMap& debug_sizes,
    const String::SubString& name,
    const std::string& value) const
    noexcept
  {
    const String::SubString::SizeType adslot_length =
      Request::Debug::AD_SLOT.length();
    if (name.compare(0, adslot_length, Request::Debug::AD_SLOT) != 0)
    {
      return false;
    }

    const String::SubString::SizeType dot_pos = name.find('.', adslot_length);
    if (dot_pos == String::SubString::NPOS)
    {
      return false;
    }

    unsigned long si;
    if(!String::StringManip::str_to_int(String::SubString(
      name.substr(adslot_length, dot_pos - adslot_length)), si))
    {
      return false;
    }

    const String::SubString param_name(name.substr(dot_pos + 1));

    if(name.compare(dot_pos + 1,
        Request::Debug::AdSlot::SIZE.length(),
        Request::Debug::AdSlot::SIZE) == 0)
    {
      debug_sizes.insert(std::make_pair(--si, value));
      return true;
    }

    return false;
  }

  void
  RequestInfoFiller::fill(
    RequestInfo& request_info,
    const FCGI::HttpRequest& request,
    const Generics::Time& now) const
    noexcept
  {
    const HTTP::ParamList& params = request.params();

    for(HTTP::ParamList::const_iterator it = params.begin();
        it != params.end(); ++it)
    {
      if (!parse_debug_size_param_(
            request_info.debug_sizes, it->name, it->value))
      {
        ParamProcessorMap::const_iterator param_it =
          param_processors_.find(it->name);

        if(param_it != param_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      }
    }

    if(request_info.current_time == Generics::Time::ZERO)
    {
      request_info.current_time = now;
    }

    if(request_info.random >= CampaignSvcs::RANDOM_PARAM_MAX)
    {
      request_info.random = Generics::safe_rand(
        CampaignSvcs::RANDOM_PARAM_MAX);
    }
  }

  void
  RequestInfoFiller::fill_by_google_request(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    RequestInfo& request_info,
    std::string& keywords,
    GoogleAdSlotContextArray& as_slots_context,
    const ::Google::BidRequest& bid_request) const
    /*throw(InvalidParamException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill_by_google_request()";

    request_params.context_info.client << Google::APPLICATION;
    request_params.context_info.client_version << Google::APPLICATION_VERSION;
    request_params.common_info.request_type = AdServer::CampaignSvcs::AR_GOOGLE;

    // Common data

    init_request_param(request_params, request_info);

    Stream::Stack<16> ip_str;
 
    if (bid_request.has_ip() && bid_request.ip().size() == 3)
    {
      const std::string& ip = bid_request.ip();
      unsigned ip_byte1 = static_cast<unsigned char>(ip[0]);
      unsigned ip_byte2 = static_cast<unsigned char>(ip[1]);
      unsigned ip_byte3 = static_cast<unsigned char>(ip[2]);
      ip_str << ip_byte1 << "." << ip_byte2 << "." << ip_byte3 << ".0";
    }

    fill_by_ip(
      request_info,
      request_params,
      ip_str.str());

    request_params.common_info.user_status = static_cast<CORBA::ULong>(
      AdServer::CampaignSvcs::US_UNDEFINED);

    bool application = false;
    if (bid_request.has_mobile())
    {
      const ::Google::BidRequest_Mobile& mobile = bid_request.mobile();
      application = mobile.has_is_app() && mobile.is_app();
    }

    if(bid_request.has_user_agent() || application)
    {
      fill_by_user_agent(
        request_params,
        request_info,
        bid_request.user_agent(),
        request_info.filter_request,
        application);
    }

    if(bid_request.has_cookie_age_seconds() &&
       (bid_request.cookie_age_seconds() >= 0))
    {
      request_info.user_create_time = std::max(
        request_info.current_time - bid_request.cookie_age_seconds(),
        Generics::Time::ONE_DAY);
    }

    // Matching data
    KeywordFormatter kw_fmt("goog");

    kw_fmt.add_keyword(MatchKeywords::RTBREQ);

    if(!request_info.source_id.empty())
    {
      kw_fmt.add_dict_keyword(MatchKeywords::REQ, String::SubString());
    }

    /*
    if(bid_request.has_user_demographic())
    {
      const ::Google::BidRequest_UserDemographic& user_demographic =
        bid_request.user_demographic();
        
      if (user_demographic.has_gender() &&
        user_demographic.gender() !=
          ::Google::BidRequest_UserDemographic::UNKNOWN )
      {
        std::string gender = (
          bid_request.user_demographic().gender() ==
            ::Google::BidRequest_UserDemographic::MALE ?
          "male" : "female");
        kw_fmt.add_gender(gender);
      }

      if (user_demographic.age_high() - user_demographic.age_low() <= 10)
      {
        for (int age = user_demographic.age_low();
          age <= user_demographic.age_high();
             age++)
        {
          kw_fmt.add_age(age);
        }
      }
    }
    */

    for(int vt_kw_i = 0;
      vt_kw_i < bid_request.detected_vertical_size();
        ++vt_kw_i)
    {
      kw_fmt.add_dict_keyword(
        MatchKeywords::VT,
        bid_request.detected_vertical(vt_kw_i).id());
    }


    for(int dcl_kw_i = 0;
      dcl_kw_i < bid_request.detected_content_label_size();
        ++dcl_kw_i)
    {
      kw_fmt.add_dict_keyword(
        MatchKeywords::DCL,
        bid_request.detected_content_label(dcl_kw_i));
    }

    if(!ip_str.str().empty())
    {
      kw_fmt.add_ip(String::SubString(ip_str.str()));
    }

    int seller_id = 0;
    std::string anonymous_id;
    if (bid_request.has_seller_network_id())
    {
      seller_id = bid_request.seller_network_id();
    }

    if (bid_request.has_anonymous_id())
    {
      anonymous_id = bid_request.anonymous_id();
    }

    if(bid_request.has_url())
    {
      HTTP::BrowserAddress url;
      try
      {
        url.url(adapt_app_store_url_(bid_request.url()));
      }
      catch(eh::Exception&)
      {}

      fill_by_referer(request_params, request_info.search_words, url);
    }

    if(request_params.common_info.referer[0] == 0)
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NOREF);
    }

    request_params.search_words << request_info.search_words;

    // Test data
    request_params.common_info.log_as_test =
      bid_request.has_is_test() && bid_request.is_test();

    // User id
    std::string external_user_id;

    if (bid_request.has_google_user_id())
    {
      external_user_id = bid_request.google_user_id();
    }

    request_info.is_app = bid_request.has_mobile() && bid_request.mobile().is_app();

    if(bid_request.has_mobile() && bid_request.mobile().has_app_id())
    {
      request_info.application_id = bid_request.mobile().app_id();
    }

    if (bid_request.has_hosted_match_data())
    {
      verify_user_id_(bid_request.hosted_match_data(), request_info.source_id, request_params);
    }
    else if(!external_user_id.empty() && use_external_user_id_(external_user_id))
    {
      request_params.common_info.external_user_id << (
        !request_info.source_id.empty() ?
          request_info.source_id + "/" + external_user_id :
        external_user_id);
    }

    bool is_video = false;
    bool video_passback = false;
    if (bid_request.has_video())
    {
      is_video = true;
      const ::Google::BidRequest_Video& video = bid_request.video();
      video_passback =
        (video.allowed_video_formats().size() &&
         !test_value_in_list(
            video.allowed_video_formats(),
            ::Google::BidRequest_Video_VideoFormat_VIDEO_MP4) &&
         !test_value_in_list(
            video.allowed_video_formats(),
            ::Google::BidRequest_Video_VideoFormat_VIDEO_FLV) &&
         !test_value_in_list(
            video.allowed_video_formats(),
            ::Google::BidRequest_Video_VideoFormat_VIDEO_MOV) &&
         !test_value_in_list(
            video.allowed_video_formats(),
            ::Google::BidRequest_Video_VideoFormat_VIDEO_WEBM));
    }

    as_slots_context.resize(bid_request.adslot_size());
    request_params.ad_slots.length(bid_request.adslot_size());
    for(int slot_i = 0; slot_i < bid_request.adslot_size(); ++slot_i)
    {
      AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot_request =
        request_params.ad_slots[slot_i];

      init_adslot(ad_slot_request);

      ad_slot_request.ad_slot_id = slot_i;
      ad_slot_request.fill_track_html = false;
      ad_slot_request.tag_id = 0;

      ad_slot_request.video_allow_skippable = (
        !bid_request.has_video() ||
        bid_request.video().video_ad_skippable() != ::Google::BidRequest_Video::BLOCK_SKIPPABLE);
      ad_slot_request.video_allow_unskippable = (
        !bid_request.has_video() ||
        bid_request.video().video_ad_skippable() != ::Google::BidRequest_Video::REQUIRE_SKIPPABLE);

      const ::Google::BidRequest_AdSlot& adslot = bid_request.adslot(slot_i);
      GoogleAdSlotContext& ad_slot_context = as_slots_context[slot_i];

      bool slot_is_video = is_video || test_value_in_list(
        adslot.allowed_ad_types(),
        ::Google::BidRequest_AdSlot_AllowedAdType_ALLOWED_AD_TYPE_VIDEO);

      std::set<google::protobuf::int32> excluded_attributes(
        adslot.excluded_attribute().begin(),
        adslot.excluded_attribute().end());

      bool passback =
        (excluded_attributes.size() &&
          ((slot_is_video &&
            test_value_in_set(
              excluded_attributes,
              Google::CREATIVETYPE_VIDEO)) ||
           (!slot_is_video &&
             test_value_in_set(
               excluded_attributes,
               Google::CREATIVETYPE_HTML)) ||
            test_range_in_set(
              excluded_attributes,
              Google::CREATIVE_ATTR)));

      ad_slot_request.passback =
        request_info.filter_request || passback || video_passback;

      // Excluded categories

      int flash_exclude_size =
        test_value_in_set(
          excluded_attributes,
            Google::CREATIVETYPE_FLASH)? 1: 0;

      ad_slot_request.exclude_categories.length(
        adslot.excluded_sensitive_category_size() +
        adslot.excluded_product_category_size() +
        flash_exclude_size + 2);

      CORBA::ULong res_cat_i = 0;

      if(slot_is_video && !video_passback)
      {
        if(bid_request.has_video())
        {
          const ::Google::BidRequest_Video& video = bid_request.video();
          // https://developers.google.com/authorized-buyers/rtb/adx-video-guide
          // define that all INSTREAM support vpaid
          if(!((video.has_placement() && video.placement() == ::Google::BidRequest_Video_Placement_INSTREAM) ||
            test_value_in_list(
              video.allowed_video_formats(),
              ::Google::BidRequest_Video_VideoFormat_VPAID_JS)))
          {
            ad_slot_request.exclude_categories[res_cat_i++] << CreativeCategory::VPAID1;
            ad_slot_request.exclude_categories[res_cat_i++] << CreativeCategory::VPAID2;
          }
        }
        else
        {
          ad_slot_request.exclude_categories[res_cat_i++] << CreativeCategory::VPAID1;
          ad_slot_request.exclude_categories[res_cat_i++] << CreativeCategory::VPAID2;
        }
      }

      if (flash_exclude_size)
      {
        ad_slot_request.exclude_categories[res_cat_i++] << Google::CREATIVETYPE_FLASH_STR;
      }
      
      for(int cat_i = 0;
          cat_i < adslot.excluded_sensitive_category_size(); ++cat_i)
      {
        char cat_str[20];
        size_t len = String::StringManip::int_to_str(
          adslot.excluded_sensitive_category(cat_i), cat_str, sizeof(cat_str));
        if(len > 0)
        {
          ad_slot_request.exclude_categories[res_cat_i] << String::SubString(cat_str, len);
          ++res_cat_i;
        }
      }

      for(int cat_i = 0;
          cat_i < adslot.excluded_product_category_size(); ++cat_i)
      {
        char cat_str[20];
        size_t len = String::StringManip::int_to_str(
          adslot.excluded_product_category(cat_i), cat_str, sizeof(cat_str));
        if(len > 0)
        {
          ad_slot_request.exclude_categories[res_cat_i] << String::SubString(cat_str, len);
          ++res_cat_i;
        }
      }

      ad_slot_request.exclude_categories.length(res_cat_i);

      // Video
      if(slot_is_video)
      {
        ad_slot_request.format << OPENRTB_VAST_APP_FORMAT;
        ad_slot_request.sizes.length(1);
        ad_slot_request.sizes[0] << VAST_PROTOCOL_SIZE;

        int video_max_duration = 0;
        int video_skippable_max_duration = 0;
        int video_min_duration = 0;

        if(bid_request.has_video())
        {
          const ::Google::BidRequest_Video& video = bid_request.video();

          video_max_duration = video.has_max_ad_duration() ?
            video.max_ad_duration() : 0;
          video_skippable_max_duration = video.has_skippable_max_ad_duration() ?
            video.skippable_max_ad_duration() : video_max_duration;
          video_min_duration = video.has_min_ad_duration() ?
            video.min_ad_duration() : 0;

          // ADSC-10933
          if (video.has_playback_method())
          {
            kw_fmt.add_dict_keyword(
              MatchKeywords::PLAYBACKMETHOD,
              get_value_from_seq(
                Request::OpenRtb::VIDEO_PLAYBACKMETHODS,
                video.playback_method()));

            kw_fmt.add_dict_keyword(
              MatchKeywords::PLAYBACKMETHOD,
              get_value_from_seq(
                Request::OpenRtb::VIDEO_PLAYBACKMETHODS,
                video.playback_method()), false);
          }

          // ADSC-10919
          kw_fmt.add_dict_keyword(
            MatchKeywords::PLACEMENT,
            video.has_placement() ?
              get_value_from_seq(
                Bidding::Google::VIDEO_PLACEMENTS,
                  video.placement()):
                    Request::OpenRtb::VIDEO_PLACEMENT_NONE);
        }

        ad_slot_request.video_max_duration =
          video_max_duration > 0 ? video_max_duration / 1000 : -1;
        ad_slot_request.video_skippable_max_duration =
          video_skippable_max_duration > 0 ? video_skippable_max_duration / 1000 : -1;
        ad_slot_request.video_min_duration =
          video_min_duration / 1000;

        fill_vast_instantiate_type_(
          request_params,
          request_info.source_id);

        fill_video_size(
          kw_fmt, ad_slot_request,
          adslot.width_size() && adslot.width(0) > 0 ? adslot.width(0) : 0,
          adslot.height_size() && adslot.height(0) > 0 ? adslot.height(0) : 0);
      }
      // Banners
      else
      {
        ad_slot_request.format << Google::APP_FORMAT;
        
        ad_slot_request.video_min_duration = 0;
        ad_slot_request.video_max_duration = -1;
        ad_slot_request.video_width = 0;
        ad_slot_request.video_height = 0;

         // Sizes
        const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
        const auto size_it = debug_sizes.find(slot_i);
        if (size_it != debug_sizes.end())
        {
          ad_slot_request.sizes.length(1);
          ad_slot_request.sizes[0] << size_it->second;
        }
        else if (!request_info.default_debug_size.empty())
        {
          ad_slot_request.sizes.length(1);
          ad_slot_request.sizes[0] << request_info.default_debug_size;
        }
        else if (adslot.width_size())
        {
          if (adslot.width_size() != adslot.height_size())
          {
            if(logger_->log_level() >= Logging::Logger::NOTICE)
            {
              Stream::Error ostr;
              ostr << FUN << "Error in adslot size: width count(" << adslot.width_size() <<
                ") != height count (" << adslot.height_size() << ")";
            
              logger_->log(
                ostr.str(),
                Logging::Logger::NOTICE,
                Aspect::BIDDING_FRONTEND);
            }

            ad_slot_request.passback = true;
          }
          else
          {
            int max_sq = 0;
            int max_size_i = 0;
            for (int wi = 0; wi < adslot.width_size(); ++wi)
            {
              int sq = adslot.width(wi) * adslot.height(wi);
              if (sq > max_sq)
              {
                max_size_i = wi;
                max_sq = sq;
              }
            }
            ad_slot_context.width = adslot.width(max_size_i);
            ad_slot_context.height = adslot.height(max_size_i);

            Stream::Stack<1024> oss;
            oss << ad_slot_context.width << 'x' << ad_slot_context.height;

            ad_slot_request.sizes.length(1);
            ad_slot_request.sizes[0] << oss.str();
          }
        }
      }

      // min cpm
      int64_t min_cpm_bid = -1;
      for (int bid_i = 0; bid_i < adslot.matching_ad_data_size(); bid_i++)
      {
        const ::Google::BidRequest_AdSlot_MatchingAdData& ad_data =
          adslot.matching_ad_data(bid_i);
        int64_t minimum_cpm_micros = ad_data.has_minimum_cpm_micros() &&
          ad_data.minimum_cpm_micros() >= 0 ? ad_data.minimum_cpm_micros() : 0;
        if (min_cpm_bid < 0 || minimum_cpm_micros < min_cpm_bid)
        {
          min_cpm_bid = minimum_cpm_micros;
        }

        for (int bill_i = 0; bill_i < ad_data.billing_id_size(); ++bill_i)
        {
          ad_slot_context.billing_ids.insert(ad_data.billing_id(bill_i));
        }

        // REQ-3982
        // Store direct deal with minimum fixed_cpm_micros
        for (int deal_i = 0; deal_i < ad_data.direct_deal_size(); deal_i++)
        {
          const ::Google::BidRequest_AdSlot_MatchingAdData_DirectDeal& deal =
            ad_data.direct_deal(deal_i);
          
          if (
            (deal.deal_type() == ::Google::BidRequest_AdSlot_MatchingAdData_DirectDeal::PREFERRED_DEAL ||
              deal.deal_type() == ::Google::BidRequest_AdSlot_MatchingAdData_DirectDeal::PRIVATE_AUCTION) &&
            deal.has_direct_deal_id() && deal.direct_deal_id() > 1 &&
            deal.has_fixed_cpm_micros() && deal.fixed_cpm_micros() >= 0 &&
            deal.fixed_cpm_micros() < ad_slot_context.fixed_cpm_micros)
          {
            ad_slot_context.direct_deal_id = deal.direct_deal_id();
            ad_slot_context.fixed_cpm_micros = deal.fixed_cpm_micros();
          }
        }
      }

      min_cpm_bid = min_cpm_bid < 0 ? 0 : min_cpm_bid;

      try
      {
        AdServer::CampaignSvcs::RevenueDecimal min_ecpm;
        
        AdServer::CampaignSvcs::narrow_decimal(
          min_ecpm,
          AdServer::CampaignSvcs::ExtRevenueDecimal::mul(
            AdServer::CampaignSvcs::ExtRevenueDecimal(
              false, min_cpm_bid, 0),
            Google::BIDFLOOR_MULTIPLIER,
            Generics::DMR_ROUND));

        ad_slot_request.min_ecpm = CorbaAlgs::pack_decimal(min_ecpm);
      }
      catch (const Generics::DecimalException& e)
      {
        if(logger_->log_level() >= Logging::Logger::NOTICE)
        {
          Stream::Error ostr;
          ostr << FUN << "Minimum_cpm_micros='" << min_cpm_bid <<
            "' convertation error:" << e.what();
            
          logger_->log(
            ostr.str(),
            Logging::Logger::NOTICE,
            Aspect::BIDDING_FRONTEND);
        }

        ad_slot_request.min_ecpm =
          CorbaAlgs::pack_decimal(CampaignSvcs::RevenueDecimal::ZERO);

        ad_slot_request.passback = true;
      }
      
      // Creative expanding
      ad_slot_request.up_expand_space =
        test_range_in_set(
          excluded_attributes,
          Google::UP_EXPANDING)? 0: 0x0FFFFFFF;
      ad_slot_request.right_expand_space =
        test_range_in_set(
          excluded_attributes,
          Google::RIGHT_EXPANDING)? 0: 0x0FFFFFFF;
      ad_slot_request.down_expand_space =
        test_range_in_set(
          excluded_attributes,
          Google::DOWN_EXPANDING)? 0: 0x0FFFFFFF;
      ad_slot_request.left_expand_space =
        test_range_in_set(
          excluded_attributes,
          Google::LEFT_EXPANDING)? 0: 0x0FFFFFFF;

      if(adslot.slot_visibility() ==
        ::Google::BidRequest_AdSlot_SlotVisibility_NO_DETECTION)
      {
        // Disable filter
        ad_slot_request.tag_visibility = -1;
      }
      else if(adslot.slot_visibility() ==
        ::Google::BidRequest_AdSlot_SlotVisibility_ABOVE_THE_FOLD)
      {
        // First Screen
        ad_slot_request.tag_visibility = 100;
      }
      else 
      {
        // Other Screen
        ad_slot_request.tag_visibility = 0;
      }

      ad_slot_request.tag_predicted_viewability = adslot.viewability();
      if (ad_slot_request.tag_predicted_viewability < -1 ||
          ad_slot_request.tag_predicted_viewability > 100)
      {
        ad_slot_request.tag_predicted_viewability = -1;
      }

      // External tag id
      Stream::Stack<EXT_TAG_ID_MAX_LENGTH> oss;
      oss << seller_id << '-' <<
        (anonymous_id.empty()? "0": anonymous_id.c_str());
      ad_slot_request.ext_tag_id << normalize_ext_tag_id_(oss.str());

      ad_slot_request.debug_ccg = 0;

      // fill creative_instantiate_type if was not filled by fill_by_referer_
      if(test_value_in_set(excluded_attributes, Google::NON_SECURE) &&
           request_params.common_info.creative_instantiate_type[0] == '\0')
      {
        request_params.common_info.creative_instantiate_type <<
          FrontendCommons::SECURE_INSTANTIATE_TYPE;
      }

      if(adslot.has_viewability())
      {
        kw_fmt.add_dict_keyword(
          MatchKeywords::VIEWABILITY,
          adslot.viewability());
      }
    } // adslot fetch

    if(bid_request.has_mobile())
    {
      const auto& mobile = bid_request.mobile();

      if(mobile.has_advertising_id())
      {
        stringify_ifa(request_info.advertising_id, mobile.advertising_id());
      }
      else if(mobile.has_encrypted_advertising_id())
      {
        if(mobile.encrypted_advertising_id().size() == 36 && !request_info.publisher_account_ids.empty())
        {
          for(auto p_account_it = request_info.publisher_account_ids.begin();
            p_account_it != request_info.publisher_account_ids.end(); ++p_account_it)
          {
            auto account_it = account_traits_.find(*p_account_it);
            if(account_it != account_traits_.end() &&
               account_it->second->google_encryption_key_size > 0 &&
               account_it->second->google_integrity_key_size > 0)
            {
              decode_google_advertising_id(
                request_info.advertising_id,
                mobile.encrypted_advertising_id().data(),
                mobile.encrypted_advertising_id().size(),
                account_it->second->google_encryption_key.get(),
                account_it->second->google_encryption_key_size,
                account_it->second->google_integrity_key.get(),
                account_it->second->google_integrity_key_size);

              break;
            }
          }
        }
        else if(mobile.encrypted_advertising_id().size() == 16)
        {
          stringify_ifa(request_info.advertising_id, mobile.encrypted_advertising_id());
        }
      }
      else if(mobile.has_hashed_idfa())
      {
        stringify_ifa(request_info.idfa, mobile.hashed_idfa());
      }
    }

    // fill creative_instantiate_type if was not filled by fill_by_referer_
    if(request_params.common_info.creative_instantiate_type[0] == '\0')
    {
      request_params.common_info.creative_instantiate_type <<
        FrontendCommons::UNSECURE_INSTANTIATE_TYPE;
    }

    if(!request_info.idfa.empty() || !request_info.advertising_id.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_IDFA);
      kw_fmt.add_keyword(MatchKeywords::FULL_IDFA_KNOWN);
    }

    kw_fmt.assign_to(keywords);

    add_special_keywords_(keywords, request_info);
  }

  bool
  RequestInfoFiller::fill_adid(const RequestInfo& request_info) const noexcept
  {
    SourceMap::const_iterator source_it = sources_.find(request_info.source_id);
    if(source_it != sources_.end())
    {
      return source_it->second.fill_adid;
    }

    return false;
  }

  void
  RequestInfoFiller::fill_vast_instantiate_type_(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const std::string& source_id) const
    noexcept
  {
    SourceMap::const_iterator source_it = sources_.find(source_id);
    if(source_it != sources_.end())
    {
      request_params.ad_instantiate_type = source_it->second.vast_instantiate_type;
      request_params.context_info.enabled_notice = (
        source_it->second.vast_notice_instantiate_type != SourceTraits::NIT_NONE) &&
        source_it->second.notice_url.empty();
    }
  }

  void
  RequestInfoFiller::fill_native_instantiate_type_(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const std::string& source_id) const
    noexcept
  {
    SourceMap::const_iterator source_it = sources_.find(source_id);
    if(source_it != sources_.end())
    {
      request_params.context_info.enabled_notice = (
        source_it->second.native_notice_instantiate_type != SourceTraits::NIT_NONE);
    }
  }

  void
  RequestInfoFiller::fill_request_type_(
    RequestInfo& request_info,
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const std::string& source_id) const
    noexcept
  {
    SourceMap::const_iterator source_it = sources_.find(source_id);
    if(source_it != sources_.end())
    {
      if (request_info.publisher_account_ids.empty())
      {
        request_params.publisher_account_ids.length(1);
        request_params.publisher_account_ids[0] = *(source_it->second.default_account_id);
      }

      if(source_it->second.request_type.present())
      {
        request_params.common_info.request_type = *(source_it->second.request_type);
      }

      request_params.ad_instantiate_type = source_it->second.instantiate_type;

      request_info.notice_instantiate_type = source_it->second.notice_instantiate_type;
      request_info.vast_notice_instantiate_type = source_it->second.vast_notice_instantiate_type;
      request_info.native_notice_instantiate_type = source_it->second.native_notice_instantiate_type;

      request_params.context_info.enabled_notice = (
        source_it->second.notice_instantiate_type != SourceTraits::NIT_NONE) &&
        source_it->second.notice_url.empty();
      request_info.seat = source_it->second.seat;
      request_info.ipw_extension = source_it->second.ipw_extension;
      request_info.appnexus_member_id = source_it->second.appnexus_member_id;
      request_info.truncate_domain = source_it->second.truncate_domain;

      request_info.native_ads_instantiate_type =
        source_it->second.native_ads_instantiate_type;
      if(source_it->second.native_ads_impression_tracker_type.present())
      {
        request_info.native_ads_impression_tracker_type =
          *source_it->second.native_ads_impression_tracker_type;
      }
      request_info.skip_ext_category = source_it->second.skip_ext_category;
      request_info.notice_url = source_it->second.notice_url;
    }
    else
    {
      request_params.ad_instantiate_type = AdServer::CampaignSvcs::AIT_BODY;
      request_params.context_info.enabled_notice = false;
    }
  }

  std::string
  RequestInfoFiller::openrtb_ext_tag_id(
    const std::string& publisher_id,
    const std::string& id,
    const std::string& publisher_name,
    const std::string& name)
  {
    std::string res;
    res.reserve(EXT_TAG_ID_MAX_LENGTH);

    res += (publisher_id.empty() ? "0" : publisher_id);
    res += "-";
    res += (id.empty() ? "0" : id );
    res += "-";
    res += (publisher_name.empty() ? "0" : publisher_name);
    res += "-";
    res += (name.empty() ? "0" : name);

    if(res.size() > EXT_TAG_ID_MAX_LENGTH)
    {
      res.resize(EXT_TAG_ID_MAX_LENGTH);
    }

    return normalize_ext_tag_id_(res);
  }

  void
  RequestInfoFiller::fill_by_openrtb_request(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    RequestInfo& request_info,
    std::string& keywords,
    JsonProcessingContext& context,
    const char* bid_request) const
    /*throw(InvalidParamException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill_by_openrtb_request()";

    request_params.context_info.client << OPENRTB_APPLICATION;
    request_params.context_info.client_version << OPENRTB_APPLICATION_VERSION;
    request_params.common_info.request_type = AdServer::CampaignSvcs::AR_OPENRTB;
    request_params.common_info.user_status = static_cast<CORBA::ULong>(
      AdServer::CampaignSvcs::US_UNDEFINED);

    init_request_param(request_params, request_info);

    if(request_params.ad_instantiate_type == AdServer::CampaignSvcs::AIT_URL)
    {
      // notice can't be enabled for openrtb with url instantiate type
      // because both uses nurl response parameter
      request_params.context_info.enabled_notice = false;
    }

    if(request_params.common_info.request_type ==
       AdServer::CampaignSvcs::AR_YANDEX)
    {
      request_params.fill_track_pixel = true;
    }

    int bid_request_len = ::strlen(bid_request);
    // in some specific cases required +1 symbol
    Generics::ArrayAutoPtr<char> bid_request_holder(bid_request_len + 2);
    JsonValue root_value;
    JsonAllocator json_allocator;
    char* parse_end = bid_request_holder.get();
    ::strcpy(bid_request_holder.get(), bid_request);
    bid_request_holder.get()[bid_request_len + 1] = 0;
    JsonParseStatus status = json_parse(
      bid_request_holder.get(), &parse_end, &root_value, json_allocator);

    assert(parse_end < bid_request_holder.get() + bid_request_len + 2);

    if(status != JSON_PARSE_OK)
    {
      Stream::Error ostr;
      ostr << FUN << ": parsing error '" << json_parse_error(status) << "' at pos : ";
      if(parse_end)
      {
        ostr << std::string(parse_end, 20);
      }
      else
      {
        ostr << "null";
      }
      throw InvalidParamException(ostr);
    }

    JsonTag root_tag = root_value.getTag();
    if(root_tag != JSON_TAG_OBJECT)
    {
      Stream::Error ostr;
      ostr << FUN << ": incorrect root tag type";
      throw InvalidParamException(ostr);
    }

    try
    {
      json_root_processor_->process(request_params, context, root_value);
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": processing error: " << e.what();
      throw InvalidParamException(ostr);
    }

    request_info.bid_publisher_id = context.publisher_id;
    request_info.bid_site_id = context.site_id;
    request_info.bid_request_id = context.request_id;

    //context.print(std::cerr);
    KeywordFormatter kw_fmt(request_info.source_id);

    kw_fmt.add_keyword(MatchKeywords::FULL_REQ);

    if(!request_info.source_id.empty())
    {
      kw_fmt.add_dict_keyword(MatchKeywords::REQ, String::SubString());
    }

    if(context.user)
    {
      kw_fmt.add_keyword(context.user_keywords);

      if(context.user_yob)
      {
        kw_fmt.add_yob(context.user_yob);
      }

      if(context.user_gender == "M")
      {
        kw_fmt.add_gender("male");
      }

      if(context.user_gender == "F")
      {
        kw_fmt.add_gender("female");
      }

      // process segments
      for(auto it = context.segments.begin(); it != context.segments.end(); ++it)
      {
        // add <id>x<value with replaced chars>
        std::string keyword(std::string("rtbsegment") + norm_keyword_(it->id) + "x" + norm_keyword_(it->value));
        //std::cerr << "keyword : <" << keyword << ">" << std::endl;
        kw_fmt.add_keyword(keyword);
      }
    }

    if(context.site_content || context.app_content)
    {
      kw_fmt.add_keyword(context.content_keywords);
      kw_fmt.add_keyword(context.content_title);
      kw_fmt.add_keyword(context.content_series);
      kw_fmt.add_keyword(context.content_season);

      kw_fmt.add_cat_list(context.content_cat, true);
      kw_fmt.add_keyword_list(context.content_producer_name);
    }

    if(context.app_publisher || context.site_publisher)
    {
      kw_fmt.add_keyword(context.publisher_name);
      kw_fmt.add_cat_list(context.publisher_cat, true);
    }

    kw_fmt.add_keyword(context.site_keywords);
    kw_fmt.add_keyword(context.app_keywords);

    kw_fmt.add_cat_list(context.site_pagecat, true);
    kw_fmt.add_cat_list(context.site_sectioncat, true);
    kw_fmt.add_cat_list(context.site_cat, true);
    kw_fmt.add_cat_list(context.app_pagecat, true);
    kw_fmt.add_cat_list(context.app_sectioncat, true);
    kw_fmt.add_cat_list(context.app_cat, true);

    if(context.external_user_id.empty() && (
         request_params.common_info.request_type == AdServer::CampaignSvcs::AR_OPENX ||
         (request_params.common_info.request_type == AdServer::CampaignSvcs::AR_YANDEX &&
          !context.user_id.empty() &&
          context.user_id[0] == '~'))
      )
    {
      context.external_user_id.swap(context.user_id);
    }

    if (context.external_user_id.empty())
    {
      context.external_user_id = make_ssp_uid_by_device_(context);
    }

    if(!context.external_user_id.empty() &&
      use_external_user_id_(context.external_user_id))
    {
      request_params.common_info.external_user_id << (
        !request_info.source_id.empty() ?
          request_info.source_id + "/" + context.external_user_id :
          context.external_user_id);
    }

    if(request_params.common_info.external_user_id[0] == 0 &&
       context.user_id.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NO_ID);

      if(!request_info.source_id.empty())
      {
        kw_fmt.add_dict_keyword(MatchKeywords::NO_ID, String::SubString());
      }
    }

    // Possible IPMatcher and code in CampaignManager not support IPv6
    if (!context.ipv6.empty())
    {
      fill_by_ip(request_info, request_params, context.ipv6);
    }
    else
    {
      fill_by_ip(request_info, request_params, context.ip);
    }

    if(!context.ip.empty())
    {
      kw_fmt.add_ip(String::SubString(context.ip));
    }

    if (context.regs_coppa)
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_COPPA);
    }

    // don't use buyer_id if external_user_id defined
    if(request_params.common_info.external_user_id[0] == 0)
    {
      verify_user_id_(context.user_id, request_info.source_id, request_params);
    }

    request_info.is_app = context.app;

    fill_by_user_agent(
      request_params,
      request_info,
      context.user_agent,
      request_info.filter_request,
      context.app);

    std::string ext_tag_id;

    HTTP::HTTPAddress selected_referer;
    if(context.site)
    {
      ext_tag_id = openrtb_ext_tag_id(
        context.publisher_id, context.site_id,
        context.publisher_name, context.site_name);

      if(context.site_page.secure() ||
        context.site_domain.secure() ||
        context.site_ref.secure() ||
        context.site_referer.secure() ||
        context.site_rereferer.secure())
      {
        request_params.common_info.creative_instantiate_type <<
          FrontendCommons::SECURE_INSTANTIATE_TYPE;
      }

      select_referer_(request_params, context, selected_referer);

      fill_by_referer(
        request_params,
        request_info.search_words,
        selected_referer,
        true, // fill_search_words
        false // fill_instantiate_type
        );
    }
    else if(context.app)
    {
      request_params.context_info.profile_referer = enable_profile_referer_;
      ext_tag_id = openrtb_ext_tag_id(
        context.publisher_id, context.app_id,
        context.publisher_name, context.app_name);

      std::string app_bundle_url;

      try
      {
        // workaround: some SSP provide url with spaces at beginning
        String::SubString url_sstr(context.app_bundle);
        String::StringManip::trim(url_sstr);
        request_info.application_id = url_sstr.str();
        app_bundle_url = HTTP::BrowserAddress(url_sstr).url();
      }
      catch(const eh::Exception&)
      {}

      if(!app_bundle_url.empty())
      {
        request_params.common_info.referer << app_bundle_url;
      }
      else
      {
        if(!context.app_domain.url().empty() && (
           context.app_store_url.url().empty() || (
            context.app_domain.host().compare(GooglePlay::STORE_HOST) != 0 &&
            context.app_domain.host().compare(AppStore::STORE_HOST) != 0)
            )
          )
        {
          request_params.common_info.referer <<
            FrontendCommons::normalize_abs_url(context.app_domain);
        }
        else if(!context.app_store_url.url().empty())
        {
          request_params.common_info.referer <<
            adapt_app_store_url_(
              FrontendCommons::normalize_abs_url(context.app_store_url));
        }
      }
    }

    if(!context.site_ref.url().empty())
    {
      try
      {
        // select_referer_() can return context.site_ref,
        //   to prevent double search phrase
        //    compare selected_referer with site_ref
        // ADSC-10103
        if (context.site_ref.url() != selected_referer.url())
        {
          fill_additional_url_(
            request_params,
            request_info.search_words,
            context.site_ref);
        }
      }
      catch(eh::Exception&)
      {}
    }

    if(request_params.common_info.referer[0] == 0)
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NOREF);
    }

    // add search_words if present
    if(!context.site_search.empty())
    {
      if(!request_info.search_words.empty())
      {
        request_info.search_words += '\n';
      }
      request_info.search_words += context.site_search;
    }

    request_params.search_words << request_info.search_words;

    request_params.common_info.log_as_test = context.test;

    request_params.ad_slots.length(context.ad_slots.size());
    CORBA::ULong slot_i = 0;
    for(JsonAdSlotProcessingContextList::iterator slot_it =
          context.ad_slots.begin();
        slot_it != context.ad_slots.end(); )
    {
      const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
      const std::string& default_debug_size = request_info.default_debug_size;

      if(slot_it->imp_ext_type == "popup")
      {
        // add popup banner
        JsonAdSlotProcessingContext::Banner_var popup_banner = new JsonAdSlotProcessingContext::Banner();
        JsonAdSlotProcessingContext::BannerFormat_var popup_banner_format = new JsonAdSlotProcessingContext::BannerFormat();
        popup_banner_format->ext_format = "popup";
        popup_banner->formats.push_back(popup_banner_format);
        slot_it->banners.push_back(popup_banner);
      }

      // FIXME: check valid used parameters...
      if(slot_it->video || slot_it->native || !(slot_it->banners.empty() &&
          debug_sizes.empty() && default_debug_size.empty()))
      {
        bool parse_request_error_flag = false;
        if (slot_it->video_protocols.fail() ||
            slot_it->video_playbackmethods.fail() ||
            slot_it->video_start_delay.fail() ||
            slot_it->video_linearity.fail() ||
            slot_it->video_adtype.fail() ||
            slot_it->video_min_duration.fail() ||
            slot_it->video_max_duration.fail() ||
            (
              slot_it->video_min_duration.good() &&
              slot_it->video_max_duration.good() &&
              (*(slot_it->video_min_duration) > *(slot_it->video_max_duration))
            ) ||
            (
              (slot_it->video_linearity).good() &&
              (*(slot_it->video_linearity) != 1) &&
              (*(slot_it->video_linearity) != 2))) // linearity allowed values 1 and 2 only
        {
          if(logger_->log_level() >= Logging::Logger::NOTICE)
          {
            Stream::Error ostr;
            /*
            ostr << FUN << "Error in format or allowed values video_protocols, "
              "video_playbackmethods, video_start_delay, video_linearity, video_adtype";
            */
            ostr << FUN << "Error in format " <<
              (slot_it->video_protocols.fail() ? ", video_protocols fail" : "") <<
              (slot_it->video_playbackmethods.fail() ? ", video_playbackmethods fail" : "") <<
              (slot_it->video_start_delay.fail() ? ", video_start_delay fail" : "") <<
              (slot_it->video_linearity.fail() ? ", video_linearity fail" : "") <<
              (slot_it->video_adtype.fail() ? ", video_adtype fail" : "") <<
              (slot_it->video_min_duration.fail() ? ", video_min_duration fail" : "") <<
               (slot_it->video_max_duration.fail() ? ", video_max_duration fail" : "") <<
              ((slot_it->video_min_duration.good() && slot_it->video_max_duration.good() &&
                (*(slot_it->video_min_duration) > *(slot_it->video_max_duration))) ?
                "video_min_duration/video_max_duration check" : "") <<
              ((slot_it->video_linearity).good() && (*(slot_it->video_linearity) != 1) &&
               (*(slot_it->video_linearity) != 2) ? ", video_linearity check" : "");

            logger_->log(
              ostr.str(),
              Logging::Logger::NOTICE,
              Aspect::BIDDING_FRONTEND);
          }

          parse_request_error_flag = true;
        }

        bool serve_video = !parse_request_error_flag && !slot_it->native &&
          slot_it->video && (
          (slot_it->video_mimes.empty() ||
           slot_it->video_mimes.find(VIDEO_MIME_MP4) != slot_it->video_mimes.end() ||
           slot_it->video_mimes.find(VIDEO_MIME_FLV) != slot_it->video_mimes.end() ||
           slot_it->video_mimes.find(VIDEO_MIME_MOV) != slot_it->video_mimes.end()) &&
          (slot_it->video_protocols.empty() ||
           slot_it->video_protocols->empty() ||
           slot_it->video_protocols->find(2) != slot_it->video_protocols->end() ||
           slot_it->video_protocols->find(3) != slot_it->video_protocols->end() ||
           slot_it->video_protocols->find(5) != slot_it->video_protocols->end()) &&
          (slot_it->video_linearity.empty() ||
            *(slot_it->video_linearity) == 1 ||
            *(slot_it->video_linearity) == 2) &&
          (slot_it->video_adtype.empty() ||
            *(slot_it->video_adtype) == 0 || *(slot_it->video_adtype) == 3));

        if (!parse_request_error_flag && slot_it->video)
        {
         // ADSC-10919
          kw_fmt.add_dict_keyword(
            MatchKeywords::PLACEMENT,
            slot_it->video_placement.present()?
              get_value_from_seq(
                Request::OpenRtb::VIDEO_PLACEMENTS,
                *slot_it->video_placement) :
              Request::OpenRtb::VIDEO_PLACEMENT_NONE);

          kw_fmt.add_dict_keyword(
            MatchKeywords::PLACEMENT,
            slot_it->video_placement.present() ?
              get_value_from_seq(
                Request::OpenRtb::VIDEO_PLACEMENTS,
                *slot_it->video_placement) :
              Request::OpenRtb::VIDEO_PLACEMENT_NONE,
            false);

          // ADSC-10933
          if (!slot_it->video_playbackmethods.empty())
          {
            for (auto it = slot_it->video_playbackmethods->begin();
                 it != slot_it->video_playbackmethods->end(); ++it)
            {
              kw_fmt.add_dict_keyword(
                MatchKeywords::PLAYBACKMETHOD,
                get_value_from_seq(
                  Request::OpenRtb::VIDEO_PLAYBACKMETHODS, *it));
              kw_fmt.add_dict_keyword(
                MatchKeywords::PLAYBACKMETHOD,
                get_value_from_seq(
                  Request::OpenRtb::VIDEO_PLAYBACKMETHODS, *it), false);
            }
          }
        }

        if(serve_video && slot_it == context.ad_slots.begin())
        {
          fill_vast_instantiate_type_(
            request_params,
            request_info.source_id);
        }

        AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot_request =
          request_params.ad_slots[slot_i];

        init_adslot(ad_slot_request);

        ad_slot_request.ad_slot_id = slot_i;
        ad_slot_request.tag_id = 0;
        ad_slot_request.fill_track_html = false;
        if (slot_it->video_min_duration.good())
        {
          ad_slot_request.video_min_duration = *(slot_it->video_min_duration);
        }
        else
        {
          ad_slot_request.video_min_duration = 0;
        }
        ad_slot_request.video_max_duration = -1;
        ad_slot_request.video_width = 0;
        ad_slot_request.video_height = 0;

        CorbaAlgs::fill_sequence(
          context.exclude_categories.begin(),
          context.exclude_categories.end(),
          ad_slot_request.exclude_categories);

        if(!request_info.skip_ext_category &&
          !context.required_category.empty() &&
          context.required_category != "0")
        {
          ad_slot_request.required_categories.length(1);
          ad_slot_request.required_categories[0] << context.required_category;
        }

        unsigned long pos = 0;

        if(!request_info.format.empty())
        {
          ad_slot_request.format << request_info.format;
        }

        if(serve_video || (!slot_it->native && slot_it->banners.empty() &&
            debug_sizes.empty() && default_debug_size.empty()))
        {
          // push skipped vast into stats if banner can't be shown
          if(ad_slot_request.format[0] == 0)
          {
            ad_slot_request.format << OPENRTB_VAST_APP_FORMAT;
          }

          ad_slot_request.sizes.length(1);
          ad_slot_request.sizes[0] << VAST_PROTOCOL_SIZE;

          if(serve_video)
          {
            // process api
            std::vector<std::string> excluded_api_categories;
            
            if(slot_it->video_api.fail())
            {
              excluded_api_categories.push_back(CreativeCategory::VPAID1);
              excluded_api_categories.push_back(CreativeCategory::VPAID2);
            }
            else
            {
              if(slot_it->video_api->find(1) == slot_it->video_api->end() &&
                slot_it->video_api->find(2) == slot_it->video_api->end())
              {
                excluded_api_categories.push_back(CreativeCategory::VPAID1);
              }

              if(slot_it->video_api->find(2) == slot_it->video_api->end())
              {
                excluded_api_categories.push_back(CreativeCategory::VPAID2);
              }
            }

            CorbaAlgs::fill_sequence(
              excluded_api_categories.begin(),
              excluded_api_categories.end(),
              ad_slot_request.exclude_categories,
              true);

            CorbaAlgs::fill_sequence(
              slot_it->video_exclude_categories.begin(),
              slot_it->video_exclude_categories.end(),
              ad_slot_request.exclude_categories,
              true);
          }

          ad_slot_request.passback = request_info.filter_request || !serve_video;
          if(slot_it->video_max_duration.present())
          {
            ad_slot_request.video_max_duration = static_cast<CORBA::Long>(
              *slot_it->video_max_duration);
            ad_slot_request.video_skippable_max_duration =
              ad_slot_request.video_max_duration;
          }
          String::StringManip::str_to_int(slot_it->video_pos, pos);
          slot_it->banners.clear();

          fill_video_size(
            kw_fmt,
            ad_slot_request,
            slot_it->video_width.present() ? static_cast<unsigned long>(*slot_it->video_width) : 0,
            slot_it->video_height.present() ? static_cast<unsigned long>(*slot_it->video_height) : 0);
        }
        else if (slot_it->native)
        {
          // Native
          bool serve_native = (
            request_info.native_ads_instantiate_type != SourceTraits::NAIT_NONE);

          kw_fmt.add_dict_keyword(
            MatchKeywords::NATIVE_PLACEMENT,
            get_value_from_seq(
              Request::OpenRtb::NATIVE_PLACEMENTS,
              slot_it->native->placement.present() ?
                *slot_it->native->placement: 0));

          kw_fmt.add_dict_keyword(
            MatchKeywords::NATIVE_PLACEMENT,
            get_value_from_seq(
              Request::OpenRtb::NATIVE_PLACEMENTS,
              slot_it->native->placement.present() ?
                *slot_it->native->placement: 0), false);
          
          if(ad_slot_request.format[0] == 0)
          {
            ad_slot_request.format << OPENRTB_NATIVE_APP_FORMAT;
          }

          request_params.fill_track_pixel = true;
          ad_slot_request.fill_track_html =
            request_info.native_ads_impression_tracker_type == AdServer::CampaignSvcs::NAITT_JS;
          ad_slot_request.native_ads_impression_tracker_type =
            request_info.native_ads_impression_tracker_type;

          if (slot_it->native->video_assets.size() == 1)
          {
            const NativeVideo& video = slot_it->native->video_assets[0];

            serve_native &=
              (video.mimes.empty() ||
                video.mimes.find(VIDEO_MIME_MP4) != video.mimes.end() ||
                video.mimes.find(VIDEO_MIME_FLV) != video.mimes.end() ||
                video.mimes.find(VIDEO_MIME_MOV) != video.mimes.end()) &&
              (video.protocols.empty() ||
               video.protocols->find(2) != video.protocols->end() ||
               video.protocols->find(3) != video.protocols->end() ||
               video.protocols->find(5) != video.protocols->end());

            if (serve_native)
            {
              ad_slot_request.sizes.length(1);
              ad_slot_request.sizes[0] << VAST_PROTOCOL_SIZE;

              if (video.max_duration.present())
              {
                ad_slot_request.video_max_duration = static_cast<CORBA::Long>(
                  *video.max_duration);
                ad_slot_request.video_skippable_max_duration =
                  ad_slot_request.video_max_duration;
              }

              slot_it->banners.clear();

              fill_video_size(
                kw_fmt, ad_slot_request, 0, 0);
            }
          }
          else if(serve_native)
          {
            NativeImage main_image;
            bool main_image_found = find_main_image(slot_it->native->image_assets, main_image);

            ad_slot_request.sizes.length(1);
            const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
            const auto size_it = debug_sizes.find(slot_i);
            if (size_it != debug_sizes.end() || !request_info.default_debug_size.empty())
            {
              ad_slot_request.sizes[0] << (
                size_it != debug_sizes.end() ?
                size_it->second : request_info.default_debug_size);
            }
            else if(main_image_found)
            {
              Stream::Stack<1024> oss;
              oss << main_image.width << 'x' << main_image.height;
              ad_slot_request.sizes[0] << oss.str();
            }
            else
            {
              ad_slot_request.sizes[0] << NATIVE_TEXT_SIZE;
            }
          }

          if(serve_native)
          {
            fill_asset_tokens(
              ad_slot_request.native_data_tokens,
              slot_it->native->data_assets,
              Request::OpenRtb::NATIVE_DATA_ASSET_TOKENS,
              &NativeData::data_type,
              "NATIVEDATA");

            if(!fill_asset_tokens(
              ad_slot_request.native_image_tokens,
              slot_it->native->image_assets,
              Request::OpenRtb::NATIVE_IMAGE_ASSET_TOKENS,
              &NativeImage::image_type,
              "NATIVEIMAGE"))
            {
              serve_native = false;
            }
          }

          if(serve_native)
          {
            fill_native_instantiate_type_(
              request_params,
              request_info.source_id);
          }

          ad_slot_request.passback = request_info.filter_request || !serve_native;
        }
        else // banners not empty here
        {
          if(ad_slot_request.format[0] == 0)
          {
            ad_slot_request.format << OPENRTB_APP_FORMAT;
          }

          ad_slot_request.passback = request_info.filter_request;

          if(!slot_it->banners.empty())
          {
            request_params.common_info.hpos = (*slot_it->banners.begin())->ext_hpos;
            String::StringManip::str_to_int((*slot_it->banners.begin())->pos, pos);
          }
          else
          {
            request_params.common_info.hpos = 0;
          }

          bool skip_sizes = false;

          unsigned long banner_format_count = 0;
          for(auto banner_it = slot_it->banners.rbegin();
              banner_it != slot_it->banners.rend(); ++banner_it)
          {
            banner_format_count += (*banner_it)->formats.size();
          }

          ad_slot_request.sizes.length(banner_format_count);
          CORBA::ULong size_i = 0;

          for(auto banner_it = slot_it->banners.rbegin();
              banner_it != slot_it->banners.rend(); ++banner_it)
          {
            const JsonAdSlotProcessingContext::Banner& banner = **banner_it;

            // union exclude categories from all banner objects
            CorbaAlgs::fill_sequence(
              banner.exclude_categories.begin(),
              banner.exclude_categories.end(),
              ad_slot_request.exclude_categories,
              true);

            if(!skip_sizes)
            {
              for(auto banner_format_it = banner.formats.begin();
                  banner_format_it != banner.formats.end(); ++banner_format_it)
              {
                const JsonAdSlotProcessingContext::BannerFormat& banner_format =
                  **banner_format_it;

                if((banner_format.width == "2" && banner_format.height == "2") ||
                  banner_format.ext_type == "20")
                {
                  ad_slot_request.sizes[size_i++] << RM_SIZE;
                  slot_it->size_banner.insert(std::make_pair(
                    RM_SIZE,
                    JsonAdSlotProcessingContext::BannerFormatHolder(*banner_it, *banner_format_it)));
                }
                else if (banner_format.ext_format == "popup")
                {
                  ad_slot_request.sizes[size_i++] << POPUP_SIZE;
                  slot_it->size_banner.insert(std::make_pair(
                    POPUP_SIZE,
                    JsonAdSlotProcessingContext::BannerFormatHolder(*banner_it, *banner_format_it)));
                }
                else if (banner_format.ext_format == "overlay")
                {
                  ad_slot_request.sizes[size_i++] << RM_DTO_SIZE;
                  slot_it->size_banner.insert(std::make_pair(
                    RM_DTO_SIZE,
                    JsonAdSlotProcessingContext::BannerFormatHolder(*banner_it, *banner_format_it)));
                }
                else if (banner_format.ext_format == "rich")
                {
                  ad_slot_request.sizes[size_i++] << RM_RICH_SIZE;
                  slot_it->size_banner.insert(std::make_pair(
                    RM_RICH_SIZE,
                    JsonAdSlotProcessingContext::BannerFormatHolder(*banner_it, *banner_format_it)));
                }
                else if(!banner_format.width.empty() && !banner_format.height.empty())
                {
                  const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
                  const auto size_it = debug_sizes.find(slot_i);
                  std::string res_size = (size_it != debug_sizes.end() ? size_it->second : (
                    request_info.default_debug_size.empty() ?
                    banner_format.width + "x" + banner_format.height :
                    request_info.default_debug_size) );
                  ad_slot_request.sizes[size_i++] << res_size;
                  slot_it->size_banner.insert(std::make_pair(
                    res_size,
                    JsonAdSlotProcessingContext::BannerFormatHolder(*banner_it, *banner_format_it)));
                }
              }
            }
          }

          ad_slot_request.sizes.length(size_i);
        }

        if(pos == 1) // above the fold
        {
          ad_slot_request.tag_visibility = 100;
        }
        else if(pos == 3) // below the fold
        {
          ad_slot_request.tag_visibility = 0;
        }
        else
        {
          ad_slot_request.tag_visibility = -1;
        }

        ad_slot_request.tag_predicted_viewability = -1;

        // REQ-3982 Store direct deal with minimum cpm_price
        
        if (!slot_it->deals.empty())
        {
          std::map<AdServer::CampaignSvcs::RevenueDecimal, std::string> prices;

          const String::SubString currency_code =
            slot_it->min_cpm_price_currency_code.empty() ?
            Request::OpenRtb::DEFAULT_BIDFLOORCUR_CURRENCY :
            slot_it->min_cpm_price_currency_code;
          
          if (slot_it->min_cpm_price != AdServer::CampaignSvcs::RevenueDecimal::ZERO)
          {
            prices.insert(
              std::make_pair(
                slot_it->min_cpm_price,
                std::string()));
          }

          for (auto it = slot_it->deals.begin(); it != slot_it->deals.end();
               ++it)
          {
            if (caseless_compare(it->currency_code, currency_code))
            {
              prices.insert(
                std::make_pair(it->cpm_price, it->id));
            }     
          }

          if (!prices.empty())
          {
            auto min_price = prices.begin();
            slot_it->min_cpm_price = min_price->first;
            if (!min_price->second.empty())
            {
              slot_it->deal_id = min_price->second;
            }
          }
        }

        // Unit is 1.00 / 1000 imps for OpenRTB, 1.00 / 1M for Yandex
        
        const AdServer::CampaignSvcs::RevenueDecimal& min_ecpm_multiplier = (
          request_params.common_info.request_type !=
            AdServer::CampaignSvcs::AR_YANDEX ?
            OPENRTB_BIDFLOOR_MULTIPLIER :
            YANDEX_BIDFLOOR_MULTIPLIER);

        try
        {
          ad_slot_request.min_ecpm = CorbaAlgs::pack_decimal(
            AdServer::CampaignSvcs::RevenueDecimal::mul(
              slot_it->min_cpm_price,
              min_ecpm_multiplier,
              Generics::DMR_ROUND));
        }
        catch(const AdServer::CampaignSvcs::RevenueDecimal::Overflow&)
        {
          ad_slot_request.min_ecpm = CorbaAlgs::pack_decimal(
            AdServer::CampaignSvcs::RevenueDecimal::MAXIMUM);
        }

        // normalize currency codes
        if(!slot_it->min_cpm_price_currency_code.empty())
        {
          String::AsciiStringManip::to_lower(slot_it->min_cpm_price_currency_code);
        }

        if(!context.currencies.empty())
        {
          for(auto currency_it = context.currencies.begin();
            currency_it != context.currencies.end(); ++currency_it)
          {
            String::AsciiStringManip::to_lower(*currency_it);
          }
        }

        if(!context.currencies.empty())
        {
          CorbaAlgs::fill_sequence(
            context.currencies.begin(),
            context.currencies.end(),
            ad_slot_request.currency_codes);
        }
        else if(!slot_it->min_cpm_price_currency_code.empty())
        {
          ad_slot_request.currency_codes.length(1);
          ad_slot_request.currency_codes[0] << slot_it->min_cpm_price_currency_code;
        }

        if(!slot_it->min_cpm_price_currency_code.empty())
        {
          ad_slot_request.min_ecpm_currency_code << slot_it->min_cpm_price_currency_code;
        }
        else
        {
          ad_slot_request.min_ecpm_currency_code << Request::OpenRtb::DEFAULT_BIDFLOORCUR_CURRENCY;
        }

        ad_slot_request.up_expand_space = -1;
        ad_slot_request.right_expand_space = -1;
        ad_slot_request.down_expand_space = -1;
        ad_slot_request.left_expand_space = -1;

        ad_slot_request.debug_ccg = 0;

        ad_slot_request.ext_tag_id << ext_tag_id;

        ++slot_i;
        ++slot_it;
      }
      else
      {
        context.ad_slots.erase(slot_it++);
      }
    }

    request_params.ad_slots.length(slot_i);

    if((!context.ad_slots.empty() &&
         context.ad_slots.front().secure) ||
       context.secure)
    {
      request_params.common_info.creative_instantiate_type <<
        FrontendCommons::SECURE_INSTANTIATE_TYPE;
    }
    else if(request_params.common_info.creative_instantiate_type[0] == 0)
    {
      // was not filled by fill_by_referer_ and later
      request_params.common_info.creative_instantiate_type <<
        FrontendCommons::UNSECURE_INSTANTIATE_TYPE;
    }

    // fill idfa or advertising_id by ifa depends on device
    context.ifa = FrontendCommons::normalize_ifa(context.ifa);

    if(!context.ifa.empty())
    {
      // check idfa
      if(request_info.platform_names.find(PlatformNames::IPAD) != request_info.platform_names.end() ||
        request_info.platform_names.find(PlatformNames::IPHONE) != request_info.platform_names.end() ||
        request_info.platform_names.find(PlatformNames::IOS) != request_info.platform_names.end())
      {
        request_info.idfa = context.ifa;
      }
      else
      {
        request_info.advertising_id = context.ifa;
      }
    }

    // fill ext_track_params
    if(!context.puid1.empty() || !context.puid2.empty())
    {
      std::string ext_track_params;
      if(!context.puid1.empty())
      {
        ext_track_params += "puid1=";
        std::string mimed_puid1;
        String::StringManip::mime_url_encode(context.puid1, mimed_puid1);
        ext_track_params += mimed_puid1;
      }

      if(!context.puid2.empty())
      {
        if(!ext_track_params.empty())
        {
          ext_track_params += '&';
        }
        ext_track_params += "puid2=";
        std::string mimed_puid2;
        String::StringManip::mime_url_encode(context.puid2, mimed_puid2);
        ext_track_params += mimed_puid2;
      }

      request_params.common_info.ext_track_params << ext_track_params;
    }

    if(!request_info.idfa.empty() || !request_info.advertising_id.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_IDFA);
      kw_fmt.add_keyword(MatchKeywords::FULL_IDFA_KNOWN);
    }

    kw_fmt.assign_to(keywords);

    add_special_keywords_(keywords, request_info, &context, context.app_id);

    request_params.ssp_location << (
      context.ssp_country + '/' + context.ssp_region +
      '/' + context.ssp_city);

    // push eids to request_info
    for(auto it = context.user_eids.begin(); it != context.user_eids.end(); ++it)
    {
      for(auto sub_it = it->uids.begin(); sub_it != it->uids.end(); ++sub_it)
      {
        std::string source = (!it->source.empty() ? it->source : request_info.source_id);
        auto source_mapping_it = source_mapping_.find(source);
        if(source_mapping_it != source_mapping_.end())
        {
          source = source_mapping_it->second;
        }

        if(!sub_it->id.empty())
        {
          request_info.ext_user_ids.emplace_back(source + "/" + sub_it->id);
        }
        else if(!sub_it->stable_id.empty())
        {
          request_info.ext_user_ids.emplace_back(
            (!source.empty() ? source : Request::OpenRtb::STABLE_SOURCE) + "/" + sub_it->stable_id);
        }
      }
    }
  }

  void
  RequestInfoFiller::fill_by_appnexus_request(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    RequestInfo& request_info,
    std::string& keywords,
    JsonProcessingContext& context,
    const char* bid_request) const
    /*throw(InvalidParamException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill_by_appnexus_request()";

    request_params.common_info.request_type = AdServer::CampaignSvcs::AR_APPNEXUS;
    request_params.context_info.client << APPNEXUS_APPLICATION;
    request_params.context_info.client_version << APPNEXUS_APPLICATION_VERSION;

    init_request_param(request_params, request_info);

    JsonValue root_value;
    JsonAllocator json_allocator;
    Generics::ArrayAutoPtr<char> bid_request_holder(strlen(bid_request) + 1);
    char* parse_end;
    ::strcpy(bid_request_holder.get(), bid_request);
    JsonParseStatus status = json_parse(
      bid_request_holder.get(), &parse_end, &root_value, json_allocator);

    if(status != JSON_PARSE_OK)
    {
      Stream::Error ostr;
      ostr << FUN << ": parsing error '" <<
        json_parse_error(status) << "' at pos : ";
      if(parse_end)
      {
        ostr << std::string(parse_end, 20);
      }
      else
      {
        ostr << "null";
      }
      throw InvalidParamException(ostr);
    }

    JsonTag root_tag = root_value.getTag();
    if(root_tag != JSON_TAG_OBJECT)
    {
      Stream::Error ostr;
      ostr << FUN << ": incorrect root tag type";
      throw InvalidParamException(ostr);
    }

    try
    {
      appnexus_root_processor_->process(request_params, context, root_value);
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": processing error: " << e.what();
      throw InvalidParamException(ostr);
    }

    //context.print(std::cerr);
    KeywordFormatter kw_fmt(request_info.source_id);

    kw_fmt.add_keyword(MatchKeywords::FULL_REQ);

    if(!request_info.source_id.empty())
    {
      kw_fmt.add_dict_keyword(MatchKeywords::REQ, String::SubString());
    }

    kw_fmt.add_age(context.age);
    kw_fmt.add_gender(context.gender);

    // filter by member_id
    bool ignore_request_by_member_id = false;

    if(request_info.appnexus_member_id.present() &&
       context.member_ids.find(*request_info.appnexus_member_id) ==
         context.member_ids.end())
    {
      ignore_request_by_member_id = true;
    }

    // from bid_info
    for(ULongSet::const_iterator
          ul_set_iter = context.inventory_audits_content_categories.begin();
          ul_set_iter != context.inventory_audits_content_categories.end();
          ++ul_set_iter)
    {
      kw_fmt.add_cat(*ul_set_iter);
    }

    // from tags
    for(JsonAdSlotProcessingContextList::const_iterator
          slot_iter = context.ad_slots.begin();
          slot_iter != context.ad_slots.end();
          ++slot_iter)
    {
      for(ULongSet::const_iterator
            ul_set_iter = slot_iter->inventory_audits_content_categories.begin();
            ul_set_iter != slot_iter->inventory_audits_content_categories.end();
            ++ul_set_iter)
      {
        kw_fmt.add_cat(*ul_set_iter);
      }
    }

    // Moved from BiddingFrontend.cpp (ADSC-9180), old keyword format support
    if(!context.age.empty())
    {
      kw_fmt.add_keyword(std::string("appnexusage") + context.age);
    }

    if(!context.gender.empty())
    {
      kw_fmt.add_keyword(std::string("appnexus") + context.gender);
    }

    if (context.external_user_id.empty())
    {
      context.external_user_id = make_ssp_uid_by_device_(context);
    }

    if(!context.external_user_id.empty() &&
      use_external_user_id_(context.external_user_id))
    {
      request_params.common_info.external_user_id << (
        !request_info.source_id.empty() ?
          request_info.source_id + "/" + context.external_user_id :
          context.external_user_id);
    }

    fill_by_ip(request_info, request_params, context.ip);

    if(!context.ip.empty())
    {
      kw_fmt.add_ip(String::SubString(context.ip));
    }

    verify_user_id_(context.user_id, request_info.source_id, request_params);

    request_info.is_app = context.app;

    fill_by_user_agent(
      request_params,
      request_info,
      context.user_agent,
      request_info.filter_request,
      context.app);

    {
      HTTP::HTTPAddress referer;
      select_referer_(request_params, context, referer);
      if(referer.url().empty())
      {
        try
        {
          referer = HTTP::BrowserAddress(context.app_id);
          request_params.context_info.profile_referer = false;
        }
        catch(const eh::Exception&)
        {}
      }

      fill_by_referer(request_params, request_info.search_words, referer);

      if(request_params.common_info.referer[0] == 0)
      {
        kw_fmt.add_keyword(MatchKeywords::FULL_NOREF);
      }
    }

    request_params.search_words << request_info.search_words;

    request_params.common_info.log_as_test = context.test;

    request_params.ad_slots.length(context.ad_slots.size());
    CORBA::ULong slot_i = 0;
    for(JsonAdSlotProcessingContextList::iterator slot_it =
          context.ad_slots.begin();
        slot_it != context.ad_slots.end(); )
    {
      AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& ad_slot_request =
        request_params.ad_slots[slot_i];

      init_adslot(ad_slot_request);

      ad_slot_request.fill_track_html = false;
      ad_slot_request.ad_slot_id = slot_i;
      ad_slot_request.tag_id = 0;
      if (slot_it->video_min_duration.good())
      {
        ad_slot_request.video_min_duration = *(slot_it->video_min_duration);
      }
      else
      {
        ad_slot_request.video_min_duration = 0;
      }
      ad_slot_request.video_max_duration = -1;
      ad_slot_request.video_width = 0;
      ad_slot_request.video_height = 0;


      // VAST
      bool appnexus_vast = false;
      if (slot_it->allowed_media_types.find(4) != slot_it->allowed_media_types.end())
      {
        appnexus_vast = true;
        ad_slot_request.sizes.length(1);
        ad_slot_request.sizes[0] << VAST_PROTOCOL_SIZE;
        ad_slot_request.format << OPENRTB_VAST_APP_FORMAT;

        AdServer::CampaignSvcs::AllowedDurationSet allowed_durations;

        if (slot_it->allowed_media_subtypes.empty() ||
            (slot_it->allowed_media_subtypes.find(23) !=
              slot_it->allowed_media_subtypes.end()))
        {
          // Use empty allowed_durations
        }
        else
        {
          if (slot_it->allowed_media_subtypes.find(7) !=
                slot_it->allowed_media_subtypes.end())
          {
            allowed_durations.insert(15);
          }
          if (slot_it->allowed_media_subtypes.find(8) !=
            slot_it->allowed_media_subtypes.end())
          {
            allowed_durations.insert(30);
          }
        }

        CorbaAlgs::fill_sequence(
          allowed_durations.begin(),
          allowed_durations.end(),
          ad_slot_request.allowed_durations);
      }
      else
      {
        ad_slot_request.format << APPNEXUS_APP_FORMAT;
      }

      CorbaAlgs::fill_sequence(
        context.exclude_categories.begin(),
        context.exclude_categories.end(),
        ad_slot_request.exclude_categories);

      if (context.no_flash)
      {
        ad_slot_request.exclude_categories.length(
          ad_slot_request.exclude_categories.length() + 1);
        ad_slot_request.exclude_categories[
          ad_slot_request.exclude_categories.length() - 1] << std::string("flash");
      }

      // ADSC-9852: Ignore geometric size in request when VAST tag should be selected
      if (!appnexus_vast)
      {
        const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
        const auto size_it = debug_sizes.find(slot_i);
        if (size_it != debug_sizes.end())
        {
          ad_slot_request.sizes.length(1);
          ad_slot_request.sizes[0] << size_it->second;
        }
        else if (!request_info.default_debug_size.empty())
        {
          ad_slot_request.sizes.length(1);
          ad_slot_request.sizes[0] << request_info.default_debug_size;
        }
        else if(!slot_it->appnexus_sizes.empty())
        {
          ad_slot_request.sizes.length(1);
          ad_slot_request.sizes[0] << *(slot_it->appnexus_sizes.begin());
        }
      }
      else if (!slot_it->appnexus_sizes.empty())
      {
        unsigned long video_w = 0;
        unsigned long video_h = 0;
        size_decode(*(slot_it->appnexus_sizes.begin()), video_w, video_h);
        if (video_w && video_h)
        {
          fill_video_size( kw_fmt, ad_slot_request,  video_w,  video_h);
        }
      }
      else
      {
        fill_video_size(kw_fmt, ad_slot_request,  0, 0);
      }

      ad_slot_request.passback = ignore_request_by_member_id ||
        request_info.filter_request;

      // Unit is 1.00 / 1000 imps
      ad_slot_request.min_ecpm = CorbaAlgs::pack_decimal(
        AdServer::CampaignSvcs::RevenueDecimal::mul(
          slot_it->min_cpm_price,
          AdServer::CampaignSvcs::RevenueDecimal(false, 100, 0),
          Generics::DMR_ROUND));

      ad_slot_request.up_expand_space = -1;
      ad_slot_request.right_expand_space = -1;
      ad_slot_request.down_expand_space = -1;
      ad_slot_request.left_expand_space = -1;
      if(slot_it->appnexus_pos.empty() || slot_it->appnexus_pos == "unknown")
      {
        ad_slot_request.tag_visibility = -1;
      }
      else if(slot_it->appnexus_pos == "above")
      {
        ad_slot_request.tag_visibility = 100;
      }
      else // below
      {
        ad_slot_request.tag_visibility = 0;
      }
      ad_slot_request.debug_ccg = 0;

      // AppNexus: predicted_view_rate * 100 and rounded to nearest integer, -1 if not sent
      ad_slot_request.tag_predicted_viewability =
        (slot_it->predicted_view_rate == -1) ? -1 : round(slot_it->predicted_view_rate * 100);
      if (ad_slot_request.tag_predicted_viewability < -1 || ad_slot_request.tag_predicted_viewability > 100)
      {
        ad_slot_request.tag_predicted_viewability = -1;
      }

      if (context.selling_member_id.present() ||
        slot_it->inventory_source_id.present() ||
        slot_it->site_id.present() ||
        slot_it->slot_id.present())
      {
        Stream::Stack<EXT_TAG_ID_MAX_LENGTH> oss;
        oss << context.selling_member_id << '-' << slot_it->inventory_source_id
            << '-' << slot_it->site_id << '-' << slot_it->slot_id;
        ad_slot_request.ext_tag_id << normalize_ext_tag_id_(oss.str());
      }

      ++slot_i;
      ++slot_it;
    }

    request_params.ad_slots.length(slot_i);

    if((!context.ad_slots.empty() &&
         context.ad_slots.front().secure) ||
       context.secure)
    {
      request_params.common_info.creative_instantiate_type <<
        FrontendCommons::SECURE_INSTANTIATE_TYPE;
    }
    else if(request_params.common_info.creative_instantiate_type[0] == 0)
    {
      // was not filled by fill_by_referer_ and later
      request_params.common_info.creative_instantiate_type <<
        FrontendCommons::UNSECURE_INSTANTIATE_TYPE;
    }

    if(!request_info.idfa.empty() || !request_info.advertising_id.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_IDFA);
      kw_fmt.add_keyword(MatchKeywords::FULL_IDFA_KNOWN);
    }

    kw_fmt.assign_to(keywords);

    add_special_keywords_(keywords, request_info);
  }

  void
  RequestInfoFiller::init_param_processors_()
    noexcept
  {
    add_param_processor_(
      Request::Context::RANDOM,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::random));
    add_param_processor_(
      Request::Context::PUBLISHER_ACCOUNT_ID,
      new FrontendCommons::NumberContainerParamProcessor<
        RequestInfo,
        RequestInfo::AccountIdArray,
        String::AsciiStringManip::SepComma>(&RequestInfo::publisher_account_ids));
    add_param_processor_(
      Request::Context::PUBLISHER_SITE_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::publisher_site_id));
    add_param_processor_(
      Request::Context::FLAG,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::flag));
    add_param_processor_(
      Request::Context::SOURCE_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::source_id));
    add_param_processor_(
      Request::Context::FORMAT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::format));
    add_param_processor_(
      Request::Debug::CURRENT_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(
        &RequestInfo::current_time, Generics::Time::ONE_DAY));
    add_param_processor_(
      Request::Context::LOCATION_NAME,
      new FrontendCommons::LocationNameParamProcessor<RequestInfo>(
        &RequestInfo::location));
    add_param_processor_(
      Request::Debug::EXPECTED_CCG,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::debug_ccg));
    add_param_processor_(
      Request::Debug::ADSLOTS_SIZE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::default_debug_size));
  }

  void
  RequestInfoFiller::add_param_processor_(
    const String::SubString& name,
    RequestInfoParamProcessor* processor)
    noexcept
  {
    param_processors_.insert(std::make_pair(
      name,
      RequestInfoParamProcessor_var(processor)));
  }

  void
  RequestInfoFiller::init_appnexus_processors_()
    noexcept
  {
    JsonCompositeRequestParamProcessor_var root_processor =
      new JsonCompositeRequestParamProcessor();

    JsonCompositeRequestParamProcessor_var request_processor =
      new JsonCompositeRequestParamProcessor();

    request_processor->add_processor(
      Request::AppNexus::REQUEST_DEBUGREQUESTED,
      JsonRequestParamProcessor_var(
        new JsonContextBoolParamProcessor<JsonProcessingContext>(
          &JsonProcessingContext::test, std::logical_or<bool>())));

    request_processor->add_processor(
      Request::AppNexus::REQUEST_TEST,
      JsonRequestParamProcessor_var(
        new JsonContextBoolParamProcessor<JsonProcessingContext>(
          &JsonProcessingContext::test, std::logical_or<bool>())));

    {
      JsonCompositeRequestParamProcessor_var bid_info_processor =
        new JsonCompositeRequestParamProcessor();

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_USERID,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::external_user_id)));

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_IP,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::ip)));

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_USERAGENT,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::user_agent)));

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_URL,
        JsonRequestParamProcessor_var(
          new JsonContextURLParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::site_page)));

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_GENDER,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::gender)));

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_AGE,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::age)));

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_SELLING_MEMBER_ID,
        JsonRequestParamProcessor_var(
          new JsonContextNumberParamProcessor<JsonProcessingContext,
            AdServer::Commons::Optional<long> >(
            &JsonProcessingContext::selling_member_id)));

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_APPID,
        JsonRequestParamProcessor_var(
          new JsonContextStringParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::app_id)));

      bid_info_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO_NO_FLASH,
        JsonRequestParamProcessor_var(
          new JsonContextBoolParamProcessor<JsonProcessingContext>(
            &JsonProcessingContext::no_flash)));

      bid_info_processor->add_processor(
        Request::AppNexus::INVENTORY_AUDITS,
        JsonRequestParamProcessor_var(
          new BidInfoInventoryAuditsParamProcessor()));

      request_processor->add_processor(
        Request::AppNexus::REQUEST_BIDINFO,
        bid_info_processor);
    }

    {
      ReferenceCounting::SmartPtr<
        JsonCompositeParamProcessor<JsonProcessingContext> >
          member_processor =
            new JsonCompositeParamProcessor<JsonProcessingContext>();

      member_processor->add_processor(
        Request::AppNexus::REQUEST_MEMBERS_ID,
        JsonRequestParamProcessor_var(
          new AppNexusMemberIdParamProcessor()));

      request_processor->add_processor(
        Request::AppNexus::REQUEST_MEMBERS,
        JsonRequestParamProcessor_var(
          new JsonArrayParamProcessor<JsonProcessingContext>(member_processor)));
    }

    request_processor->add_processor(
      Request::AppNexus::REQUEST_TAGS,
      JsonRequestParamProcessor_var(
        new AppNexusTagParamProcessor()));

    root_processor->add_processor(
      Request::AppNexus::REQUEST,
      request_processor);

    appnexus_root_processor_ = root_processor;
  }

  void
  RequestInfoFiller::fill_by_user_agent(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    RequestInfo& request_info,
    String::SubString user_agent,
    bool filter_request,
    bool application)
    const
    noexcept
  {
    static const char* FUN = "RequestInfoFiller::fill_by_user_agent()";

    request_params.common_info.set_cookie = !application;

    if(!user_agent.empty())
    {
      FrontendCommons::WebBrowserMatcher_var web_browser_matcher =
        common_module_->web_browser_matcher();

      std::string web_browser;

      if(web_browser_matcher.in())
      {
        try
        {
          web_browser_matcher->match(
            web_browser,
            user_agent);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Web browser matching exception: " << ex.what();
          logger_->log(
            ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-109");
        }
      }

      request_params.common_info.user_agent << user_agent;
      request_params.context_info.web_browser << web_browser;
    }

    if (!user_agent.empty() || application)
    {
      FrontendCommons::PlatformMatcher_var platform_matcher =
        common_module_->platform_matcher();

      std::string platform;
      std::string full_platform;
      FrontendCommons::PlatformMatcher::PlatformIdSet platform_ids;

      if(!filter_request && platform_matcher.in())
      {
        try
        {
          platform_matcher->match(
            &platform_ids,
            &request_info.platform_names,
            platform,
            full_platform,
            user_agent,
            application);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Platform matching exception: " << ex.what();
          logger_->log(
            ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::BIDDING_FRONTEND,
            "ADS-IMPL-109");
        }
      }

      request_params.context_info.platform << platform;
      request_params.context_info.full_platform << full_platform;
      CorbaAlgs::fill_sequence(
        platform_ids.begin(),
        platform_ids.end(),
        request_params.context_info.platform_ids);
    }
  }

  void
  RequestInfoFiller::fill_by_referer(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    std::string& search_words,
    const HTTP::HTTPAddress& referer,
    bool fill_search_words,
    bool fill_instantiate_type)
    const
    noexcept
  {
    if(!referer.url().empty())
    {
      try
      {
        if(fill_instantiate_type && referer.secure())
        {
          request_params.common_info.creative_instantiate_type <<
            FrontendCommons::SECURE_INSTANTIATE_TYPE;
        }

        std::string normalized_referer =
          FrontendCommons::normalize_abs_url(referer);
        request_params.common_info.referer << normalized_referer;
        request_params.context_info.full_referer_hash =
           FrontendCommons::referer_hash(normalized_referer);
        request_params.context_info.short_referer_hash =
          FrontendCommons::short_referer_hash(normalized_referer);

        if (fill_search_words)
        {
          fill_search_words_(request_params, search_words, referer);
        }
      }
      catch(...)
      {}
    }
  }

  void
  RequestInfoFiller::fill_additional_url_(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    std::string& search_words,
    const HTTP::HTTPAddress& add_url)
    const
    noexcept
  {
    if(!add_url.url().empty())
    {
      try
      {
        std::string normalized_add_url =
          FrontendCommons::normalize_abs_url(add_url);
        CORBA::ULong pos = request_params.common_info.urls.length();
        request_params.common_info.urls.length(pos + 1);
        request_params.common_info.urls[pos] << normalized_add_url;

        fill_search_words_(request_params, search_words, add_url);
      }
      catch(...)
      {}
    }
  }

  void
  RequestInfoFiller::fill_search_words_(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    std::string& search_words,
    const HTTP::HTTPAddress& url)
    const
    noexcept
  {
    if(!url.url().empty())
    {
      FrontendCommons::UrlMatcher_var url_matcher =
        common_module_->url_matcher();
      if(url_matcher.in())
      {
        unsigned long search_engine_id;
        std::string search_words_value;

        Language::Segmentor::SegmentorInterface_var segmentor =
          common_module_->segmentor();

        if(url_matcher->match(
             search_engine_id,
             search_words_value,
             url,
             segmentor))
        {
          // don't override search_engine_id got by referer parsing
          if(request_params.search_engine_id == 0)
          {
            request_params.search_engine_id = search_engine_id;
          }

          if(search_words.empty())
          {
            search_words.swap(search_words_value);
          }
          else
          {
            search_words += "\n";
            search_words += search_words_value;
          }
        }
      }
    }
  }

  void
  RequestInfoFiller::fill_by_ip(
    RequestInfo& request_info,
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    String::SubString ip)
    const
    noexcept
  {
    if(request_info.location.in())
    {
      request_params.common_info.location.length(1);
      request_params.common_info.location[0].country << request_info.location->country;
      request_params.common_info.location[0].region << request_info.location->region;
      request_params.common_info.location[0].city << request_info.location->city;
    }
    else
    {
      GeoIPMapping::IPMapCity2::CityLocation geo_location;

      if(!ip.empty())
      {
        request_params.common_info.peer_ip << ip;

        if(ip_logging_enabled_)
        {
          std::string ip_hash;
          FrontendCommons::ip_hash(ip_hash, ip, ip_salt_);
          request_params.context_info.ip_hash << ip_hash;
        }

        if(ip_map_.get())
        {
          try
          {
            if(ip_map_->city_location_by_addr(
                 ip.str().c_str(),
                 geo_location,
                 false))
            {
              request_params.common_info.location.length(1);
              request_params.common_info.location[0].country << geo_location.country_code;
              request_params.common_info.location[0].region << geo_location.region;
              request_params.common_info.location[0].city << geo_location.city;
            }
          }
          catch(const eh::Exception&)
          {}
        }
      }

      FrontendCommons::CountryFilter_var country_filter =
        common_module_->country_filter();

      if(country_filter.in())
      {
        if(geo_location.country_code.empty() ||
           !country_filter->enabled(geo_location.country_code))
        {
          request_info.filter_request = true;
        }
      }
    }
  }

  bool
  RequestInfoFiller::use_external_user_id_(
    String::SubString external_user_id)
    const noexcept
  {
    return skip_external_ids_.find(external_user_id.str()) ==
      skip_external_ids_.end();
  }

  void
  RequestInfoFiller::verify_user_id_(
    const std::string& signed_user_id,
    const std::string& source_id,
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params)
    const noexcept
  {
    static const size_t UNSIGNED_SSP_USER_ID_LEN = 22;

    if (!signed_user_id.empty())
    {
      try
      {
        Generics::Uuid ssp_uid;
        uint8_t ssp_uid_marker;
        if (signed_user_id.length() == UNSIGNED_SSP_USER_ID_LEN)
        {
          std::string ssp_id_buf;
          String::StringManip::base64mod_decode(
            ssp_id_buf, signed_user_id, false, &ssp_uid_marker);
          ssp_uid = Generics::Uuid(ssp_id_buf.begin(), ssp_id_buf.end());
        }
        else
        {
          Generics::SignedUuid signed_ssp_uid =
            common_module_->user_id_controller()->verify(
              signed_user_id, UserIdController::SSP);
          ssp_uid = signed_ssp_uid.uuid();
          ssp_uid_marker = signed_ssp_uid.data();
        }
        // Don't check for empty ssp_uid (UserIdBlackList)

        const Generics::Uuid uid =
          common_module_->user_id_controller()->get_by_ssp_user_id(
            ssp_uid, source_id, ssp_uid_marker);

        if (uid.is_null())
        {
          // for appnexus, google: keep undefined state.
          // Set it in in case openrtb.
          request_params.common_info.user_status = static_cast<CORBA::ULong>(
            AdServer::CampaignSvcs::US_UNDEFINED);
          return;
        }

        request_params.common_info.user_id = CorbaAlgs::pack_user_id(uid);
        request_params.common_info.track_user_id =
          request_params.common_info.user_id;

        if(AdServer::Commons::PROBE_USER_ID.to_string() == signed_user_id)
        {
          request_params.common_info.user_status = static_cast<CORBA::ULong>(
            AdServer::CampaignSvcs::US_PROBE);
          return;
        }

        const Generics::SignedUuid signed_uid =
          common_module_->user_id_controller()->sign(uid);

        request_params.common_info.signed_user_id << signed_uid.str();
      }
      catch(...)
      {
      }
    }
  }

  void
  RequestInfoFiller::select_referer_(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const JsonProcessingContext& context,
    HTTP::HTTPAddress& referer) const
    /*throw(eh::Exception)*/
  {
    enum SelectedReferer
    {
      SR_PAGE,
      SR_DOMAIN,
      SR_REF,
      SR_REFERER,
      SR_REREFERER,
      SR_NAME,
      SR_NONE
    };

    const HTTP::HTTPAddress* URL_FIELDS[] = {
      &context.site_page, &context.site_domain, &context.site_ref, &context.site_referer,
      &context.site_rereferer
    };

    bool site_name_url_initialized = false;
    HTTP::HTTPAddress site_name_url;
    SelectedReferer selected_referer_field = SR_NONE;

    if (!context.site_page.url().empty())
    {
      const std::string site_page =
        first_significant_domain_part_(context.site_page.host());

      if (!context.site_domain.url().empty())
      {
        const std::string site_domain =
          first_significant_domain_part_(context.site_domain.host());

        if (site_page == site_domain)
        {
          referer = context.site_page;
          selected_referer_field = SR_PAGE;
        }
      }

      if (referer.url().empty() && !context.site_name.empty())
      {
        JsonContextURLParamProcessor<JsonProcessingContext>::fill_url(
          context.site_name,
          site_name_url);

        const std::string site_name =
          first_significant_domain_part_(site_name_url.host());

        if (site_page == site_name)
        {
          referer = context.site_page;
          selected_referer_field = SR_PAGE;
        }

        site_name_url_initialized = true;
      }
    }

    if(referer.url().empty()) 
    {
      if (!context.site_domain.url().empty())
      {
        referer = context.site_domain;
        selected_referer_field = SR_DOMAIN;
      }
      else
      {
        if(!site_name_url_initialized)
        {
          JsonContextURLParamProcessor<JsonProcessingContext>::fill_url(
            context.site_name,
            site_name_url);
          site_name_url_initialized = true;
        }

        if (!site_name_url.url().empty())
        {
          referer = site_name_url;
          selected_referer_field = SR_NAME;
        }
        else if(!context.site_page.url().empty())
        {
          referer = context.site_page;
          selected_referer_field = SR_PAGE;
        }
        else
        {
          referer = context.site_ref;
          selected_referer_field = SR_REF;
        }
      }
    }

    if(referer.url().empty())
    {
      referer = context.site_referer;
      selected_referer_field = SR_REFERER;
    }

    for(unsigned int url_i = 0; url_i < sizeof(URL_FIELDS) / sizeof(URL_FIELDS[0]); ++url_i)
    {
      try
      {
        if(selected_referer_field != url_i && !URL_FIELDS[url_i]->url().empty())
        {
          std::string search_words; // ignore search words value (push to match only)
          fill_additional_url_(
            request_params,
            search_words,
            *URL_FIELDS[url_i]);
        }
      }
      catch(const eh::Exception&)
      {//ignory bad url
      }
    }

    try
    {
      if(selected_referer_field != SR_NAME && !context.site_name.empty())
      {
        std::string search_words; // ignore search words value (push to match only)
        HTTP::BrowserAddress site_name_url(context.site_name);
        fill_additional_url_(
          request_params,
          search_words,
          site_name_url);
      }
    }
    catch(const eh::Exception&)
    {//ignory bad url
    }
  }

  std::string
  RequestInfoFiller::first_significant_domain_part_(
    const String::SubString& host) const
    /*throw(eh::Exception)*/
  {
    std::string domain;
    common_module_->domain_parser()->specific_domain(host, domain);
    const std::size_t pos = domain.find('.');

    if (pos != std::string::npos)
    {
      domain.erase(pos);
    }

    return domain;
  }

  std::string
  RequestInfoFiller::normalize_ext_tag_id_(const String::SubString& src)
    noexcept
  {
    String::SubString res;
    if(String::StringManip::utf8_substr(src, EXT_TAG_ID_MAX_LENGTH, res))
    {
      return res.str();
    }

    std::string enc_res;
    AdServer::LogProcessing::undisplayable_mime_encode(
      enc_res, src.substr(0, EXT_TAG_ID_MAX_LENGTH));
    return enc_res.substr(0, EXT_TAG_ID_MAX_LENGTH);
  }

  void
  RequestInfoFiller::init_adslot(
    AdServer::CampaignSvcs::CampaignManager::AdSlotInfo& adslot_info)
    noexcept
  {
    adslot_info.video_allow_skippable = true;
    adslot_info.video_allow_unskippable = true;
    adslot_info.video_skippable_max_duration = -1;
  }

  void
  RequestInfoFiller::init_request_param(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    RequestInfo& request_info) const noexcept
  {
    CorbaAlgs::fill_sequence(
      request_info.publisher_account_ids.begin(),
      request_info.publisher_account_ids.end(),
      request_params.publisher_account_ids);

    request_params.publisher_site_id = request_info.publisher_site_id;

    request_params.common_info.source_id << request_info.source_id;
    request_params.common_info.time = CorbaAlgs::pack_time(request_info.current_time);

    request_params.context_info.profile_referer = true;
    request_params.context_info.full_referer_hash = 0;
    request_params.context_info.short_referer_hash = 0;
    request_params.context_info.page_load_id = 0;

    request_params.fill_track_pixel = false;
    request_params.fill_iurl = request_info.flag & 0x01;

    request_params.common_info.request_id = CorbaAlgs::pack_request_id(
      AdServer::Commons::RequestId::create_random_based());

    request_params.common_info.colo_id = colo_id_;
    request_params.search_engine_id = 0;
    request_params.page_keywords_present = false;
    request_params.profiling_available = true;
    request_params.fraud = false;
    request_params.disable_fraud_detection = false;
    request_params.common_info.random = request_info.random;
    request_params.required_passback = false;
    request_params.need_debug_info = false;
    request_params.only_display_ad = true;
    request_params.common_info.test_request = false;
    request_params.preview_ccid = 0;
    request_params.common_info.hpos = CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM;

    fill_request_type_(request_info, request_params, request_info.source_id);
  }

  std::string
  RequestInfoFiller::make_ssp_uid_by_device_(const JsonProcessingContext& ctx)
    /*throw(std::exception)*/
  {
    /*
    if (!ctx.ifa.empty())
    {
      return ctx.ifa;
    }
    */

    if (!ctx.didmd5.empty())
    {
      return ctx.didmd5;
    }

    if (!ctx.didsha1.empty())
    {
      return ctx.didsha1;
    }

    if (!ctx.dpidmd5.empty())
    {
      return ctx.dpidmd5;
    }

    if (!ctx.dpisha1.empty())
    {
      return ctx.dpisha1;
    }

    if (!ctx.macsha1.empty())
    {
      return ctx.macsha1;
    }

    return ctx.macmd5;
  }

  std::string
  RequestInfoFiller::adapt_app_store_url_(
    const String::SubString& store_url_str)
    /*throw(eh::Exception)*/
  {
    HTTP::BrowserAddress store_url(store_url_str);

    if(store_url.host().compare(GooglePlay::STORE_HOST) == 0 &&
      store_url.path().compare(
        0, GooglePlay::STORE_PATH.size(), GooglePlay::STORE_PATH) == 0)
    {
      const String::SubString args = store_url.query();

      HTTP::ParamList params;
      FCGI::HttpRequest::parse_params(args, params);
      for(auto param_it = params.begin(); param_it != params.end(); ++param_it)
      {
        if(GooglePlay::STORE_APP_ID == param_it->name)
        {
          try
          {
            String::SubString url_sstr(param_it->value);
            String::StringManip::trim(url_sstr);
            HTTP::BrowserAddress app_id(url_sstr);
            return app_id.host().str();
          }
          catch(const eh::Exception&)
          {}
        }
      }
    }
    else if(store_url.host().compare(AppStore::STORE_HOST) == 0)
    {
      // example : https://itunes.apple.com/us/app/youcam-perfect/id768469908
      String::StringManip::Splitter<String::AsciiStringManip::SepSlash>
        tokenizer(store_url.path());
      String::SubString token;
      while(tokenizer.get_token(token))
      {
        if(token.compare(0, 2, "id") == 0 && token.size() > 3)
        {
          try
          {
            HTTP::BrowserAddress app_id(token);
            return app_id.host().str();
          }
          catch(const eh::Exception&)
          {}
        }
      }
    }

    return store_url_str.str();
  }

  std::string
  RequestInfoFiller::norm_keyword_(const String::SubString& kw) noexcept
  {
    std::string simplified_kw;
    String::case_change<String::Simplify>(kw, simplified_kw);
    std::replace(
      simplified_kw.begin(),
      simplified_kw.end(),
      ' ',
      'x');
    return simplified_kw;
  }

  void
  RequestInfoFiller::add_special_keywords_(
    std::string& keywords,
    const RequestInfo& request_info,
    const JsonProcessingContext* context,
    const String::SubString& alt_app_id)
    const
    noexcept
  {
    KeywordFormatter kw_fmt(request_info.source_id);

    for(auto platform_name_it = request_info.platform_names.begin();
      platform_name_it != request_info.platform_names.end();
      ++platform_name_it)
    {
      kw_fmt.add_keyword(std::string("rtbplatform") + norm_keyword_(*platform_name_it));
    }

    if(request_info.is_app)
    {
      std::string app_id = norm_keyword_(request_info.application_id);
      if(!app_id.empty())
      {
        kw_fmt.add_keyword(std::string("rtbapp") + app_id);
      }

      if(!alt_app_id.empty())
      {
        app_id = norm_keyword_(alt_app_id);
        if(!app_id.empty())
        {
          kw_fmt.add_keyword(std::string("rtbapp") + app_id);
        }
      }
    }

    std::string special_keywords;
    kw_fmt.assign_to(special_keywords);

    if(!special_keywords.empty())
    {
      if(!keywords.empty())
      {
        keywords += "\n";
      }

      keywords += special_keywords;
    }

    if(context)
    {
      if(!context->language.empty())
      {
        kw_fmt.add_keyword(std::string("rtblanguage") + norm_keyword_(context->language));
      }
    }    
  }
}
}
