#include <cassert>
#include <fstream>
#include <Generics/MMap.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/Rand.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/BitAlgs.hpp>
#include <String/RegEx.hpp>
#include <Commons/JsonParamProcessor.hpp>
#include <ProfilingCommons/PlainStorage3/FileReader.hpp>

#include "CTRProvider.hpp"
#include "CTRFeatureCalculators.hpp"

static_assert(
  std::numeric_limits<float>::is_iec559,
  "Platform must use IEC 559 (IEEE 754) for float");

static_assert(
  sizeof(float) == 4,
  "float size must be 4");

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    const unsigned long WEIGHT_NORM_SUM = 0xFFFFFFFF;

    namespace Config
    {
      const String::SubString ALGORITHMS("algorithms");
      const String::SubString DEFAULT_WEIGHT("default_weight");
      const String::SubString VERSION("version");
      const String::SubString FEATURE_MAPPING_FILE("feature_mapping_file");

      const String::SubString ALGORITHM_ID("id");
      const String::SubString ALGORITHM_WEIGHT("weight");
      const String::SubString ALGORITHM_THRESHOLD("threshold");
      const String::SubString ALGORITHM_MODELS("models");
      const String::SubString ALGORITHM_PARAMS("params");
      const String::SubString ALGORITHM_PARAMS_CAMPAIGNS_WHITELIST_FILE("campaigns_whitelist_file");
      const String::SubString ALGORITHM_PARAMS_CAMPAIGNS_BLACKLIST_FILE("campaigns_blacklist_file");

      const String::SubString ALGORITHM_MODEL_METHOD("method");
      const String::SubString ALGORITHM_MODEL_WEIGHT("weight");
      const String::SubString ALGORITHM_MODEL_FEATURES("features");
      const String::SubString ALGORITHM_MODEL_FEATURES_DIMENSION("features_dimension");
      const String::SubString ALGORITHM_MODEL_FILE("file");
    };

    const String::SubString JSON_CONFIG_FILE_NAME("config.json");

    const String::SubString CTR_CONFIG_FOLDER_NAME_REGEXP("\\d{8}\\.\\d{6}");
  };

  namespace
  {
    // trivial feature hash functions
#   define DEFINE_REQUEST_FEATURE_HASH_FUN(NAME) \
    void NAME(CTR::Murmur32v3Adapter& hash, \
      const CampaignSelectParams& request_params, \
      const Tag::Size*, \
      const Creative*)

#   define DEFINE_AUCTION_FEATURE_HASH_FUN(NAME) \
    void NAME(CTR::Murmur32v3Adapter& hash, \
      const CampaignSelectParams&, \
      const Tag::Size* tag_size, \
      const Creative*)

#   define DEFINE_CANDIDATE_FEATURE_HASH_FUN(NAME) \
    void NAME(CTR::Murmur32v3Adapter& hash, \
      const CampaignSelectParams&, \
      const Tag::Size*, \
      const Creative* creative)

#   define DEFINE_CANDIDATE_FEATURE_HASH_WITH_RP_FUN(NAME) \
    void NAME(CTR::Murmur32v3Adapter& hash, \
      const CampaignSelectParams& request_params, \
      const Tag::Size*, \
      const Creative* creative)

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_publisher_id_)
    {
      hash.add(
        static_cast<uint32_t>(request_params.tag->site->account->account_id));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_site_id_)
    {
      hash.add(
        static_cast<uint32_t>(request_params.tag->site->site_id));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_tag_id_)
    {
      hash.add(static_cast<uint32_t>(request_params.tag->tag_id));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_etag_id_)
    {
      hash.add(request_params.ext_tag_id);
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_domain_)
    {
      hash.add(request_params.referer_hostname);
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_url_)
    {
      hash.add(static_cast<uint32_t>(request_params.short_referer_hash));
    }

    DEFINE_AUCTION_FEATURE_HASH_FUN(add_hash_size_type_id_)
    {
      hash.add(static_cast<uint32_t>(tag_size->size->size_type_id));
    }

    DEFINE_AUCTION_FEATURE_HASH_FUN(add_hash_size_id_)
    {
      hash.add(static_cast<uint32_t>(tag_size->size->size_id));
    }

    DEFINE_CANDIDATE_FEATURE_HASH_FUN(add_hash_advertiser_id_)
    {
      hash.add(static_cast<uint32_t>(creative->campaign->advertiser->account_id));
    }

    DEFINE_CANDIDATE_FEATURE_HASH_FUN(add_hash_campaign_id_)
    {
      // campaign_group_id = campaign_id in OIX termines
      hash.add(static_cast<uint32_t>(creative->campaign->campaign_group_id));
    }

    DEFINE_CANDIDATE_FEATURE_HASH_FUN(add_hash_ccg_id_)
    {
      // campaign_group_id = campaign_id in OIX termines
      hash.add(static_cast<uint32_t>(creative->campaign->campaign_id));
    }

    DEFINE_CANDIDATE_FEATURE_HASH_FUN(add_hash_creative_id_)
    {
      // campaign_group_id = campaign_id in OIX termines
      hash.add(static_cast<uint32_t>(creative->creative_id));
    }

    DEFINE_CANDIDATE_FEATURE_HASH_FUN(add_hash_campaign_creative_id_)
    {
      // campaign_group_id = campaign_id in OIX termines
      hash.add(static_cast<uint32_t>(creative->ccid));
    }

    DEFINE_CANDIDATE_FEATURE_HASH_WITH_RP_FUN(add_hash_campaign_freq_id_)
    {
      uint32_t imps = 0;

      CampaignSelectParams::CampaignImpsMap::const_iterator it =
        request_params.campaign_imps.find(
          creative->campaign->campaign_group_id);
      if(it != request_params.campaign_imps.end())
      {
        imps = it->second;
      }

      hash.add(imps);
    }

    DEFINE_CANDIDATE_FEATURE_HASH_WITH_RP_FUN(add_hash_campaign_freq_log_id_)
    {
      uint32_t imps = 0;

      CampaignSelectParams::CampaignImpsMap::const_iterator it =
        request_params.campaign_imps.find(
          creative->campaign->campaign_group_id);
      if(it != request_params.campaign_imps.end())
      {
        imps = it->second;
      }

      hash.add(static_cast<uint32_t>(Generics::BitAlgs::highest_bit_32(imps + 1)));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_hour_)
    {
      hash.add(static_cast<unsigned char>(request_params.time_hour));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_week_day_)
    {
      hash.add(static_cast<unsigned char>(request_params.time_week_day));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_isp_id_)
    {
      hash.add(static_cast<uint32_t>(request_params.colocation->account->account_id));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_colo_id_)
    {
      hash.add(static_cast<uint32_t>(request_params.colocation->colo_id));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_hash_device_channel_id_)
    {
      hash.add(static_cast<uint32_t>(request_params.last_platform_channel_id));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_tag_visibility_)
    {
      hash.add(static_cast<uint32_t>(request_params.tag_visibility));
    }

    DEFINE_REQUEST_FEATURE_HASH_FUN(add_tag_predicted_viewability_)
    {
      hash.add(static_cast<uint32_t>(request_params.tag_predicted_viewability));
    }

    struct AfterHourFeatureCalculatorCreator: public CTR::FeatureCalculatorCreator
    {
    public:
      virtual CTR::FeatureCalculator_var
      create_final(const CTR::FeatureWeightTable& weight_table)
      {
        return new AfterHourFeatureCalculatorFinalImpl(weight_table);
      }

      virtual CTR::FeatureCalculator_var
      create_delegate(
        CTR::FeatureCalculator* next_calculator)
      {
        return new AfterHourFeatureCalculatorDelegateImpl(next_calculator);
      }

    protected:
      class AfterHourFeatureCalculatorFinalImpl:
        public CTR::FeatureCalculator,
        public CTR::FeatureCalculatorFinalImplHelper
      {
      public:
        AfterHourFeatureCalculatorFinalImpl(
          const CTR::FeatureWeightTable& weight_table)
          noexcept
          : FeatureCalculatorFinalImplHelper(weight_table)
        {}

        virtual float
        eval(
          CTR::Murmur32v3Adapter& hash_adapter,
          const CTR::HashMap* hash_mapping,
          const CampaignSelectParams& request_params,
          const Tag::Size*,
          const Creative*)
          noexcept
        {
          float result_weight = 0;

          for(uint32_t time_hour_i = 0; time_hour_i < request_params.time_hour; ++time_hour_i)
          {
            // need local hasher
            CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(time_hour_i);
            result_weight += weight_(hash_mapping, hash_adapter_copy.finalize());
          }

          return result_weight;
        }

        virtual void
        eval_hashes(
          CTR::HashArray& result_hashes,
          CTR::Murmur32v3Adapter& hash_adapter,
          const CTR::HashMap* hash_mapping,
          const CampaignSelectParams& request_params,
          const Tag::Size*,
          const Creative*)
          noexcept
        {
          for(uint32_t time_hour_i = 0; time_hour_i < request_params.time_hour; ++time_hour_i)
          {
            // need local hasher
            CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(time_hour_i);
            uint32_t index;
            if(hash_index_(index, hash_mapping, hash_adapter_copy.finalize()))
            {
              result_hashes.push_back(std::make_pair(index, 1));
            }
          }
        }
      };

      class AfterHourFeatureCalculatorDelegateImpl:
        public CTR::FeatureCalculator
      {
      public:
        AfterHourFeatureCalculatorDelegateImpl(
          CTR::FeatureCalculator* next_calculator)
          noexcept
          : next_calculator_(ReferenceCounting::add_ref(next_calculator))
        {}

        virtual float
        eval(
          CTR::Murmur32v3Adapter& hash_adapter,
          const CTR::HashMap* hash_mapping,
          const CampaignSelectParams& request_params,
          const Tag::Size* tag_size,
          const Creative* creative)
          noexcept
        {
          float result_weight = 0;

          for(uint32_t time_hour_i = 0; time_hour_i < request_params.time_hour; ++time_hour_i)
          {
            CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(time_hour_i);
            result_weight += next_calculator_->eval(
              hash_adapter_copy,
              hash_mapping,
              request_params,
              tag_size,
              creative);
          }

          return result_weight;
        }

        virtual void
        eval_hashes(
          CTR::HashArray& result_hashes,
          CTR::Murmur32v3Adapter& hash_adapter,
          const CTR::HashMap* hash_mapping,
          const CampaignSelectParams& request_params,
          const Tag::Size* tag_size,
          const Creative* creative)
          noexcept
        {
          for(uint32_t time_hour_i = 0; time_hour_i < request_params.time_hour; ++time_hour_i)
          {
            CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(time_hour_i);
            next_calculator_->eval_hashes(
              result_hashes,
              hash_adapter_copy,
              hash_mapping,
              request_params,
              tag_size,
              creative);
          }
        }

      protected:
        CTR::FeatureCalculator_var next_calculator_;
      };
    };

    struct FeatureDescriptor
    {
      // smart ptr ownership semantic !!!
      // don't increase ref count on passed calculator creator
      // and hash calculator
      FeatureDescriptor(
        CTR::BasicFeature feature_val,
        const char* name_val, // only literal can be passed here
        CTR::FeatureCalculatorCreator* calculator_creator_val)
        : feature(feature_val),
          name(name_val),
          calculator_creator(calculator_creator_val)
      {}

      CTR::BasicFeature feature;
      String::SubString name;
      CTR::FeatureCalculatorCreator_var calculator_creator;
    };

    class FeatureDescriptorResolver_
    {
    public:
      FeatureDescriptorResolver_()
      {
        using namespace CTR;

#       define F2C(NAME) new CTR::TrivialFeatureCalculatorCreator<NAME>()

        // FEATURE_DESCRIPTORS contains full features description
        const FeatureDescriptor FEATURE_DESCRIPTORS[] = {
          { BF_PUBLISHER_ID, "publisher", F2C(add_hash_publisher_id_) },
          { BF_SITE_ID, "site", F2C(add_hash_site_id_) },
          { BF_TAG_ID, "tag", F2C(add_hash_tag_id_) },
          { BF_ETAG_ID, "etag", F2C(add_hash_etag_id_) },
          { BF_DOMAIN , "domain", F2C(add_hash_domain_) },
          { BF_URL, "url", F2C(add_hash_url_) },
          { BF_HOUR, "hour", F2C(add_hash_hour_) },
          { BF_WEEK_DAY, "wd", F2C(add_hash_week_day_) },
          { BF_ISP_ID, "isp", F2C(add_hash_isp_id_) },
          { BF_COLO_ID, "colo", F2C(add_hash_colo_id_) },
          { BF_DEVICE_CHANNEL_ID, "device", F2C(add_hash_device_channel_id_) },
          {
            BF_AFTER_HOUR,
            "afterhour",
            new AfterHourFeatureCalculatorCreator()
          },
          { BF_VISIBILITY, "visibility", F2C(add_tag_visibility_) },
          { BF_PREDICTED_VIEWABILITY, "viewability", F2C(add_tag_predicted_viewability_) },

          { BF_SIZE_TYPE_ID, "sizetype", F2C(add_hash_size_type_id_) },
          { BF_SIZE_ID, "sizeid", F2C(add_hash_size_id_) },

          { BF_ADVERTISER_ID, "advertiser", F2C(add_hash_advertiser_id_) },
          { BF_CAMPAIGN_ID, "campaign", F2C(add_hash_campaign_id_) },
          { BF_CCG_ID, "group", F2C(add_hash_ccg_id_) },
          { BF_CREATIVE_ID, "creative", F2C(add_hash_creative_id_) },
          { BF_CC_ID, "ccid", F2C(add_hash_campaign_creative_id_) },
          { BF_CAMPAIGN_FREQ_ID, "campaign_freq", F2C(add_hash_campaign_freq_id_) },
          { BF_CAMPAIGN_FREQ_LOG_ID, "campaign_freq_log", F2C(add_hash_campaign_freq_log_id_) },

          {
            BF_HISTORY_CHANNELS,
            "userch",
            new CTR::ArrayParamFeatureCalculatorCreator<ChannelIdHashSet>(
              &CampaignSelectParams::channels)
          },
          {
            BF_GEO_CHANNELS,
            "geoch",
            new CTR::ArrayParamFeatureCalculatorCreator<ChannelIdSet>(
              &CampaignSelectParams::geo_channels)
          },
          {
            BF_CONTENT_CATEGORIES,
            "crcatcont",
            new CTR::ArrayCreativeFeatureCalculatorCreator<
              Creative::CategoryIdArray>(
                &Creative::content_categories)
          },
          {
            BF_VISUAL_CATEGORIES,
            "crcatvis",
            new CTR::ArrayCreativeFeatureCalculatorCreator<
              Creative::CategoryIdArray>(
                &Creative::visual_categories)
          },
        };

#       undef F2C

        for(size_t i = 0;
           i < sizeof(FEATURE_DESCRIPTORS) / sizeof(FEATURE_DESCRIPTORS[0]); ++i)
        {
          feature_descriptors_.insert(std::make_pair(
            FEATURE_DESCRIPTORS[i].feature,
            FEATURE_DESCRIPTORS[i]));
          feature_descriptors_by_name_.insert(std::make_pair(
            FEATURE_DESCRIPTORS[i].name,
            FEATURE_DESCRIPTORS[i]));
        }
      }

      // return null if feature unknown (return value life before exit call)
      const FeatureDescriptor*
      resolve(CTR::BasicFeature basic_feature) const
      {
        FeatureDescriptorMap::const_iterator fit =
          feature_descriptors_.find(basic_feature);

        if(fit != feature_descriptors_.end())
        {
          return &(fit->second);
        }

        return 0;
      }

      const FeatureDescriptor*
      resolve_by_name(const String::SubString& feature_name) const
      {
        if(!feature_name.empty())
        {
          FeatureDescriptorByNameMap::const_iterator fit =
            feature_descriptors_by_name_.find(feature_name);

          if(fit != feature_descriptors_by_name_.end())
          {
            return &(fit->second);
          }
        }

        return 0;
      }

    private:
      typedef std::map<CTR::BasicFeature, FeatureDescriptor>
        FeatureDescriptorMap;
      typedef std::map<String::SubString, FeatureDescriptor>
        FeatureDescriptorByNameMap;

    private:
      FeatureDescriptorMap feature_descriptors_;
      FeatureDescriptorByNameMap feature_descriptors_by_name_;
    };

    typedef Generics::Singleton<FeatureDescriptorResolver_>
      FeatureDescriptorResolver;
  }

  struct FastFeatureSetHolder:
    public Vanga::FastFeatureSet,
    public ReferenceCounting::AtomicImpl
  {
  protected:
    virtual ~FastFeatureSetHolder() noexcept
    {}
  };

  namespace
  {
    class FeatureSetProvider_: public ReferenceCounting::AtomicImpl
    {
    public:
      FastFeatureSetHolder_var
      get()
      {
        FastFeatureSetHolder_var res;

        {
          SyncPolicy::WriteGuard lock(lock_);
          if(!feature_sets_.empty())
          {
            feature_sets_.rbegin()->swap(res);
            feature_sets_.pop_back();
          }
        }

        return res.in() ? res :
          FastFeatureSetHolder_var(new FastFeatureSetHolder());
      }

      void
      release(FastFeatureSetHolder_var& feature_set)
      {
        // DEBUG - need remove after test
        //assert(feature_set->check(0));

        SyncPolicy::WriteGuard lock(lock_);
        feature_sets_.push_back(FastFeatureSetHolder_var());
        feature_sets_.rbegin()->swap(feature_set);
      }

    protected:
      typedef Sync::Policy::PosixThread SyncPolicy;

    protected:
      virtual ~FeatureSetProvider_() noexcept
      {}

    protected:
      SyncPolicy::Mutex lock_;
      std::deque<FastFeatureSetHolder_var> feature_sets_;
    };

    typedef ReferenceCounting::SmartPtr<FeatureSetProvider_> FeatureSetProvider_var;

    typedef Generics::Singleton<FeatureSetProvider_, FeatureSetProvider_var>
      FeatureSetProvider;
  }

  namespace CTR
  {
    // FeatureNameResolver
    FeatureNameResolver::FeatureNameResolver() noexcept
    {}

    bool
    FeatureNameResolver::basic_feature_by_name(
      BasicFeature& basic_feature,
      const String::SubString& feature_name)
      const
      noexcept
    {
      const FeatureDescriptor* feature_descriptor =
        FeatureDescriptorResolver::instance().resolve_by_name(feature_name);

      if(feature_descriptor)
      {
        basic_feature = feature_descriptor->feature;
        return true;
      }

      return false;
    }
  }

  class CTRProvider::ConfigParser: public ReferenceCounting::AtomicImpl
  {
  public:
    struct ModelDescriptor
    {
      ModelDescriptor()
        : dimension(0),
          weight(RevenueDecimal(false, 1, 0))
      {}

      std::string method;
      unsigned long dimension;
      RevenueDecimal weight;
      FeatureArray features;
      std::string file;
    };

    typedef std::list<ModelDescriptor> ModelDescriptorList;

    struct AlgorithmDescriptor
    {
      AlgorithmDescriptor()
        : threshold(RevenueDecimal::ZERO)
      {}

      std::string id;
      unsigned long weight;
      std::string campaigns_whitelist_file;
      std::string campaigns_blacklist_file;
      ModelDescriptorList models;
      RevenueDecimal threshold;
    };

    typedef std::list<AlgorithmDescriptor> AlgorithmDescriptorList;

    class ConfigDescriptor: public ReferenceCounting::AtomicImpl
    {
    public:
      ConfigDescriptor() noexcept
        : version(1),
          default_weight(0)
      {}

      unsigned long version;
      unsigned long default_weight;
      std::string feature_mapping_file;
      AlgorithmDescriptorList algorithms;

    protected:
      virtual
      ~ConfigDescriptor() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<ConfigDescriptor>
      ConfigDescriptor_var;

  public:
    ConfigParser() noexcept;

    ConfigDescriptor_var
    parse(const char* file) /*throw(InvalidConfig)*/;

  protected:
    virtual
    ~ConfigParser() noexcept = default;

    typedef Commons::JsonParamProcessor<ModelDescriptor>
      JsonModelParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonModelParamProcessor>
      JsonModelParamProcessor_var;

    typedef Commons::JsonParamProcessor<AlgorithmDescriptor>
      JsonAlgorithmParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonAlgorithmParamProcessor>
      JsonAlgorithmParamProcessor_var;

    typedef Commons::JsonParamProcessor<ConfigDescriptor>
      JsonConfigParamProcessor;
    typedef ReferenceCounting::SmartPtr<JsonConfigParamProcessor>
      JsonConfigParamProcessor_var;

    class JsonBasicFeatureProcessor:
      public JsonModelParamProcessor,
      protected CTR::FeatureNameResolver
    {
    public:
      JsonBasicFeatureProcessor();

      virtual void
      process(ModelDescriptor& model_descriptor,
        const JsonValue& value) const
        /*throw(InvalidConfig)*/;

    protected:
      virtual
      ~JsonBasicFeatureProcessor() noexcept = default;
    };

    class JsonFeatureArrayProcessor:
      public Commons::JsonArrayParamProcessor<ModelDescriptor>
    {
    public:
      JsonFeatureArrayProcessor();

      virtual void
      process(
        ModelDescriptor& model_descriptor,
        const JsonValue& value) const
        /*throw(InvalidConfig)*/;

    protected:
      virtual
      ~JsonFeatureArrayProcessor() noexcept = default;
    };

    struct AddAlgorithm: public std::unary_function<
      ConfigDescriptor&,
      AlgorithmDescriptor&>
    {
      result_type
      operator() (argument_type config_descriptor) const noexcept
      {
        config_descriptor.algorithms.push_back(AlgorithmDescriptor());
        return config_descriptor.algorithms.back();
      }
    };

    struct AddModel: public std::unary_function<
      AlgorithmDescriptor&,
      ModelDescriptor&>
    {
      result_type
      operator()(argument_type alg_descriptor) const noexcept
      {
        alg_descriptor.models.push_back(ModelDescriptor());
        return alg_descriptor.models.back();
      }
    };

  protected:
    JsonConfigParamProcessor_var root_processor_;
  };

  class RemoveConfigTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef std::list<std::string> FileList;

  public:
    RemoveConfigTask(
      const FileList& config_files,
      const FileList& config_directories)
      /*throw(eh::Exception)*/;

    virtual void
    execute() noexcept;

  protected:
    virtual
    ~RemoveConfigTask() noexcept = default;

  private:
    const FileList config_files_;
    const FileList config_directories_;
  };

  // RemoveConfigTask
  RemoveConfigTask::RemoveConfigTask(
    const FileList& config_files,
    const FileList& config_directories)
    /*throw(eh::Exception)*/
    : config_files_(config_files),
      config_directories_(config_directories)
  {}

  void
  RemoveConfigTask::execute() noexcept
  {
    for(FileList::const_iterator fit = config_files_.begin();
      fit != config_files_.end();
      ++fit)
    {
      ::unlink(fit->c_str());
    }

    for(FileList::const_iterator fit = config_directories_.begin();
      fit != config_directories_.end(); ++fit)
    {
      ::rmdir(fit->c_str());
    }
  }

  // CTRProvider::ConfigParser::JsonBasicFeatureProcessor
  CTRProvider::ConfigParser::
  JsonBasicFeatureProcessor::JsonBasicFeatureProcessor()
  {}

  void
  CTRProvider::ConfigParser::
  JsonBasicFeatureProcessor::process(
    ModelDescriptor& model_descriptor,
    const JsonValue& value) const /*throw(InvalidConfig)*/
  {
    // value is array of strings
    if(value.getTag() != JSON_TAG_STRING)
    {
      Stream::Error ostr;
      ostr << "Invalid feature type";
      throw InvalidConfig(ostr);
    }

    std::string feature_name;
    value.toString(feature_name);

    CTR::BasicFeature basic_feature;

    if(basic_feature_by_name(basic_feature, feature_name))
    {
      model_descriptor.features.back().basic_features.insert(
        basic_feature);
    }
    else
    {
      Stream::Error ostr;
      ostr << "Invalid feature name '" << feature_name << "'";
      throw InvalidConfig(ostr);
    }
  }

  // ConfigParser::JsonFeatureArrayProcessor
  CTRProvider::ConfigParser::
  JsonFeatureArrayProcessor::JsonFeatureArrayProcessor()
    : Commons::JsonArrayParamProcessor<ModelDescriptor>(
        ReferenceCounting::SmartPtr<Commons::JsonParamProcessor<ModelDescriptor> >(
          new JsonBasicFeatureProcessor()))
  {}

  void
  CTRProvider::ConfigParser::
  JsonFeatureArrayProcessor::process(
    ModelDescriptor& model_descriptor,
    const JsonValue& value) const /*throw(InvalidConfig)*/
  {
    // value is array of strings
    model_descriptor.features.push_back(Feature());

    Commons::JsonArrayParamProcessor<ModelDescriptor>::process(
      model_descriptor,
      value);
  }

  // ConfigParser
  CTRProvider::ConfigParser::ConfigParser() noexcept
  {
    ReferenceCounting::SmartPtr<
      Commons::JsonCompositeParamProcessor<ConfigDescriptor> >
        root_processor =
          new Commons::JsonCompositeParamProcessor<ConfigDescriptor>();

    root_processor->add_processor(
      Config::DEFAULT_WEIGHT,
      JsonConfigParamProcessor_var(
        new Commons::JsonNumberParamProcessor<ConfigDescriptor, unsigned long>(
          &ConfigDescriptor::default_weight)));

    root_processor->add_processor(
      Config::VERSION,
      JsonConfigParamProcessor_var(
        new Commons::JsonNumberParamProcessor<ConfigDescriptor, unsigned long>(
          &ConfigDescriptor::version)));

    root_processor->add_processor(
      Config::FEATURE_MAPPING_FILE,
      JsonConfigParamProcessor_var(
        new Commons::JsonStringParamProcessor<ConfigDescriptor>(
          &ConfigDescriptor::feature_mapping_file)));

    {
      ReferenceCounting::SmartPtr<
        Commons::JsonCompositeParamProcessor<ModelDescriptor> >
          model_processor =
            new Commons::JsonCompositeParamProcessor<ModelDescriptor>();

      // fill model processor
      model_processor->add_processor(
        Config::ALGORITHM_MODEL_FEATURES_DIMENSION,
        JsonModelParamProcessor_var(
          new Commons::JsonNumberParamProcessor<ModelDescriptor, unsigned long>(
            &ModelDescriptor::dimension)));

      model_processor->add_processor(
        Config::ALGORITHM_MODEL_WEIGHT,
        JsonModelParamProcessor_var(
          new Commons::JsonDecimalParamProcessor<ModelDescriptor, RevenueDecimal>(
            &ModelDescriptor::weight)));

      model_processor->add_processor(
        Config::ALGORITHM_MODEL_METHOD,
        JsonModelParamProcessor_var(
          new Commons::JsonStringParamProcessor<ModelDescriptor>(
            &ModelDescriptor::method)));

      model_processor->add_processor(
        Config::ALGORITHM_MODEL_FILE,
        JsonModelParamProcessor_var(
          new Commons::JsonStringParamProcessor<ModelDescriptor>(
            &ModelDescriptor::file)));

      // features processor
      model_processor->add_processor(
        Config::ALGORITHM_MODEL_FEATURES,
        JsonModelParamProcessor_var(
          new Commons::JsonArrayParamProcessor<ModelDescriptor>(
            JsonModelParamProcessor_var(
              new JsonFeatureArrayProcessor()))));

      // create algorithm processor
      ReferenceCounting::SmartPtr<
        Commons::JsonCompositeParamProcessor<AlgorithmDescriptor> >
          alg_processor =
            new Commons::JsonCompositeParamProcessor<AlgorithmDescriptor>();

      alg_processor->add_processor(
        Config::ALGORITHM_ID,
        JsonAlgorithmParamProcessor_var(
          new Commons::JsonStringParamProcessor<AlgorithmDescriptor>(
            &AlgorithmDescriptor::id)));

      alg_processor->add_processor(
        Config::ALGORITHM_WEIGHT,
        JsonAlgorithmParamProcessor_var(
          new Commons::JsonNumberParamProcessor<AlgorithmDescriptor, unsigned long>(
            &AlgorithmDescriptor::weight)));

      alg_processor->add_processor(
        Config::ALGORITHM_THRESHOLD,
        JsonAlgorithmParamProcessor_var(
          new Commons::JsonDecimalParamProcessor<AlgorithmDescriptor, RevenueDecimal>(
            &AlgorithmDescriptor::threshold)));

      alg_processor->add_processor(
        Config::ALGORITHM_MODELS,
        JsonAlgorithmParamProcessor_var(
          new Commons::JsonArrayParamProcessor<
            AlgorithmDescriptor, AddModel>(model_processor)));

      {
        ReferenceCounting::SmartPtr<
          Commons::JsonCompositeParamProcessor<AlgorithmDescriptor> >
            params_processor =
              new Commons::JsonCompositeParamProcessor<AlgorithmDescriptor>();

        params_processor->add_processor(
          Config::ALGORITHM_PARAMS_CAMPAIGNS_WHITELIST_FILE,
          JsonAlgorithmParamProcessor_var(
            new Commons::JsonStringParamProcessor<AlgorithmDescriptor>(
              &AlgorithmDescriptor::campaigns_whitelist_file)));

        params_processor->add_processor(
          Config::ALGORITHM_PARAMS_CAMPAIGNS_BLACKLIST_FILE,
          JsonAlgorithmParamProcessor_var(
            new Commons::JsonStringParamProcessor<AlgorithmDescriptor>(
              &AlgorithmDescriptor::campaigns_blacklist_file)));

        alg_processor->add_processor(
          Config::ALGORITHM_PARAMS,
          params_processor);
      }

      root_processor->add_processor(
        Config::ALGORITHMS,
        JsonConfigParamProcessor_var(
          new Commons::JsonArrayParamProcessor<
            ConfigDescriptor, AddAlgorithm>(alg_processor)));
    }

    root_processor_ = root_processor;
  }

  CTRProvider::ConfigParser::ConfigDescriptor_var
  CTRProvider::ConfigParser::parse(const char* file)
    /*throw(InvalidConfig)*/
  {
    static const char* FUN = "CTRProvider::ConfigParser::parse()";

    try
    {
      JsonValue root_value;
      JsonAllocator json_allocator;
      Generics::MMapFile mmap_file(file);
      Generics::ArrayAutoPtr<char> json_holder(mmap_file.length() + 1);
      ::memcpy(json_holder.get(), mmap_file.memory(), mmap_file.length());
      json_holder.get()[mmap_file.length()] = 0;
      char* parse_end;
      JsonParseStatus status = json_parse(
        json_holder.get(),
        &parse_end,
        &root_value,
        json_allocator);

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
        throw InvalidConfig(ostr);
      }

      JsonTag root_tag = root_value.getTag();

      if(root_tag != JSON_TAG_OBJECT)
      {
        Stream::Error ostr;
        ostr << FUN << ": incorrect root tag type";
        throw InvalidConfig(ostr);
      }

      ConfigDescriptor_var result = new ConfigDescriptor();
      root_processor_->process(*result, root_value);

      if(result->version != 2)
      {
        Stream::Error ostr;
        ostr << FUN << ": unsupported version = " << result->version;
        throw InvalidConfig(ostr);
      }

      return result;
    }
    catch(const Generics::MMap::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't open file '" << file << "': " << ex.what();
      throw InvalidConfig(ostr);
    }
  }

  // CTRProvider::Feature
  void
  CTRProvider::Feature::print(std::ostream& out) const
  {
    for(CTR::BasicFeatureSet::const_iterator fit = basic_features.begin();
        fit != basic_features.end(); ++fit)
    {
      if(fit != basic_features.begin())
      {
        out << ",";
      }

      const FeatureDescriptor* feature_descriptor =
        FeatureDescriptorResolver::instance().resolve(*fit);
      if(feature_descriptor)
      {
        out << "'" << feature_descriptor->name << "'";
      }
      else
      {
        out << "'unknown'";
      }
    }
    out << ": hash_seed = " << hash_seed << "(signed = " << (int32_t)hash_seed << ")";
  }

  bool
  CTRProvider::Feature::operator<(const CTRProvider::Feature& right) const
  {
    return std::lexicographical_compare(
      basic_features.begin(),
      basic_features.end(),
      right.basic_features.begin(),
      right.basic_features.end());
  }

  // CTRProvider::Model
  CTRProvider::Model::Model(unsigned long model_id_val)
    noexcept
    : model_id(model_id_val),
      push_hour(false),
      push_week_day(false),
      push_campaign_freq(false),
      push_campaign_freq_log(false),
      creative_dependent(true)
  {}

  void
  CTRProvider::Model::load_feature_weights(
    const String::SubString& file)
    /*throw(InvalidConfig)*/
  {
    static const char* FUN = "CTRProvider::Model::load_feature_weights()";

    uint64_t size = 1;
    size = size << dimension;
    if(size > std::numeric_limits<size_t>::max())
    {
      Stream::Error ostr;
      ostr << FUN << ": very big dimension=" << dimension;
      throw InvalidConfig(ostr);
    }

    feature_weights.resize(size);

    try
    {
      AdServer::ProfilingCommons::FileReader reader(
        file.str().c_str(),
        1024 * 1024);

      if(reader.file_size() != size * sizeof(float))
      {
        Stream::Error ostr;
        ostr << FUN << ": incorrect file size = " << reader.file_size() <<
          " instead " << (size * sizeof(float));
        throw InvalidConfig(ostr);
      }

      size_t index = 0;
      //float value; // IEEE 754 by static check
      uint32_t loaded_segment;
      while(reader.read(&loaded_segment, sizeof(loaded_segment)))
      {
        loaded_segment = htonl(loaded_segment);
        //todo: this line was replaced with next 3 ones
        // feature_weights[index++] = *reinterpret_cast<float*>(&loaded_segment);
        float weight;
        memcpy(&weight, &loaded_segment, sizeof(weight));
        feature_weights[index++] = weight;
      }
    }
    catch(const AdServer::ProfilingCommons::FileReader::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught FileReader::Exception: " << ex.what();
      throw InvalidConfig(ostr);
    }
  }

  struct FirstLess
  {
    template<typename PairType>
    bool
    operator()(const PairType& left, const PairType& right) const
    {
      return left.first < right.first;
    }
  };

  // CTRConfigSelector
  struct CTRConfigSelector
  {
    CTRConfigSelector()
      : reg_exp_(CTR_CONFIG_FOLDER_NAME_REGEXP)
    {}

    bool
    operator ()(const char* full_path, const struct stat& file_stat)
      noexcept
    {
      if(S_ISDIR(file_stat.st_mode))
      {
        String::RegEx::Result sub_strs;

        String::SubString file_name(
          Generics::DirSelect::file_name(full_path));

        if(reg_exp_.search(sub_strs, file_name) &&
          (assert(!sub_strs.empty()), sub_strs[0].length() == file_name.size()))
        {
          if(result_folder < file_name)
          {
            file_name.assign_to(result_folder);
          }
        }
      }

      return false; // check only top dirs
    }

    std::string result_folder;

  protected:
    String::RegEx reg_exp_;
  };

  // CTRProvider::AlgsRef
  void
  CTRProvider::AlgsRef::print(std::ostream& out) const noexcept
  {
    for(auto it = algs.begin(); it != algs.end(); ++it)
    {
      out << "[" << it->first << "," << it->second << "]";
    }
  }

  // CTRProvider::CalculationContext
  CTRProvider::CalculationContext::CalculationContext(
    const Calculation* calculation,
    const Tag::Size* tag_size)
    noexcept
    : calculation_(ReferenceCounting::add_ref(calculation)),
      tag_size_(ReferenceCounting::add_ref(tag_size))
  {}

  CTRProvider::CalculationContext::~CalculationContext()
    noexcept
  {}

  // XGBoost specific methods
  CTRProvider::HashArrayHolder_var
  CTRProvider::CalculationContext::get_features_hashes_(
    const Algorithm& algorithm,
    const Model& model,
    FeatureType feature_type,
    const CampaignSelectParams& request_params,
    const Tag::Size* tag_size,
    const Creative* creative)
    const
    noexcept
  {
    if(model.feature_set_indexes[feature_type] == 0)
    {
      return HashArrayHolder_var();
    }

    HashArrayHolder_var res_hashes =
      feature_type != FT_CANDIDATE ?
      check_feature_set_level_eval_hashes_(
        feature_type,
        model.feature_set_indexes[feature_type]) :
      HashArrayHolder_var();

    if(res_hashes)
    {
      return res_hashes;
    }

    res_hashes = new HashArrayHolder();

    calculation_->eval_features_hashes_(
      *res_hashes,
      algorithm,
      model,
      feature_type,
      request_params,
      tag_size,
      creative);

    // push direct feature hashes: wd, hour
    if(model.push_hour)
    {
      res_hashes->push_back(std::make_pair(CTR::BF_HOUR, request_params.time_hour));
    }

    if(model.push_week_day)
    {
      res_hashes->push_back(std::make_pair(CTR::BF_WEEK_DAY, request_params.time_week_day));
    }

    insert_feature_set_level_eval_hashes_(
      feature_type,
      model.feature_set_indexes[feature_type],
      res_hashes);

    return res_hashes;
  }

  CTRProvider::HashArrayHolder_var
  CTRProvider::CalculationContext::check_feature_set_level_eval_hashes_(
    FeatureType feature_type,
    unsigned long feature_set_index)
    const
    noexcept
  {
    HashArrayHolder_var res;

    if(feature_type == FT_AUCTION)
    {
      auto it = feature_set_level_eval_hashes_.find(feature_set_index);

      if(it != feature_set_level_eval_hashes_.end())
      {
        res = it->second;
      }
    }
    else if(feature_type == FT_REQUEST)
    {
      auto it = calculation_->feature_set_level_eval_hashes_.find(feature_set_index);

      if(it != calculation_->feature_set_level_eval_hashes_.end())
      {
        res = it->second;
      }
    }

    if(DEBUG_CTR_CALCULATION_)
    {
      std::cout << "CTR DEBUG(): check_feature_set_level_eval_hashes_("
        "feature_set_index = " << feature_set_index <<
        ", feature_type = " << feature_type << ") => " <<
        (res ? "found" : "not found") <<
        std::endl;
    }

    return res;
  }

  void
  CTRProvider::CalculationContext::insert_feature_set_level_eval_hashes_(
    FeatureType feature_type,
    unsigned long feature_set_index,
    HashArrayHolder* hashes)
    const
    noexcept
  {
    if(feature_type == FT_AUCTION)
    {
      feature_set_level_eval_hashes_[feature_set_index] =
        ReferenceCounting::add_ref(hashes);
    }
    else if(feature_type == FT_REQUEST)
    {
      calculation_->feature_set_level_eval_hashes_[feature_set_index] =
        ReferenceCounting::add_ref(hashes);
    }
  }

  bool
  CTRProvider::CalculationContext::check_feature_set_level_eval_weights_(
    FeatureType feature_type,
    float& weight)
    const
    noexcept
  {
    if(feature_type == FT_AUCTION)
    {
      auto it = feature_set_level_eval_weights_.find(feature_type);

      if(it != feature_set_level_eval_weights_.end())
      {
        weight = it->second;
        return true;
      }
    }
    else if(feature_type == FT_REQUEST)
    {
      auto it = calculation_->feature_set_level_eval_weights_.find(feature_type);

      if(it != calculation_->feature_set_level_eval_weights_.end())
      {
        weight = it->second;
        return true;
      }
    }

    return false;
  }

  void
  CTRProvider::CalculationContext::insert_feature_set_level_eval_weights_(
    FeatureType feature_type,
    float weight)
    const
    noexcept
  {
    if(feature_type == FT_AUCTION)
    {
      feature_set_level_eval_weights_[feature_type] = weight;
    }
    else if(feature_type == FT_REQUEST)
    {
      calculation_->feature_set_level_eval_weights_[feature_type] = weight;
    }
  }

  // FTRL specific methods
  float
  CTRProvider::CalculationContext::get_features_weight_(
    const Algorithm& algorithm,
    const Model& model,
    FeatureType feature_type,
    const CampaignSelectParams& request_params,
    const Tag::Size* tag_size,
    const Creative* creative)
    const
    noexcept
  {
    float weight;

    bool check_weight = check_feature_set_level_eval_weights_(
      feature_type, weight);

    if(check_weight)
    {
      return weight;
    }
    else
    {
      float ins_weight;
      ins_weight = calculation_->eval_features_weight_(
        algorithm,
        model,
        feature_type,
        request_params,
        tag_size,
        creative);

      insert_feature_set_level_eval_weights_(feature_type, ins_weight);
      return ins_weight;
    }
  }

  void
  CTRProvider::CalculationContext::get_xgb_hashes_i(
    CTR::HashArray& res,
    const Creative* creative) const
    noexcept
  {
    res = opt_hashes_;

    const long alg_index = calculation_->select_alg_index_(creative);

    if(alg_index >= 0)
    {
      const Algorithm* algorithm =
        calculation_->ctr_provider_->ctr_algorithms_[alg_index];

      for(ModelList::const_iterator model_it = algorithm->models.begin();
        model_it != algorithm->models.end();
        ++model_it)
      {
        HashArrayHolder_var local_hashes = get_features_hashes_(
          *algorithm,
          **model_it,
          FT_CANDIDATE,
          *(calculation_->request_params_),
          tag_size_.in(),
          creative);

        if(local_hashes)
        {
          res.insert(res.end(), local_hashes->begin(), local_hashes->end());
        }
      }
    }
  }

  inline
  RevenueDecimal
  CTRProvider::CalculationContext::get_model_ctr_(
    const Algorithm& algorithm,
    const Model& model,
    const Creative* creative)
    const /*throw(Overflow)*/
  {
    // check cached ctr
    auto model_ctr_it = calculation_->model_ctrs_.find(model.model_id);

    if(model_ctr_it != calculation_->model_ctrs_.end())
    {
      return model_ctr_it->second;
    }

    model_ctr_it = model_ctrs_.find(model.model_id);

    if(model_ctr_it != model_ctrs_.end())
    {
      return model_ctr_it->second;
    }

    // eval ctr
    RevenueDecimal model_ctr;

    if(model.method == MM_FTRL)
    {
      float result_weight = get_features_weight_(
        algorithm,
        model,
        FT_REQUEST,
        *(calculation_->request_params_),
        tag_size_,
        creative);

      result_weight += get_features_weight_(
        algorithm,
        model,
        FT_AUCTION,
        *(calculation_->request_params_),
        tag_size_,
        creative);

      // fetch candidate features
      result_weight += get_features_weight_(
        algorithm,
        model,
        FT_CANDIDATE,
        *(calculation_->request_params_),
        tag_size_,
        creative);

      model_ctr = CTRProvider::Calculation::eval_ctr_(result_weight);
    }
    else if(model.method == MM_XGBOOST || model.method == MM_VANGA)
    {
      // by hash eval methods
      HashArrayHolder_var request_hashes = get_features_hashes_(
        algorithm,
        model,
        FT_REQUEST,
        *(calculation_->request_params_),
        0, // tag size
        0 // creative
        );

      HashArrayHolder_var auction_hashes = get_features_hashes_(
        algorithm,
        model,
        FT_AUCTION,
        *(calculation_->request_params_),
        tag_size_,
        0 // creative
        );

      HashArrayHolder_var candidate_hashes = get_features_hashes_(
        algorithm,
        model,
        FT_CANDIDATE,
        *(calculation_->request_params_),
        tag_size_,
        creative
        );

      if(model.method == MM_XGBOOST)
      {
        opt_hashes_.clear();

        // push candidate level direct features
        if(model.push_campaign_freq || model.push_campaign_freq_log)
        {
          uint32_t imps = 0;

          CampaignSelectParams::CampaignImpsMap::const_iterator it =
            calculation_->request_params_->campaign_imps.find(
              creative->campaign->campaign_group_id);
          if(it != calculation_->request_params_->campaign_imps.end())
          {
            imps = it->second;
          }

          if(model.push_campaign_freq)
          {
            opt_hashes_.push_back(std::make_pair(CTR::BF_CAMPAIGN_FREQ_ID, imps));
          }

          if(model.push_campaign_freq_log)
          {
            opt_hashes_.push_back(
              std::make_pair(
                CTR::BF_CAMPAIGN_FREQ_LOG_ID,
                static_cast<uint32_t>(Generics::BitAlgs::highest_bit_32(imps + 1))));
          }
        }

        model_ctr = calculation_->xgboost_eval_ctr_(
          model,
          request_hashes ? *request_hashes : calculation_->ctr_provider_->empty_hash_array_,
          auction_hashes,
          candidate_hashes,
          &opt_hashes_);
      }
      else
      {
        model_ctr = calculation_->vanga_eval_ctr_(
          model,
          request_hashes ? *request_hashes : calculation_->ctr_provider_->empty_hash_array_,
          auction_hashes,
          candidate_hashes);
      }

      assert((model_ctr.is_zero(), true));
    }
    else if(model.method == MM_TRIVIAL)
    {
      model_ctr = calculation_->trivial_eval_ctr_(model, creative);
    }
    else
    {
      assert(0);
    }

    if(model.max_feature_type < FT_CANDIDATE)
    {
      if(model.max_feature_type == FT_AUCTION)
      {
        model_ctrs_.insert(std::make_pair(model.model_id, model_ctr));
      }
      else if(model.max_feature_type == FT_REQUEST)
      {
        calculation_->model_ctrs_.insert(std::make_pair(model.model_id, model_ctr));
      }
    }

    return model_ctr;
  }

  std::pair<RevenueDecimal, const CTRProvider::Algorithm*>
  CTRProvider::CalculationContext::get_ctr_(
    const Creative* creative,
    bool* creative_dependent)
    const /*throw(Overflow)*/
  {
    // find algorithms(indexes) that can be applied for this campaign
    const long alg_index = calculation_->select_alg_index_(creative);

    if(alg_index < 0)
    {
      // default algorithm
      return std::make_pair(creative->campaign->ctr, nullptr);
    }

    const Algorithm* algorithm =
      calculation_->ctr_provider_->ctr_algorithms_[alg_index];

    // eval weight for each model of selected algorithm
    // ModelValue can't be changed here because can be reused
    RevenueDecimal ctr_sum = RevenueDecimal::ZERO;
    unsigned long model_count = 0;

    bool res_creative_dependent = false;

    for(ModelList::const_iterator model_it = algorithm->models.begin();
      model_it != algorithm->models.end();
      ++model_it, ++model_count)
    {
      res_creative_dependent |= (*model_it)->creative_dependent;

      const RevenueDecimal model_ctr = get_model_ctr_(
        *algorithm,
        **model_it,
        creative);

      const RevenueDecimal ctr_fee = RevenueDecimal::mul(
        model_ctr,
        (*model_it)->weight,
        Generics::DMR_FLOOR);

      if(DEBUG_CTR_CALCULATION_)
      {
        std::cout << "CTR DEBUG(" << (*model_it)->method_name << "): "
          "ctr = " << model_ctr <<
          ", ctr fee = " << ctr_fee << "(by model #" << model_count <<
          " with weight = " << (*model_it)->weight << ")" <<
          std::endl;
      }

      ctr_sum += ctr_fee;
    }

    if(creative_dependent)
    {
      *creative_dependent = res_creative_dependent;
    }

    if(DEBUG_CTR_CALCULATION_)
    {
      std::cout << "CTR DEBUG: result ctr for ccg_id = " <<
        creative->campaign->campaign_id <<
        ", cc_id = " << creative->ccid <<
        ": " << ctr_sum << std::endl;
    }

    return std::make_pair(ctr_sum, algorithm);
  }

  RevenueDecimal
  CTRProvider::CalculationContext::get_ctr(
    const Creative* creative) const
    /*throw(Overflow)*/
  {
    return get_ctr_(creative, 0).first;
  }

  void
  CTRProvider::CalculationContext::get_ctr_details(
    CTRList& ctrs,
    const Creative* creative) const
    /*throw(Overflow)*/
  {
    const long alg_index = calculation_->select_alg_index_(creative);

    if(alg_index >= 0) // empty list for default algo
    {
      const Algorithm* algorithm =
        calculation_->ctr_provider_->ctr_algorithms_[alg_index];

      for(ModelList::const_iterator model_it = algorithm->models.begin();
        model_it != algorithm->models.end();
        ++model_it)
      {
        RevenueDecimal model_ctr = get_model_ctr_(
          *algorithm,
          **model_it,
          creative);

        ctrs.push_back(model_ctr);
      }
    }
  }

  bool
  CTRProvider::CalculationContext::check_rate(
    const Creative* creative,
    RevenueDecimal* rate,
    bool* creative_dependent) const
    /*throw(Overflow)*/
  {
    std::pair<RevenueDecimal, const Algorithm*> ctr_alg = get_ctr_(
      creative, creative_dependent);
    if(!ctr_alg.second || ctr_alg.first >= ctr_alg.second->threshold)
    {
      if(rate)
      {
        *rate = !ctr_alg.second ? RevenueDecimal::ZERO : ctr_alg.first;
      }
      return true;
    }

    return false;
  }

  // CTRProvider::Calculation
  CTRProvider::Calculation::Calculation(
    const CTRProvider* ctr_provider,
    const CampaignSelectParams* request_params)
    noexcept
    : rand_(Generics::safe_rand(WEIGHT_NORM_SUM)),
      ctr_provider_(ReferenceCounting::add_ref(ctr_provider)),
      request_params_(ReferenceCounting::add_ref(request_params))
  {}

  CTRProvider::Calculation::~Calculation() noexcept
  {}

  CTRProvider::CalculationContext_var
  CTRProvider::Calculation::create_context(
    const Tag::Size* tag_size) const
    noexcept
  {
    // eval context features
    return new CTRProvider::CalculationContext(
      this,
      tag_size);
  }

  std::string
  CTRProvider::Calculation::algorithm_id(
    const Creative* creative) const
    noexcept
  {
    const long alg_index = select_alg_index_(creative);
    if(alg_index < 0)
    {
      return std::string();
    }
    return ctr_provider_->ctr_algorithms_[alg_index]->id;
  }

  long
  CTRProvider::Calculation::select_alg_(
    const AlgsRef& alg_ref,
    unsigned long rand)
    noexcept
  {
    if(!alg_ref.algs.empty())
    {
      AlgIdMap::const_iterator alg_id_it =
        alg_ref.algs.upper_bound(rand);

      assert(alg_id_it != alg_ref.algs.end());

      return alg_id_it->second;
    }

    return -1;
  }

  long
  CTRProvider::Calculation::select_alg_index_(
    const Creative* creative) const
    noexcept
  {
    // find algorithms(indexes) that can be applied for this campaign
    const unsigned long campaign_id = creative->campaign->campaign_group_id;

    AlgsRefByCampaignIdMap::const_iterator algs_ref_it =
      ctr_provider_->campaign_algs_.find(campaign_id);

    unsigned long alg_index;

    if(algs_ref_it != ctr_provider_->campaign_algs_.end())
    {
      alg_index = select_alg_(algs_ref_it->second, rand_);
    }
    else
    {
      alg_index = select_alg_(
        ctr_provider_->non_campaign_algs_,
        rand_);
    }

    if(DEBUG_CTR_CALCULATION_)
    {
      std::cout << "CTR DEBUG: get_ctr, select alg for campaign id #" <<
        campaign_id << ": result alg index = " << alg_index <<
        ", candidate algs (";
      for(AlgIdMap::const_iterator alg_it = algs_ref_it->second.algs.begin();
          alg_it != algs_ref_it->second.algs.end(); ++alg_it)
      {
        std::cout << (alg_it != algs_ref_it->second.algs.begin() ? "," : "") <<
          alg_it->first << ":" << alg_it->second;
      }
      std::cout << ")" << std::endl;
    }

    return alg_index;
  }

  /*
  void
  CTRProvider::Calculation::eval_features_(
    ModelValue& res_model_value,
    const Algorithm& algorithm,
    const Model& model,
    FeatureType feature_type,
    const CampaignSelectParams& request_params,
    const Tag::Size* tag_size,
    const Creative* creative)
    const
  {
    if(model.method == MM_FTRL)
    {
      res_model_value.request_features_weight +=
        get_features_weight_(
          algorithm,
          model,
          feature_type,
          request_params,
          tag_size,
          creative);
    }
    else
    {
      assert(model.method == MM_XGBOOST || model.method == MM_VANGA);

      get_features_hashes_(
        res_model_value.hashes,
        algorithm,
        model,
        feature_type,
        request_params,
        tag_size,
        creative);
    }

    bool no_next_level_features = true;

    for(int i = feature_type + 1; i < FT_MAX; ++i)
    {
      no_next_level_features &= model.features[i].empty();
    }

    if(no_next_level_features)
    {
      if(model.method == MM_FTRL)
      {
        res_model_value.ctr = eval_ctr_(
          res_model_value.request_features_weight);
      }
      else if(model.method == MM_XGBOOST)
      {
        res_model_value.ctr = CTRProvider::Calculation::xgboost_eval_ctr_(
          res_model_value,
          model);
      }
      else if(model.method == MM_VANGA)
      {
        res_model_value.ctr = CTRProvider::Calculation::vanga_eval_ctr_(
          res_model_value,
          model);
      }
      else
      {
        assert(0);
      }
    }
  };
  */

  void
  CTRProvider::Calculation::eval_features_hashes_(
    CTR::HashArray& hashes,
    const Algorithm& /*algorithm*/,
    const Model& model,
    FeatureType feature_type,
    const CampaignSelectParams& request_params,
    const Tag::Size* tag_size,
    const Creative* creative)
    const
    noexcept
  {
    for(FeatureArray::const_iterator feature_it =
          model.features[feature_type].begin();
        feature_it != model.features[feature_type].end();
        ++feature_it)
    {
      if(DEBUG_CTR_CALCULATION_)
      {
        std::cout << "CTR DEBUG: to eval request feature hashes: ";
        feature_it->print(std::cout);
        std::cout << std::endl;
      }

      CTR::Murmur32v3Adapter hash_adapter(feature_it->hash_seed);
      feature_it->feature_calculator->eval_hashes(
        hashes,
        hash_adapter,
        ctr_provider_->hash_mapping_.get(),
        request_params,
        tag_size,
        creative
        );
    }

    if(DEBUG_CTR_CALCULATION_)
    {
      std::cout << "CTR DEBUG: eval_features_hashes_ hashes: ";
      for(auto it = hashes.begin(); it != hashes.end(); ++it)
      {
        std::cout << (it != hashes.begin() ? ", " : "") << it->first;
      }
      std::cout << std::endl;
    }
  }

  float
  CTRProvider::Calculation::eval_features_weight_(
    const Algorithm& /*algorithm*/,
    const Model& model,
    FeatureType feature_type,
    const CampaignSelectParams& request_params,
    const Tag::Size* tag_size,
    const Creative* creative)
    const
    noexcept
  {
    float res = 0;

    for(FeatureArray::const_iterator feature_it =
          model.features[feature_type].begin();
        feature_it != model.features[feature_type].end();
        ++feature_it)
    {
      if(DEBUG_CTR_CALCULATION_)
      {
        std::cout << "CTR DEBUG(ftrl): to eval request feature weight: ";
        feature_it->print(std::cout);
        std::cout << std::endl;
      }

      CTR::Murmur32v3Adapter hash_adapter(feature_it->hash_seed);
      res += feature_it->feature_calculator->eval(
        hash_adapter,
        ctr_provider_->hash_mapping_.get(),
        request_params,
        tag_size,
        creative
        );
    }

    return res;
  }

  RevenueDecimal
  CTRProvider::Calculation::eval_ctr_(float weight)
    /*throw(Overflow)*/
  {
    static const char* FUN = "CTRProvider::Calculation::eval_ctr_()";

    try
    {
      return adapt_ctr_(1. / (1. + ::expf(-weight)));
    }
    catch(const RevenueDecimal::Overflow& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": overflow on exp of " << weight;
      throw CTRProvider::Overflow(ostr);
    }
  }

  RevenueDecimal
  CTRProvider::Calculation::xgboost_eval_ctr_(
    const Model& model,
    const CTR::HashArray& hashes,
    const CTR::HashArray* add_hashes1,
    const CTR::HashArray* add_hashes2,
    const CTR::HashArray* add_hashes3)
    const
    /*throw(Overflow)*/
  {
    static const char* FUN = "CTRProvider::xgboost_eval_ctr_()";

    auto model_predictor_it = model_xgboost_predictors_.find(model.model_id);

    CTR::XGBoostPredictorPool::Predictor* xgboost_predictor;

    if(model_predictor_it != model_xgboost_predictors_.end())
    {
      xgboost_predictor = model_predictor_it->second.get();
    }
    else
    {
      assert(model.xgboost_predictor_pool.get());

      CTR::XGBoostPredictorPool::Predictor_var new_xgboost_predictor =
        model.xgboost_predictor_pool->get_predictor();

      model_xgboost_predictors_.insert(
        std::make_pair(model.model_id, new_xgboost_predictor));

      xgboost_predictor = new_xgboost_predictor.get();
    }

    float ctr = xgboost_predictor->predict(
      hashes,
      add_hashes1,
      add_hashes2,
      add_hashes3);

    try
    {
      return adapt_ctr_(ctr);
    }
    catch(const RevenueDecimal::Overflow& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": overflow on ctr=" << ctr << " got from XGBoostPredictor ";
      throw CTRProvider::Overflow(ostr);
    }
  }

  RevenueDecimal
  CTRProvider::Calculation::vanga_eval_ctr_(
    const Model& model,
    const CTR::HashArray& hashes,
    const CTR::HashArray* add_hashes1,
    const CTR::HashArray* add_hashes2)
    const
    /*throw(Overflow)*/
  {
    static const char* FUN = "CTRProvider::vanga_eval_ctr_()";

    assert(model.vanga_predictor.in());

    /*
    CTR::HashArray ordered_hashes(model_value.hashes);

    if(add_hashes)
    {
      ordered_hashes.insert(ordered_hashes.end(), add_hashes->begin(), add_hashes->end());
    }

    std::sort(ordered_hashes.begin(), ordered_hashes.end(), FirstLess());

    float ctr = model.vanga_predictor->fpredict(FeatureArrayWrapper(ordered_hashes));
    */

    FeatureSetProvider_& feature_set_provider = FeatureSetProvider::instance();

    FastFeatureSetHolder_var vanga_feature_set = feature_set_provider.get();
    vanga_feature_set->set(hashes.begin(), hashes.end());

    if(add_hashes1)
    {
      vanga_feature_set->set(add_hashes1->begin(), add_hashes1->end());
    }

    if(add_hashes2)
    {
      vanga_feature_set->set(add_hashes2->begin(), add_hashes2->end());
    }

    if(DEBUG_CTR_CALCULATION_)
    {
      std::cout << "CTR DEBUG: vanga hashes: ";
      for(auto it = hashes.begin(); it != hashes.end(); ++it)
      {
        std::cout << (it != hashes.begin() ? ", " : "") << it->first;
      }

      std::cout << "; ";
      if(add_hashes1)
      {
        for(auto it = add_hashes1->begin(); it != add_hashes1->end(); ++it)
        {
          std::cout << (it != add_hashes1->begin() ? ", " : "") << it->first;
        }
      }

      std::cout << "; ";
      if(add_hashes2)
      {
        for(auto it = add_hashes2->begin(); it != add_hashes2->end(); ++it)
        {
          std::cout << (it != add_hashes2->begin() ? ", " : "") << it->first;
        }
      }

      std::cout << std::endl;
    }

    float ctr = model.vanga_predictor->fpredict(*vanga_feature_set); // no throw

    vanga_feature_set->rollback(hashes.begin(), hashes.end());

    if(add_hashes1)
    {
      vanga_feature_set->rollback(add_hashes1->begin(), add_hashes1->end());
    }

    if(add_hashes2)
    {
      vanga_feature_set->rollback(add_hashes2->begin(), add_hashes2->end());
    }

    feature_set_provider.release(vanga_feature_set);

    try
    {
      return adapt_ctr_(ctr);
    }
    catch(const RevenueDecimal::Overflow& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": overflow on ctr=" << ctr << " got from Vanga Predictor ";
      throw CTRProvider::Overflow(ostr);
    }
  }

  RevenueDecimal
  CTRProvider::Calculation::trivial_eval_ctr_(
    const Model& model,
    const Creative* creative) const
  {
    //static const char* FUN = "CTRProvider::trivial_eval_ctr_()";

    assert(model.trivial_predictor);

    // eval ctr by request_params_.tag->tag_id, request_params.referer_hostname
    return model.trivial_predictor->predict(
      request_params_->tag ? request_params_->tag->tag_id : 0,
      request_params_->referer_hostname,
      creative->content_categories);
  }

  RevenueDecimal
  CTRProvider::Calculation::adapt_ctr_(double ctr)
    /*throw(Overflow)*/
  {
    if(ctr < DBL_MIN) // prevent sub normal states (FP_ZERO, FP_SUBNORMAL)
    {
      return RevenueDecimal::ZERO;
    }

    return Generics::convert_float<RevenueDecimal>(ctr);
  }

  // CTRProvider
  CTRProvider::CTRProvider(
    const String::SubString& directory,
    const Generics::Time& config_timestamp,
    Generics::TaskRunner* task_runner)
    /*throw(Exception)*/
    : empty_hash_array_(),
      config_timestamp_(config_timestamp),
      task_runner_(ReferenceCounting::add_ref(task_runner)),
      remove_config_files_at_destroy_(false)
  {
    load_(directory, JSON_CONFIG_FILE_NAME);
  }

  CTRProvider::~CTRProvider() noexcept
  {
    try
    {
      clear_config_files_();
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << "CTRProvider::~CTRProvider(): " << ex.what() << std::endl;
    }
  }

  CTRProvider::Calculation_var
  CTRProvider::create_calculation(
    const CampaignSelectParams* request_params)
    const
    noexcept
  {
    return new CTRProvider::Calculation(this, request_params);
  }

  void
  CTRProvider::remove_config_files_at_destroy(bool val)
    const noexcept
  {
    RemoveFilesSyncPolicy::WriteGuard lock(remove_config_files_lock_);
    remove_config_files_at_destroy_ = val;
  }

  Generics::Time
  CTRProvider::check_config_appearance(
    std::string& config_root,
    const String::SubString& check_root)
    /*throw(Exception)*/
  {
    const char FOLDER_NAME_FORMAT[] = "%Y%m%d.%H%M%S";
    CTRConfigSelector ctr_config_selector;

    std::string check_root_s = check_root.str();

    Generics::DirSelect::directory_selector(
      check_root_s.c_str(),
      ctr_config_selector,
      "*",
      Generics::DirSelect::DSF_NON_RECURSIVE | Generics::DirSelect::DSF_ALL_FILES);

    if(!ctr_config_selector.result_folder.empty())
    {
      config_root = check_root_s + "/" + ctr_config_selector.result_folder;
      try
      {
        return Generics::Time(ctr_config_selector.result_folder, FOLDER_NAME_FORMAT);
      }
      catch(const eh::Exception& ex)
      {}
    }

    config_root.clear();
    return Generics::Time::ZERO;
  }

  void
  CTRProvider::load_campaign_list_(
    CampaignIdSet& campaigns,
    const String::SubString& campaigns_file_path)
    /*throw(InvalidConfig)*/
  {
    // parse campaign list file
    std::ifstream campaigns_file(campaigns_file_path.str().c_str());
    if(!campaigns_file.is_open())
    {
      Stream::Error ostr;
      ostr << "Can't open '" << campaigns_file_path << "'";
      throw InvalidConfig(ostr);
    }

    while(!campaigns_file.eof())
    {
      std::string str;
      std::getline(campaigns_file, str);
      unsigned long campaign_id;
      if(String::StringManip::str_to_int(str, campaign_id))
      {
        campaigns.insert(campaign_id);
      }
    }

    config_files_.push_back(campaigns_file_path.str());
  }

  std::unique_ptr<CTR::HashMap>
  CTRProvider::load_hash_mapping_(
    const String::SubString& hash_file_path)
    /*throw(InvalidConfig)*/
  {
    std::unique_ptr<CTR::HashMap> res(new CTR::HashMap());

    std::ifstream hash_file(hash_file_path.str().c_str());
    if(!hash_file.is_open())
    {
      Stream::Error ostr;
      ostr << "Can't open '" << hash_file_path << "'";
      throw InvalidConfig(ostr);
    }

    while(!hash_file.eof())
    {
      std::string line;
      std::getline(hash_file, line);

      if(!line.empty())
      {
        String::StringManip::Splitter<
          String::AsciiStringManip::SepComma> tokenizer(line);

        String::SubString orig_hash_str;
        String::SubString result_hash_str;
        uint32_t orig_hash;
        uint32_t result_hash;

        if(!tokenizer.get_token(orig_hash_str) ||
          !tokenizer.get_token(result_hash_str) ||
          !String::StringManip::str_to_int(orig_hash_str, orig_hash) ||
          !String::StringManip::str_to_int(result_hash_str, result_hash))
        {
          Stream::Error ostr;
          ostr << "Invalid line in '" << hash_file_path << "': line ='" << line << "'";
          throw InvalidConfig(ostr);
        }

        res->insert(std::make_pair(orig_hash, result_hash));
      }
    }

    config_files_.push_back(hash_file_path.str());

    return res;
  }

  struct FeatureArrayWrapper
  {
    FeatureArrayWrapper(const CTR::HashArray& hash_array)
      : hash_array_(hash_array)
    {}

    std::pair<bool, uint32_t>
    get(uint32_t feature_id) const
    {
      if(std::binary_search(
           hash_array_.begin(),
           hash_array_.end(),
           feature_id,
           Vanga::FeatureLess()))
      {
        return std::make_pair(true, 1);
      }
      else
      {
        return std::make_pair(false, 0);
      }
    }

  protected:
    const CTR::HashArray& hash_array_;
  };

  // util methods
  void
  CTRProvider::clear_config_files_() /*throw(Exception)*/
  {
    bool remove_config_files_at_destroy;

    {
      RemoveFilesSyncPolicy::ReadGuard lock(remove_config_files_lock_);
      remove_config_files_at_destroy = remove_config_files_at_destroy_;
    }

    if(remove_config_files_at_destroy && task_runner_)
    {
      task_runner_->enqueue_task(Generics::Task_var(
        new RemoveConfigTask(
          config_files_, config_directories_)));
    }
  }

  void
  CTRProvider::load_(
    const String::SubString& directory,
    const String::SubString& file)
    /*throw(InvalidConfig, Exception)*/
  {
    // preindex feature weight calculator creators

    // parse config files
    config_files_.clear();

    std::string directory_str;
    directory.assign_to(directory_str);

    std::string config_file = directory_str + "/" + file.str();

    ConfigParser::ConfigDescriptor_var config_descriptor =
      ConfigParserFactory::instance().parse(config_file.c_str());

    config_files_.push_back(config_file);

    // load data files
    if(config_descriptor->default_weight > 0)
    {
      non_campaign_algs_.algs.insert(
        std::make_pair(config_descriptor->default_weight, -1));
    }

    if(!config_descriptor->feature_mapping_file.empty())
    {
      hash_mapping_ = load_hash_mapping_(
        directory_str + "/" + config_descriptor->feature_mapping_file);
    }

    unsigned long global_model_id = 0;

    for(ConfigParser::AlgorithmDescriptorList::const_iterator alg_it =
          config_descriptor->algorithms.begin();
        alg_it != config_descriptor->algorithms.end(); ++alg_it)
    {
      if(alg_it->weight > 0)
      {
        Algorithm_var alg(new Algorithm());
        alg->id = alg_it->id;
        alg->threshold = alg_it->threshold;

        for(ConfigParser::ModelDescriptorList::const_iterator model_it =
              alg_it->models.begin();
            model_it != alg_it->models.end(); ++model_it)
        {
          Model_var model(new Model(++global_model_id));
          model->method_name = model_it->method;

          if(model_it->method == "ftrl" || model_it->method.empty())
          {
            model->method_name = "ftrl";
            model->method = MM_FTRL;
          }
          else if(model_it->method == "xgboost")
          {
            model->method = MM_XGBOOST;
          }
          else if(model_it->method == "vanga")
          {
            model->method = MM_VANGA;
          }
          else if(model_it->method == "trivial")
          {
            model->method = MM_TRIVIAL;
          }
          else
          {
            Stream::Error ostr;
            ostr << "incorrect model method = '" << model_it->method << "'";
            throw InvalidConfig(ostr);
          }

          if(model->method != MM_TRIVIAL && (
               model_it->dimension == 0 || model_it->dimension >= 32))
          {
            Stream::Error ostr;
            ostr << "incorrect dimension value = " << model_it->dimension;
            throw InvalidConfig(ostr);
          }

          model->weight = model_it->weight;
          model->feature_weights.shifter = sizeof(uint32_t)*8 - model_it->dimension;
          model->dimension = model_it->dimension;
          model->feature_weights.resize(1, 0);

          const std::string model_file = directory_str + "/" + model_it->file;

          if(model->method == MM_FTRL)
          {
            // fill FTRL specific fields
            model->load_feature_weights(model_file);
          }
          else if(model->method == MM_XGBOOST)
          {
            // init XGBoost predictor pool
            try
            {
              model->xgboost_predictor_pool.reset(
                new CTR::XGBoostPredictorPool(model_file));
            }
            catch(const CTR::XGBoostPredictorPool::Exception& ex)
            {
              Stream::Error ostr;
              ostr << "Can't init XGBoost pool: " << ex.what();
              throw InvalidConfig(ostr);
            }
          }
          else if(model->method == MM_VANGA)
          {
            try
            {
              std::ifstream model_file_istr(model_file.c_str());
              Vanga::DTree_var regression_predictor =
                Vanga::DTree::load(model_file_istr);
              model->vanga_predictor =
                new Vanga::LogRegPredictor<Vanga::DTree>(regression_predictor);
            }
            catch(const eh::Exception& ex)
            {
              Stream::Error ostr;
              ostr << "Can't init Vanga predictor: " << ex.what();
              throw InvalidConfig(ostr);
            }
          }
          else if(model->method == MM_TRIVIAL)
          {
            try
            {
              using CtrPredictor = PredictorSvcs::BidCostPredictor::CtrPredictor;
              using CtrPredictor_var = PredictorSvcs::BidCostPredictor::CtrPredictor_var;

              CtrPredictor_var ctr_predictor(new CtrPredictor(logger_));
              ctr_predictor->load(model_file);
              model->trivial_predictor = ctr_predictor;
            }
            catch(const eh::Exception& ex)
            {
              Stream::Error ostr;
              ostr << "Can't init Trivial predictor: " << ex.what();
              throw InvalidConfig(ostr);
            }
          }
          else
          {
            assert(0);
          }

          config_files_.push_back(model_file);

          bool creative_dependent = false;

          for(FeatureArray::const_iterator feature_it = model_it->features.begin();
              feature_it != model_it->features.end(); ++feature_it)
          {
            if(!feature_it->basic_features.empty()) // skip feature
            {
              for(auto basic_feature_it = feature_it->basic_features.begin();
                basic_feature_it != feature_it->basic_features.end(); ++basic_feature_it)
              {
                creative_dependent |= (
                  *basic_feature_it == CTR::BF_CREATIVE_ID ||
                  *basic_feature_it == CTR::BF_CC_ID);
              }

              // optimize basic feature order for calculation
              if(model->method == MM_XGBOOST && feature_it->basic_features.size() == 1)
              {
                // preprocess direct features
                auto feature_id = *feature_it->basic_features.begin();
                if(feature_id == CTR::BF_HOUR)
                {
                  model->push_hour = true;
                }
                else if(feature_id == CTR::BF_WEEK_DAY)
                {
                  model->push_week_day = true;
                }
                else if(feature_id == CTR::BF_CAMPAIGN_FREQ_ID)
                {
                  model->push_campaign_freq = true;
                }
                else if(feature_id == CTR::BF_CAMPAIGN_FREQ_LOG_ID)
                {
                  model->push_campaign_freq_log = true;
                }
              }

              Feature new_feature(*feature_it);
              new_feature.hash_seed = eval_feature_hash_seed_(new_feature);

              CTR::FeatureCalculator_var feature_calculator;

              {
                // init final feature
                const FeatureDescriptor* feature_descriptor =
                  FeatureDescriptorResolver::instance().resolve(
                    *feature_it->basic_features.rbegin());
                assert(feature_descriptor); // feature must be validated before
                feature_calculator =
                  feature_descriptor->calculator_creator->create_final(
                    model->feature_weights);
              }

              for(CTR::BasicFeatureSet::const_reverse_iterator fit =
                    ++feature_it->basic_features.rbegin();
                  fit != feature_it->basic_features.rend(); ++fit)
              {
                const FeatureDescriptor* feature_descriptor =
                  FeatureDescriptorResolver::instance().resolve(*fit);
                assert(feature_descriptor); // feature must be validated before
                feature_calculator =
                  feature_descriptor->calculator_creator->create_delegate(
                    feature_calculator);
              }

              new_feature.feature_calculator = feature_calculator;

              model->features[feature_type_(new_feature)].push_back(new_feature);
            }
            else
            {
              /*
              std::cout << "skipped feature" << std::endl;
              */
            }
          }

          model->creative_dependent = creative_dependent;
          model->max_feature_type = FT_REQUEST;

          // normalize feature sets
          for(int i = 0; i < FT_MAX; ++i)
          {
            std::sort(
              model->features[i].begin(),
              model->features[i].end());
          }

          // eval max feature type
          for(int i = FT_MAX - 1; i >= 0; --i)
          {
            if(!model->features[i].empty())
            {
              model->max_feature_type = static_cast<FeatureType>(i);
              break;
            }
          }

          // eval feature_set_indexes
          for(int i = 0; i < FT_MAX; ++i)
          {
            model->feature_set_indexes[i] = eval_feature_set_index_(model->features[i]);
          }

          alg->models.push_back(model);
        }

        ctr_algorithms_.push_back(alg);

        unsigned long alg_index = ctr_algorithms_.size() - 1;

        if(alg_it->campaigns_whitelist_file.empty())
        {
          AlgsRef& algs_ref = non_campaign_algs_;
          if(algs_ref.algs.empty())
          {
            algs_ref.algs.insert(
              std::make_pair(alg_it->weight, alg_index));
          }
          else
          {
            algs_ref.algs.insert(std::make_pair(
              algs_ref.algs.rbegin()->first + alg_it->weight,
              alg_index));
          }
        }
        else
        {
          CampaignIdSet enabled_campaigns;
          load_campaign_list_(
            enabled_campaigns,
            directory_str + "/" + alg_it->campaigns_whitelist_file);

          for(CampaignIdSet::const_iterator cmp_it = enabled_campaigns.begin();
              cmp_it != enabled_campaigns.end(); ++cmp_it)
          {
            AlgsRef& algs_ref = campaign_algs_[*cmp_it];

            if(algs_ref.algs.empty())
            {
              algs_ref.algs.insert(
                std::make_pair(alg_it->weight, alg_index));
            }
            else
            {
              algs_ref.algs.insert(std::make_pair(
                algs_ref.algs.rbegin()->first + alg_it->weight,
                alg_index));
            }
          }
        }
      }
    }

    for(AlgsRefByCampaignIdMap::iterator alg_ref_it =
          campaign_algs_.begin();
        alg_ref_it != campaign_algs_.end(); ++alg_ref_it)
    {
      assert(!alg_ref_it->second.algs.empty());
      unsigned long sum_weight = alg_ref_it->second.algs.rbegin()->first;

      for(AlgIdMap::const_iterator nalg_ref_it =
            non_campaign_algs_.algs.begin();
          nalg_ref_it != non_campaign_algs_.algs.end(); ++nalg_ref_it)
      {
        alg_ref_it->second.algs.insert(std::make_pair(
          sum_weight + nalg_ref_it->first,
          nalg_ref_it->second));
      }
    }

    // remove disabled campaigns from alg lists
    long alg_index = 0;

    for(ConfigParser::AlgorithmDescriptorList::const_iterator alg_it =
          config_descriptor->algorithms.begin();
        alg_it != config_descriptor->algorithms.end();
        ++alg_it, ++alg_index)
    {
      if(alg_it->weight > 0 && !alg_it->campaigns_blacklist_file.empty())
      {
        CampaignIdSet disabled_campaigns;

        load_campaign_list_(
          disabled_campaigns,
          directory_str + "/" + alg_it->campaigns_blacklist_file);

        for(CampaignIdSet::const_iterator dcmp_it = disabled_campaigns.begin();
            dcmp_it != disabled_campaigns.end(); ++dcmp_it)
        {
          unsigned long campaign_id = *dcmp_it;
          AlgsRefByCampaignIdMap::iterator alg_ref_it = campaign_algs_.find(campaign_id);
          if(alg_ref_it == campaign_algs_.end())
          {
            // create node with all algs excluding current
            alg_ref_it = campaign_algs_.insert(
              std::make_pair(campaign_id, non_campaign_algs_)).first;
          }

          unsigned long sub_weight = 0;
          std::list<std::pair<unsigned long, unsigned long> > change_algs;
          AlgsRef& args_ref = alg_ref_it->second;

          for(AlgIdMap::iterator it = args_ref.algs.begin();
            it != args_ref.algs.end(); ++it)
          {
            if(it->second == alg_index)
            {
              sub_weight = it->first;
              if(it != args_ref.algs.begin())
              {
                sub_weight -= (--AlgIdMap::const_iterator(it))->first;
              }
              args_ref.algs.erase(it++);
              std::copy(it, args_ref.algs.end(), std::back_inserter(change_algs));
              args_ref.algs.erase(it, args_ref.algs.end());
              break;
            }
          }

          for(auto it = change_algs.begin(); it != change_algs.end(); ++it)
          {
            args_ref.algs.insert(std::make_pair(it->first - sub_weight, it->second));
          }
        }
      }
    }

    // normalize alg ref arrays for common sum weight base
    for(AlgsRefByCampaignIdMap::iterator alg_ref_it =
          campaign_algs_.begin();
        alg_ref_it != campaign_algs_.end(); ++alg_ref_it)
    {
      normalize_algs_ref_(alg_ref_it->second);
    }

    normalize_algs_ref_(non_campaign_algs_);

    //
    config_directories_.push_back(directory_str);
  }

  void
  CTRProvider::normalize_algs_ref_(AlgsRef& algs_ref)
    noexcept
  {
    if(!algs_ref.algs.empty())
    {
      unsigned long max_weight = algs_ref.algs.rbegin()->first;
      assert(max_weight != 0);

      AlgIdMap new_algs;
      for(AlgIdMap::const_iterator it = algs_ref.algs.begin();
          it != algs_ref.algs.end(); ++it)
      {
        unsigned long new_weight = static_cast<uint64_t>(it->first) *
          WEIGHT_NORM_SUM / max_weight;

        if(new_weight > 0)
        {
          if(!new_algs.empty())
          {
            new_algs.insert(std::make_pair(
              new_weight,
              it->second));
          }
          else
          {
            new_algs.insert(std::make_pair(new_weight, it->second));
          }
        }
      }

      algs_ref.algs.swap(new_algs);
    }
  }

  CTRProvider::FeatureType
  CTRProvider::feature_type_(
    const Feature& feature) noexcept
  {
    for(CTR::BasicFeatureSet::const_iterator fit = feature.basic_features.begin();
        fit != feature.basic_features.end(); ++fit)
    {
      if(*fit >= CTR::BF_ARRAY_CANDIDATE_LEVEL_FIRST_ID ||
         (*fit >= CTR::BF_CANDIDATE_LEVEL_FIRST_ID && *fit < CTR::BF_ARRAY_REQUEST_LEVEL_FIRST_ID))
      {
        return FT_CANDIDATE;
      }
    }

    for(CTR::BasicFeatureSet::const_iterator fit = feature.basic_features.begin();
        fit != feature.basic_features.end(); ++fit)
    {
      if(*fit >= CTR::BF_ARRAY_AUCTION_LEVEL_FIRST_ID ||
         (*fit >= CTR::BF_AUCTION_LEVEL_FIRST_ID && *fit < CTR::BF_ARRAY_REQUEST_LEVEL_FIRST_ID))
      {
        return FT_AUCTION;
      }
    }

    return FT_REQUEST;
  }

  std::size_t
  CTRProvider::eval_feature_hash_seed_(
    const Feature& feature) noexcept
  {
    std::size_t res_hash;

    {
      Generics::Murmur32v3Hash hash(res_hash);
      for(CTR::BasicFeatureSet::const_iterator fit =
            feature.basic_features.begin();
          fit != feature.basic_features.end(); ++fit)
      {
        hash_add(hash, static_cast<unsigned char>(*fit));
      }
    }

    return res_hash;
  }

  uint64_t
  CTRProvider::eval_feature_set_index_(const FeatureArray& features)
    noexcept
  {
    if(features.empty())
    {
      return 0;
    }

    auto feature_set_it = feature_set_indexes_.find(features);
    if(feature_set_it != feature_set_indexes_.end())
    {
      return feature_set_it->second;
    }
    else
    {
      unsigned long new_index = feature_set_indexes_.size() + 1;
      feature_set_indexes_.insert(
        std::make_pair(features, new_index));
      return new_index;
    }
  }
}
}
