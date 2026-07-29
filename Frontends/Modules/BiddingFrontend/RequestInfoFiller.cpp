#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <limits>
#include <math.h>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <String/AsciiStringManip.hpp>
#include <String/UTF8Case.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>
#include <Commons/DecimalUtils.hpp>
#include <Generics/Rand.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <Language/SegmentorManager/SegmentorManager.hpp>

#include <Frontends/FrontendCommons/OptOutManip.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "Utils.hpp"
#include "KeywordFormatter.hpp"
#include "RequestInfoFiller.hpp"

namespace Aspect
{
  const char BIDDING_FRONTEND[] = "BiddingFrontend";
}

namespace Request::Context
{
  extern const std::string_view PUBLISHER_ACCOUNT_ID("aid");
  extern const std::string_view PUBLISHER_SITE_ID("sid");
  extern const std::string_view SOURCE_ID("src");
  extern const std::string_view RANDOM("random");
  extern const std::string_view FLAG("f");
  extern const std::string_view FORMAT("format");
  extern const std::string_view LOCATION_NAME("loc.name");
  extern const std::string_view REQUIRE_DEBUG_INFO("require-debug-info");
}

namespace Request::Debug
{
  const std::string_view EXPECTED_CCG("debug.ccg");
  const std::string_view CURRENT_TIME("debug.time");
  const std::string_view ADSLOTS_SIZE("debug.size");
  const std::string_view AD_SLOT("debug.adslot");

  namespace AdSlot
  {
    const std::string_view SIZE("size");
  }
}

namespace Request::OpenRtb
{
  const std::string STABLE_SOURCE("stable");

  // ADSC-10919
  const std::string_view DEFAULT_BIDFLOORCUR_CURRENCY("usd");

  const std::string_view VIDEO_PLACEMENT_NONE("none");

  const std::string_view VIDEO_PLACEMENTS[] =
  {
    "unknown",
    "instream",
    "inbanner",
    "inread",  // ADSC-10951 for google comatibility
    "infeed",
    "interstitial"
  };

  // TEAM-313
  const std::string_view VIDEO_SKIPS[] =
  {
    "rtbvideotypeunskip",
    "rtbvideotypeskip"
  };

  const std::string_view VIDEO_REWARDS[] =
  {
    "rtbvideotypenonward",
    "rtbvideotypereward"
  };

  const unsigned long VIDEO_PLAYBACKMETHOD_SOUND_ON(1);
  const unsigned long VIDEO_PLAYBACKMETHOD_SOUND_OFF(2);
  const std::string_view VIDEO_SOUND_ON("rtbsoundon");
  const std::string_view VIDEO_SOUND_OFF("rtbsoundoff");

  const std::string_view VIDEO_PLACEMENTS_STREAM_NAMES[] =
  {
    "",
    "rtbvideotypeinstream",
    "rtbvideotypebanner",
    "rtbvideotypearticle",
    "rtbvideotypefeed",
    "rtbvideotypeinterstitial"
  };

  const std::array<int, 5> VIDEO_PLACEMENTS_OUT_STREAM_INDEXES =
  {
    2, 3, 4, 5, 7
  };
  const std::string_view VIDEO_TYPE_OUT_STREAM_NAME("rtbvideotypeoutstream");

  const long VIDEO_START_DELAY_TYPE_PRE(0);
  const long VIDEO_START_DELAY_TYPE_POST(-2);
  const std::string_view VIDEO_TYPE_PRE("rtbvideotypepre");
  const std::string_view VIDEO_TYPE_MID("rtbvideotypemid");
  const std::string_view VIDEO_TYPE_POST("rtbvideotypepost");

  // ADSC-10933
  const std::string_view VIDEO_PLAYBACKMETHODS[] =
  {
    "unknown",
    "autoplaysoundon",
    "autoplaysoundoff",
    "clicktoplay",
    "mouseover"
  };

  // ADSC-10918 Native ads

  const std::string_view NATIVE_DATA_ASSET_TOKENS[] =
  {
    "ADTITLE",      // supported - title asset
    "SPONSORED",
    "ALTTEXT",      // supported - Descriptive text associated with the product
    "RATING",
    "LIKES",
    "DOWNLOADS",
    "PRICE",
    "SALEPRICE",
    "PHONE",
    "ADDRESS",
    "DESCRIPTION2", // supported - Additional descriptive text
    "DISPLAY_URL",  // supported - Display url for the text ad.
    "CTA"           // supported - CTA description
  };

  const std::string_view NATIVE_IMAGE_ASSET_TOKENS[] =
  {
    "", // unknown type
    "ADICON",
    "ADICON",
    "ADIMAGE",
    "ADIMAGE_SQUARE"
  };

  const std::string_view NATIVE_PLACEMENTS[] =
  {
    "unknown",
    "infeed",
    "inread",
    "outside",
    "recommendation"
  };
}

namespace CreativeCategory
{
  const std::string VPAID1("vpaid1");
  const std::string VPAID2("vpaid2");
}

namespace GooglePlay
{
  const std::string STORE_HOST("play.google.com");
  const std::string STORE_PATH("/store/apps/details");
  const std::string STORE_APP_ID("id");
}

namespace AppStore
{
  const std::string STORE_HOST("itunes.apple.com");
}

namespace AdServer::Bidding
{
  namespace Google
  {
    const std::string_view APPLICATION("google");
    const std::string_view APPLICATION_VERSION("1");
    const std::string_view APP_FORMAT("html");

    // https://storage.googleapis.com/adx-rtb-dictionaries/
    // publisher-excludable-creative-attributes.txt
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
    const std::string_view CREATIVETYPE_FLASH_STR("34");

    // RichMediaCapabilityType: RichMediaCapabilityNonSSL
    const ::google::protobuf::int32 NON_SECURE = 48;

    const AdServer::CampaignSvcs::ExtRevenueDecimal BIDFLOOR_MULTIPLIER =
      AdServer::CampaignSvcs::ExtRevenueDecimal::div(
        AdServer::CampaignSvcs::ExtRevenueDecimal(false, 100, 0),
        AdServer::CampaignSvcs::ExtRevenueDecimal(false, 1000000, 0));


    // ADSC-10919
    const std::string_view VIDEO_PLACEMENTS[] =
    {
      "unknown",
      "instream",
      "interstitial",
      "inread",
      "audio",
      "inarticle"
    };
  }

  namespace
  {
    // Openrtb
    const std::string_view OPENRTB_APPLICATION("openrtb");
    const std::string_view OPENRTB_APPLICATION_VERSION("1");
    const std::string_view OPENRTB_APP_FORMAT("html");
    const std::string_view OPENRTB_VAST_APP_FORMAT("vast");
    const std::string_view OPENRTB_NATIVE_APP_FORMAT("native");
    const std::string_view VAST_PROTOCOL_SIZE("vast");

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
      if(lhs >= 'A' && lhs <= 'Z')
      {
        lhs += 'a' - 'A';
      }

      if(rhs >= 'A' && rhs <= 'Z')
      {
        rhs += 'a' - 'A';
      }

      return lhs == rhs;
    }

    std::string_view
    to_string_view(const String::SubString& value) noexcept
    {
      return std::string_view(value.data(), value.size());
    }

    Generics::MonoString
    normalize_ifa(std::string_view idfa, Generics::MonoAllocatorArena* resource)
    {
      Generics::MonoString res(idfa.data(), idfa.size(), resource);
      String::AsciiStringManip::to_lower(res.begin(), res.end());

      Generics::MonoString norm_res(resource);
      norm_res.reserve(32);

      const String::SubString res_sub_string(res.data(), res.size());
      String::StringManip::Splitter<String::AsciiStringManip::SepMinus> tokenizer(
        res_sub_string);
      String::SubString token;
      while(tokenizer.get_token(token))
      {
        norm_res.append(token.data(), token.size());
      }

      if(norm_res.size() == 32 &&
        String::AsciiStringManip::HEX_NUMBER.find_nonowned(
          norm_res.data(),
          norm_res.data() + norm_res.size()) ==
            norm_res.data() + norm_res.size())
      {
        Generics::MonoString minus_norm_res(resource);
        minus_norm_res.reserve(36);
        minus_norm_res.append(norm_res.data(), 8);
        minus_norm_res += '-';
        minus_norm_res.append(norm_res.data() + 8, 4);
        minus_norm_res += '-';
        minus_norm_res.append(norm_res.data() + 12, 4);
        minus_norm_res += '-';
        minus_norm_res.append(norm_res.data() + 16, 4);
        minus_norm_res += '-';
        minus_norm_res.append(norm_res.data() + 20, 12);

        return minus_norm_res;
      }

      return Generics::MonoString(resource);
    }

    std::string_view
    hold_size_string(
      RequestInfo::AdSlotInfo& ad_slot_request,
      unsigned long width,
      unsigned long height)
    {
      Generics::MonoString result(ad_slot_request.resource());
      result.resize(64);
      char* ptr = result.data();
      char* const end = ptr + result.size();

      const auto width_result = std::to_chars(ptr, end, width);
      ptr = width_result.ptr;
      *ptr++ = 'x';
      const auto height_result = std::to_chars(ptr, end, height);
      result.resize(static_cast<std::size_t>(height_result.ptr - result.data()));
      const Generics::MonoString& held_result =
        ad_slot_request.hold_string(std::move(result));
      return std::string_view(held_result.data(), held_result.size());
    }

    std::string_view
    hold_lowercase_string(
      RequestInfo::AdSlotInfo& ad_slot_request,
      std::string_view value)
    {
      Generics::MonoString result(ad_slot_request.resource());
      result.reserve(value.size());
      for(char ch : value)
      {
        if(ch >= 'A' && ch <= 'Z')
        {
          ch += 'a' - 'A';
        }
        result += ch;
      }

      const Generics::MonoString& held_result =
        ad_slot_request.hold_string(std::move(result));
      return std::string_view(held_result.data(), held_result.size());
    }

    bool caseless_compare(
      std::string_view lhs,
      std::string_view rhs)
    {
      if (lhs.size() == rhs.size())
      {
        return std::equal(rhs.begin(), rhs.end(),
          lhs.begin(), char_caseless_compare);
      }
      return false;
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
    typedef Native::DataArray NativeDataArray;
    typedef Native::Data NativeData;
    typedef Native::ImageArray NativeImageArray;
    typedef Native::Image NativeImage;
    typedef Native::Video NativeVideo;

    bool find_main_image(
      const NativeImageArray& image_assets,
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
      AdServer::Bidding::RequestInfo::NativeDataToken&,
      const NativeData&)
    {}

    void fill_token(
      AdServer::Bidding::RequestInfo::NativeImageToken& res_token,
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
      const std::string_view(&dict)[DictSize],
      long AssetSeq::value_type::* type_field,
      const char* /*token_prefix*/)
    {
      res_seq.resize(assets.size());
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
            res_seq[i].name = dict[asset_type];
            res_seq[i].required = it->required;
            fill_token(res_seq[i], *it);
            i++;
          }
        }
      }

      res_seq.resize(i);

      return true;
    }

    // REQ-3939
    // Player VAST size
    template<typename KeywordFormatterType>
    void
    fill_video_size(
      KeywordFormatterType& kw_fmt,
      AdServer::Bidding::RequestInfo::AdSlotInfo& ad_slot_request,
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

        oss.append(buf, buf_size);
      }
      oss += "x";
      if (height)
      {
        ad_slot_request.video_height = height;

        char buf[15];
        size_t buf_size = String::StringManip::int_to_str(
          height, buf, sizeof(buf));

        oss.append(buf, buf_size);
      }

      kw_fmt.add_keyword_owned(std::move(oss));
    }

    bool
    try_parse_float_(
      float& result,
      const std::string_view value) noexcept
    {
      if(value.empty())
      {
        return false;
      }

      char* end = nullptr;
      const float parsed = std::strtof(value.data(), &end);
      if(end == value.data() || (end != nullptr && *end != '\0'))
      {
        return false;
      }

      result = parsed;
      return true;
    }

    void
    json_escape_append_(
      Generics::MonoString& result,
      std::string_view value)
    {
      static constexpr std::string_view REPL[] =
      {
        "\\u0000", "\\u0001", "\\u0002", "\\u0003",
        "\\u0004", "\\u0005", "\\u0006", "\\u0007",
        "\\b", "\\t", "\\n", "\\u000B",
        "\\f", "\\r", "\\u000E", "\\u000F",
        "\\u0010", "\\u0011", "\\u0012", "\\u0013",
        "\\u0014", "\\u0015", "\\u0016", "\\u0017",
        "\\u0018", "\\u0019", "\\u001A", "\\u001B",
        "\\u001C", "\\u001D", "\\u001E", "\\u001F"
      };

      std::size_t chunk_begin = 0;
      for(std::size_t pos = 0; pos < value.size(); ++pos)
      {
        const unsigned char ch = value[pos];
        if(ch < 0x20 || ch == '"' || ch == '\\')
        {
          result.append(value.data() + chunk_begin, pos - chunk_begin);
          if(ch == '"')
          {
            result += "\\\"";
          }
          else if(ch == '\\')
          {
            result += "\\\\";
          }
          else
          {
            const std::string_view repl = REPL[ch];
            result.append(repl.data(), repl.size());
          }

          chunk_begin = pos + 1;
        }
      }

      result.append(value.data() + chunk_begin, value.size() - chunk_begin);
    }

    void
    make_additional_info_json_(
      Generics::MonoString& result,
      const RequestInfo::AdditionalInfo& additional_info)
    {
      result.clear();
      result.reserve(additional_info.tagid.size() + 64);
      result += "{\"ssp_tag_id\":\"";
      json_escape_append_(result, additional_info.tagid);
      result += '"';

      auto add_optional_metric =
        [&result](const char* name, const std::optional<float>& value)
      {
        result += ",\"";
        result += name;
        result += "\":";
        if(value.has_value())
        {
          char buf[32];
          const int size = std::snprintf(
            buf,
            sizeof(buf),
            "%.6g",
            static_cast<double>(*value));
          if(size > 0)
          {
            result.append(buf, static_cast<std::size_t>(size));
          }
        }
        else
        {
          result += "null";
        }
      };

      add_optional_metric("ctr", additional_info.ctr);
      add_optional_metric("viewability", additional_info.viewability);
      add_optional_metric("vtr", additional_info.vtr);
      result += '}';
    }

    void
    fill_openrtb_geo_(
      RequestInfo& request_info,
      const JsonProcessingContext& context)
    {
      if(!context.ssp_country.empty() ||
        !context.ssp_region.empty() ||
        !context.ssp_city.empty())
      {
        request_info.location = std::make_shared<FrontendCommons::Location>();
        request_info.location->country.assign(
          context.ssp_country.data(),
          context.ssp_country.size());
        request_info.location->region.assign(
          context.ssp_region.data(),
          context.ssp_region.size());
        request_info.location->city.assign(
          context.ssp_city.data(),
          context.ssp_city.size());
        request_info.location->normalize();

        request_info.geo_location.resize(1);
        request_info.geo_location[0].country = request_info.location->country;
        request_info.geo_location[0].region = request_info.location->region;
        request_info.geo_location[0].city = request_info.location->city;
      }

      if(context.ssp_latitude && context.ssp_longitude)
      {
        using CoordDecimal = AdServer::CampaignSvcs::CoordDecimal;
        using AccuracyDecimal = AdServer::CampaignSvcs::AccuracyDecimal;

        static const CoordDecimal MIN_LAT("-90");
        static const CoordDecimal MAX_LAT("90");
        static const CoordDecimal MIN_LON("-180");
        static const CoordDecimal MAX_LON("180");
        static const AccuracyDecimal MIN_ACCURACY("0");
        static const AccuracyDecimal MAX_ACCURACY("21000000");
        static const AccuracyDecimal DEFAULT_ACCURACY("50000");

        if(*context.ssp_latitude < MIN_LAT ||
          *context.ssp_latitude > MAX_LAT ||
          *context.ssp_longitude < MIN_LON ||
          *context.ssp_longitude > MAX_LON)
        {
          return;
        }

        AccuracyDecimal accuracy = context.ssp_accuracy.value_or(
          DEFAULT_ACCURACY);
        if(accuracy <= MIN_ACCURACY)
        {
          return;
        }

        if(accuracy > MAX_ACCURACY)
        {
          accuracy = MAX_ACCURACY;
        }

        request_info.coord_location.resize(1);
        request_info.coord_location[0].latitude = *context.ssp_latitude;
        request_info.coord_location[0].longitude = *context.ssp_longitude;
        request_info.coord_location[0].accuracy = accuracy;
      }
    }
  }

  template<typename StringType>
  void
  stringify_ifa(StringType& res, std::string_view bin_idfa)
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

  template<typename StringType>
  bool
  decode_google_advertising_id(
    StringType& advertising_id,
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
        std::string_view(
          reinterpret_cast<char*>(plaintext_bytes),
          CIPHER_TEXT_SIZE));

      return true;
    }

    // value = ntohllprice_pad ^ ciphertext_bytes)
    return false;
  }

  // RequestInfoFiller
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    unsigned long colo_id,
    CommonModule* common_module,
    std::shared_ptr<GeoIPMapping::IPMapCity2> ip_map,
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
      ip_map_(std::move(ip_map)),
      sources_(sources),
      enable_profile_referer_(enable_profile_referer),
      account_traits_(account_traits),
      adxml_request_info_filler_(new AdXmlRequestInfoFiller(this)),
      openrtb_devicetype_mapping_({
        {0, "Unknown"},
        {1, "Mobile/Tablet - General"},
        {2, "Personal Computer"},
        {3, "Connected TV"},
        {4, "Phone"},
        {5, "Tablet"},
        {6, "Connected Device"},
        {7, "Set Top Box"},
        {8, "OOH Device"}
      }),
      openrtb_video_placement_mapping_({
        {1, "In-Stream"},
        {2, "In-Banner"},
        {3, "In-Article"},
        {4, "In-Feed"},
        {5, "Interstitial"}
      })
  {
    source_mapping_.emplace(
      Generics::SubStringHashAdapter(String::SubString("MegafonID")),
      "megafon-stableid");
    source_mapping_.emplace(
      Generics::SubStringHashAdapter(String::SubString("quietmedia")),
      "megafon-stableid");
    source_mapping_.emplace(
      Generics::SubStringHashAdapter(String::SubString("const.uno")),
      "megafon-stableid");

    if(user_agent_filter_path[0])
    {
      user_agent_matcher_.init(user_agent_filter_path);
    }

    init_param_processors_();

    init_fast_json_processors_();
  }

  AdXmlRequestInfoFiller*
  RequestInfoFiller::adxml_request_info_filler() noexcept
  {
    return adxml_request_info_filler_.get();
  }

  bool
  RequestInfoFiller::parse_debug_size_param_(
    DebugAdSlotSizeMap& debug_sizes,
    std::string_view name,
    std::string_view value) const
    noexcept
  {
    const std::string_view::size_type adslot_length =
      Request::Debug::AD_SLOT.size();
    if (name.compare(0, adslot_length, Request::Debug::AD_SLOT) != 0)
    {
      return false;
    }

    const std::string_view::size_type dot_pos = name.find('.', adslot_length);
    if (dot_pos == std::string_view::npos)
    {
      return false;
    }

    unsigned long si;
    if(!String::StringManip::str_to_int(
      name.substr(adslot_length, dot_pos - adslot_length), si))
    {
      return false;
    }

    if(name.compare(dot_pos + 1,
        Request::Debug::AdSlot::SIZE.size(),
        Request::Debug::AdSlot::SIZE) == 0)
    {
      Generics::MonoString debug_size(
        debug_sizes.get_allocator().arena());
      debug_size.assign(value.data(), value.size());
      debug_sizes.emplace(--si, std::move(debug_size));
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
            request_info.debug_sizes,
            std::string_view(it->name.data(), it->name.size()),
            std::string_view(it->value.data(), it->value.size())))
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
      request_info.random = Generics::unsafe_rand(CampaignSvcs::RANDOM_PARAM_MAX);
    }
  }

  void
  RequestInfoFiller::fill_by_google_request(
    RequestInfo& request_info,
    GoogleAdSlotContextArray& as_slots_context,
    const ::Google::BidRequest& bid_request) const
    /*throw(InvalidParamException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill_by_google_request()";
    Generics::MonoString& keywords = request_info.keywords;
    request_info.client = Google::APPLICATION;
    request_info.client_version = Google::APPLICATION_VERSION;
    request_info.request_type = AdServer::CampaignSvcs::AR_GOOGLE;

    // Common data

    init_request_param(request_info);

    std::string ip_str;

    if(bid_request.has_ip() && bid_request.ip().size() == 3)
    {
      const std::string& ip = bid_request.ip();
      unsigned ip_byte1 = static_cast<unsigned char>(ip[0]);
      unsigned ip_byte2 = static_cast<unsigned char>(ip[1]);
      unsigned ip_byte3 = static_cast<unsigned char>(ip[2]);
      ip_str.reserve(16);
      ip_str += std::to_string(ip_byte1);
      ip_str += '.';
      ip_str += std::to_string(ip_byte2);
      ip_str += '.';
      ip_str += std::to_string(ip_byte3);
      ip_str += ".0";
    }

    if(!ip_str.empty())
    {
      fill_by_ip(request_info, request_info.hold_string(std::move(ip_str)));
    }
    else
    {
      fill_by_ip(request_info, std::string_view());
    }

    request_info.user_status = static_cast<std::size_t>(
      AdServer::CampaignSvcs::US_UNDEFINED);

    bool application = false;
    if(bid_request.has_mobile())
    {
      const ::Google::BidRequest_Mobile& mobile = bid_request.mobile();
      application = mobile.has_is_app() && mobile.is_app();
    }

    if(bid_request.has_user_agent() || application)
    {
      std::string_view user_agent;
      if(bid_request.has_user_agent())
      {
        user_agent = request_info.hold_string(std::string(
          bid_request.user_agent()));
      }

      fill_by_user_agent(
        request_info,
        user_agent,
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
      kw_fmt.add_dict_keyword(MatchKeywords::REQ, std::string_view());
    }

    for(int vt_kw_i = 0; vt_kw_i < bid_request.detected_vertical_size(); ++vt_kw_i)
    {
      kw_fmt.add_dict_keyword(
        MatchKeywords::VT,
        bid_request.detected_vertical(vt_kw_i).id());
    }


    for(int dcl_kw_i = 0; dcl_kw_i < bid_request.detected_content_label_size(); ++dcl_kw_i)
    {
      kw_fmt.add_dict_keyword(
        MatchKeywords::DCL,
        bid_request.detected_content_label(dcl_kw_i));
    }

    if(!ip_str.empty())
    {
      kw_fmt.add_ip(ip_str);
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

      fill_by_referer(request_info, request_info.search_words, url);
    }

    if(request_info.referer.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NOREF);
    }

    // Test data
    request_info.log_as_test =
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
      verify_user_id_(bid_request.hosted_match_data(), request_info.source_id, request_info);
    }
    else if(!external_user_id.empty() && use_external_user_id_(external_user_id))
    {
      request_info.external_user_id = (
        !request_info.source_id.empty() ?
          std::string(request_info.source_id.data(), request_info.source_id.size()) +
            "/" + external_user_id :
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
    request_info.ad_slots.clear();
    request_info.ad_slots.reserve(bid_request.adslot_size());
    for(int slot_i = 0; slot_i < bid_request.adslot_size(); ++slot_i)
    {
      request_info.ad_slots.emplace_back(request_info.arena());
    }

    for(int slot_i = 0; slot_i < bid_request.adslot_size(); ++slot_i)
    {
      AdServer::Bidding::RequestInfo::AdSlotInfo& ad_slot_request =
        request_info.ad_slots[slot_i];

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

      int flash_exclude_size = test_value_in_set(
        excluded_attributes, Google::CREATIVETYPE_FLASH) ? 1: 0;

      ad_slot_request.exclude_categories.resize(
        adslot.excluded_sensitive_category_size() +
        adslot.excluded_product_category_size() +
        flash_exclude_size + 2);

      std::size_t res_cat_i = 0;

      if(slot_is_video && !video_passback)
      {
        if(bid_request.has_video())
        {
          const ::Google::BidRequest_Video& video = bid_request.video();
          // https://developers.google.com/authorized-buyers/rtb/adx-video-guide
          // define that all INSTREAM support vpaid
          if(!((video.has_placement() &&
              video.placement() ==
                ::Google::BidRequest_Video_Placement_INSTREAM) ||
            test_value_in_list(
              video.allowed_video_formats(),
              ::Google::BidRequest_Video_VideoFormat_VPAID_JS)))
          {
            ad_slot_request.exclude_categories[res_cat_i++] = CreativeCategory::VPAID1;
            ad_slot_request.exclude_categories[res_cat_i++] = CreativeCategory::VPAID2;
          }
        }
        else
        {
          ad_slot_request.exclude_categories[res_cat_i++] = CreativeCategory::VPAID1;
          ad_slot_request.exclude_categories[res_cat_i++] = CreativeCategory::VPAID2;
        }
      }

      if (flash_exclude_size)
      {
        ad_slot_request.exclude_categories[res_cat_i++] = Google::CREATIVETYPE_FLASH_STR;
      }

      for(int cat_i = 0; cat_i < adslot.excluded_sensitive_category_size(); ++cat_i)
      {
        char cat_str[20];
        size_t len = String::StringManip::int_to_str(
          adslot.excluded_sensitive_category(cat_i), cat_str, sizeof(cat_str));
        if(len > 0)
        {
          ad_slot_request.exclude_categories[res_cat_i] =
            ad_slot_request.hold_string(Generics::MonoString(
              cat_str,
              len,
              ad_slot_request.resource()));
          ++res_cat_i;
        }
      }

      for(int cat_i = 0; cat_i < adslot.excluded_product_category_size(); ++cat_i)
      {
        char cat_str[20];
        size_t len = String::StringManip::int_to_str(
          adslot.excluded_product_category(cat_i), cat_str, sizeof(cat_str));
        if(len > 0)
        {
          ad_slot_request.exclude_categories[res_cat_i] =
            ad_slot_request.hold_string(Generics::MonoString(
              cat_str,
              len,
              ad_slot_request.resource()));
          ++res_cat_i;
        }
      }

      ad_slot_request.exclude_categories.resize(res_cat_i);

      // Video
      if(slot_is_video)
      {
        ad_slot_request.format = OPENRTB_VAST_APP_FORMAT;
        ad_slot_request.sizes.resize(1);
        ad_slot_request.sizes[0] = VAST_PROTOCOL_SIZE;

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

        fill_vast_instantiate_type_(request_info, request_info.source_id);

        fill_video_size(
          kw_fmt, ad_slot_request,
          adslot.width_size() && adslot.width(0) > 0 ? adslot.width(0) : 0,
          adslot.height_size() && adslot.height(0) > 0 ? adslot.height(0) : 0);
      }
      // Banners
      else
      {
        ad_slot_request.format = Google::APP_FORMAT;

        ad_slot_request.video_min_duration = 0;
        ad_slot_request.video_max_duration = -1;
        ad_slot_request.video_width = 0;
        ad_slot_request.video_height = 0;

         // Sizes
        const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
        const auto size_it = debug_sizes.find(slot_i);
        if (size_it != debug_sizes.end())
        {
          ad_slot_request.sizes.resize(1);
          ad_slot_request.sizes[0] = std::string_view(
            size_it->second.data(),
            size_it->second.size());
        }
        else if (!request_info.default_debug_size.empty())
        {
          ad_slot_request.sizes.resize(1);
          ad_slot_request.sizes[0] = std::string_view(
            request_info.default_debug_size.data(),
            request_info.default_debug_size.size());
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

            ad_slot_request.sizes.resize(1);
            ad_slot_request.sizes[0] = hold_size_string(
              ad_slot_request,
              ad_slot_context.width,
              ad_slot_context.height);
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
            (deal.deal_type() ==
                ::Google::BidRequest_AdSlot_MatchingAdData_DirectDeal::PREFERRED_DEAL ||
              deal.deal_type() ==
                ::Google::BidRequest_AdSlot_MatchingAdData_DirectDeal::PRIVATE_AUCTION) &&
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

        ad_slot_request.min_ecpm = min_ecpm;
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

        ad_slot_request.min_ecpm = CampaignSvcs::RevenueDecimal::ZERO;
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
      std::string ext_tag_id = std::to_string(seller_id);
      ext_tag_id += '-';
      ext_tag_id += anonymous_id.empty() ? "0" : anonymous_id;
      ad_slot_request.ext_tag_id = normalize_ext_tag_id_(ext_tag_id);

      ad_slot_request.debug_ccg = 0;

      // fill creative_instantiate_type if was not filled by fill_by_referer_
      if(test_value_in_set(excluded_attributes, Google::NON_SECURE) &&
        request_info.creative_instantiate_type.empty())
      {
        request_info.creative_instantiate_type = FrontendCommons::SECURE_INSTANTIATE_TYPE;
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
        if(mobile.encrypted_advertising_id().size() == 36 &&
          !request_info.publisher_account_ids.empty())
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
    if(request_info.creative_instantiate_type.empty())
    {
      request_info.creative_instantiate_type = FrontendCommons::UNSECURE_INSTANTIATE_TYPE;
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
  RequestInfoFiller::fill_adid(std::string_view source_id) const noexcept
  {
    SourceMap::const_iterator source_it = sources_.find(std::string(source_id));
    if(source_it != sources_.end())
    {
      return source_it->second.fill_adid;
    }

    return false;
  }

  void
  RequestInfoFiller::fill_vast_instantiate_type_(
    RequestInfo& request_info,
    std::string_view source_id) const
    noexcept
  {
    SourceMap::const_iterator source_it = sources_.find(std::string(source_id));
    if(source_it != sources_.end())
    {
      request_info.ad_instantiate_type = source_it->second.vast_instantiate_type;
      request_info.enabled_notice = (
        source_it->second.vast_notice_instantiate_type != SourceTraits::NIT_NONE) &&
        source_it->second.notice_url.empty();
    }
  }

  void
  RequestInfoFiller::fill_native_instantiate_type_(
    RequestInfo& request_info,
    std::string_view source_id) const
    noexcept
  {
    SourceMap::const_iterator source_it = sources_.find(std::string(source_id));
    if(source_it != sources_.end())
    {
      request_info.enabled_notice = (
        source_it->second.native_notice_instantiate_type != SourceTraits::NIT_NONE);
    }
  }

  void
  RequestInfoFiller::fill_request_type_(
    RequestInfo& request_info,
    std::string_view source_id) const
    noexcept
  {
    SourceMap::const_iterator source_it = sources_.find(std::string(source_id));
    if(source_it != sources_.end())
    {
      if (request_info.publisher_account_ids.empty())
      {
        request_info.publisher_account_ids.resize(1);
        request_info.publisher_account_ids[0] = *(source_it->second.default_account_id);
      }

      if(source_it->second.request_type)
      {
        request_info.request_type = *(source_it->second.request_type);
      }

      request_info.ad_instantiate_type = source_it->second.instantiate_type;

      request_info.notice_instantiate_type = source_it->second.notice_instantiate_type;
      request_info.vast_notice_instantiate_type = source_it->second.vast_notice_instantiate_type;
      request_info.native_notice_instantiate_type =
        source_it->second.native_notice_instantiate_type;

      request_info.enabled_notice = (
        source_it->second.notice_instantiate_type != SourceTraits::NIT_NONE) &&
        source_it->second.notice_url.empty();
      request_info.seat = source_it->second.seat;
      request_info.ipw_extension = source_it->second.ipw_extension;
      request_info.truncate_domain = source_it->second.truncate_domain;

      request_info.native_ads_instantiate_type =
        source_it->second.native_ads_instantiate_type;
      if(source_it->second.native_ads_impression_tracker_type)
      {
        request_info.native_ads_impression_tracker_type =
          *source_it->second.native_ads_impression_tracker_type;
      }
      request_info.skip_ext_category = source_it->second.skip_ext_category;
      request_info.notice_url = source_it->second.notice_url;
      request_info.erid_return_type = source_it->second.erid_return_type;
    }
    else
    {
      request_info.ad_instantiate_type = AdServer::CampaignSvcs::AIT_BODY;
      request_info.enabled_notice = false;
    }
  }

  std::string
  RequestInfoFiller::openrtb_ext_tag_id(
    std::string_view publisher_id,
    std::string_view id,
    std::string_view publisher_name,
    std::string_view name)
  {
    std::string res;
    res.reserve(EXT_TAG_ID_MAX_LENGTH);

    const auto append_or_zero = [&res](std::string_view value)
    {
      if(value.empty())
      {
        res += '0';
      }
      else
      {
        res.append(value.data(), value.size());
      }
    };

    append_or_zero(publisher_id);
    res += '-';
    append_or_zero(id);
    res += '-';
    append_or_zero(publisher_name);
    res += '-';
    append_or_zero(name);

    if(res.size() > EXT_TAG_ID_MAX_LENGTH)
    {
      res.resize(EXT_TAG_ID_MAX_LENGTH);
    }

    return normalize_ext_tag_id_(res);
  }

  void
  RequestInfoFiller::fill_by_openrtb_request(
    RequestInfo& request_info,
    JsonProcessingContext& context,
    std::string&& bid_request) const
    /*throw(InvalidParamException, Exception)*/
  {
    // static const char* FUN = "RequestInfoFiller::fill_by_openrtb_request()";
    Generics::MonoString& keywords = request_info.keywords;
    request_info.client = OPENRTB_APPLICATION;
    request_info.client_version = OPENRTB_APPLICATION_VERSION;
    request_info.request_type = AdServer::CampaignSvcs::AR_OPENRTB;
    request_info.user_status = static_cast<std::size_t>(
      AdServer::CampaignSvcs::US_UNDEFINED);

    init_request_param(request_info);
    request_info.additional_info.clear();

    if(request_info.ad_instantiate_type == AdServer::CampaignSvcs::AIT_URL)
    {
      // notice can't be enabled for openrtb with url instantiate type
      // because both uses nurl response parameter
      request_info.enabled_notice = false;
    }

    if(request_info.request_type == AdServer::CampaignSvcs::AR_YANDEX)
    {
      request_info.fill_track_pixel = true;
    }

    const std::string_view held_bid_request = request_info.hold_string(
      std::move(bid_request));

    parse_openrtb_request_(
      request_info,
      context,
      held_bid_request);

    BasicKeywordFormatter<Generics::MonoString> kw_fmt(
      request_info.source_id,
      request_info.arena());

    kw_fmt.add_keyword(MatchKeywords::FULL_REQ);

    if(!request_info.source_id.empty())
    {
      kw_fmt.add_dict_keyword(MatchKeywords::REQ, std::string_view());
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
        const std::string_view id = norm_keyword_ext_(request_info, it->id);
        const std::string_view value = norm_keyword_ext_(request_info, it->value);
        std::string keyword;
        keyword.reserve(10 + id.size() + 1 + value.size());
        keyword += "rtbsegment";
        keyword.append(id.data(), id.size());
        keyword += 'x';
        keyword.append(value.data(), value.size());
        //std::cerr << "keyword : <" << keyword << ">" << std::endl;
        kw_fmt.add_keyword_owned(std::move(keyword));
      }
    }

    for (auto ad_slot_it = context.ad_slots.begin(); ad_slot_it != context.ad_slots.end();
      ++ad_slot_it)
    {
      if(request_info.additional_info.tagid.empty() && !ad_slot_it->tagid.empty())
      {
        request_info.additional_info.tagid = ad_slot_it->tagid;
      }

      for (auto metric_it = ad_slot_it->metrics.begin(); metric_it != ad_slot_it->metrics.end();
        ++metric_it)
      {
        const std::string_view type = norm_keyword_ext_(request_info, metric_it->type);
        const std::string_view value = norm_keyword_ext_(request_info, metric_it->value);
        std::string keyword;
        keyword.reserve(9 + type.size() + 1 + value.size());
        keyword += "rtbmetric";
        keyword.append(type.data(), type.size());
        keyword += 'x';
        keyword.append(value.data(), value.size());
        kw_fmt.add_keyword_owned(std::move(keyword));

        float metric_value = 0.0f;
        if(try_parse_float_(metric_value, metric_it->value))
        {
          if(caseless_compare(metric_it->type, "ctr"))
          {
            request_info.additional_info.ctr = metric_value;
          }
          else if(caseless_compare(metric_it->type, "viewability"))
          {
            request_info.additional_info.viewability = metric_value;
          }
          else if(caseless_compare(metric_it->type, "vtr"))
          {
            request_info.additional_info.vtr = metric_value;
          }
        }
      }
    }

    if(!request_info.additional_info.tagid.empty() ||
      request_info.additional_info.ctr.has_value() ||
      request_info.additional_info.viewability.has_value() ||
      request_info.additional_info.vtr.has_value())
    {
      make_additional_info_json_(
        request_info.campaign_additional_info,
        request_info.additional_info);
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

    if(!context.language.empty())
    {
      const std::string_view language = norm_keyword_ext_(request_info, context.language);
      std::string keyword;
      keyword.reserve(11 + language.size());
      keyword += "rtblanguage";
      keyword.append(language.data(), language.size());
      kw_fmt.add_keyword_owned(std::move(keyword));
    }

    if(!context.carrier.empty())
    {
      const std::string_view carrier = norm_keyword_ext_(request_info, context.carrier);
      std::string keyword;
      keyword.reserve(10 + carrier.size());
      keyword += "rtbcarrier";
      keyword.append(carrier.data(), carrier.size());
      kw_fmt.add_keyword_owned(std::move(keyword));
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
         request_info.request_type == AdServer::CampaignSvcs::AR_OPENX ||
         (request_info.request_type == AdServer::CampaignSvcs::AR_YANDEX &&
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

    if(!context.external_user_id.empty() && use_external_user_id_(context.external_user_id))
    {
      if(!request_info.source_id.empty())
      {
        std::string external_user_id;
        external_user_id.reserve(
          request_info.source_id.size() + 1 + context.external_user_id.size());
        external_user_id.append(request_info.source_id.data(), request_info.source_id.size());
        external_user_id += '/';
        external_user_id.append(context.external_user_id.data(), context.external_user_id.size());
        request_info.external_user_id = external_user_id;
      }
      else
      {
        request_info.external_user_id = context.external_user_id;
      }
    }

    if(request_info.external_user_id.empty() && context.user_id.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_NO_ID);

      if(!request_info.source_id.empty())
      {
        kw_fmt.add_dict_keyword(MatchKeywords::NO_ID, std::string_view());
      }
    }

    // Possible IPMatcher and code in CampaignManager not support IPv6
    if (!context.ipv6.empty())
    {
      fill_by_ip(request_info, context.ipv6);
    }
    else
    {
      fill_by_ip(request_info, request_info.peer_ip);
    }

    fill_openrtb_geo_(request_info, context);

    if(!request_info.peer_ip.empty())
    {
      kw_fmt.add_ip(request_info.peer_ip);
    }

    if (context.regs_coppa)
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_COPPA);
    }

    // don't use buyer_id if external_user_id defined
    if(request_info.external_user_id[0] == 0)
    {
      verify_user_id_(context.user_id, request_info.source_id, request_info);
    }

    request_info.is_app = context.app;
    fill_by_user_agent(
      request_info,
      request_info.user_agent,
      request_info.filter_request,
      context.app);

    std::string ext_tag_id;

    HTTP::HTTPAddress selected_referer;
    if(context.site)
    {
      ext_tag_id = openrtb_ext_tag_id(
        request_info.bid_publisher_id,
        request_info.bid_site_id,
        context.publisher_name,
        context.site_name);

      if(context.site_page.secure() ||
        context.site_domain.secure() ||
        context.site_ref.secure() ||
        context.site_referer.secure() ||
        context.site_rereferer.secure())
      {
        request_info.creative_instantiate_type = FrontendCommons::SECURE_INSTANTIATE_TYPE;
      }

      select_referer_(request_info, context, selected_referer);

      fill_by_referer(
        request_info,
        request_info.search_words,
        selected_referer,
        true, // fill_search_words
        false // fill_instantiate_type
        );
    }
    else if(context.app)
    {
      request_info.profile_referer = enable_profile_referer_;
      ext_tag_id = openrtb_ext_tag_id(
        request_info.bid_publisher_id,
        context.app_id,
        context.publisher_name,
        context.app_name);

      std::string app_bundle_url;

      try
      {
        // workaround: some SSP provide url with spaces at beginning
        String::SubString url_sstr(context.app_bundle.data(), context.app_bundle.size());
        String::StringManip::trim(url_sstr);
        request_info.application_id = url_sstr.str();
        app_bundle_url = HTTP::BrowserAddress(url_sstr).url();
      }
      catch(const eh::Exception&)
      {}

      if(!app_bundle_url.empty())
      {
        request_info.referer = std::move(app_bundle_url);
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
          request_info.referer = FrontendCommons::normalize_abs_url(context.app_domain);
        }
        else if(!context.app_store_url.url().empty())
        {
          request_info.referer = adapt_app_store_url_(
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
          fill_additional_url_(request_info, request_info.search_words, context.site_ref);
        }
      }
      catch(eh::Exception&)
      {}
    }

    if(request_info.referer.empty())
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

    request_info.log_as_test = context.test;

    request_info.ad_slots.clear();
    request_info.ad_slots.reserve(context.ad_slots.size());
    for(std::size_t i = 0; i < context.ad_slots.size(); ++i)
    {
      request_info.ad_slots.emplace_back(request_info.arena());
    }
    std::size_t slot_i = 0;
    for(Generics::MonoList<JsonAdSlotProcessingContext>::iterator slot_it = context.ad_slots.begin();
        slot_it != context.ad_slots.end(); )
    {
      const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
      const auto& default_debug_size = request_info.default_debug_size;

      if(slot_it->imp_ext_type == "popup")
      {
        // add popup banner
        JsonAdSlotProcessingContext::Banner popup_banner(slot_it->resource());
        popup_banner.formats.emplace_back(slot_it->resource());
        popup_banner.formats.back().ext_format = "popup";
        slot_it->banners.push_back(std::move(popup_banner));
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
          /*
          if(logger_->log_level() >= Logging::Logger::NOTICE)
          {
            Stream::Error ostr;
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

            logger_->log(ostr.str(), Logging::Logger::NOTICE, Aspect::BIDDING_FRONTEND);
          }
          */

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
            slot_it->video_placement ?
              get_value_from_seq(
                Request::OpenRtb::VIDEO_PLACEMENTS,
                *slot_it->video_placement) :
              Request::OpenRtb::VIDEO_PLACEMENT_NONE);

          kw_fmt.add_dict_keyword(
            MatchKeywords::PLACEMENT,
            slot_it->video_placement ?
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

          // TEAM-313
          if (std::find(
                slot_it->video_playbackmethods->begin(),
                slot_it->video_playbackmethods->end(),
                Request::OpenRtb::VIDEO_PLAYBACKMETHOD_SOUND_ON)
              != slot_it->video_playbackmethods->end())
          {
            kw_fmt.add_full_rtb_keyword(Request::OpenRtb::VIDEO_SOUND_ON);
          }
          if (std::find(
                slot_it->video_playbackmethods->begin(),
                slot_it->video_playbackmethods->end(),
                Request::OpenRtb::VIDEO_PLAYBACKMETHOD_SOUND_OFF)
              != slot_it->video_playbackmethods->end())
          {
            kw_fmt.add_full_rtb_keyword(Request::OpenRtb::VIDEO_SOUND_OFF);
          }
        }

        // TEAM-313
        if (!slot_it->video_skip.empty())
        {
          kw_fmt.add_full_rtb_keyword(
            get_value_from_seq(
              Request::OpenRtb::VIDEO_SKIPS,
              *slot_it->video_skip));
        }

        if (!slot_it->video_reward.empty())
        {
          kw_fmt.add_full_rtb_keyword(
            get_value_from_seq(
              Request::OpenRtb::VIDEO_REWARDS,
              *slot_it->video_reward));
        }

        if (slot_it->video_placement.has_value())
        {
          kw_fmt.add_full_rtb_keyword(
            get_value_from_seq(
              Request::OpenRtb::VIDEO_PLACEMENTS_STREAM_NAMES,
              *slot_it->video_placement));

          if (std::find(
                Request::OpenRtb::VIDEO_PLACEMENTS_OUT_STREAM_INDEXES.begin(),
                Request::OpenRtb::VIDEO_PLACEMENTS_OUT_STREAM_INDEXES.end(),
                *slot_it->video_placement)
              != Request::OpenRtb::VIDEO_PLACEMENTS_OUT_STREAM_INDEXES.end())
          {
            kw_fmt.add_full_rtb_keyword(Request::OpenRtb::VIDEO_TYPE_OUT_STREAM_NAME);
          }
        }

        if (!slot_it->video_start_delay.empty())
        {
          if (*slot_it->video_start_delay == Request::OpenRtb::VIDEO_START_DELAY_TYPE_PRE)
          {
            kw_fmt.add_full_rtb_keyword(Request::OpenRtb::VIDEO_TYPE_PRE);
          }
          else if (*slot_it->video_start_delay > Request::OpenRtb::VIDEO_START_DELAY_TYPE_PRE)
          {
            kw_fmt.add_full_rtb_keyword(Request::OpenRtb::VIDEO_TYPE_MID);
          }
          else if (*slot_it->video_start_delay == Request::OpenRtb::VIDEO_START_DELAY_TYPE_POST)
          {
            kw_fmt.add_full_rtb_keyword(Request::OpenRtb::VIDEO_TYPE_POST);
          }
        }

        if(serve_video && slot_it == context.ad_slots.begin())
        {
          fill_vast_instantiate_type_(request_info, request_info.source_id);
        }

        AdServer::Bidding::RequestInfo::AdSlotInfo& ad_slot_request =
          request_info.ad_slots[slot_i];

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

        CampaignManager::fill_sequence(
          context.exclude_categories.begin(),
          context.exclude_categories.end(),
          ad_slot_request.exclude_categories);

        if(!request_info.skip_ext_category &&
          !context.required_category.empty() &&
          context.required_category != "0")
        {
          ad_slot_request.required_categories.resize(1);
          ad_slot_request.required_categories[0] = context.required_category;
        }

        unsigned long pos = 0;

        if(!request_info.format.empty())
        {
          ad_slot_request.format = request_info.format;
        }

        if(serve_video || (!slot_it->native && slot_it->banners.empty() &&
            debug_sizes.empty() && default_debug_size.empty()))
        {
          // push skipped vast into stats if banner can't be shown
          if(ad_slot_request.format.empty())
          {
            ad_slot_request.format = OPENRTB_VAST_APP_FORMAT;
          }

          ad_slot_request.sizes.resize(1);
          ad_slot_request.sizes[0] = VAST_PROTOCOL_SIZE;

          if(serve_video)
          {
            // process api
            if(slot_it->video_api.fail())
            {
              ad_slot_request.exclude_categories.push_back(CreativeCategory::VPAID1);
              ad_slot_request.exclude_categories.push_back(CreativeCategory::VPAID2);
            }
            else
            {
              if(slot_it->video_api->find(1) == slot_it->video_api->end() &&
                slot_it->video_api->find(2) == slot_it->video_api->end())
              {
                ad_slot_request.exclude_categories.push_back(CreativeCategory::VPAID1);
              }

              if(slot_it->video_api->find(2) == slot_it->video_api->end())
              {
                ad_slot_request.exclude_categories.push_back(CreativeCategory::VPAID2);
              }
            }

            CampaignManager::fill_sequence(
              slot_it->video_exclude_categories.begin(),
              slot_it->video_exclude_categories.end(),
              ad_slot_request.exclude_categories,
              true);
          }

          ad_slot_request.passback = request_info.filter_request || !serve_video;
          if(slot_it->video_max_duration.present())
          {
            ad_slot_request.video_max_duration = static_cast<long>(
              *slot_it->video_max_duration);
            ad_slot_request.video_skippable_max_duration =
              ad_slot_request.video_max_duration;
          }

          String::StringManip::str_to_int(slot_it->video_pos, pos);
          slot_it->banners.clear();

          fill_video_size(
            kw_fmt,
            ad_slot_request,
            slot_it->video_width.present() ?
              static_cast<unsigned long>(*slot_it->video_width) : 0,
            slot_it->video_height.present() ?
              static_cast<unsigned long>(*slot_it->video_height) : 0);
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
              slot_it->native->placement ?
                *slot_it->native->placement: 0));

          kw_fmt.add_dict_keyword(
            MatchKeywords::NATIVE_PLACEMENT,
            get_value_from_seq(
              Request::OpenRtb::NATIVE_PLACEMENTS,
              slot_it->native->placement ?
                *slot_it->native->placement: 0), false);

          if(ad_slot_request.format.empty())
          {
            ad_slot_request.format = OPENRTB_NATIVE_APP_FORMAT;
          }

          request_info.fill_track_pixel = true;
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
              ad_slot_request.sizes.resize(1);
              ad_slot_request.sizes[0] = VAST_PROTOCOL_SIZE;

              if (video.max_duration.present())
              {
                ad_slot_request.video_max_duration = static_cast<long>(
                  *video.max_duration);
                ad_slot_request.video_skippable_max_duration =
                  ad_slot_request.video_max_duration;
              }

              slot_it->banners.clear();

              fill_video_size(kw_fmt, ad_slot_request, 0, 0);
            }
          }
          else if(serve_native)
          {
            NativeImage main_image(slot_it->resource());
            bool main_image_found = find_main_image(slot_it->native->image_assets, main_image);

            ad_slot_request.sizes.resize(1);
            const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
            const auto size_it = debug_sizes.find(slot_i);
            if (size_it != debug_sizes.end() || !request_info.default_debug_size.empty())
            {
              ad_slot_request.sizes[0] = (
                size_it != debug_sizes.end() ?
                std::string_view(size_it->second.data(), size_it->second.size()) :
                std::string_view(
                  request_info.default_debug_size.data(),
                  request_info.default_debug_size.size()));
            }
            else if(main_image_found)
            {
              ad_slot_request.sizes[0] = hold_size_string(
                ad_slot_request,
                main_image.width,
                main_image.height);
            }
            else
            {
              ad_slot_request.sizes[0] = NATIVE_TEXT_SIZE;
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
            fill_native_instantiate_type_(request_info, request_info.source_id);
          }

          ad_slot_request.passback = request_info.filter_request || !serve_native;
        }
        else // banners not empty here
        {
          if(ad_slot_request.format.empty())
          {
            ad_slot_request.format = OPENRTB_APP_FORMAT;
          }

          ad_slot_request.passback = request_info.filter_request;

          if(!slot_it->banners.empty())
          {
            request_info.hpos = slot_it->banners.begin()->ext_hpos;
            String::StringManip::str_to_int(slot_it->banners.begin()->pos, pos);
          }
          else
          {
            request_info.hpos = 0;
          }

          bool skip_sizes = false;

          unsigned long banner_format_count = 0;
          for(auto banner_it = slot_it->banners.rbegin();
            banner_it != slot_it->banners.rend(); ++banner_it)
          {
            banner_format_count += banner_it->formats.size();
          }

          ad_slot_request.sizes.resize(banner_format_count);
          std::size_t size_i = 0;

          for(auto banner_it = slot_it->banners.rbegin();
            banner_it != slot_it->banners.rend(); ++banner_it)
          {
            const JsonAdSlotProcessingContext::Banner& banner = *banner_it;

            // union exclude categories from all banner objects
            CampaignManager::fill_sequence(
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
                  *banner_format_it;

                if((banner_format.width == "2" && banner_format.height == "2") ||
                  banner_format.ext_type == "20")
                {
                  ad_slot_request.sizes[size_i++] = RM_SIZE;
                  slot_it->size_banner.emplace(
                    RM_SIZE,
                    JsonAdSlotProcessingContext::BannerFormatHolder(
                      &*banner_it,
                      &*banner_format_it));
                }
                else if (banner_format.ext_format == "popup")
                {
                  ad_slot_request.sizes[size_i++] = POPUP_SIZE;
                  slot_it->size_banner.emplace(
                    POPUP_SIZE,
                    JsonAdSlotProcessingContext::BannerFormatHolder(
                      &*banner_it,
                      &*banner_format_it));
                }
                else if (banner_format.ext_format == "overlay")
                {
                  ad_slot_request.sizes[size_i++] = RM_DTO_SIZE;
                  slot_it->size_banner.emplace(
                    RM_DTO_SIZE,
                    JsonAdSlotProcessingContext::BannerFormatHolder(
                      &*banner_it,
                      &*banner_format_it));
                }
                else if (banner_format.ext_format == "rich")
                {
                  ad_slot_request.sizes[size_i++] = RM_RICH_SIZE;
                  slot_it->size_banner.emplace(
                    RM_RICH_SIZE,
                    JsonAdSlotProcessingContext::BannerFormatHolder(
                      &*banner_it,
                      &*banner_format_it));
                }
                else if(!banner_format.width.empty() && !banner_format.height.empty())
                {
                  const DebugAdSlotSizeMap& debug_sizes = request_info.debug_sizes;
                  const auto size_it = debug_sizes.find(slot_i);
                  std::string_view res_size_view;
                  if(size_it != debug_sizes.end())
                  {
                    res_size_view = std::string_view(
                      size_it->second.data(),
                      size_it->second.size());
                  }
                  else if(request_info.default_debug_size.empty())
                  {
                    Generics::MonoString res_size(ad_slot_request.resource());
                    res_size.reserve(
                      banner_format.width.size() + 1 + banner_format.height.size());
                    res_size.append(
                      banner_format.width.data(),
                      banner_format.width.size());
                    res_size += 'x';
                    res_size.append(
                      banner_format.height.data(),
                      banner_format.height.size());
                    const Generics::MonoString& held_res_size =
                      ad_slot_request.hold_string(std::move(res_size));
                    res_size_view = std::string_view(
                      held_res_size.data(),
                      held_res_size.size());
                  }
                  else
                  {
                    res_size_view = std::string_view(
                      request_info.default_debug_size.data(),
                      request_info.default_debug_size.size());
                  }
                  ad_slot_request.sizes[size_i++] = res_size_view;
                  slot_it->size_banner.emplace(
                    res_size_view,
                    JsonAdSlotProcessingContext::BannerFormatHolder(
                      &*banner_it,
                      &*banner_format_it));
                }
              }
            }
          }

          ad_slot_request.sizes.resize(size_i);
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
          const std::string_view currency_code =
            slot_it->min_cpm_price_currency_code.empty() ?
            Request::OpenRtb::DEFAULT_BIDFLOORCUR_CURRENCY :
            std::string_view(
              slot_it->min_cpm_price_currency_code.data(),
              slot_it->min_cpm_price_currency_code.size());

          bool min_price_present = false;
          AdServer::CampaignSvcs::RevenueDecimal min_price =
            AdServer::CampaignSvcs::RevenueDecimal::ZERO;
          const std::string_view* min_deal_id = nullptr;

          if (slot_it->min_cpm_price != AdServer::CampaignSvcs::RevenueDecimal::ZERO)
          {
            min_price_present = true;
            min_price = slot_it->min_cpm_price;
          }

          for (auto it = slot_it->deals.begin(); it != slot_it->deals.end(); ++it)
          {
            if (caseless_compare(it->currency_code, currency_code))
            {
              if(!min_price_present || it->cpm_price < min_price)
              {
                min_price_present = true;
                min_price = it->cpm_price;
                min_deal_id = &it->id;
              }
            }
          }

          if (min_price_present)
          {
            slot_it->min_cpm_price = min_price;
            if (min_deal_id && !min_deal_id->empty())
            {
              slot_it->deal_id = *min_deal_id;
            }
          }
        }

        // Unit is 1.00 / 1000 imps for OpenRTB, 1.00 / 1M for Yandex

        const AdServer::CampaignSvcs::RevenueDecimal& min_ecpm_multiplier = (
          request_info.request_type !=
            AdServer::CampaignSvcs::AR_YANDEX ?
            OPENRTB_BIDFLOOR_MULTIPLIER :
            YANDEX_BIDFLOOR_MULTIPLIER);

        try
        {
          ad_slot_request.min_ecpm =
            AdServer::CampaignSvcs::RevenueDecimal::mul(
              slot_it->min_cpm_price,
              min_ecpm_multiplier,
              Generics::DMR_ROUND);
        }
        catch(const AdServer::CampaignSvcs::RevenueDecimal::Overflow&)
        {
          ad_slot_request.min_ecpm =
            AdServer::CampaignSvcs::RevenueDecimal::MAXIMUM;
        }

        if(!context.currencies.empty())
        {
          ad_slot_request.currency_codes.resize(context.currencies.size());
          std::size_t currency_i = 0;
          for(const std::string_view currency : context.currencies)
          {
            ad_slot_request.currency_codes[currency_i++] =
              hold_lowercase_string(ad_slot_request, currency);
          }
        }
        else if(!slot_it->min_cpm_price_currency_code.empty())
        {
          ad_slot_request.currency_codes.resize(1);
          ad_slot_request.currency_codes[0] = hold_lowercase_string(
            ad_slot_request,
            slot_it->min_cpm_price_currency_code);
        }

        if(!slot_it->min_cpm_price_currency_code.empty())
        {
          ad_slot_request.min_ecpm_currency_code = hold_lowercase_string(
            ad_slot_request,
            slot_it->min_cpm_price_currency_code);
        }
        else
        {
          ad_slot_request.min_ecpm_currency_code = Request::OpenRtb::DEFAULT_BIDFLOORCUR_CURRENCY;
        }

        ad_slot_request.up_expand_space = -1;
        ad_slot_request.right_expand_space = -1;
        ad_slot_request.down_expand_space = -1;
        ad_slot_request.left_expand_space = -1;

        ad_slot_request.debug_ccg = 0;

        ad_slot_request.ext_tag_id = ext_tag_id;

        // add deal id's as keywords
        for(auto deal_it = slot_it->deals.begin(); deal_it != slot_it->deals.end(); ++deal_it)
        {
          kw_fmt.add_dict_keyword_norm_spaces(MatchKeywords::DEAL_ID, deal_it->id);
        }

        if (slot_it->video_placement.has_value())
        {
          add_token(ad_slot_request.tokens,
            "SSP_VIDEO_PLACEMENT",
            openrtb_video_placement_to_string_(*slot_it->video_placement));
        }

        ++slot_i;
        ++slot_it;
      }
      else
      {
        context.ad_slots.erase(slot_it++);
      }
    }

    request_info.ad_slots.resize(slot_i);

    if ((!context.ad_slots.empty() && context.ad_slots.front().secure) || context.secure)
    {
      request_info.creative_instantiate_type = FrontendCommons::SECURE_INSTANTIATE_TYPE;
    }
    else if(request_info.creative_instantiate_type.empty())
    {
      // was not filled by fill_by_referer_ and later
      request_info.creative_instantiate_type = FrontendCommons::UNSECURE_INSTANTIATE_TYPE;
    }

    // fill idfa or advertising_id by ifa depends on device
    context.ifa = context.hold_string(normalize_ifa(
      context.ifa,
      context.resource()));

    if(!context.ifa.empty())
    {
      // check idfa
      if(request_info.platform_names.find(
           std::string_view(PlatformNames::IPAD)) !=
           request_info.platform_names.end() ||
        request_info.platform_names.find(
          std::string_view(PlatformNames::IPHONE)) !=
          request_info.platform_names.end() ||
        request_info.platform_names.find(
          std::string_view(PlatformNames::IOS)) !=
          request_info.platform_names.end())
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

      request_info.ext_track_params = std::move(ext_track_params);
    }

    if(!request_info.idfa.empty() || !request_info.advertising_id.empty())
    {
      kw_fmt.add_keyword(MatchKeywords::FULL_IDFA);
      kw_fmt.add_keyword(MatchKeywords::FULL_IDFA_KNOWN);
    }

    kw_fmt.assign_to(keywords);

    add_special_keywords_(keywords, request_info, &context, context.app_id);

    // push eids to request_info
    for(auto it = context.user_eids.begin(); it != context.user_eids.end(); ++it)
    {
      for(auto sub_it = it->uids.begin(); sub_it != it->uids.end(); ++sub_it)
      {
        std::string source;
        if(!it->source.empty())
        {
          source.assign(it->source.data(), it->source.size());
        }
        else
        {
          source.assign(
            request_info.source_id.data(),
            request_info.source_id.size());
        }

        auto source_mapping_it = source_mapping_.find(source);
        if(source_mapping_it != source_mapping_.end())
        {
          source = source_mapping_it->second;
        }

        if(!sub_it->id.empty() && use_external_user_id_(sub_it->id))
        {
          Generics::MonoString user_id(request_info.resource());
          user_id.reserve(source.size() + 1 + sub_it->id.size());
          user_id += source;
          user_id += '/';
          user_id.append(sub_it->id.data(), sub_it->id.size());
          request_info.ext_user_ids.emplace_back(std::move(user_id));
        }
        else if(!sub_it->stable_id.empty() && use_external_user_id_(sub_it->stable_id))
        {
          const std::string& use_source =
            !source.empty() ? source : Request::OpenRtb::STABLE_SOURCE;
          Generics::MonoString user_id(request_info.resource());
          user_id.reserve(use_source.size() + 1 + sub_it->stable_id.size());
          user_id += use_source;
          user_id += '/';
          user_id.append(sub_it->stable_id.data(), sub_it->stable_id.size());
          request_info.ext_user_ids.emplace_back(std::move(user_id));
        }
      }
    }
  }

  void
  RequestInfoFiller::init_param_processors_() noexcept
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
      new FrontendCommons::StringParamProcessor<RequestInfo, Generics::MonoString>(
        &RequestInfo::source_id));
    add_param_processor_(
      Request::Context::FORMAT,
      new FrontendCommons::StringParamProcessor<RequestInfo, Generics::MonoString>(
        &RequestInfo::format));
    add_param_processor_(
      Request::Context::REQUIRE_DEBUG_INFO,
      new FrontendCommons::StringParamProcessor<RequestInfo, Generics::MonoString>(
        &RequestInfo::require_debug_info));
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
      new FrontendCommons::StringParamProcessor<RequestInfo, Generics::MonoString>(
        &RequestInfo::default_debug_size));
  }

  void
  RequestInfoFiller::add_param_processor_(
    std::string_view name,
    RequestInfoParamProcessor* processor)
    noexcept
  {
    param_processors_.insert(std::make_pair(
      String::SubString(name.data(), name.size()),
      RequestInfoParamProcessor_var(processor)));
  }

  void
  RequestInfoFiller::fill_by_user_agent(
    RequestInfo& request_info,
    std::string_view user_agent,
    bool filter_request,
    bool application)
    const
    noexcept
  {
    static const char* FUN = "RequestInfoFiller::fill_by_user_agent()";

    request_info.set_cookie = !application;

    if(!user_agent.empty())
    {
      FrontendCommons::WebBrowserMatcher_var web_browser_matcher =
        common_module_->web_browser_matcher();

      std::string web_browser;

      if(web_browser_matcher.in())
      {
        try
        {
          web_browser_matcher->match(web_browser, user_agent);
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

      request_info.user_agent = user_agent;
      request_info.web_browser = std::move(web_browser);
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

      request_info.platform = std::move(platform);
      request_info.full_platform = std::move(full_platform);
      CampaignManager::fill_sequence(
        platform_ids.begin(),
        platform_ids.end(),
        request_info.platform_ids);
    }
  }

  template<typename StringType>
  void
  RequestInfoFiller::fill_by_referer(
    RequestInfo& request_info,
    StringType& search_words,
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
          request_info.creative_instantiate_type = FrontendCommons::SECURE_INSTANTIATE_TYPE;
        }

        std::string normalized_referer =
          FrontendCommons::normalize_abs_url(referer);
        request_info.full_referer_hash =
           FrontendCommons::referer_hash(normalized_referer);
        request_info.short_referer_hash =
          FrontendCommons::short_referer_hash(normalized_referer);
        request_info.referer = std::move(normalized_referer);

        if (fill_search_words)
        {
          fill_search_words_(request_info, search_words, referer);
        }
      }
      catch(...)
      {}
    }
  }

  template<typename StringType>
  void
  RequestInfoFiller::fill_additional_url_(
    RequestInfo& request_info,
    StringType& search_words,
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
        std::size_t pos = request_info.urls.size();
        request_info.urls.resize(pos + 1);
        request_info.urls[pos] = std::move(normalized_add_url);

        fill_search_words_(request_info, search_words, add_url);
      }
      catch(...)
      {}
    }
  }

  template<typename StringType>
  void
  RequestInfoFiller::fill_search_words_(
    RequestInfo& request_info,
    StringType& search_words,
    const HTTP::HTTPAddress& url)
    const
    noexcept
  {
    if(!url.url().empty())
    {
      FrontendCommons::UrlMatcher_var url_matcher = common_module_->url_matcher();
      if(url_matcher.in())
      {
        unsigned long search_engine_id;
        std::string search_words_value;

        Language::Segmentor::SegmentorInterface_var segmentor = common_module_->segmentor();

        if(url_matcher->match(search_engine_id, search_words_value, url, segmentor))
        {
          // don't override search_engine_id got by referer parsing
          if(request_info.search_engine_id == 0)
          {
            request_info.search_engine_id = search_engine_id;
          }

          if(search_words.empty())
          {
            search_words.assign(search_words_value.data(), search_words_value.size());
          }
          else
          {
            search_words += "\n";
            search_words.append(search_words_value.data(), search_words_value.size());
          }
        }
      }
    }
  }

  void
  RequestInfoFiller::fill_by_ip(RequestInfo& request_info, std::string_view ip) const
    noexcept
  {
    if(request_info.location)
    {
      request_info.geo_location.resize(1);
      request_info.geo_location[0].country = request_info.location->country;
      request_info.geo_location[0].region = request_info.location->region;
      request_info.geo_location[0].city = request_info.location->city;
    }
    else
    {
      GeoIPMapping::IPMapCity2::CityLocation geo_location;

      if(!ip.empty())
      {
        request_info.peer_ip = ip;

        if(ip_logging_enabled_)
        {
          std::string ip_hash;
          FrontendCommons::ip_hash(ip_hash, ip, ip_salt_);
          request_info.ip_hash = std::move(ip_hash);
        }

        if(ip_map_.get())
        {
          try
          {
            if(ip_map_->city_location_by_addr(
                 std::string(ip).c_str(),
                 geo_location,
                 false,
                 true))
            {
              request_info.location = std::make_shared<FrontendCommons::Location>();
              request_info.location->country = geo_location.country_code.str();
              geo_location.region.assign_to(request_info.location->region);
              request_info.location->city = geo_location.city.str();
              request_info.location->normalize();

              request_info.geo_location.resize(1);
              request_info.geo_location[0].country = request_info.location->country;
              request_info.geo_location[0].region = request_info.location->region;
              request_info.geo_location[0].city = request_info.location->city;
            }
          }
          catch(const eh::Exception&)
          {}
        }
      }

      FrontendCommons::CountryFilter_var country_filter = common_module_->country_filter();

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
  RequestInfoFiller::use_external_user_id_(std::string_view external_user_id) const noexcept
  {
    return skip_external_ids_.find(std::string(external_user_id)) == skip_external_ids_.end();
  }

  void
  RequestInfoFiller::verify_user_id_(
    std::string_view signed_user_id,
    std::string_view source_id,
    RequestInfo& request_info)
    const noexcept
  {
    static const size_t UNSIGNED_SSP_USER_ID_LEN = 22;
    static const size_t UNSIGNED_ALIGNED_SSP_USER_ID_LEN = 24;

    if (!signed_user_id.empty())
    {
      try
      {
        Generics::Uuid ssp_uid;
        uint8_t ssp_uid_marker;
        if (signed_user_id.size() == UNSIGNED_SSP_USER_ID_LEN ||
          signed_user_id.size() == UNSIGNED_ALIGNED_SSP_USER_ID_LEN)
        {
          std::string ssp_id_buf;
          String::StringManip::base64mod_decode(
            ssp_id_buf,
            signed_user_id,
            false,
            &ssp_uid_marker);
          ssp_uid = Generics::Uuid(ssp_id_buf.begin(), ssp_id_buf.end());
        }
        else
        {
          Generics::SignedUuid signed_ssp_uid =
            common_module_->user_id_controller()->verify(
              signed_user_id,
              UserIdController::SSP);
          ssp_uid = signed_ssp_uid.uuid();
          ssp_uid_marker = signed_ssp_uid.data();
        }
        // Don't check for empty ssp_uid (UserIdBlackList)

        const Generics::Uuid uid = UserIdController::get_by_ssp_user_id(
          ssp_uid, source_id, ssp_uid_marker);

        if (uid.is_null())
        {
          // for google: keep undefined state.
          // Set it in in case openrtb.
          request_info.user_status = static_cast<std::size_t>(
            AdServer::CampaignSvcs::US_UNDEFINED);
          return;
        }

        request_info.user_id = uid;
        request_info.track_user_id = uid;

        if(std::string_view(AdServer::Commons::PROBE_USER_ID.to_string()) == signed_user_id)
        {
          request_info.user_status = static_cast<std::size_t>(AdServer::CampaignSvcs::US_PROBE);
          return;
        }

        request_info.user_status = static_cast<std::size_t>(AdServer::CampaignSvcs::US_OPTIN);
      }
      catch(...)
      {
      }
    }
  }

  void
  RequestInfoFiller::select_referer_(
    RequestInfo& request_info,
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
      const std::string site_page = first_significant_domain_part_(
        to_string_view(context.site_page.host()));

      if (!context.site_domain.url().empty())
      {
        const std::string site_domain = first_significant_domain_part_(
          to_string_view(context.site_domain.host()));

        if (site_page == site_domain)
        {
          referer = context.site_page;
          selected_referer_field = SR_PAGE;
        }
      }

      if (referer.url().empty() && !context.site_name.empty())
      {
        fill_json_url(context.site_name, site_name_url);

        const std::string site_name = first_significant_domain_part_(
          to_string_view(site_name_url.host()));

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
          fill_json_url(context.site_name, site_name_url);
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
          fill_additional_url_(request_info, search_words, *URL_FIELDS[url_i]);
        }
      }
      catch(const eh::Exception&)
      {
        // ignory bad url
      }
    }

    try
    {
      if(selected_referer_field != SR_NAME && !context.site_name.empty())
      {
        std::string search_words; // ignore search words value (push to match only)
        HTTP::BrowserAddress site_name_url(context.site_name);
        fill_additional_url_(request_info, search_words, site_name_url);
      }
    }
    catch(const eh::Exception&)
    {
      // ignory bad url
    }
  }

  std::string
  RequestInfoFiller::first_significant_domain_part_(std::string_view host) const
    /*throw(eh::Exception)*/
  {
    std::string domain;
    common_module_->domain_parser()->specific_domain(
      String::SubString(host.data(), host.size()),
      domain);
    const std::size_t pos = domain.find('.');

    if (pos != std::string::npos)
    {
      domain.erase(pos);
    }

    return domain;
  }

  std::string
  RequestInfoFiller::normalize_ext_tag_id_(std::string_view src)
    noexcept
  {
    String::SubString res;
    const String::SubString src_sub_string(src.data(), src.size());
    if(String::StringManip::utf8_substr(src_sub_string, EXT_TAG_ID_MAX_LENGTH, res))
    {
      return res.str();
    }

    std::string enc_res;
    AdServer::LogProcessing::undisplayable_mime_encode(
      enc_res, src_sub_string.substr(0, EXT_TAG_ID_MAX_LENGTH));
    return enc_res.substr(0, EXT_TAG_ID_MAX_LENGTH);
  }

  void
  RequestInfoFiller::init_adslot(AdServer::Bidding::RequestInfo::AdSlotInfo& adslot_info)
    noexcept
  {
    adslot_info.video_allow_skippable = true;
    adslot_info.video_allow_unskippable = true;
    adslot_info.video_skippable_max_duration = -1;
  }

  void
  RequestInfoFiller::init_request_param(RequestInfo& request_info) const noexcept
  {
    request_info.time = request_info.current_time;

    request_info.profile_referer = true;
    request_info.full_referer_hash = 0;
    request_info.short_referer_hash = 0;
    request_info.page_load_id = 0;

    request_info.fill_track_pixel = false;
    request_info.fill_iurl = request_info.flag & 0x01;

    request_info.request_id = AdServer::Commons::RequestId::create_random_based();

    request_info.colo_id = colo_id_;
    request_info.search_engine_id = 0;
    request_info.page_keywords_present = false;
    request_info.need_debug_info = false;
    request_info.only_display_ad = true;
    request_info.test_request = false;
    request_info.preview_ccid = 0;
    request_info.hpos = CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM;

    fill_request_type_(request_info, request_info.source_id);
  }

  std::string_view
  RequestInfoFiller::make_ssp_uid_by_device_(const JsonProcessingContext& ctx)
    /*throw(std::exception)*/
  {
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
  RequestInfoFiller::adapt_app_store_url_(std::string_view store_url_str)
    /*throw(eh::Exception)*/
  {
    HTTP::BrowserAddress store_url(store_url_str);

    if(store_url.host().compare(GooglePlay::STORE_HOST) == 0 &&
      store_url.path().compare(0, GooglePlay::STORE_PATH.size(), GooglePlay::STORE_PATH) == 0)
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

    return std::string(store_url_str);
  }

  std::string_view
  RequestInfoFiller::norm_keyword_ext_(RequestInfo& request_info, std::string_view kw) noexcept
  {
    bool changed = false;
    for(const char ch_ref : kw)
    {
      const unsigned char ch = ch_ref;
      if(ch >= 0x80 || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z'))
      {
        continue;
      }

      changed = true;
      break;
    }

    if(!changed)
    {
      return kw;
    }

    std::string simplified_kw(kw);
    for(auto& ch_ref : simplified_kw)
    {
      const unsigned char ch = ch_ref;
      if(ch >= 0x80 || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z'))
      {
        continue;
      }
      else if(ch >= 'A' && ch <= 'Z')
      {
        ch_ref = static_cast<char>(ch - 'A' + 'a');
      }
      else
      {
        ch_ref = 'x';
      }
    }

    return request_info.hold_string(std::move(simplified_kw));
  }

  template<typename StringType>
  void
  RequestInfoFiller::add_special_keywords_impl_(
    StringType& keywords,
    RequestInfo& request_info,
    const JsonProcessingContext* context,
    std::string_view alt_app_id)
    const
    noexcept
  {
    BasicKeywordFormatter<Generics::MonoString> kw_fmt(
      request_info.source_id,
      request_info.arena());

    for(auto platform_name_it = request_info.platform_names.begin();
      platform_name_it != request_info.platform_names.end();
      ++platform_name_it)
    {
      const std::string_view platform = norm_keyword_ext_(request_info, *platform_name_it);
      std::string keyword;
      keyword.reserve(11 + platform.size());
      keyword += "rtbplatform";
      keyword.append(platform.data(), platform.size());
      kw_fmt.add_keyword_owned(std::move(keyword));
    }

    if(request_info.is_app)
    {
      std::string_view app_id = norm_keyword_ext_(request_info, request_info.application_id);
      if(!app_id.empty())
      {
        std::string keyword;
        keyword.reserve(6 + app_id.size());
        keyword += "rtbapp";
        keyword.append(app_id.data(), app_id.size());
        kw_fmt.add_keyword_owned(std::move(keyword));
      }

      if(!alt_app_id.empty())
      {
        app_id = norm_keyword_ext_(request_info, alt_app_id);
        if(!app_id.empty())
        {
          std::string keyword;
          keyword.reserve(6 + app_id.size());
          keyword += "rtbapp";
          keyword.append(app_id.data(), app_id.size());
          kw_fmt.add_keyword_owned(std::move(keyword));
        }
      }
    }

    kw_fmt.assign_to(keywords);

    if(context)
    {
      if(!context->language.empty())
      {
        const std::string_view language = norm_keyword_ext_(request_info, context->language);
        std::string keyword;
        keyword.reserve(11 + language.size());
        keyword += "rtblanguage";
        keyword.append(language.data(), language.size());
        kw_fmt.add_keyword_owned(std::move(keyword));
      }
    }
  }

  void
  RequestInfoFiller::add_special_keywords_(
    std::string& keywords,
    RequestInfo& request_info,
    const JsonProcessingContext* context,
    std::string_view alt_app_id)
    const
    noexcept
  {
    add_special_keywords_impl_(keywords, request_info, context, alt_app_id);
  }

  void
  RequestInfoFiller::add_special_keywords_(
    Generics::MonoString& keywords,
    RequestInfo& request_info,
    const JsonProcessingContext* context,
    std::string_view alt_app_id)
    const
    noexcept
  {
    add_special_keywords_impl_(keywords, request_info, context, alt_app_id);
  }

  std::string_view
  RequestInfoFiller::openrtb_devicetype_to_string_(unsigned int devicetype) const
  {
    auto it = openrtb_devicetype_mapping_.find(devicetype);
    if (it != openrtb_devicetype_mapping_.end())
    {
      return it->second;
    }

    return {};
  }

  std::string_view
  RequestInfoFiller::openrtb_video_placement_to_string_(unsigned int video_placement_type)
    const
  {
    auto it = openrtb_video_placement_mapping_.find(video_placement_type);
    if (it != openrtb_video_placement_mapping_.end())
    {
      return it->second;
    }

    return {};
  }

  namespace
  {
    using FastJsonParser = AdServer::Commons::FastJsonParser<Generics::MonoString>;
    using ValueProcessor = FastJsonParser::ValueProcessor;
    using AdSlotContext = JsonAdSlotProcessingContext;
    using Banner = AdSlotContext::Banner;
    using BannerFormat = AdSlotContext::BannerFormat;
    using Native = AdSlotContext::Native;
    using NativeAsset = Native::Asset;

    struct FastOpenRtbState
    {
      RequestInfo* request_info = nullptr;
      JsonProcessingContext* context = nullptr;
      AdSlotContext* ad_slot = nullptr;
      Banner* banner = nullptr;
      BannerFormat* banner_format = nullptr;
      AdSlotContext::Deal* deal = nullptr;
      AdSlotContext::Metric* metric = nullptr;
      JsonProcessingContext::Segment* segment = nullptr;
      JsonProcessingContext::UserEid* user_eid = nullptr;
      JsonProcessingContext::UserEidUid* user_eid_uid = nullptr;
      Native* native = nullptr;
      Native::Data* native_data = nullptr;
      Native::Image* native_image = nullptr;
      Native::Video* native_video = nullptr;
      NativeAsset* native_asset = nullptr;
      long pending_native_asset_id = 0;
      bool pending_native_asset_required = false;
      bool pending_native_asset_id_present = false;
      bool pending_native_asset_required_present = false;
      bool matching_ad_processed = false;
    };

    Generics::MonoString
    integer_to_string(FastOpenRtbState& state, int64_t value)
    {
      char buf[32];
      const auto result = std::to_chars(buf, buf + sizeof(buf), value);
      return Generics::MonoString(
        buf,
        result.ptr,
        state.context->resource());
    }

    Generics::MonoString
    float_to_string(FastOpenRtbState& state, double value)
    {
      char buf[64];
      const int size = std::snprintf(buf, sizeof(buf), "%.17g", value);
      if(size <= 0)
      {
        return Generics::MonoString(state.context->resource());
      }

      return Generics::MonoString(
        buf,
        static_cast<std::size_t>(size),
        state.context->resource());
    }

    template<typename NumberType>
    bool
    parse_integer(std::string_view value, NumberType& result)
    {
      const auto parsed = std::from_chars(
        value.data(),
        value.data() + value.size(),
        result);
      return parsed.ec == std::errc() && parsed.ptr == value.data() + value.size();
    }

    template<typename DecimalType>
    DecimalType
    parse_decimal(
      std::string_view value,
      Generics::DecimalMulRemainder round_type,
      const DecimalType& invalid_value = DecimalType())
    {
      try
      {
        return AdServer::Commons::extract_decimal<DecimalType>(
          value,
          round_type);
      }
      catch(const typename DecimalType::Exception&)
      {
        return invalid_value;
      }
    }

    template<typename DecimalType>
    bool
    try_parse_decimal(
      DecimalType& result,
      std::string_view value,
      Generics::DecimalMulRemainder round_type)
    {
      try
      {
        result = AdServer::Commons::extract_decimal<DecimalType>(
          value,
          round_type,
          true);
        return true;
      }
      catch(const typename DecimalType::Exception&)
      {
        return false;
      }
    }

    template<typename NumberType>
    bool
    convert_integer(int64_t value, NumberType& result)
    {
      if constexpr(std::is_unsigned_v<NumberType>)
      {
        if(value < 0 ||
          static_cast<uint64_t>(value) >
            static_cast<uint64_t>(std::numeric_limits<NumberType>::max()))
        {
          return false;
        }
      }
      else
      {
        if(value < static_cast<int64_t>(std::numeric_limits<NumberType>::min()) ||
          value > static_cast<int64_t>(std::numeric_limits<NumberType>::max()))
        {
          return false;
        }
      }

      result = static_cast<NumberType>(value);
      return true;
    }

    template<typename NumberType>
    bool
    convert_integer(double value, NumberType& result)
    {
      if(!std::isfinite(value) || std::trunc(value) != value)
      {
        return false;
      }

      const auto min_value = static_cast<long double>(
        std::numeric_limits<NumberType>::lowest());
      const auto max_value = static_cast<long double>(
        std::numeric_limits<NumberType>::max());
      const auto check_value = static_cast<long double>(value);
      if(check_value < min_value || check_value > max_value)
      {
        return false;
      }

      result = static_cast<NumberType>(value);
      return true;
    }

    void
    fill_url(std::string_view value, HTTP::HTTPAddress& target)
    {
      fill_json_url(value, target);
    }

    void
    set_state_number(AdSlotContext::ULValueStateHolder& target, std::string_view value)
    {
      unsigned long result = 0;
      if(parse_integer(value, result))
      {
        target = result;
      }
      else
      {
        target.set_state(AdSlotContext::ULValueStateHolder::S_FAIL);
      }
    }

    template<typename HolderType>
    void
    set_state_number(HolderType& target, int64_t value)
    {
      typename HolderType::ValueType result = 0;
      if(convert_integer(value, result))
      {
        target = result;
      }
      else
      {
        target.set_state(HolderType::S_FAIL);
      }
    }

    template<typename HolderType>
    void
    set_state_number(HolderType& target, double value)
    {
      typename HolderType::ValueType result = 0;
      if(convert_integer(value, result))
      {
        target = result;
      }
      else
      {
        target.set_state(HolderType::S_FAIL);
      }
    }

    void
    set_state_number(AdSlotContext::LValueStateHolder& target, std::string_view value)
    {
      long result = 0;
      if(parse_integer(value, result))
      {
        target = result;
      }
      else
      {
        target.set_state(AdSlotContext::LValueStateHolder::S_FAIL);
      }
    }

    template<typename SetType>
    inline void
    insert_string(SetType& target, std::string_view value)
    {
      target.emplace(value);
    }

    template<typename VectorType>
    inline void
    append_string(VectorType& target, std::string_view value)
    {
      target.emplace_back(value);
    }

    template<typename HolderType>
    inline void
    ensure_number_holder_present(HolderType& holder)
    {
      if(!holder.present())
      {
        holder->clear();
        holder.set_state(HolderType::S_GOOD);
      }
    }

    template<typename HolderType>
    inline void
    insert_number(HolderType& holder, std::string_view value)
    {
      typename HolderType::ValueType::value_type result = 0;
      if(parse_integer(value, result))
      {
        ensure_number_holder_present(holder);
        holder->insert(holder->end(), result);
      }
      else
      {
        holder.set_state(HolderType::S_FAIL);
      }
    }

    template<typename HolderType>
    inline void
    insert_number(HolderType& holder, int64_t value)
    {
      typename HolderType::ValueType::value_type result = 0;
      if(convert_integer(value, result))
      {
        ensure_number_holder_present(holder);
        holder->insert(holder->end(), result);
      }
      else
      {
        holder.set_state(HolderType::S_FAIL);
      }
    }

    template<typename HolderType>
    inline void
    insert_number(HolderType& holder, double value)
    {
      typename HolderType::ValueType::value_type result = 0;
      if(convert_integer(value, result))
      {
        ensure_number_holder_present(holder);
        holder->insert(holder->end(), result);
      }
      else
      {
        holder.set_state(HolderType::S_FAIL);
      }
    }

    inline FastOpenRtbState&
    state_from_context(void* context)
    {
      return *static_cast<FastOpenRtbState*>(context);
    }

    class IgnoringValueProcessor: public ValueProcessor
    {
    public:
      void
      object_started(std::string_view, void*) const override
      {}

      void
      array_started(std::string_view, void*) const override
      {}

      void
      process_integer(int64_t, std::string_view, void*) const override
      {}

      void
      process_float(double, std::string_view, void*) const override
      {}

      void
      process_string(std::string_view, std::string_view, void*) const override
      {}

      void
      process_string(Generics::MonoString&&, std::string_view, void*) const override
      {}

      void
      process_bool(bool, std::string_view, void*) const override
      {}

      void
      process_null(std::string_view, void*) const override
      {}
    };

    template<typename ProcessorType>
    void
    add_processor(
      FastJsonParser::ProcessorSet& processors,
      std::string_view path,
      std::shared_ptr<ProcessorType> processor,
      bool as_string = false)
    {
      processors.add_processor(path, std::move(processor), as_string);
    }

    template<typename Context, typename Field>
    class StringFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      StringFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        Field Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = value;
        }
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        if(Context* target = get_context_(state))
        {
          target->*field_ = state.context->hold_string(std::move(value));
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      Field Context::* field_;
    };

    template<typename Context, typename Field>
    class UrlFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      UrlFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        Field Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          fill_url(value, target->*field_);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      Field Context::* field_;
    };

    template<typename Context, typename NumberType>
    class IntegerFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      IntegerFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        NumberType Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = static_cast<NumberType>(value);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      NumberType Context::* field_;
    };

    template<typename Context>
    class OptionalLongFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      OptionalLongFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        std::optional<long> Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = static_cast<long>(value);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      std::optional<long> Context::* field_;
    };

    template<typename Context>
    class BoolFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      BoolFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        bool Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_bool(bool value, std::string_view, void* context) const override
      {
        set_(value, context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        set_(value != 0, context);
      }

    private:
      void
      set_(bool value, void* context) const
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = value;
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      bool Context::* field_;
    };

    template<typename Context, typename DecimalType>
    class DecimalFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      DecimalFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        DecimalType Context::* field,
        Generics::DecimalMulRemainder round_type,
        const DecimalType& invalid_value)
        : get_context_(get_context),
          field_(field),
          round_type_(round_type),
          invalid_value_(invalid_value)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        parse_(value, context);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        parse_(value, context);
      }

      void
      process_number(
        std::string_view,
        bool,
        std::string_view,
        void*) const override
      {
        assert(false && "DecimalFieldProcessor must be registered as string");
      }

      void
      process_integer(int64_t, std::string_view, void*) const override
      {
        assert(false && "DecimalFieldProcessor must be registered as string");
      }

      void
      process_float(double, std::string_view, void*) const override
      {
        assert(false && "DecimalFieldProcessor must be registered as string");
      }

    private:
      void
      parse_(std::string_view value, void* context) const
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = parse_decimal(
            value,
            round_type_,
            invalid_value_);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      DecimalType Context::* field_;
      Generics::DecimalMulRemainder round_type_;
      DecimalType invalid_value_;
    };

    std::string
    make_path(std::string_view prefix, std::string_view suffix)
    {
      if(prefix.empty())
      {
        return std::string(suffix);
      }

      std::string path;
      path.reserve(prefix.size() + 1 + suffix.size());
      path.append(prefix);
      path += '.';
      path.append(suffix);
      return path;
    }

    template<typename Context, typename Field>
    void
    add_string(
      FastJsonParser::ProcessorSet& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      Field Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<StringFieldProcessor<Context, Field>>(
          get_context,
          field),
        true);
    }

    template<typename Context, typename Field>
    void
    add_url(
      FastJsonParser::ProcessorSet& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      Field Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<UrlFieldProcessor<Context, Field>>(get_context, field));
    }

    template<typename Context, typename NumberType>
    void
    add_integer(
      FastJsonParser::ProcessorSet& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      NumberType Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<IntegerFieldProcessor<Context, NumberType>>(
          get_context,
          field));
    }

    template<typename Context>
    void
    add_optional_long(
      FastJsonParser::ProcessorSet& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      std::optional<long> Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<OptionalLongFieldProcessor<Context>>(get_context, field));
    }

    template<typename Context>
    void
    add_bool(
      FastJsonParser::ProcessorSet& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      bool Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<BoolFieldProcessor<Context>>(get_context, field));
    }

    template<typename Context, typename DecimalType>
    void
    add_decimal(
      FastJsonParser::ProcessorSet& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      DecimalType Context::* field,
      Generics::DecimalMulRemainder round_type,
      const DecimalType& invalid_value = DecimalType())
    {
      add_processor(
        parser,
        path,
        std::make_shared<DecimalFieldProcessor<Context, DecimalType>>(
          get_context,
          field,
          round_type,
          invalid_value),
        true);
    }

    JsonProcessingContext*
    root_context(FastOpenRtbState& state)
    {
      return state.context;
    }

    AdSlotContext*
    ad_slot_context(FastOpenRtbState& state)
    {
      return state.ad_slot;
    }

    Banner*
    banner_context(FastOpenRtbState& state)
    {
      return state.banner;
    }

    BannerFormat*
    banner_format_context(FastOpenRtbState& state)
    {
      return state.banner_format;
    }

    AdSlotContext::Deal*
    deal_context(FastOpenRtbState& state)
    {
      return state.deal;
    }

    AdSlotContext::Metric*
    metric_context(FastOpenRtbState& state)
    {
      return state.metric;
    }

    JsonProcessingContext::Segment*
    segment_context(FastOpenRtbState& state)
    {
      return state.segment;
    }

    JsonProcessingContext::UserEid*
    user_eid_context(FastOpenRtbState& state)
    {
      return state.user_eid;
    }

    JsonProcessingContext::UserEidUid*
    user_eid_uid_context(FastOpenRtbState& state)
    {
      return state.user_eid_uid;
    }

    Native*
    native_context(FastOpenRtbState& state)
    {
      return state.native;
    }

    Native::Data*
    native_data_context(FastOpenRtbState& state)
    {
      return state.native_data;
    }

    Native::Image*
    native_image_context(FastOpenRtbState& state)
    {
      return state.native_image;
    }

    void
    start_ad_slot(FastOpenRtbState& state)
    {
      state.context->ad_slots.emplace_back(state.context->resource());
      state.ad_slot = &state.context->ad_slots.back();
      state.banner = nullptr;
      state.banner_format = nullptr;
      state.deal = nullptr;
      state.metric = nullptr;
      state.native = nullptr;
    }

    void
    start_banner(FastOpenRtbState& state)
    {
      if(!state.ad_slot)
      {
        return;
      }

      state.ad_slot->banners.emplace_back(state.ad_slot->resource());
      state.banner = &state.ad_slot->banners.back();
      state.banner_format = nullptr;
      state.matching_ad_processed = false;
    }

    BannerFormat*
    ensure_banner_format(FastOpenRtbState& state)
    {
      if(!state.banner)
      {
        return nullptr;
      }

      if(!state.banner_format)
      {
        state.banner->formats.emplace_back(state.banner->resource());
        state.banner_format = &state.banner->formats.back();
      }

      return state.banner_format;
    }

    void
    start_banner_format(FastOpenRtbState& state)
    {
      if(!state.banner)
      {
        return;
      }
      state.banner->formats.emplace_back(state.banner->resource());
      state.banner_format = &state.banner->formats.back();
    }

    void
    start_deal(FastOpenRtbState& state)
    {
      if(!state.ad_slot)
      {
        return;
      }
      state.ad_slot->deals.emplace_back(state.ad_slot->resource());
      state.deal = &state.ad_slot->deals.back();
    }

    void
    start_metric(FastOpenRtbState& state)
    {
      if(!state.ad_slot)
      {
        return;
      }
      state.ad_slot->metrics.emplace_back(state.ad_slot->resource());
      state.metric = &state.ad_slot->metrics.back();
    }

    void
    start_segment(FastOpenRtbState& state)
    {
      state.context->segments.emplace_back(state.context->resource());
      state.segment = &state.context->segments.back();
    }

    void
    start_user_eid(FastOpenRtbState& state)
    {
      state.context->user_eids.emplace_back(state.context->resource());
      state.user_eid = &state.context->user_eids.back();
      state.user_eid_uid = nullptr;
    }

    void
    start_user_eid_uid(FastOpenRtbState& state)
    {
      if(!state.user_eid)
      {
        return;
      }
      state.user_eid->uids.emplace_back(
        state.user_eid->uids.get_allocator().arena());
      state.user_eid_uid = &state.user_eid->uids.back();
    }

    void
    start_native(FastOpenRtbState& state)
    {
      if(!state.ad_slot)
      {
        return;
      }
      state.ad_slot->native = AdSlotContext::Native_var(
        new Native(state.ad_slot->resource()));
      state.native = state.ad_slot->native.in();
    }

    void
    start_native_asset(FastOpenRtbState& state)
    {
      state.native_data = nullptr;
      state.native_image = nullptr;
      state.native_video = nullptr;
      state.native_asset = nullptr;
      state.pending_native_asset_id = 0;
      state.pending_native_asset_required = false;
      state.pending_native_asset_id_present = false;
      state.pending_native_asset_required_present = false;
    }

    template<typename Asset>
    void
    apply_pending_native_asset(FastOpenRtbState& state, Asset& asset)
    {
      if(state.pending_native_asset_id_present)
      {
        asset.id = state.pending_native_asset_id;
      }

      if(state.pending_native_asset_required_present)
      {
        asset.required = state.pending_native_asset_required;
      }

      state.native_asset = &asset;
    }

    void
    start_native_title(FastOpenRtbState& state)
    {
      if(!state.native)
      {
        return;
      }
      state.native->data_assets.emplace_back(state.native->resource());
      state.native_data = &state.native->data_assets.back();
      state.native_data->data_type = Native::NDTE_TITLE;
      apply_pending_native_asset(state, *state.native_data);
    }

    void
    start_native_data(FastOpenRtbState& state)
    {
      if(!state.native)
      {
        return;
      }
      state.native->data_assets.emplace_back(state.native->resource());
      state.native_data = &state.native->data_assets.back();
      apply_pending_native_asset(state, *state.native_data);
    }

    void
    start_native_image(FastOpenRtbState& state)
    {
      if(!state.native)
      {
        return;
      }
      state.native->image_assets.emplace_back(state.native->resource());
      state.native_image = &state.native->image_assets.back();
      apply_pending_native_asset(state, *state.native_image);
    }

    void
    start_native_video(FastOpenRtbState& state)
    {
      if(!state.native)
      {
        return;
      }
      state.native->video_assets.emplace_back(state.native->resource());
      state.native_video = &state.native->video_assets.back();
      apply_pending_native_asset(state, *state.native_video);
    }

    struct EmptyStateHandler
    {
      void
      operator()(FastOpenRtbState&) const
      {}
    };

    template<typename ObjectHandler, typename ArrayHandler>
    class ObjectProcessor final: public IgnoringValueProcessor
    {
    public:
      ObjectProcessor(ObjectHandler object_handler, ArrayHandler array_handler)
        : object_handler_(std::move(object_handler)),
          array_handler_(std::move(array_handler))
      {}

      void
      object_started(std::string_view, void* context) const override
      {
        object_handler_(state_from_context(context));
      }

      void
      array_started(std::string_view, void* context) const override
      {
        array_handler_(state_from_context(context));
      }

    private:
      ObjectHandler object_handler_;
      ArrayHandler array_handler_;
    };

    template<typename Append>
    class StringArrayProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      StringArrayProcessor(Append append)
        : append_(std::move(append))
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        append_(state_from_context(context), value);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        append_(state, state.context->hold_string(std::move(value)));
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        append_(state, state.context->hold_string(integer_to_string(state, value)));
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        FastOpenRtbState& state = state_from_context(context);
        append_(state, state.context->hold_string(float_to_string(state, value)));
      }

    private:
      Append append_;
    };

    template<typename HolderGetter>
    class StateValueNumberProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      StateValueNumberProcessor(HolderGetter get_holder)
        : get_holder_(std::move(get_holder))
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          set_state_number(*holder, value);
        }
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          set_state_number(*holder, value);
        }
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_string_(value, context);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        process_string_(value, context);
      }

    private:
      void
      process_string_(std::string_view value, void* context) const
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          set_state_number(*holder, value);
        }
      }

    private:
      HolderGetter get_holder_;
    };

    template<typename HolderGetter>
    class StateNumberSetProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      StateNumberSetProcessor(HolderGetter get_holder)
        : get_holder_(std::move(get_holder))
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          insert_number(*holder, value);
        }
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          insert_number(*holder, value);
        }
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_string_(value, context);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        process_string_(value, context);
      }

    private:
      void
      process_string_(std::string_view value, void* context) const
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          insert_number(*holder, value);
        }
      }

    private:
      HolderGetter get_holder_;
    };

    template<typename Handler>
    class StringHandlerProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      StringHandlerProcessor(Handler handler)
        : handler_(std::move(handler))
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        handler_(state_from_context(context), value);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        handler_(state, state.context->hold_string(std::move(value)));
      }

    private:
      Handler handler_;
    };

    template<typename Handler>
    class RequestStringHandlerProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      RequestStringHandlerProcessor(Handler handler)
        : handler_(std::move(handler))
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        handler_(state_from_context(context), value);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        handler_(state, state.request_info->hold_string(std::move(value)));
      }

    private:
      Handler handler_;
    };

    template<typename Handler>
    class IntegerHandlerProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      IntegerHandlerProcessor(Handler handler)
        : handler_(std::move(handler))
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        handler_(state_from_context(context), value);
      }

    private:
      Handler handler_;
    };

    class MatchingAdProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      array_started(std::string_view, void* context) const override
      {
        state_from_context(context).matching_ad_processed = false;
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_(value, context);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        process_(state.context->hold_string(std::move(value)), context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        process_(state.context->hold_string(integer_to_string(state, value)), context);
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        FastOpenRtbState& state = state_from_context(context);
        process_(state.context->hold_string(float_to_string(state, value)), context);
      }

    private:
      void
      process_(std::string_view value, void* context) const
      {
        FastOpenRtbState& state = state_from_context(context);
        if(state.banner && !state.matching_ad_processed)
        {
          state.banner->matching_ad = value;
          state.matching_ad_processed = true;
        }
      }
    };

    template<bool Width>
    class BannerSizeProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_(value, context);
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        process_(state.context->hold_string(std::move(value)), context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        process_(state.context->hold_string(integer_to_string(state, value)), context);
      }

    private:
      void
      process_(std::string_view value, void* context) const
      {
        if(BannerFormat* format = ensure_banner_format(state_from_context(context)))
        {
          if constexpr(Width)
          {
            format->width = value;
          }
          else
          {
            format->height = value;
          }
        }
      }
    };

    class NativeAssetIdProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        if(state.native_asset)
        {
          state.native_asset->id = static_cast<long>(value);
        }
        else
        {
          state.pending_native_asset_id = static_cast<long>(value);
          state.pending_native_asset_id_present = true;
        }
      }
    };

    class NativeAssetRequiredProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_bool(bool value, std::string_view, void* context) const override
      {
        set_(value, context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        set_(value != 0, context);
      }

    private:
      void
      set_(bool value, void* context) const
      {
        FastOpenRtbState& state = state_from_context(context);
        if(state.native_asset)
        {
          state.native_asset->required = value;
        }
        else
        {
          state.pending_native_asset_required = value;
          state.pending_native_asset_required_present = true;
        }
      }
    };

    class MetricValueProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        if(AdSlotContext::Metric* metric = state_from_context(context).metric)
        {
          metric->value = value;
        }
      }

      void
      process_string(Generics::MonoString&& value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        if(AdSlotContext::Metric* metric = state.metric)
        {
          metric->value = state.context->hold_string(std::move(value));
        }
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        process_number_(static_cast<double>(value), context);
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        process_number_(value, context);
      }

    private:
      void
      process_number_(double value, void* context) const
      {
        if(AdSlotContext::Metric* metric = state_from_context(context).metric)
        {
          char buf[64];
          const int size = std::snprintf(buf, sizeof(buf), "%.3f", value);
          FastOpenRtbState& state = state_from_context(context);
          metric->value = state.context->hold_string(Generics::MonoString(
            buf,
            static_cast<std::size_t>(size),
            state.context->resource()));
        }
      }
    };

    class VideoStartedProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      object_started(std::string_view, void* context) const override
      {
        if(AdSlotContext* ad_slot = state_from_context(context).ad_slot)
        {
          ad_slot->video = true;
        }
      }
    };

    template<typename ObjectHandler, typename ArrayHandler = EmptyStateHandler>
    void
    add_object_processor(
      FastJsonParser::ProcessorSet& parser,
      std::string_view path,
      ObjectHandler object_handler,
      ArrayHandler array_handler = {})
    {
      add_processor(
        parser,
        path,
        std::make_shared<ObjectProcessor<ObjectHandler, ArrayHandler>>(
          std::move(object_handler),
          std::move(array_handler)));
    }

    template<typename Append>
    void
    add_string_array(FastJsonParser::ProcessorSet& parser, std::string_view path, Append append)
    {
      add_processor(
        parser,
        path,
        std::make_shared<StringArrayProcessor<Append>>(std::move(append)));
    }

    template<typename Handler>
    void
    add_request_string_handler(FastJsonParser::ProcessorSet& parser, std::string_view path, Handler handler)
    {
      add_processor(
        parser,
        path,
        std::make_shared<RequestStringHandlerProcessor<Handler>>(
          std::move(handler)),
        true);
    }

    template<typename Handler>
    void
    add_integer_handler(FastJsonParser::ProcessorSet& parser, std::string_view path, Handler handler)
    {
      add_processor(
        parser,
        path,
        std::make_shared<IntegerHandlerProcessor<Handler>>(std::move(handler)));
    }

    template<typename HolderGetter>
    void
    add_state_value_number(FastJsonParser::ProcessorSet& parser, std::string_view path, HolderGetter get_holder)
    {
      add_processor(
        parser,
        path,
        std::make_shared<StateValueNumberProcessor<HolderGetter>>(
          std::move(get_holder)));
    }

    template<typename HolderGetter>
    void
    add_state_number_set(FastJsonParser::ProcessorSet& parser, std::string_view path, HolderGetter get_holder)
    {
      add_processor(
        parser,
        path,
        std::make_shared<StateNumberSetProcessor<HolderGetter>>(
          std::move(get_holder)));
    }

    void
    init_banner_processors(FastJsonParser::ProcessorSet& parser, std::string_view base)
    {
      add_object_processor(parser, base, start_banner, [](FastOpenRtbState&) {});
      add_string(parser, std::string(base) + ".pos", banner_context, &Banner::pos);
      add_string_array(
        parser,
        std::string(base) + ".battr",
        [](FastOpenRtbState& state, std::string_view value)
        {
          if(state.banner)
          {
            append_string(state.banner->exclude_categories, value);
          }
        });
      add_string_array(
        parser,
        std::string(base) + ".ext.bcont",
        [](FastOpenRtbState& state, std::string_view value)
        {
          if(state.banner)
          {
            append_string(state.banner->exclude_categories, value);
          }
        });
      add_integer(
        parser,
        std::string(base) + ".ext.overlay.hpos",
        banner_context,
        &Banner::ext_hpos);
      add_processor(
        parser,
        std::string(base) + ".ext.matching_ad_id",
        std::make_shared<MatchingAdProcessor>(),
        true);

      add_object_processor(parser, std::string(base) + ".format", start_banner_format);
      add_object_processor(
        parser,
        std::string(base) + ".format",
        start_banner_format,
        [](FastOpenRtbState&) {});

      add_processor(
        parser,
        std::string(base) + ".w",
        std::make_shared<BannerSizeProcessor<true>>(),
        true);
      add_processor(
        parser,
        std::string(base) + ".h",
        std::make_shared<BannerSizeProcessor<false>>(),
        true);

      add_processor(
        parser,
        std::string(base) + ".format.w",
        std::make_shared<BannerSizeProcessor<true>>(),
        true);
      add_processor(
        parser,
        std::string(base) + ".format.h",
        std::make_shared<BannerSizeProcessor<false>>(),
        true);
      add_string(
        parser,
        std::string(base) + ".format.ext.type",
        banner_format_context,
        &BannerFormat::ext_type);
      add_string(
        parser,
        std::string(base) + ".format.ext.fmt",
        banner_format_context,
        &BannerFormat::ext_format);
    }

    void
    init_native_payload_processors(FastJsonParser::ProcessorSet& parser, std::string_view prefix)
    {
      add_string(parser, make_path(prefix, "ver"), native_context, &Native::version);
      add_optional_long(
        parser,
        make_path(prefix, "plcmttype"),
        native_context,
        &Native::placement);
      add_object_processor(
        parser,
        make_path(prefix, "assets"),
        start_native_asset,
        [](FastOpenRtbState&) {});

      add_processor(
        parser,
        make_path(prefix, "assets.id"),
        std::make_shared<NativeAssetIdProcessor>());
      add_processor(
        parser,
        make_path(prefix, "assets.required"),
        std::make_shared<NativeAssetRequiredProcessor>());

      add_object_processor(
        parser,
        make_path(prefix, "assets.title"),
        start_native_title);
      add_integer(
        parser,
        make_path(prefix, "assets.title.len"),
        native_data_context,
        &Native::Data::len);

      add_object_processor(
        parser,
        make_path(prefix, "assets.data"),
        start_native_data);
      add_integer(
        parser,
        make_path(prefix, "assets.data.type"),
        native_data_context,
        &Native::Data::data_type);
      add_integer(
        parser,
        make_path(prefix, "assets.data.len"),
        native_data_context,
        &Native::Data::len);

      add_object_processor(
        parser,
        make_path(prefix, "assets.img"),
        start_native_image);
      add_integer(
        parser,
        make_path(prefix, "assets.img.type"),
        native_image_context,
        &Native::Image::image_type);
      add_integer(
        parser,
        make_path(prefix, "assets.img.w"),
        native_image_context,
        &Native::Image::width);
      add_integer(
        parser,
        make_path(prefix, "assets.img.wmin"),
        native_image_context,
        &Native::Image::width);
      add_integer(
        parser,
        make_path(prefix, "assets.img.h"),
        native_image_context,
        &Native::Image::height);
      add_integer(
        parser,
        make_path(prefix, "assets.img.hmin"),
        native_image_context,
        &Native::Image::height);
      add_string_array(
        parser,
        make_path(prefix, "assets.img.mimes"),
        [](FastOpenRtbState& state, std::string_view value)
        {
          if(state.native_image)
          {
            insert_string(state.native_image->mimes, value);
          }
        });

      add_object_processor(
        parser,
        make_path(prefix, "assets.video"),
        start_native_video);
      add_string_array(
        parser,
        make_path(prefix, "assets.video.mimes"),
        [](FastOpenRtbState& state, std::string_view value)
        {
          if(state.native_video)
          {
            insert_string(state.native_video->mimes, value);
          }
        });
      add_state_value_number(
        parser,
        make_path(prefix, "assets.video.minduration"),
        [](FastOpenRtbState& state)
          -> AdSlotContext::ULValueStateHolder*
        {
          return state.native_video ? &state.native_video->min_duration : nullptr;
        });
      add_state_value_number(
        parser,
        make_path(prefix, "assets.video.maxduration"),
        [](FastOpenRtbState& state)
          -> AdSlotContext::ULValueStateHolder*
        {
          return state.native_video ? &state.native_video->max_duration : nullptr;
        });
      add_state_number_set(
        parser,
        make_path(prefix, "assets.video.protocols"),
        [](FastOpenRtbState& state)
          -> AdSlotContext::ULSetStateHolder*
        {
          return state.native_video ? &state.native_video->protocols : nullptr;
        });
    }

    void
    parse_native_request(FastOpenRtbState& state, std::string_view value)
    {
      if(!state.native)
      {
        return;
      }

      FastJsonParser::ProcessorSet processors;
      init_native_payload_processors(processors, "");
      init_native_payload_processors(processors, "native");
      FastJsonParser native_parser(std::move(processors), false);
      native_parser.parse(
        value,
        &state,
        [&state]()
        {
          return Generics::MonoString(state.request_info->resource());
        });
    }

    void
    init_native_processors(FastJsonParser::ProcessorSet& parser, std::string_view base)
    {
      add_object_processor(parser, base, start_native);

      add_processor(
        parser,
        std::string(base) + ".request",
        std::make_shared<StringHandlerProcessor<decltype(&parse_native_request)>>(
          &parse_native_request));

      init_native_payload_processors(parser, std::string(base) + ".request");
      init_native_payload_processors(parser, std::string(base) + ".request.native");
    }
  }

  void
  RequestInfoFiller::init_fast_json_processors_()
  {
    FastJsonParser::ProcessorSet parser;

    add_request_string_handler(
      parser,
      "id",
      [](FastOpenRtbState& state, std::string_view value)
      {
        state.request_info->bid_request_id = value;
      });
    add_string_array(
      parser,
      "cur",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->currencies, value);
      });
    add_string_array(
      parser,
      "bcat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->exclude_categories, value);
      });
    add_bool(parser, "test", root_context, &JsonProcessingContext::test);
    add_bool(parser, "ext.is_test", root_context, &JsonProcessingContext::test);
    add_bool(parser, "ext.secure", root_context, &JsonProcessingContext::secure);
    add_string(parser, "ext.category", root_context, &JsonProcessingContext::required_category);
    add_string(parser, "ext.udi.idfa", root_context, &JsonProcessingContext::ifa);
    add_string(parser, "ext.udi.gaid", root_context, &JsonProcessingContext::ifa);

    add_object_processor(
      parser,
      "imp",
      start_ad_slot,
      [](FastOpenRtbState&) {});
    add_string(parser, "imp.id", ad_slot_context, &AdSlotContext::id);
    add_decimal(
      parser,
      "imp.bidfloor",
      ad_slot_context,
      &AdSlotContext::min_cpm_price,
      Generics::DMR_CEIL,
      AdServer::CampaignSvcs::RevenueDecimal::MAXIMUM);
    add_string(
      parser,
      "imp.bidfloorcur",
      ad_slot_context,
      &AdSlotContext::min_cpm_price_currency_code);
    add_bool(parser, "imp.secure", ad_slot_context, &AdSlotContext::secure);
    add_string(parser, "imp.tagid", ad_slot_context, &AdSlotContext::tagid);
    add_string(parser, "imp.ext.type", ad_slot_context, &AdSlotContext::imp_ext_type);
    add_optional_long(
      parser,
      "imp.pmp.private_auction",
      ad_slot_context,
      &AdSlotContext::private_auction);
    add_object_processor(
      parser,
      "imp.pmp.deals",
      start_deal,
      [](FastOpenRtbState&) {});
    add_string(parser, "imp.pmp.deals.id", deal_context, &AdSlotContext::Deal::id);
    add_decimal(
      parser,
      "imp.pmp.deals.bidfloor",
      deal_context,
      &AdSlotContext::Deal::cpm_price,
      Generics::DMR_CEIL,
      AdServer::CampaignSvcs::RevenueDecimal::MAXIMUM);
    add_string(
      parser,
      "imp.pmp.deals.bidfloorcur",
      deal_context,
      &AdSlotContext::Deal::currency_code);

    add_object_processor(
      parser,
      "imp.metric",
      start_metric,
      [](FastOpenRtbState&) {});
    add_string(parser, "imp.metric.type", metric_context, &AdSlotContext::Metric::type);
    add_processor(
      parser,
      "imp.metric.value",
      std::make_shared<MetricValueProcessor>());

    init_banner_processors(parser, "imp.banner");
    init_banner_processors(parser, "imp.banners");

    add_processor(
      parser,
      "imp.video",
      std::make_shared<VideoStartedProcessor>());
    add_state_value_number(
      parser,
      "imp.video.w",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_width : nullptr;
      });
    add_state_value_number(
      parser,
      "imp.video.h",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_height : nullptr;
      });
    add_string(parser, "imp.video.pos", ad_slot_context, &AdSlotContext::video_pos);
    add_string_array(
      parser,
      "imp.video.mimes",
      [](FastOpenRtbState& state, std::string_view value)
      {
        if(state.ad_slot)
        {
          insert_string(state.ad_slot->video_mimes, value);
        }
      });
    add_string_array(
      parser,
      "imp.video.battr",
      [](FastOpenRtbState& state, std::string_view value)
      {
        if(state.ad_slot)
        {
          append_string(state.ad_slot->video_exclude_categories, value);
        }
      });
    add_state_value_number(
      parser,
      "imp.video.minduration",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_min_duration : nullptr;
      });
    add_state_value_number(
      parser,
      "imp.video.maxduration",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_max_duration : nullptr;
      });
    add_state_number_set(
      parser,
      "imp.video.protocol",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULSetStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_protocols : nullptr;
      });
    add_state_number_set(
      parser,
      "imp.video.protocols",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULSetStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_protocols : nullptr;
      });
    add_state_number_set(
      parser,
      "imp.video.playbackmethod",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULSetStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_playbackmethods : nullptr;
      });
    add_state_number_set(
      parser,
      "imp.video.api",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULSetStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_api : nullptr;
      });
    add_state_value_number(
      parser,
      "imp.video.startdelay",
      [](FastOpenRtbState& state)
        -> AdSlotContext::LValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_start_delay : nullptr;
      });
    add_state_value_number(
      parser,
      "imp.video.linearity",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_linearity : nullptr;
      });
    add_state_value_number(
      parser,
      "imp.video.skip",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_skip : nullptr;
      });
    add_state_value_number(
      parser,
      "imp.video.reward",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_reward : nullptr;
      });
    add_optional_long(
      parser,
      "imp.video.placement",
      ad_slot_context,
      &AdSlotContext::video_placement);
    add_state_value_number(
      parser,
      "imp.video.ext.adtype",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_adtype : nullptr;
      });

    init_native_processors(parser, "imp.native");

    add_object_processor(
      parser,
      "site",
      [](FastOpenRtbState& state) { state.context->site = true; }
    );
    add_url(parser, "site.page", root_context, &JsonProcessingContext::site_page);
    add_string(parser, "site.name", root_context, &JsonProcessingContext::site_name);
    add_url(parser, "site.domain", root_context, &JsonProcessingContext::site_domain);
    add_request_string_handler(
      parser,
      "site.id",
      [](FastOpenRtbState& state, std::string_view value)
      {
        state.request_info->bid_site_id = value;
      });
    add_string(parser, "site.search", root_context, &JsonProcessingContext::site_search);
    add_url(parser, "site.ref", root_context, &JsonProcessingContext::site_ref);
    add_url(parser, "site.referer", root_context, &JsonProcessingContext::site_referer);
    add_url(parser, "site.rereferer", root_context, &JsonProcessingContext::site_rereferer);
    add_string(parser, "site.keywords", root_context, &JsonProcessingContext::site_keywords);
    add_string_array(
      parser,
      "site.pagecat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->site_pagecat, value);
      });
    add_string_array(
      parser,
      "site.sectioncat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->site_sectioncat, value);
      });
    add_string_array(
      parser,
      "site.cat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->site_cat, value);
      });
    add_bool(parser, "site.ext.ssl_enabled", root_context, &JsonProcessingContext::secure);
    add_string(parser, "site.ext.puid1", root_context, &JsonProcessingContext::puid1);
    add_string(parser, "site.ext.puid2", root_context, &JsonProcessingContext::puid2);
    add_object_processor(
      parser,
      "site.content",
      [](FastOpenRtbState& state)
      {
        state.context->site_content = true;
      });
    add_string(
      parser,
      "site.content.keywords",
      root_context,
      &JsonProcessingContext::content_keywords);
    add_string(parser, "site.content.title", root_context, &JsonProcessingContext::content_title);
    add_string(
      parser,
      "site.content.series",
      root_context,
      &JsonProcessingContext::content_series);
    add_string(
      parser,
      "site.content.season",
      root_context,
      &JsonProcessingContext::content_season);
    add_string_array(
      parser,
      "site.content.cat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->content_cat, value);
      });
    add_object_processor(parser, "site.content.producer",
      [](FastOpenRtbState& state) { state.context->site_content_producer = true; });
    add_string_array(
      parser,
      "site.content.producer.name",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->content_producer_name, value);
      });
    add_object_processor(parser, "site.publisher",
      [](FastOpenRtbState& state) { state.context->site_publisher = true; });
    add_string(
      parser,
      "site.publisher.name",
      root_context,
      &JsonProcessingContext::publisher_name);
    add_request_string_handler(
      parser,
      "site.publisher.id",
      [](FastOpenRtbState& state, std::string_view value)
      {
        state.request_info->bid_publisher_id = value;
      });
    add_string_array(
      parser,
      "site.publisher.cat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->publisher_cat, value);
      });

    add_object_processor(
      parser,
      "app",
      [](FastOpenRtbState& state) { state.context->app = true; });
    add_string(parser, "app.id", root_context, &JsonProcessingContext::app_id);
    add_string(parser, "app.name", root_context, &JsonProcessingContext::app_name);
    add_string(parser, "app.bundle", root_context, &JsonProcessingContext::app_bundle);
    add_url(parser, "app.domain", root_context, &JsonProcessingContext::app_domain);
    add_url(parser, "app.storeurl", root_context, &JsonProcessingContext::app_store_url);
    add_string(parser, "app.keywords", root_context, &JsonProcessingContext::app_keywords);
    add_string_array(
      parser,
      "app.pagecat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->app_pagecat, value);
      });
    add_string_array(
      parser,
      "app.sectioncat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->app_sectioncat, value);
      });
    add_string_array(
      parser,
      "app.cat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->app_cat, value);
      });
    add_object_processor(
      parser,
      "app.content",
      [](FastOpenRtbState& state)
      {
        state.context->app_content = true;
      });
    add_string(
      parser,
      "app.content.keywords",
      root_context,
      &JsonProcessingContext::content_keywords);
    add_string(parser, "app.content.title", root_context, &JsonProcessingContext::content_title);
    add_string(parser, "app.content.series", root_context, &JsonProcessingContext::content_series);
    add_string(parser, "app.content.season", root_context, &JsonProcessingContext::content_season);
    add_string_array(
      parser,
      "app.content.cat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->content_cat, value);
      });
    add_object_processor(
      parser,
      "app.content.producer",
      [](FastOpenRtbState& state) { state.context->app_content_producer = true; }
    );
    add_string_array(
      parser,
      "app.content.producer.name",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->content_producer_name, value);
      }
    );
    add_object_processor(
      parser,
      "app.publisher",
      [](FastOpenRtbState& state) { state.context->app_publisher = true; }
    );
    add_string(parser, "app.publisher.name", root_context, &JsonProcessingContext::publisher_name);
    add_request_string_handler(
      parser,
      "app.publisher.id",
      [](FastOpenRtbState& state, std::string_view value)
      {
        state.request_info->bid_publisher_id = value;
      });
    add_string_array(
      parser,
      "app.publisher.cat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->publisher_cat, value);
      });

    add_request_string_handler(
      parser,
      "device.ip",
      [](FastOpenRtbState& state, std::string_view value)
      {
        state.request_info->peer_ip = value;
      });
    add_string(parser, "device.ipv6", root_context, &JsonProcessingContext::ipv6);
    add_request_string_handler(
      parser,
      "device.ua",
      [](FastOpenRtbState& state, std::string_view value)
      {
        state.request_info->user_agent = value;
      });
    add_string(parser, "device.userdata", root_context, &JsonProcessingContext::user_id);
    add_string(parser, "device.ifa", root_context, &JsonProcessingContext::ifa);
    add_string(parser, "device.didmd5", root_context, &JsonProcessingContext::didmd5);
    add_string(parser, "device.didsha1", root_context, &JsonProcessingContext::didsha1);
    add_string(parser, "device.dpidmd5", root_context, &JsonProcessingContext::dpidmd5);
    add_string(parser, "device.dpisha1", root_context, &JsonProcessingContext::dpisha1);
    add_string(parser, "device.macsha1", root_context, &JsonProcessingContext::macsha1);
    add_string(parser, "device.macmd5", root_context, &JsonProcessingContext::macmd5);
    add_string(parser, "device.language", root_context, &JsonProcessingContext::language);
    add_string(parser, "device.carrier", root_context, &JsonProcessingContext::carrier);
    add_integer_handler(
      parser,
      "device.devicetype",
      [this](FastOpenRtbState& state, int64_t value)
      {
        if(value >= 0 &&
          static_cast<uint64_t>(value) <=
            static_cast<uint64_t>(std::numeric_limits<unsigned int>::max()))
        {
          state.request_info->ssp_devicetype_str =
            openrtb_devicetype_to_string_(static_cast<unsigned int>(value));
        }
      });
    add_string(parser, "device.geo.country", root_context, &JsonProcessingContext::ssp_country);
    add_string(parser, "device.geo.region", root_context, &JsonProcessingContext::ssp_region);
    add_string(parser, "device.geo.city", root_context, &JsonProcessingContext::ssp_city);
    add_request_string_handler(
      parser,
      "device.geo.lat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        AdServer::CampaignSvcs::CoordDecimal latitude;
        if(try_parse_decimal(latitude, value, Generics::DMR_ROUND))
        {
          state.context->ssp_latitude = latitude;
        }
      });
    add_request_string_handler(
      parser,
      "device.geo.lon",
      [](FastOpenRtbState& state, std::string_view value)
      {
        AdServer::CampaignSvcs::CoordDecimal longitude;
        if(try_parse_decimal(longitude, value, Generics::DMR_ROUND))
        {
          state.context->ssp_longitude = longitude;
        }
      });
    add_request_string_handler(
      parser,
      "device.geo.accuracy",
      [](FastOpenRtbState& state, std::string_view value)
      {
        AdServer::CampaignSvcs::AccuracyDecimal accuracy;
        if(try_parse_decimal(accuracy, value, Generics::DMR_ROUND))
        {
          state.context->ssp_accuracy = accuracy;
        }
      });
    add_integer_handler(
      parser,
      "device.geo.type",
      [](FastOpenRtbState& state, int64_t value)
      {
        if(value >= 0)
        {
          state.request_info->ssp_geo_type = integer_to_string(state, value);
        }
      });

    add_object_processor(
      parser,
      "user",
      [](FastOpenRtbState& state)
      {
        state.context->user = true;
      });
    add_string(parser, "user.id", root_context, &JsonProcessingContext::external_user_id);
    add_string(parser, "user.buyeruid", root_context, &JsonProcessingContext::user_id);
    add_string(parser, "user.buyerid", root_context, &JsonProcessingContext::user_id);
    add_string(parser, "user.keywords", root_context, &JsonProcessingContext::user_keywords);
    add_string(parser, "user.gender", root_context, &JsonProcessingContext::user_gender);
    add_integer(parser, "user.yob", root_context, &JsonProcessingContext::user_yob);
    add_object_processor(
      parser,
      "user.data.segment",
      start_segment,
      [](FastOpenRtbState&) {});
    add_string(
      parser,
      "user.data.segment.id",
      segment_context,
      &JsonProcessingContext::Segment::id);
    add_string(
      parser,
      "user.data.segment.name",
      segment_context,
      &JsonProcessingContext::Segment::name);
    add_string(
      parser,
      "user.data.segment.value",
      segment_context,
      &JsonProcessingContext::Segment::value);
    add_object_processor(
      parser,
      "user.ext.eids",
      start_user_eid,
      [](FastOpenRtbState&) {});
    add_string(
      parser,
      "user.ext.eids.source",
      user_eid_context,
      &JsonProcessingContext::UserEid::source);
    add_object_processor(
      parser,
      "user.ext.eids.uids",
      start_user_eid_uid,
      [](FastOpenRtbState&) {});
    add_string(
      parser,
      "user.ext.eids.uids.id",
      user_eid_uid_context,
      &JsonProcessingContext::UserEidUid::id);
    add_string(
      parser,
      "user.ext.eids.uids.stableid",
      user_eid_uid_context,
      &JsonProcessingContext::UserEidUid::stable_id);

    add_bool(parser, "regs.coppa", root_context, &JsonProcessingContext::regs_coppa);

    fast_json_parser_ = std::make_unique<FastJsonParser>(std::move(parser), false);
  }

  void
  RequestInfoFiller::parse_openrtb_request_(
    RequestInfo& request_info,
    JsonProcessingContext& context,
    std::string_view bid_request) const
  {
    static const char* FUN = "RequestInfoFiller::parse_openrtb_request_()";

    FastOpenRtbState state;
    state.request_info = &request_info;
    state.context = &context;

    try
    {
      fast_json_parser_->parse(
        bid_request,
        &state,
        [&request_info]()
        {
          return Generics::MonoString(request_info.resource());
        });
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": parsing error: " << e.what();
      throw InvalidParamException(ostr);
    }

    for(auto& ad_slot : context.ad_slots)
    {
      ad_slot.metrics.remove_if(
        [](const AdSlotContext::Metric& metric)
        {
          return metric.type.empty() || metric.value.empty();
        });
    }
  }
}
