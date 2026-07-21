#pragma once

#include <functional>
#include <list>
#include <new>
#include <optional>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <Generics/GnuHashTable.hpp>
#include <HTTP/UrlAddress.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>

#include <Commons/Containers.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include <Commons/MonoAllocator.hpp>

namespace AdServer::Bidding
{
  using StringArray = AdServer::Commons::MonoVector<std::string_view>;
  using ULongSet = AdServer::Commons::MonoSet<unsigned long>;

  struct TransparentStringLess
  {
    using is_transparent = void;

    template<typename Left, typename Right>
    bool
    operator()(const Left& left, const Right& right) const noexcept
    {
      return std::string_view(left.data(), left.size()) <
        std::string_view(right.data(), right.size());
    }
  };

  struct JsonAdSlotProcessingContext
  {
    using StringSet = AdServer::Commons::MonoSet<std::string_view, TransparentStringLess>;
    using LValueStateHolder = Commons::ValueStateHolder<long>;
    using ULValueStateHolder = Commons::ValueStateHolder<unsigned long>;
    using ULSetStateHolder = Commons::ValueStateHolder<ULongSet>;

    struct BannerFormat
    {
      explicit BannerFormat(
        AdServer::Commons::MonoAllocatorArena* resource)
        : resource_(resource),
          width(),
          height(),
          ext_type(),
          ext_format()
      {}

      BannerFormat(BannerFormat&&) noexcept = default;
      BannerFormat& operator=(BannerFormat&&) noexcept = default;

      AdServer::Commons::MonoAllocatorArena*
      resource() const noexcept
      {
        return resource_;
      }

      AdServer::Commons::MonoAllocatorArena* resource_;
      std::string_view width;
      std::string_view height;
      std::string_view ext_type;
      std::string_view ext_format;
    };

    typedef AdServer::Commons::MonoVector<BannerFormat> BannerFormatArray;

    struct Banner
    {
      explicit Banner(
        AdServer::Commons::MonoAllocatorArena* resource)
        : resource_(resource),
          formats(resource),
          default_format(resource),
          pos("0"),
          matching_ad(),
          exclude_categories(resource),
          ext_hpos(CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM)
      {}

      Banner(Banner&&) noexcept = default;
      Banner& operator=(Banner&&) noexcept = default;

      AdServer::Commons::MonoAllocatorArena*
      resource() const noexcept
      {
        return resource_;
      }

      AdServer::Commons::MonoAllocatorArena* resource_;
      BannerFormatArray formats;
      BannerFormat default_format;

      std::string_view pos;
      std::string_view matching_ad;
      StringArray exclude_categories;

      unsigned long ext_hpos;
    };

    typedef AdServer::Commons::MonoVector<Banner> BannerArray;

    struct BannerFormatHolder
    {
      BannerFormatHolder()
        : banner(nullptr)
      {}

      BannerFormatHolder(
        const Banner* banner_val,
        const BannerFormat* banner_format_val)
        : banner(banner_val),
          banner_format(banner_format_val)
      {}

      const Banner* banner;
      const BannerFormat* banner_format;
    };

    using SizeBannerMap = AdServer::Commons::MonoMap<
      std::string_view,
      BannerFormatHolder,
      TransparentStringLess>;

    struct Deal
    {
      explicit Deal(
        AdServer::Commons::MonoAllocatorArena* resource) :
        resource_(resource),
        id(),
        cpm_price(AdServer::CampaignSvcs::RevenueDecimal::ZERO),
        currency_code("USD")
      {}

      Deal(Deal&&) noexcept = default;
      Deal& operator=(Deal&&) noexcept = default;

      AdServer::Commons::MonoAllocatorArena*
      resource() const noexcept
      {
        return resource_;
      }

      AdServer::Commons::MonoAllocatorArena* resource_;
      std::string_view id;
      AdServer::CampaignSvcs::RevenueDecimal cpm_price;
      std::string_view currency_code;
    };

    struct Metric
    {
      explicit Metric(
        AdServer::Commons::MonoAllocatorArena* resource)
        : resource_(resource),
          type(),
          value()
      {}

      Metric(Metric&&) noexcept = default;
      Metric& operator=(Metric&&) noexcept = default;

      AdServer::Commons::MonoAllocatorArena*
      resource() const noexcept
      {
        return resource_;
      }

      AdServer::Commons::MonoAllocatorArena* resource_;
      std::string_view type;
      std::string_view value;
    };

    typedef AdServer::Commons::MonoList<Deal> DealList;
    typedef AdServer::Commons::MonoList<Metric> MetricList;

    struct Native: public ReferenceCounting::DefaultImpl<>
    {

      struct Asset
      {
        explicit Asset(
          AdServer::Commons::MonoAllocatorArena* /*resource*/)
          : id(0),
            required(false)
        {}

        virtual ~Asset() noexcept = default;

        long id;
        bool required;
      };

      enum DataTypeEnum
      {
        NDTE_TITLE = 0,
        NDTE_SPONSORED = 1,
        NDTE_DESC = 2,
        NDTE_RATING = 3,
        NDTE_LIKES = 4,
        NDTE_DOWNLOADS = 5,
        NDTE_PRICE = 6,
        NDTE_SALEPRICE = 7,
        NDTE_PHONE = 8,
        NDTE_ADDRESS = 9,
        NDTE_DESC2 = 10,
        NDTE_DISPLAY_URL = 11,
        NDTE_CTA_TEXT = 12
      };

      struct Data: Asset
      {
        explicit Data(
          AdServer::Commons::MonoAllocatorArena* resource)
          : Asset(resource),
            data_type(NDTE_TITLE),
            len(0)
        {}

        long data_type;
        long len;

        virtual ~Data() noexcept = default;
      };

      typedef AdServer::Commons::MonoVector<Data> DataArray;

      enum ImageTypeEnum
      {
        NITE_ICON = 1,
        NITE_LOGO = 2,
        NITE_MAIN = 3
      };

      struct Image : Asset
      {
        explicit Image(
          AdServer::Commons::MonoAllocatorArena* resource) :
          Asset(resource),
          image_type(NITE_MAIN),
          height(0),
          width(0),
          min_height(0),
          min_width(0),
          mimes(resource)
        {}

        bool is_main() const
        {
          return image_type == NITE_MAIN;
        }

        virtual ~Image() noexcept = default;

        long image_type;

        long height;
        long width;

        long min_height;
        long min_width;

        StringSet mimes;
      };

      typedef AdServer::Commons::MonoVector<Image> ImageArray;

      struct Video: Asset
      {
        explicit Video(
          AdServer::Commons::MonoAllocatorArena* resource)
          : Asset(resource),
            protocols(
              ULSetStateHolder::S_NOT_INITED,
              std::less<unsigned long>(),
              resource),
            mimes(resource)
        {}

        ULValueStateHolder min_duration;
        ULValueStateHolder max_duration;
        ULSetStateHolder protocols;
        StringSet mimes;

        virtual ~Video() noexcept = default;
      };

      explicit Native(
        AdServer::Commons::MonoAllocatorArena* resource)
        : resource_(resource),
          version("1.1"),
          data_assets(resource),
          image_assets(resource),
          video_assets(resource)
      {}

      AdServer::Commons::MonoAllocatorArena*
      resource() const noexcept
      {
        return resource_;
      }

      AdServer::Commons::MonoAllocatorArena* resource_;
      std::string_view version;
      std::optional<long> placement;
      DataArray data_assets;
      ImageArray image_assets;
      AdServer::Commons::MonoVector<Video> video_assets;

    protected:
      virtual ~Native() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Native> Native_var;


    explicit JsonAdSlotProcessingContext(
      AdServer::Commons::MonoAllocatorArena* resource)
      : resource_(resource),
        id(),
        min_cpm_price(AdServer::CampaignSvcs::RevenueDecimal::ZERO),
        deal_id(),
        deals(resource),
        metrics(resource),
        tagid(),
        min_cpm_price_currency_code(),
        secure(false),
        banners(resource),
        size_banner(resource),
        video(false),
        video_pos(),
        video_mimes(resource),
        video_protocols(
          ULSetStateHolder::S_NOT_INITED,
          std::less<unsigned long>(),
          resource),
        video_playbackmethods(
          ULSetStateHolder::S_NOT_INITED,
          std::less<unsigned long>(),
          resource),
        video_api(
          ULSetStateHolder::S_NOT_INITED,
          std::less<unsigned long>(),
          resource),
        video_exclude_categories(resource),
        imp_ext_type()
    {}

    JsonAdSlotProcessingContext(JsonAdSlotProcessingContext&&) noexcept = default;
    JsonAdSlotProcessingContext& operator=(JsonAdSlotProcessingContext&&) noexcept = default;

    AdServer::Commons::MonoAllocatorArena*
    resource() const noexcept
    {
      return resource_;
    }

    AdServer::Commons::MonoAllocatorArena* resource_;

    std::string_view id;
    AdServer::CampaignSvcs::RevenueDecimal min_cpm_price;
    std::optional<long> private_auction;
    std::string_view deal_id;
    DealList deals;
    MetricList metrics;
    std::string_view tagid;
    std::string_view min_cpm_price_currency_code;

    bool secure;
    BannerArray banners;
    SizeBannerMap size_banner;

    bool video;
    ULValueStateHolder video_width;
    ULValueStateHolder video_height;
    std::string_view video_pos;
    StringSet video_mimes;
    ULValueStateHolder video_min_duration;
    ULValueStateHolder video_max_duration;

    ULSetStateHolder video_protocols;
    ULSetStateHolder video_playbackmethods;
    ULSetStateHolder video_api;

    LValueStateHolder video_start_delay;
    ULValueStateHolder video_linearity;
    ULValueStateHolder video_adtype;

    ULValueStateHolder video_skip;
    ULValueStateHolder video_reward;


    StringArray video_exclude_categories;

    // v 2.5 video .placement
    std::optional<long> video_placement;

    Native_var native;

    std::string_view imp_ext_type;
  };

  struct JsonProcessingContext
  {
    struct Segment
    {
      explicit Segment(
        AdServer::Commons::MonoAllocatorArena* resource)
        : resource_(resource),
          id(),
          name(),
          value()
      {}

      Segment(Segment&&) noexcept = default;
      Segment& operator=(Segment&&) noexcept = default;

      AdServer::Commons::MonoAllocatorArena*
      resource() const noexcept
      {
        return resource_;
      }

      AdServer::Commons::MonoAllocatorArena* resource_;
      std::string_view id;
      std::string_view name;
      std::string_view value;
    };

    typedef AdServer::Commons::MonoList<Segment> SegmentList;

    struct UserEidUid
    {
      explicit UserEidUid(
        AdServer::Commons::MonoAllocatorArena* resource)
        : resource_(resource),
          id(),
          stable_id()
      {}

      UserEidUid(UserEidUid&&) noexcept = default;
      UserEidUid& operator=(UserEidUid&&) noexcept = default;

      AdServer::Commons::MonoAllocatorArena*
      resource() const noexcept
      {
        return resource_;
      }

      AdServer::Commons::MonoAllocatorArena* resource_;
      std::string_view id;
      std::string_view stable_id;
    };

    typedef AdServer::Commons::MonoVector<UserEidUid> UserEidUidArray;

    struct UserEid
    {
      explicit UserEid(
        AdServer::Commons::MonoAllocatorArena* resource)
        : resource_(resource),
          source(),
          uids(resource)
      {}

      UserEid(UserEid&&) noexcept = default;
      UserEid& operator=(UserEid&&) noexcept = default;

      AdServer::Commons::MonoAllocatorArena*
      resource() const noexcept
      {
        return resource_;
      }

      AdServer::Commons::MonoAllocatorArena* resource_;
      std::string_view source;
      UserEidUidArray uids;
    };

    typedef AdServer::Commons::MonoList<UserEid> UserEidList;

    explicit JsonProcessingContext(
      AdServer::Commons::MonoAllocatorArena& resource)
      : resource_(&resource),
        external_user_id(),
        user_id(),
        user_eids(resource_),
        ipv6(),
        ifa(),
        didmd5(),
        didsha1(),
        dpidmd5(),
        dpisha1(),
        macsha1(),
        macmd5(),
        language(),
        carrier(),
        currencies(resource_),
        required_category(),
        exclude_categories(resource_),
        gender(),
        age(),
        segments(resource_),
        ad_slots(resource_),
        site(false),
        site_name(),
        site_pagecat(resource_),
        site_sectioncat(resource_),
        site_cat(resource_),
        site_keywords(),
        site_search(),
        app(false),
        app_id(),
        app_name(),
        app_bundle(),
        app_pagecat(resource_),
        app_sectioncat(resource_),
        app_cat(resource_),
        app_keywords(),
        secure(false),
        test(false),
        user(false),
        user_keywords(),
        user_yob(0),
        user_gender(),
        site_content(false),
        app_content(false),
        content_keywords(),
        content_title(),
        content_series(),
        content_season(),
        content_cat(resource_),
        app_publisher(false),
        site_publisher(false),
        publisher_cat(resource_),
        publisher_name(),
        app_content_producer(false),
        site_content_producer(false),
        content_producer_name(resource_),
        allyessitetype(),
        puid1(),
        puid2(),
        regs_coppa(false),
        ssp_country(),
        ssp_region(),
        ssp_city(),
        string_holders_(resource_)
    {}

    JsonProcessingContext(const JsonProcessingContext&) = delete;
    JsonProcessingContext& operator=(const JsonProcessingContext&) = delete;
    JsonProcessingContext(JsonProcessingContext&&) noexcept = default;
    JsonProcessingContext& operator=(JsonProcessingContext&&) = delete;

    void
    clear()
    {
      AdServer::Commons::MonoAllocatorArena* resource = resource_;
      this->~JsonProcessingContext();
      new(this) JsonProcessingContext(*resource);
    }

    AdServer::Commons::MonoAllocatorArena*
    resource() const noexcept
    {
      return resource_;
    }

    std::string_view
    hold_string(std::string&& value)
    {
      AdServer::Commons::MonoString& held_value =
        string_holders_.emplace_back(resource_);
      held_value.assign(value.data(), value.size());
      return std::string_view(held_value.data(), held_value.size());
    }

    std::string_view
    hold_string(AdServer::Commons::MonoString&& value)
    {
      AdServer::Commons::MonoString& held_value = string_holders_.emplace_back(std::move(value));
      return std::string_view(held_value.data(), held_value.size());
    }

    AdServer::Commons::MonoAllocatorArena* resource_;

    std::string_view external_user_id;
    std::string_view user_id;
    UserEidList user_eids;

    // Device object
    std::string_view ipv6;
    std::string_view ifa;
    std::string_view didmd5;
    std::string_view didsha1;
    std::string_view dpidmd5;
    std::string_view dpisha1;
    std::string_view macsha1;
    std::string_view macmd5;
    std::string_view language;
    std::string_view carrier;

    AdServer::Commons::MonoVector<std::string_view> currencies;
    std::string_view required_category;
    AdServer::Commons::MonoVector<std::string_view> exclude_categories;

    std::string_view gender;
    std::string_view age;
    SegmentList segments;

    std::set<unsigned long> member_ids;
    AdServer::Commons::MonoList<JsonAdSlotProcessingContext> ad_slots;

    bool site;
    std::string_view site_name;
    HTTP::HTTPAddress site_page;
    HTTP::HTTPAddress site_domain;
    AdServer::Commons::MonoVector<std::string_view> site_pagecat;
    AdServer::Commons::MonoVector<std::string_view> site_sectioncat;
    AdServer::Commons::MonoVector<std::string_view> site_cat;
    std::string_view site_keywords;
    std::string_view site_search;
    HTTP::HTTPAddress site_ref;
    HTTP::HTTPAddress site_referer;
    HTTP::HTTPAddress site_rereferer;

    bool app;
    std::string_view app_id;
    std::string_view app_name;
    std::string_view app_bundle;
    HTTP::HTTPAddress app_domain;
    HTTP::HTTPAddress app_store_url;
    AdServer::Commons::MonoVector<std::string_view> app_pagecat;
    AdServer::Commons::MonoVector<std::string_view> app_sectioncat;
    AdServer::Commons::MonoVector<std::string_view> app_cat;
    std::string_view app_keywords;

    bool secure;
    bool test;

    bool user;
    std::string_view user_keywords;
    unsigned long user_yob;
    std::string_view user_gender;

    // Only one of app or site object can present in bid request for OpenRtb.
    //   content and publisher presents in content and app objects.
    //   so does not need separate (app|site)_content_* variables
    //   same for publisher.

    // content
    bool site_content;
    bool app_content;
    std::string_view content_keywords;
    std::string_view content_title;
    std::string_view content_series;
    std::string_view content_season;
    AdServer::Commons::MonoVector<std::string_view> content_cat;

    // publisher from site or app
    bool app_publisher;
    bool site_publisher;
    AdServer::Commons::MonoVector<std::string_view> publisher_cat;
    std::string_view publisher_name;

    bool app_content_producer;
    bool site_content_producer;
    // collect here all names from all producer objects
    AdServer::Commons::MonoVector<std::string_view> content_producer_name;

    std::string_view allyessitetype; //ALLYES specific in site object

    // ext
    std::string_view puid1;
    std::string_view puid2;

    bool regs_coppa;

    std::string_view ssp_country;
    std::string_view ssp_region;
    std::string_view ssp_city;

    template <typename ContType>
    void
    print(std::ostream& out, const ContType& sl, const char* sl_name) const
    {
      out << sl_name << "[";
      for(typename ContType::const_iterator iter = sl.begin(); iter != sl.end(); ++iter)
      {
        out << *iter << ",";
      }
      out << "]" << std::endl;
    }

    void
    print(std::ostream& out) const
    {
      out << "ipv6 = " << ipv6 << std::endl <<
        "site_page = " << site_page.url() << std::endl <<
        "site_name = " << site_name << std::endl <<
        "site_domain = " << site_domain.url() << std::endl <<
        "user_yob = " << user_yob << std::endl <<
        "user_gender = " << user_gender << std::endl <<
        "user_keywords = " << user_keywords << std::endl <<
        "site_search = " << site_search << std::endl <<
        "site_ref = " << site_ref.url() << std::endl <<
        "site_referer = " << site_referer.url() << std::endl <<
        "site_rereferer = " << site_rereferer.url() << std::endl <<
        "site_keywords = " << site_keywords << std::endl <<
        "app_keywords = " << app_keywords << std::endl <<
        "content_keywords = " << content_keywords << std::endl <<
        "content_title = " << content_title << std::endl <<
        "content_series = " << content_series << std::endl <<
        "content_season = " << content_season << std::endl <<
        "publisher_name = " << publisher_name << std::endl <<
        "allyessitetype = " << allyessitetype << std::endl <<
        "user = " << user << std::endl;

      print(out, content_producer_name, "content_producer_names");
      print(out, publisher_cat, "publisher_cat");
      print(out, content_cat, "content_cat");
      print(out, app_pagecat, "app_pagecat");
      print(out, app_sectioncat, "app_sectioncat");
      print(out, app_cat, "app_cat");
      print(out, site_pagecat, "site_pagecat");
      print(out, site_sectioncat, "site_sectioncat");
      print(out, site_cat, "site_cat");

      for(AdServer::Commons::MonoList<JsonAdSlotProcessingContext>::const_iterator slot_it =
            ad_slots.begin();
          slot_it != ad_slots.end(); ++slot_it)
      {
        out << "{ id = " << slot_it->id <<
          ": banners = [";

        for(auto banner_it = slot_it->banners.begin();
            banner_it != slot_it->banners.end(); ++banner_it)
        {
          out << "{ formats = [";
          for(auto format_it = banner_it->formats.begin();
              format_it != banner_it->formats.end(); ++format_it)
          {
            out << "{ width = " << format_it->width <<
              ", height = " << format_it->height <<
              " }";
          }
          out << "], pos = " << banner_it->pos <<
            ", hpos = " << banner_it->ext_hpos <<
            "}";
        }

        out << "], min_cpm_price = " << slot_it->min_cpm_price;
        if(slot_it->private_auction)
        {
          out << "private_auction = " << *(slot_it->private_auction) << ",";
        }

        // Print deals

        out << " deals = [";

        for(JsonAdSlotProcessingContext::DealList::const_iterator deal_it =
              slot_it->deals.begin();
            deal_it != slot_it->deals.end(); ++deal_it)
        {
          out << "{ id = " << deal_it->id <<
            ", cpm_price = " << deal_it->cpm_price <<
            ", currency_code = " << deal_it->currency_code << " }";
        }

        out << "] }";
      }
    }

  private:
    AdServer::Commons::MonoList<AdServer::Commons::MonoString> string_holders_;
  };

  inline std::string
  decode_json_url_if_possible(
    std::string_view url)
    noexcept
  {
    const String::AsciiStringManip::Caseless MIME_PREFIX("%3a%2f%2f");
    const String::AsciiStringManip::Caseless JS_PREFIX(
      "\\u00253a\\u00252f\\u00252f");
    const size_t MIN_LEN =
      HTTP::HTTP_BEGIN.str.size() + MIME_PREFIX.str.size();
    if(url.size() > MIN_LEN)
    {
      size_t offset = HTTP::HTTP_SCHEME.str.size();
      const String::SubString http_scheme(url.data(), offset);
      if(http_scheme == HTTP::HTTP_SCHEME)
      {
        if(url[offset] == 's' || url[offset] == 'S')
        {
          ++offset;
        }

        if(url.size() > offset + MIME_PREFIX.str.size())
        {
          if(String::SubString(url.data() + offset, MIME_PREFIX.str.size()) ==
            MIME_PREFIX)
          {
            try
            {
              std::string res;
              String::StringManip::mime_url_decode(
                String::SubString(url.data(), url.size()),
                res);
              return res;
            }
            catch(const eh::Exception&)
            {}
          }
          else if(url.size() > offset + JS_PREFIX.str.size() &&
            String::SubString(url.data() + offset, JS_PREFIX.str.size()) ==
            JS_PREFIX)
          {
            try
            {
              std::string res;
              String::StringManip::js_unicode_decode(
                String::SubString(url.data(), url.size()),
                res);
              return res;
            }
            catch(const eh::Exception&)
            {}
          }
        }
      }
    }

    return std::string(url);
  }

  inline void
  fill_json_url(
    std::string_view url_value,
    HTTP::HTTPAddress& url)
    noexcept
  {
    try
    {
      String::SubString url_sstr(url_value.data(), url_value.size());
      String::StringManip::trim(url_sstr);
      url = HTTP::BrowserAddress(decode_json_url_if_possible(
        std::string_view(url_sstr.data(), url_sstr.size())));
    }
    catch(const eh::Exception&)
    {}
  }
}
