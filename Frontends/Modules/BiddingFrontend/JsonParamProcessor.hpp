#ifndef JSONPARAMPROCESSOR_HPP
#define JSONPARAMPROCESSOR_HPP

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/GnuHashTable.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/Containers.hpp>
#include <Commons/Gason.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

namespace AdServer
{
namespace Bidding
{
  typedef std::list<std::string> StringList;
  typedef std::set<unsigned long> ULongSet;
  
  struct JsonAdSlotProcessingContext
  {
    typedef std::set<std::string> StringSet;
    typedef Commons::ValueStateHolder<unsigned long> ULValueStateHolder;
    typedef Commons::ValueStateHolder<ULongSet> ULSetStateHolder;

    struct BannerFormat: public ReferenceCounting::DefaultImpl<>
    {
      BannerFormat()
      {}

      std::string width;
      std::string height;
      std::string ext_type;
      std::string ext_format;

    protected:
      virtual ~BannerFormat() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<BannerFormat> BannerFormat_var;
    typedef std::vector<BannerFormat_var> BannerFormatArray;

    struct Banner: public ReferenceCounting::DefaultImpl<>
    {
      Banner()
        : default_format(new BannerFormat()),
          pos("0"),
          ext_hpos(CampaignSvcs::UNDEFINED_PUB_POSITION_BOTTOM)
      {}

      BannerFormatArray formats;
      BannerFormat_var default_format;

      std::string pos;
      std::string matching_ad;
      StringList exclude_categories;

      unsigned long ext_hpos;

    protected:
      virtual ~Banner() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Banner> Banner_var;
    typedef std::vector<Banner_var> BannerArray;

    struct BannerFormatHolder
    {
      BannerFormatHolder()
      {}

      BannerFormatHolder(
        Banner_var banner_val,
        BannerFormat_var banner_format_val)
        : banner(std::move(banner_val)),
          banner_format(std::move(banner_format_val))
      {}

      Banner_var banner;
      BannerFormat_var banner_format;
    };

    typedef std::map<std::string, BannerFormatHolder> SizeBannerMap;

    struct Deal
    {
      Deal() :
        cpm_price(AdServer::CampaignSvcs::RevenueDecimal::ZERO),
        currency_code("USD")
      {}
      
      std::string id;
      AdServer::CampaignSvcs::RevenueDecimal cpm_price;
      std::string currency_code;
    };

    typedef std::list<Deal> DealList;

    struct Native: public ReferenceCounting::DefaultImpl<>
    {

      struct Asset
      {
        Asset()
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
        Data()
          : Asset(),
            data_type(NDTE_TITLE),
            len(0)
        {}
        
        long data_type;
        long len;
        
        virtual ~Data() noexcept = default;
      };

      typedef std::vector<Data> DataList;
      
      enum ImageTypeEnum
      {
        NITE_ICON = 1,
        NITE_LOGO = 2,
        NITE_MAIN = 3
      };
      
      struct Image : Asset
      {
        Image() :
          Asset(),
          image_type(NITE_MAIN),
          height(0),
          width(0),
          min_height(0),
          min_width(0)
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

      typedef std::vector<Image> ImageList;

      struct Video: Asset
      {
        Video()
          : Asset()
        {}
        
        ULValueStateHolder min_duration;
        ULValueStateHolder max_duration;
        ULSetStateHolder protocols;
        StringSet mimes;
        
        virtual ~Video() noexcept = default;
      };

      typedef std::vector<Video> VideoList;
      
      Native()
        : version("1.1")
      {}

      std::string version;
      Commons::Optional<long> placement;
      DataList data_assets;
      ImageList image_assets;
      VideoList video_assets;

    protected:
      virtual ~Native() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Native> Native_var;


    JsonAdSlotProcessingContext()
      : min_cpm_price(AdServer::CampaignSvcs::RevenueDecimal::ZERO),
        secure(false),
        video(false),
        predicted_view_rate(-1)
    {}

    JsonAdSlotProcessingContext(const JsonAdSlotProcessingContext&) = default;
    JsonAdSlotProcessingContext(JsonAdSlotProcessingContext&&) noexcept = default ;

    JsonAdSlotProcessingContext& operator=(const JsonAdSlotProcessingContext&) = default;
    JsonAdSlotProcessingContext& operator=(JsonAdSlotProcessingContext&&) = default;

    std::string id;
    AdServer::CampaignSvcs::RevenueDecimal min_cpm_price;
    Commons::Optional<long> private_auction;
    std::string deal_id;
    DealList deals;
    std::string min_cpm_price_currency_code;

    bool secure;
    std::string appnexus_pos;

    StringList appnexus_sizes;
    BannerArray banners;
    SizeBannerMap size_banner;

    bool video;
    ULValueStateHolder video_width;
    ULValueStateHolder video_height;
    std::string video_pos;
    StringSet video_mimes;
    ULValueStateHolder video_min_duration;
    ULValueStateHolder video_max_duration;

    ULSetStateHolder video_protocols;
    ULSetStateHolder video_playbackmethods;
    ULSetStateHolder video_api;

    ULValueStateHolder video_start_delay;
    ULValueStateHolder video_linearity;
    ULValueStateHolder video_adtype;


    StringList video_exclude_categories;

    // v 2.5 video .placement
    Commons::Optional<long> video_placement;

    // AppNexus specific
    Commons::Optional<long> slot_id;
    Commons::Optional<long> site_id;
    Commons::Optional<long> inventory_source_id;
    ULongSet allowed_media_types;
    ULongSet allowed_media_subtypes;
    double predicted_view_rate;

    // CompositeProcessor->add_processor require ContextType 
    //   in added object as in parent object.
    //   So inventory_audits_content_categories required in both
    //   JsonProcessingContext and JsonAdSlotProcessingContext
    ULongSet inventory_audits_content_categories;

    Native_var native;

    std::string imp_ext_type;
  };

  typedef std::list<JsonAdSlotProcessingContext>
    JsonAdSlotProcessingContextList;

  struct JsonProcessingContext
  {
    struct Segment
    {
      std::string id;
      std::string name;
      std::string value;
    };

    typedef std::list<Segment> SegmentList;

    struct UserEidUid
    {
      std::string id;
      std::string stable_id;
    };

    typedef std::vector<UserEidUid> UserEidUidArray;

    struct UserEid
    {
      std::string source;
      UserEidUidArray uids;
    };

    typedef std::list<UserEid> UserEidList;

    JsonProcessingContext()
      : ssp_devicetype(0),
        site(false),
        app(false),
        secure(false),
        test(false),
        user(false),
        user_yob(0),
        site_content(false),
        app_content(false),
        app_publisher(false),
        site_publisher(false),
        app_content_producer(false),
        site_content_producer(false),
        no_flash(false),
        regs_coppa(false)
    {}

    std::string external_user_id;
    std::string user_id;
    UserEidList user_eids;

    // Device object
    std::string ip;
    std::string ipv6;
    std::string user_agent;
    std::string ifa;
    std::string didmd5;
    std::string didsha1;
    std::string dpidmd5;
    std::string dpisha1;
    std::string macsha1;
    std::string macmd5;
    std::string language;
    std::string carrier;
    unsigned int ssp_devicetype;

    std::string request_id;
    StringList currencies;
    std::string required_category;
    StringList exclude_categories;

    std::string gender;
    std::string age;
    SegmentList segments;

    std::set<unsigned long> member_ids;
    JsonAdSlotProcessingContextList ad_slots;

    bool site;
    std::string site_id;
    std::string site_name;
    HTTP::HTTPAddress site_page;
    HTTP::HTTPAddress site_domain;
    StringList site_pagecat;
    StringList site_sectioncat;
    StringList site_cat;
    std::string site_keywords;
    std::string site_search;
    HTTP::HTTPAddress site_ref;
    HTTP::HTTPAddress site_referer;
    HTTP::HTTPAddress site_rereferer;

    bool app;
    std::string app_id;
    std::string app_name;
    std::string app_bundle;
    HTTP::HTTPAddress app_domain;
    HTTP::HTTPAddress app_store_url;
    StringList app_pagecat;
    StringList app_sectioncat;
    StringList app_cat;
    std::string app_keywords;

    bool secure;
    bool test;

    bool user;
    std::string user_keywords;
    unsigned long user_yob;
    std::string user_gender;

    // Only one of app or site object can present in bid request for OpenRtb.
    //   content and publisher presents in content and app objects.
    //   so does not need separate (app|site)_content_* variables
    //   same for publisher.

    // content
    bool site_content;
    bool app_content;
    std::string content_keywords;
    std::string content_title;
    std::string content_series;
    std::string content_season;
    StringList content_cat;

    // publisher from site or app
    bool app_publisher;
    bool site_publisher;
    StringList publisher_cat;
    std::string publisher_name;
    std::string publisher_id;

    bool app_content_producer;
    bool site_content_producer;
    // collect here all names from all producer objects
    StringList content_producer_name;

    std::string allyessitetype; //ALLYES specific in site object

    // Appnexus specific
    Commons::Optional<long> selling_member_id;
    bool no_flash;
    ULongSet inventory_audits_content_categories; 

    // ext
    std::string puid1;
    std::string puid2;

    bool regs_coppa;

    std::string ssp_country;
    std::string ssp_region;
    std::string ssp_city;

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
          for(auto format_it = (*banner_it)->formats.begin();
              format_it != (*banner_it)->formats.end(); ++format_it)
          {
            out << "{ width = " << (*format_it)->width <<
              ", height = " << (*format_it)->height <<
              " }";
          }
          out << "], pos = " << (*banner_it)->pos <<
            ", hpos = " << (*banner_it)->ext_hpos <<
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
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

    void
    add_processor(
      const Generics::SubStringHashAdapter& key,
      std::string ContextType::* field)
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
      AdServer::CampaignSvcs::CampaignManager::
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
    JsonContextStringParamProcessor(
      std::string ContextType::* field)
      : field_(field)
    {}

    virtual void
    process(
      AdServer::CampaignSvcs::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_NUMBER ||
         value.getTag() == JSON_TAG_STRING)
      {
        value.toString(context.*field_);
      }
    }

  protected:
    virtual ~JsonContextStringParamProcessor() noexcept
    {}

  protected:
    std::string ContextType::* field_;
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
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
      AdServer::CampaignSvcs::CampaignManager::
        RequestParams& /*request_params*/,
      ContextType& context,
      const JsonValue& value) const
    {
      if(value.getTag() == JSON_TAG_ARRAY)
      {
        for(JsonIterator it = begin(value); it != end(value); ++it)
        {
          if(it->value.getTag() == JSON_TAG_STRING ||
             it->value.getTag() == JSON_TAG_NUMBER)
          {
            (context.*field_).insert((context.*field_).end(), it->value.toString());
          }
        }
      }
      else if(value.getTag() == JSON_TAG_STRING ||
        value.getTag() == JSON_TAG_NUMBER)
      {
        (context.*field_).insert((context.*field_).end(), value.toString());
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
}

#endif /*JSONPARAMPROCESSOR_HPP*/

