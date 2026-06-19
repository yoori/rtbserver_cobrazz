#pragma once

#include <functional>
#include <list>
#include <memory_resource>
#include <new>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <Generics/GnuHashTable.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/Containers.hpp>
#include <Commons/Gason.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

namespace AdServer::Bidding
{
  typedef std::pmr::string PmrString;
  typedef std::pmr::vector<PmrString> StringArray;
  typedef std::set<unsigned long> ULongSet;

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
    typedef std::set<
      PmrString,
      TransparentStringLess,
      std::pmr::polymorphic_allocator<PmrString>>
      StringSet;
    typedef Commons::ValueStateHolder<long> LValueStateHolder;
    typedef Commons::ValueStateHolder<unsigned long> ULValueStateHolder;
    typedef Commons::ValueStateHolder<ULongSet> ULSetStateHolder;

    struct BannerFormat
    {
      explicit BannerFormat(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : resource_(resource),
          width(resource),
          height(resource),
          ext_type(resource),
          ext_format(resource)
      {}

      BannerFormat(BannerFormat&&) noexcept = default;
      BannerFormat& operator=(BannerFormat&&) noexcept = default;

      std::pmr::memory_resource*
      resource() const noexcept
      {
        return resource_;
      }

      std::pmr::memory_resource* resource_;
      PmrString width;
      PmrString height;
      PmrString ext_type;
      PmrString ext_format;
    };

    typedef std::pmr::vector<BannerFormat> BannerFormatArray;

    struct Banner
    {
      explicit Banner(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : resource_(resource),
          formats(resource),
          default_format(resource),
          pos("0", resource),
          matching_ad(resource),
          exclude_categories(resource),
          ext_hpos(CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM)
      {}

      Banner(Banner&&) noexcept = default;
      Banner& operator=(Banner&&) noexcept = default;

      std::pmr::memory_resource*
      resource() const noexcept
      {
        return resource_;
      }

      std::pmr::memory_resource* resource_;
      BannerFormatArray formats;
      BannerFormat default_format;

      PmrString pos;
      PmrString matching_ad;
      StringArray exclude_categories;

      unsigned long ext_hpos;
    };

    typedef std::pmr::vector<Banner> BannerArray;

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

    typedef std::map<
      PmrString,
      BannerFormatHolder,
      TransparentStringLess,
      std::pmr::polymorphic_allocator<
        std::pair<const PmrString, BannerFormatHolder>>>
      SizeBannerMap;

    struct Deal
    {
      explicit Deal(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource()) :
        resource_(resource),
        id(resource),
        cpm_price(AdServer::CampaignSvcs::RevenueDecimal::ZERO),
        currency_code("USD", resource)
      {}

      Deal(Deal&&) noexcept = default;
      Deal& operator=(Deal&&) noexcept = default;

      std::pmr::memory_resource*
      resource() const noexcept
      {
        return resource_;
      }

      std::pmr::memory_resource* resource_;
      PmrString id;
      AdServer::CampaignSvcs::RevenueDecimal cpm_price;
      PmrString currency_code;
    };

    struct Metric
    {
      explicit Metric(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : resource_(resource),
          type(resource),
          value(resource)
      {}

      Metric(Metric&&) noexcept = default;
      Metric& operator=(Metric&&) noexcept = default;

      std::pmr::memory_resource*
      resource() const noexcept
      {
        return resource_;
      }

      std::pmr::memory_resource* resource_;
      PmrString type;
      PmrString value;
    };

    typedef std::pmr::list<Deal> DealList;
    typedef std::pmr::list<Metric> MetricList;

    struct Native: public ReferenceCounting::DefaultImpl<>
    {

      struct Asset
      {
        explicit Asset(
          std::pmr::memory_resource* /*resource*/ = std::pmr::get_default_resource())
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
          std::pmr::memory_resource* resource = std::pmr::get_default_resource())
          : Asset(resource),
            data_type(NDTE_TITLE),
            len(0)
        {}

        long data_type;
        long len;

        virtual ~Data() noexcept = default;
      };

      typedef std::pmr::vector<Data> DataArray;

      enum ImageTypeEnum
      {
        NITE_ICON = 1,
        NITE_LOGO = 2,
        NITE_MAIN = 3
      };

      struct Image : Asset
      {
        explicit Image(
          std::pmr::memory_resource* resource = std::pmr::get_default_resource()) :
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

      typedef std::pmr::vector<Image> ImageArray;

      struct Video: Asset
      {
        explicit Video(
          std::pmr::memory_resource* resource = std::pmr::get_default_resource())
          : Asset(resource),
            mimes(resource)
        {}

        ULValueStateHolder min_duration;
        ULValueStateHolder max_duration;
        ULSetStateHolder protocols;
        StringSet mimes;

        virtual ~Video() noexcept = default;
      };

      typedef std::pmr::vector<Video> VideoArray;

      explicit Native(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : resource_(resource),
          version("1.1", resource),
          data_assets(resource),
          image_assets(resource),
          video_assets(resource)
      {}

      std::pmr::memory_resource*
      resource() const noexcept
      {
        return resource_;
      }

      std::pmr::memory_resource* resource_;
      PmrString version;
      Commons::Optional<long> placement;
      DataArray data_assets;
      ImageArray image_assets;
      VideoArray video_assets;

    protected:
      virtual ~Native() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Native> Native_var;


    explicit JsonAdSlotProcessingContext(
      std::pmr::memory_resource* resource = std::pmr::get_default_resource())
      : resource_(resource),
        id(resource),
        min_cpm_price(AdServer::CampaignSvcs::RevenueDecimal::ZERO),
        deal_id(resource),
        deals(resource),
        metrics(resource),
        tagid(resource),
        min_cpm_price_currency_code(resource),
        secure(false),
        banners(resource),
        size_banner(resource),
        video(false),
        video_pos(resource),
        video_mimes(resource),
        video_exclude_categories(resource),
        imp_ext_type(resource)
    {}

    JsonAdSlotProcessingContext(JsonAdSlotProcessingContext&&) noexcept = default;
    JsonAdSlotProcessingContext& operator=(JsonAdSlotProcessingContext&&) noexcept = default;

    std::pmr::memory_resource*
    resource() const noexcept
    {
      return resource_;
    }

    std::pmr::memory_resource* resource_;

    PmrString id;
    AdServer::CampaignSvcs::RevenueDecimal min_cpm_price;
    Commons::Optional<long> private_auction;
    PmrString deal_id;
    DealList deals;
    MetricList metrics;
    PmrString tagid;
    PmrString min_cpm_price_currency_code;

    bool secure;
    BannerArray banners;
    SizeBannerMap size_banner;

    bool video;
    ULValueStateHolder video_width;
    ULValueStateHolder video_height;
    PmrString video_pos;
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
    Commons::Optional<long> video_placement;

    Native_var native;

    PmrString imp_ext_type;
  };

  typedef std::pmr::list<JsonAdSlotProcessingContext>
    JsonAdSlotProcessingContextList;

  struct JsonProcessingContext
  {
    struct Segment
    {
      explicit Segment(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : resource_(resource),
          id(resource),
          name(resource),
          value(resource)
      {}

      Segment(Segment&&) noexcept = default;
      Segment& operator=(Segment&&) noexcept = default;

      std::pmr::memory_resource*
      resource() const noexcept
      {
        return resource_;
      }

      std::pmr::memory_resource* resource_;
      PmrString id;
      PmrString name;
      PmrString value;
    };

    typedef std::pmr::list<Segment> SegmentList;

    struct UserEidUid
    {
      explicit UserEidUid(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : resource_(resource),
          id(resource),
          stable_id(resource)
      {}

      UserEidUid(UserEidUid&&) noexcept = default;
      UserEidUid& operator=(UserEidUid&&) noexcept = default;

      std::pmr::memory_resource*
      resource() const noexcept
      {
        return resource_;
      }

      std::pmr::memory_resource* resource_;
      PmrString id;
      PmrString stable_id;
    };

    typedef std::pmr::vector<UserEidUid> UserEidUidArray;

    struct UserEid
    {
      explicit UserEid(
        std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : resource_(resource),
          source(resource),
          uids(resource)
      {}

      UserEid(UserEid&&) noexcept = default;
      UserEid& operator=(UserEid&&) noexcept = default;

      std::pmr::memory_resource*
      resource() const noexcept
      {
        return resource_;
      }

      std::pmr::memory_resource* resource_;
      PmrString source;
      UserEidUidArray uids;
    };

    typedef std::pmr::list<UserEid> UserEidList;

    JsonProcessingContext()
      : JsonProcessingContext(
          std::make_unique<std::pmr::monotonic_buffer_resource>())
    {}

      explicit JsonProcessingContext(
        std::unique_ptr<std::pmr::monotonic_buffer_resource> arena) noexcept
      : arena_(std::move(arena)),
        resource_(arena_.get()),
        external_user_id(resource_),
        user_id(resource_),
        user_eids(resource_),
        ip(resource_),
        ipv6(resource_),
        user_agent(resource_),
        ifa(resource_),
        didmd5(resource_),
        didsha1(resource_),
        dpidmd5(resource_),
        dpisha1(resource_),
        macsha1(resource_),
        macmd5(resource_),
        language(resource_),
        carrier(resource_),
        ssp_devicetype(0),
        request_id(resource_),
        currencies(resource_),
        required_category(resource_),
        exclude_categories(resource_),
        gender(resource_),
        age(resource_),
        segments(resource_),
        ad_slots(resource_),
        site(false),
        site_id(resource_),
        site_name(resource_),
        site_pagecat(resource_),
        site_sectioncat(resource_),
        site_cat(resource_),
        site_keywords(resource_),
        site_search(resource_),
        app(false),
        app_id(resource_),
        app_name(resource_),
        app_bundle(resource_),
        app_pagecat(resource_),
        app_sectioncat(resource_),
        app_cat(resource_),
        app_keywords(resource_),
        secure(false),
        test(false),
        user(false),
        user_keywords(resource_),
        user_yob(0),
        user_gender(resource_),
        site_content(false),
        app_content(false),
        content_keywords(resource_),
        content_title(resource_),
        content_series(resource_),
        content_season(resource_),
        content_cat(resource_),
        app_publisher(false),
        site_publisher(false),
        publisher_cat(resource_),
        publisher_name(resource_),
        publisher_id(resource_),
        app_content_producer(false),
        site_content_producer(false),
        content_producer_name(resource_),
        allyessitetype(resource_),
        puid1(resource_),
        puid2(resource_),
        regs_coppa(false),
        ssp_country(resource_),
        ssp_region(resource_),
        ssp_city(resource_)
    {}

    JsonProcessingContext(const JsonProcessingContext&) = delete;
    JsonProcessingContext& operator=(const JsonProcessingContext&) = delete;
    JsonProcessingContext(JsonProcessingContext&&) noexcept = default;
    JsonProcessingContext& operator=(JsonProcessingContext&&) = delete;

    void
    clear()
    {
      auto arena = std::make_unique<std::pmr::monotonic_buffer_resource>();
      this->~JsonProcessingContext();
      new(this) JsonProcessingContext(std::move(arena));
    }

    std::pmr::memory_resource*
    resource() const noexcept
    {
      return resource_;
    }

    std::unique_ptr<std::pmr::monotonic_buffer_resource> arena_;
    std::pmr::memory_resource* resource_;

    PmrString external_user_id;
    PmrString user_id;
    UserEidList user_eids;

    // Device object
    PmrString ip;
    PmrString ipv6;
    PmrString user_agent;
    PmrString ifa;
    PmrString didmd5;
    PmrString didsha1;
    PmrString dpidmd5;
    PmrString dpisha1;
    PmrString macsha1;
    PmrString macmd5;
    PmrString language;
    PmrString carrier;
    unsigned int ssp_devicetype;

    PmrString request_id;
    StringArray currencies;
    PmrString required_category;
    StringArray exclude_categories;

    PmrString gender;
    PmrString age;
    SegmentList segments;

    std::set<unsigned long> member_ids;
    JsonAdSlotProcessingContextList ad_slots;

    bool site;
    PmrString site_id;
    PmrString site_name;
    HTTP::HTTPAddress site_page;
    HTTP::HTTPAddress site_domain;
    StringArray site_pagecat;
    StringArray site_sectioncat;
    StringArray site_cat;
    PmrString site_keywords;
    PmrString site_search;
    HTTP::HTTPAddress site_ref;
    HTTP::HTTPAddress site_referer;
    HTTP::HTTPAddress site_rereferer;

    bool app;
    PmrString app_id;
    PmrString app_name;
    PmrString app_bundle;
    HTTP::HTTPAddress app_domain;
    HTTP::HTTPAddress app_store_url;
    StringArray app_pagecat;
    StringArray app_sectioncat;
    StringArray app_cat;
    PmrString app_keywords;

    bool secure;
    bool test;

    bool user;
    PmrString user_keywords;
    unsigned long user_yob;
    PmrString user_gender;

    // Only one of app or site object can present in bid request for OpenRtb.
    //   content and publisher presents in content and app objects.
    //   so does not need separate (app|site)_content_* variables
    //   same for publisher.

    // content
    bool site_content;
    bool app_content;
    PmrString content_keywords;
    PmrString content_title;
    PmrString content_series;
    PmrString content_season;
    StringArray content_cat;

    // publisher from site or app
    bool app_publisher;
    bool site_publisher;
    StringArray publisher_cat;
    PmrString publisher_name;
    PmrString publisher_id;

    bool app_content_producer;
    bool site_content_producer;
    // collect here all names from all producer objects
    StringArray content_producer_name;

    PmrString allyessitetype; //ALLYES specific in site object

    // ext
    PmrString puid1;
    PmrString puid2;

    bool regs_coppa;

    PmrString ssp_country;
    PmrString ssp_region;
    PmrString ssp_city;

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
      out << "ip = " << ip << std::endl <<
        "ipv6 = " << ipv6 << std::endl <<
        "user_agent = " << user_agent << std::endl <<
        "request_id = " << request_id << std::endl <<
        "site_page = " << site_page.url() << std::endl <<
        "site_name = " << site_name << std::endl <<
        "site_id = " << site_id << std::endl <<
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

      for(JsonAdSlotProcessingContextList::const_iterator slot_it =
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
        if (slot_it->private_auction.present())
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
  };

  template<typename ContextType>
  class JsonParamProcessor: public ReferenceCounting::AtomicImpl
  {
  public:
    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& request_params,
      ContextType& context,
      const JsonValue& value) const = 0;

  protected:
    virtual ~JsonParamProcessor() noexcept
    {}
  };

  template<typename ContextType>
  struct JsonContextStringParamProcessor;

  template<typename ContextType>
  class JsonCompositeParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    JsonCompositeParamProcessor()
      : field_installed_(false), field_(0)
    {}

    JsonCompositeParamProcessor(bool ContextType::* field)
      : field_installed_(true),
        field_(field)
    {}

    void process_value_(
      AdServer::Bidding::CampaignManager::
        RequestParams& request_params,
      ContextType& context,
      const JsonValue& value) const
    {
      for(JsonIterator it = begin(value); it != end(value); ++it)
      {
        auto processor_it =
          sub_processors_.find(String::SubString(it->key));
        if(processor_it != sub_processors_.end())
        {
          processor_it->second->process(
            request_params, context, it->value);
        }
      }
    }

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& request_params,
      ContextType& context,
      const JsonValue& value) const
    {
      if(field_installed_)
      {
        context.*field_ = true;
      }

      if(value.getTag() == JSON_TAG_OBJECT)
      {
        process_value_(request_params, context, value);
      }
      else if(value.getTag() == JSON_TAG_STRING)
      {
        // Try parse string
        JsonValue root_value;
        JsonAllocator json_allocator;
        std::string value_str;
        value.toString(value_str);
        Generics::ArrayAutoPtr<char> value_holder(value_str.size() + 1);
        char* parse_end;
        strcpy(value_holder.get(), value_str.c_str());

        JsonParseStatus status = json_parse(
          value_holder.get(), &parse_end, &root_value, json_allocator);

        if (status == JSON_PARSE_OK && root_value.getTag() == JSON_TAG_OBJECT)
        {
          process_value_(request_params, context, root_value);
        }
      }
    }

    void
    add_processor(
      const Generics::SubStringHashAdapter& key,
      JsonParamProcessor<ContextType>* processor)
    {
      sub_processors_.insert(std::make_pair(
        key,
        ReferenceCounting::add_ref(processor)));
    }

    template<typename StringType>
    void
    add_processor(
      const Generics::SubStringHashAdapter& key,
      StringType ContextType::* field)
    {
      sub_processors_.insert(std::make_pair(
        key,
        new JsonContextStringParamProcessor<ContextType>(field)));
    }

  protected:
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter,
      ReferenceCounting::SmartPtr<JsonParamProcessor<ContextType> > >
      JsonParamProcessorMap;

  protected:
    virtual ~JsonCompositeParamProcessor() noexcept
    {}

  private:
    bool field_installed_;
    bool ContextType::* field_;
    JsonParamProcessorMap sub_processors_;
  };

  template<typename ContextType>
  class JsonArrayParamProcessor:
    public JsonCompositeParamProcessor<ContextType>
  {
  public:
    JsonArrayParamProcessor(
      JsonParamProcessor<ContextType>* element_processor,
      std::size_t max_count = std::numeric_limits<std::size_t>::max())
      : element_processor_(ReferenceCounting::add_ref(element_processor)),
        max_count_(max_count)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& request_params,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        std::size_t inx = 0;

        for(JsonIterator it = begin(value);
            it != end(value) && inx < max_count_;
            ++it, ++inx)
        {
          element_processor_->process(request_params, context, it->value);
        }
      }
    }

  protected:
    virtual ~JsonArrayParamProcessor() noexcept
    {}

  protected:
    ReferenceCounting::SmartPtr<JsonParamProcessor<ContextType> >
      element_processor_;
    std::size_t max_count_;
  };

  template<typename ContextType>
  class JsonContextStringParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    template<typename StringType>
    JsonContextStringParamProcessor(
      StringType ContextType::* field)
      : setter_([field](ContextType& context, const JsonValue& value)
        {
          auto& target = context.*field;
          if constexpr (
            requires(ContextType& ctx) { ctx.resource(); } &&
            requires(StringType& str) { str.get_allocator().resource(); } &&
            std::is_constructible_v<StringType, std::pmr::memory_resource*>)
          {
            if(target.get_allocator().resource() != context.resource())
            {
              target.~StringType();
              new(&target) StringType(context.resource());
            }
          }
          value.toString(target);
        })
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_NUMBER ||
         value.getTag() == JSON_TAG_STRING)
      {
        setter_(context, value);
      }
    }

  protected:
    virtual ~JsonContextStringParamProcessor() noexcept
    {}

  protected:
    std::function<void(ContextType&, const JsonValue&)> setter_;
  };

  template<typename ContextType, typename NumberType>
  class JsonContextNumberParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    JsonContextNumberParamProcessor(
      NumberType ContextType::* field)
      : field_(field)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_NUMBER)
      {
        context.*field_ = value.toNumber();
      }
    }

  protected:
    virtual ~JsonContextNumberParamProcessor() noexcept
    {}

  protected:
    NumberType ContextType::* field_;
  };

  // Support only INT types that can be passed to String::StringManip::str_to_int
  template<typename ContextType, typename NumberStateType>
  class JsonContextNumberStateParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    JsonContextNumberStateParamProcessor(
      NumberStateType ContextType::* field)
      : field_(field)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_NUMBER)
      {
        typedef typename NumberStateType::ValueType NumberType;
        NumberType tmp;
        if (String::StringManip::str_to_int<NumberType>(
              value.toSubString(), tmp))
        {
          context.*field_ = tmp;
        }
        else
        {
          (context.*field_).set_state(NumberStateType::S_FAIL);
        }
      }
      else
      {
        (context.*field_).set_state(NumberStateType::S_FAIL);
      }
    }

  protected:
    virtual ~JsonContextNumberStateParamProcessor() noexcept
    {}

  protected:
    NumberStateType ContextType::* field_;
  };


  template<typename ContextType>
  class JsonContextBoolParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    typedef std::function<bool(bool, bool)> Operand;

    static bool get_right(bool, bool right)
    {
      return right;
    }

    JsonContextBoolParamProcessor(
      bool ContextType::* field,
      Operand operand = &JsonContextBoolParamProcessor<ContextType>::get_right)
      : field_(field), operand_(operand)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_BOOL)
      {
        context.*field_ = operand_(context.*field_, value.toBool());
      }
      else if(value.getTag() == JSON_TAG_NUMBER)
      {
        context.*field_ = operand_(context.*field_, (value.toNumber() != 0));
      }
    }

  protected:
    virtual ~JsonContextBoolParamProcessor() noexcept
    {}

  protected:
    bool ContextType::* field_;
    Operand operand_;
  };

  template<
    typename ContextType,
    typename DoubleType,
    bool PARSE_STRING_AS_NUMBER = false>
  class JsonContextDoubleParamProcessor: public JsonParamProcessor<ContextType>
  {
  public:

    JsonContextDoubleParamProcessor(
      DoubleType ContextType::* field,
      Generics::DecimalMulRemainder round_type = Generics::DMR_ROUND,
      const DoubleType& invalid_value = DoubleType())
      : field_(field),
        round_type_(round_type),
        invalid_value_(invalid_value)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      try
      {
        if(value.getTag() == JSON_TAG_STRING)
        {
          if (PARSE_STRING_AS_NUMBER)
          {
            const String::SubString str(value.toString());

            if (!str.empty())
            {
              context.*field_ =
                Commons::extract_decimal<DoubleType>(str, round_type_);
            }
          }
        }
        else if(value.getTag() == JSON_TAG_NUMBER)
        {
          context.*field_ = value.toDecimal<DoubleType>(round_type_);
        }
      }
      catch(const typename DoubleType::Exception&)
      {
        context.*field_ = invalid_value_;
      }
    }

  protected:
    virtual ~JsonContextDoubleParamProcessor() noexcept
    {}

  protected:
    DoubleType ContextType::* field_;
    Generics::DecimalMulRemainder round_type_;
    DoubleType invalid_value_;
  };


  template<typename ContextType>
  class JsonContextURLParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    JsonContextURLParamProcessor(
      HTTP::HTTPAddress ContextType::* field)
      : field_(field)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if (value.getTag() == JSON_TAG_STRING)
      {
        fill_url(
          String::SubString(value.toString()),
          context.*field_);
      }
    }

    static void
    fill_url(
      String::SubString url_sstr,
      HTTP::HTTPAddress& url) noexcept
    {
      try
      {
        // workaround: some SSP provide url with spaces at beginning
        String::StringManip::trim(url_sstr);
        url = HTTP::BrowserAddress(decode_url_if_possible_(url_sstr));
      }
      catch(eh::Exception&)
      {}
    }

    static
    std::string
    decode_url_if_possible_(
      const String::SubString& url)
      noexcept
    {
      const String::AsciiStringManip::Caseless MIME_PREFIX("%3a%2f%2f");
      const String::AsciiStringManip::Caseless JS_PREFIX("\\u00253a\\u00252f\\u00252f");
      const size_t MIN_LEN =
        HTTP::HTTP_BEGIN.str.size() + MIME_PREFIX.str.size();
      if(url.size() > MIN_LEN)
      {
        size_t offset = HTTP::HTTP_SCHEME.str.size();
        String::SubString http_scheme(url.data(), offset);
        if(http_scheme == HTTP::HTTP_SCHEME)
        {//starts from http
          if(url[offset] == 's' || url[offset] == 'S')
          {//https, skip one more symbol
            offset++;
          }
          if(url.size() > offset + MIME_PREFIX.str.size())
          {
            if(String::SubString(url.data() + offset, MIME_PREFIX.str.size()) ==
               MIME_PREFIX)
            {//assume mime encoded, decode it
              try
              {
                std::string res;
                String::StringManip::mime_url_decode(url, res);
                return res;
              }
              catch(const eh::Exception&)
              {//ignory invalid
              }
            }
            else if(url.size() > offset + JS_PREFIX.str.size() &&
                    String::SubString(url.data() + offset, JS_PREFIX.str.size()) ==
                    JS_PREFIX)
            {//assume js unicode encoded, decode it
              try
              {
                std::string res;
                String::StringManip::js_unicode_decode(url, res);
                return res;
              }
              catch(const eh::Exception&)
              {//ignory invalid
              }
            }
          }
        }
      }
      return url.str();
    }

  protected:
    virtual ~JsonContextURLParamProcessor() noexcept
    {}

  protected:
    HTTP::HTTPAddress ContextType::* field_;
  };

  template<typename ContextType, typename CollectionType>
  class JsonContextNumberArrayParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    JsonContextNumberArrayParamProcessor(
      CollectionType ContextType::* field)
      : field_(field)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          if(it->value.getTag() == JSON_TAG_NUMBER)
          {
            (context.*field_).insert(
              (context.*field_).end(),
              static_cast<typename CollectionType::value_type>(
                ::round(it->value.toNumber())));
          }
        }
      }
      else if(value.getTag() == JSON_TAG_NUMBER)
      {
        (context.*field_).insert(
          (context.*field_).end(),
          static_cast<typename CollectionType::value_type>(
            value.toNumber()));
      }
    }

  protected:
    CollectionType ContextType::* field_;

  protected:
    virtual
    ~JsonContextNumberArrayParamProcessor() noexcept
    {}
  };

  template<typename ContextType, typename CollectionStateType>
  class JsonContextNumberArrayStateParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    JsonContextNumberArrayStateParamProcessor(
      CollectionStateType ContextType::* field)
      : field_(field)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      typedef typename CollectionStateType::ValueType CollectionType;
      typedef typename CollectionType::value_type ValueType;
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          if(it->value.getTag() == JSON_TAG_NUMBER)
          {
            ValueType tmp;
            if (String::StringManip::str_to_int<ValueType>(
                  it->value.toSubString(), tmp))
            {
              (context.*field_)->insert(
                (context.*field_)->end(), tmp);
              (context.*field_).set_state(CollectionStateType::S_GOOD);
            }
            else
            {
              (context.*field_).set_state(CollectionStateType::S_FAIL);
              break;
            }
          }
          else
          {
            (context.*field_).set_state(CollectionStateType::S_FAIL);
          }
        }
      }
      else if(value.getTag() == JSON_TAG_NUMBER)
      {
        ValueType tmp;
        if (String::StringManip::str_to_int<ValueType>(
              value.toSubString(), tmp))
        {
          (context.*field_)->insert(
            (context.*field_)->end(), tmp);
          (context.*field_).set_state(CollectionStateType::S_GOOD);
        }
        else
        {
          (context.*field_).set_state(CollectionStateType::S_FAIL);
        }
      }
      else
      {
        (context.*field_).set_state(CollectionStateType::S_FAIL);
      }
    }

  protected:
    CollectionStateType ContextType::* field_;

  protected:
    virtual
    ~JsonContextNumberArrayStateParamProcessor() noexcept
    {}
  };


  template<typename ContextType, typename CollectionType>
  class JsonContextStringArrayParamProcessor:
    public JsonParamProcessor<ContextType>
  {
  public:
    JsonContextStringArrayParamProcessor(
      CollectionType ContextType::* field)
      : field_(field)
    {}

    virtual void
    process(
      AdServer::Bidding::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        auto& target = context.*field_;
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          if(it->value.getTag() == JSON_TAG_STRING ||
             it->value.getTag() == JSON_TAG_NUMBER)
          {
            insert_(target, it->value);
          }
        }
      }
      else if(value.getTag() == JSON_TAG_STRING ||
        value.getTag() == JSON_TAG_NUMBER)
      {
        insert_(context.*field_, value);
      }
    }

  protected:
    void
    insert_(CollectionType& target, const JsonValue& value) const
    {
      if constexpr (
        requires { target.get_allocator().resource(); } &&
        std::is_constructible_v<
          typename CollectionType::value_type,
          std::pmr::memory_resource*>)
      {
        typename CollectionType::value_type str(
          target.get_allocator().resource());
        value.toString(str);
        target.insert(target.end(), std::move(str));
      }
      else
      {
        target.insert(target.end(), value.toString());
      }
    }

  protected:
    CollectionType ContextType::* field_;

  protected:
    virtual
    ~JsonContextStringArrayParamProcessor() noexcept
    {}
  };
}
