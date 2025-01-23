#ifndef CAMPAIGNMANAGER_CTRPROVIDER_HPP
#define CAMPAIGNMANAGER_CTRPROVIDER_HPP

#include <optional>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Hash.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/TaskRunner.hpp>
#include <Sync/SyncPolicy.hpp>

#include <DTree/DTree.hpp>
#include <DTree/LogRegPredictor.hpp>
#include <DTree/FastFeatureSet.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <PredictorSvcs/BidCostPredictor/CtrPredictor.hpp>

#include "CampaignSelectParams.hpp"
#include "CTRFeatureCalculators.hpp"
#include "XGBoostPredictor.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  using namespace AdInstances;

  /*
    Publisher ID
    Site ID
    Tag ID
    External Tag ID (provided by publisher)
    Domain of the referrer
    Full referrer
    User Channels (i.e. behavioural channels in user profile)Yes
    Hours since midnight - [0,23], UTC
    Week day - days since Sunday, [0, 6]
    ISP ID
    Colocation ID
    Device Channel ID (most specific)
    Geo Channel ID for City
    Geo Channel ID for Region

    Advertiser ID
    Campaign ID
    Campaign Creative Group ID
    Creative Protocol Size (728x90)

    Campaign Creative ID 3.4+
    Creative ID 3.4+
    Name of Size Type of Creative (Banner, Overlay, Video etc) 3.4+
  */

  namespace CTR
  {
    enum BasicFeature
    {
      // request level features
      BF_PUBLISHER_ID = 1,
      BF_SITE_ID = 2,
      BF_TAG_ID = 3,
      BF_ETAG_ID = 4,
      BF_DOMAIN = 5,
      BF_URL = 6,
      BF_HOUR = 7,
      BF_WEEK_DAY = 8,
      BF_ISP_ID = 9,
      BF_COLO_ID = 10,
      BF_DEVICE_CHANNEL_ID = 11,
      BF_AFTER_HOUR = 12,
      BF_VISIBILITY = 13,
      BF_PREDICTED_VIEWABILITY = 14,

      // auction level features
      BF_AUCTION_LEVEL_FIRST_ID = 50,
      BF_SIZE_TYPE_ID = 50,
      BF_SIZE_ID = 51,

      // candidate level features
      BF_CANDIDATE_LEVEL_FIRST_ID = 100,
      BF_ADVERTISER_ID = 100,
      BF_CAMPAIGN_ID = 101,
      BF_CCG_ID = 102,
      BF_CAMPAIGN_FREQ_ID = 105,
      BF_CAMPAIGN_FREQ_LOG_ID = 106,

      BF_CREATIVE_LEVEL_FIRST_ID = 130,
      BF_CREATIVE_ID = 130,
      BF_CC_ID = 131, // non request feature

      // candidate level, array features
      BF_ARRAY_REQUEST_LEVEL_FIRST_ID = 150,
      BF_HISTORY_CHANNELS = 150,
      BF_GEO_CHANNELS = 151,

      BF_ARRAY_AUCTION_LEVEL_FIRST_ID = 152,
      BF_ARRAY_CANDIDATE_LEVEL_FIRST_ID = 152,
      BF_CONTENT_CATEGORIES = 152,
      BF_VISUAL_CATEGORIES = 153,
    };

    typedef std::set<BasicFeature> BasicFeatureSet;

    class FeatureNameResolver
    {
    public:
      FeatureNameResolver() noexcept;

      bool
      basic_feature_by_name(
        BasicFeature& basic_feature,
        const String::SubString& name)
        const
        noexcept;

    protected:
      typedef Generics::GnuHashTable<
        Generics::SubStringHashAdapter, BasicFeature>
        FeatureNameMap;

    protected:
      FeatureNameMap feature_names_;
    };
  }

  class FastFeatureSetHolder;

  typedef ReferenceCounting::SmartPtr<FastFeatureSetHolder>
    FastFeatureSetHolder_var;

  class CTRProvider: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidConfig, Exception);
    DECLARE_EXCEPTION(Overflow, Exception);

    class Calculation;
    class CalculationContext;

  protected:
    struct Feature
    {
      void
      print(std::ostream& out) const;

      bool
      operator<(const Feature& right) const;

      CTR::BasicFeatureSet basic_features;
      std::size_t hash_seed;
      CTR::FeatureCalculator_var feature_calculator;
    };

    typedef std::vector<Feature> FeatureArray;

    enum FeatureType
    {
      FT_REQUEST = 0,
      FT_AUCTION,
      FT_CANDIDATE,
      FT_MAX
    };

    enum ModelMethodType
    {
      MM_FTRL,
      MM_XGBOOST,
      MM_VANGA,
      MM_TRIVIAL
    };

    class Model: public ReferenceCounting::AtomicImpl
    {
      friend class Calculation;
      friend class CalculationContext;

    public:
      Model(unsigned long model_id) noexcept;

      void
      load_feature_weights(
        const String::SubString& file)
        /*throw(InvalidConfig)*/;

    public:
      const unsigned long model_id;
      ModelMethodType method;
      std::string method_name;
      RevenueDecimal weight;

      // FT_REQUEST: features can be calculated once for request (
      //   at CTRProvider::create_calculation)
      // FT_AUCTION: features can be calculated only when know context (tag size)
      //   but once for all campaigns/creatives
      // FT_CANDIDATE: features can be calculated only when known candidate - campaign/creative (
      //   at CalculationContext::get_ctr
      FeatureArray features[FT_MAX];

      // TODO !!! Fill
      uint64_t feature_set_indexes[FT_MAX];
      FeatureType max_feature_type;

      // FTRL specific fields
      unsigned long dimension;
      // (sign * m.L1 - m.z[feature]) /
      //   ((m.beta + m.n[feature]) / m.alpha + m.L2)
      CTR::FeatureWeightTable feature_weights;

      // XGBoost specific fields
      CTR::XGBoostPredictorPool_var xgboost_predictor_pool;
      bool push_hour;
      bool push_week_day;
      bool push_campaign_freq;
      bool push_campaign_freq_log;
      bool creative_dependent;

      // Vanga fields
      Gears::IntrusivePtr<Vanga::LogRegPredictor<Vanga::DTree> > vanga_predictor;

      // Trivial predictor fields
      PredictorSvcs::BidCostPredictor::CtrPredictor_var trivial_predictor;

    protected:
      virtual ~Model() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<Model> Model_var;

    typedef std::list<Model_var> ModelList;

    struct Algorithm: public ReferenceCounting::AtomicImpl
    {
      friend class Calculation;
      friend class CalculationContext;

      Algorithm()
        : threshold(RevenueDecimal::ZERO)
      {}

      std::string id;
      unsigned long weight;
      ModelList models;
      RevenueDecimal threshold;

    protected:
      virtual ~Algorithm() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<const Algorithm>
      ConstAlgorithm_var;
    typedef ReferenceCounting::SmartPtr<Algorithm>
      Algorithm_var;

    typedef std::vector<Algorithm_var>
      AlgorithmArray;

    typedef std::map<unsigned long, long> AlgIdMap;

    struct AlgsRef
    {
      void
      print(std::ostream& out) const noexcept;

      AlgIdMap algs; // normalized to sum weight
      //unsigned long sum_weight;
    };

    typedef std::map<unsigned long, AlgsRef>
      AlgsRefByCampaignIdMap;

    // keep request defined feature values
    struct ModelValue
    {
      ModelValue() {}

      ModelValue(ModelValue&& init);

      // some features can not use Campaign/Creative basic features
      std::optional<RevenueDecimal> ctr;

      // FTRL weight
      float request_features_weight; // ??

      // XGBoost
      mutable CTR::XGBoostPredictorPool::Predictor_var xgboost_predictor;
    };

    typedef std::list<ModelValue> ModelValueList;

    struct AlgorithmValue
    {
      ModelValueList model_values;
    };

    typedef std::vector<AlgorithmValue> AlgorithmValueArray;

    typedef Sync::Policy::PosixThreadRW RemoveFilesSyncPolicy;

    typedef std::set<unsigned long> CampaignIdSet;

  protected:
    struct HashArrayHolder:
      public CTR::HashArray,
      public ReferenceCounting::DefaultImpl<>
    {
    protected:
      virtual ~HashArrayHolder() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<HashArrayHolder>
      HashArrayHolder_var;

    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>, HashArrayHolder_var>
      FeatureSetLevelEvalHashMap;

    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>, float>
      FeatureSetLevelEvalWeightMap;

    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>, RevenueDecimal>
      ModelCTRMap;

    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>,
      CTR::XGBoostPredictorPool::Predictor_var>
      ModelXGBoostPredictorMap;

  public:
    typedef float Priority;

    class Calculation;

    // keep second level context of ctr estimation (tag size)
    class CalculationContextBase: public ReferenceCounting::AtomicImpl
    {
      virtual RevenueDecimal
      get_ctr(const Creative* creative) const = 0;
    };

    // complex CalculationContext
    class CalculationContext: public CalculationContextBase
    {
    public:
      typedef std::list<RevenueDecimal> CTRList;

    public:
      CalculationContext(
        const Calculation* calculation,
        const Tag::Size* tag_size)
        noexcept;

      RevenueDecimal
      get_ctr(const Creative* creative) const
        /*throw(Overflow)*/;

      bool
      check_rate(
        const Creative* creative,
        RevenueDecimal* rate = 0,
        bool* creative_dependent = 0) const
        /*throw(Overflow)*/;

      void
      get_ctr_details(
        CTRList& ctrs,
        const Creative* creative) const
        /*throw(Overflow)*/;

      void
      get_xgb_hashes_i(
        CTR::HashArray& res,
        const Creative* creative) const
        noexcept;

    protected:
      virtual ~CalculationContext() noexcept;

      std::pair<RevenueDecimal, const Algorithm*>
      get_ctr_(
        const Creative* creative,
        bool* creative_dependent) const
        /*throw(Overflow)*/;

      RevenueDecimal
      get_model_ctr_(
        const Algorithm& algorithm,
        const Model& model,
        const Creative* creative)
        const /*throw(Overflow)*/;

      // Vanga & XGBoost specific methods
      CTRProvider::HashArrayHolder_var
      get_features_hashes_(
        const Algorithm& algorithm,
        const Model& model,
        FeatureType feature_type,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        const
        noexcept;

      // FTRL specific methods
      float
      get_features_weight_(
        const Algorithm& algorithm,
        const Model& model,
        FeatureType feature_type,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        const
        noexcept;

      HashArrayHolder_var
      check_feature_set_level_eval_hashes_(
        FeatureType feature_type,
        unsigned long feature_set_index)
        const
        noexcept;

      void
      insert_feature_set_level_eval_hashes_(
        FeatureType feature_type,
        unsigned long feature_set_index,
        HashArrayHolder* hashes)
        const
        noexcept;

      bool
      check_feature_set_level_eval_weights_(
        FeatureType feature_type,
        float& weight)
        const
        noexcept;

      void
      insert_feature_set_level_eval_weights_(
        FeatureType feature_type,
        float weight)
        const
        noexcept;

    private:
      static
      RevenueDecimal
      eval_ctr_(float weight) /*throw(Overflow)*/;

    private:
      const ReferenceCounting::ConstPtr<Calculation> calculation_;
      const ReferenceCounting::ConstPtr<Tag::Size> tag_size_;

      //AlgorithmValueArray algorithm_values_;
      mutable CTR::HashArray opt_hashes_;

      //ModelValueList model_values_;

      // XGBoost & Vanga & Trivial
      mutable FeatureSetLevelEvalHashMap feature_set_level_eval_hashes_;

      // FTRL
      mutable FeatureSetLevelEvalWeightMap feature_set_level_eval_weights_;

      // cached result ctr's
      mutable ModelCTRMap model_ctrs_;
    };

    typedef ReferenceCounting::SmartPtr<CalculationContext>
      CalculationContext_var;

    // keep first level context of ctr estimation
    class Calculation: public ReferenceCounting::AtomicImpl
    {
      friend class CTRProvider;
      friend class CalculationContext;

    public:
      CalculationContext_var
      create_context(const Tag::Size* tag_size) const
        noexcept;

      std::string
      algorithm_id(const Creative* creative) const
        noexcept;

    protected:
      Calculation(
        const CTRProvider* ctr_provider,
        const CampaignSelectParams* request_params)
        noexcept;

      virtual
      ~Calculation() noexcept;

      static long
      select_alg_(
        const AlgsRef& alg_ref,
        unsigned long rand)
        noexcept;

      long
      select_alg_index_(const Creative* creative) const
        noexcept;

      void
      eval_features_hashes_(
        CTR::HashArray& hashes,
        const Algorithm& algorithm,
        const Model& model,
        FeatureType feature_type,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        const
        noexcept;

      float
      eval_features_weight_(
        const Algorithm& algorithm,
        const Model& model,
        FeatureType feature_type,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        const
        noexcept;

      static RevenueDecimal
      eval_ctr_(float weight)
        /*throw(Overflow)*/;

      RevenueDecimal
      xgboost_eval_ctr_(
        const Model& model,
        const CTR::HashArray& hashes,
        const CTR::HashArray* add_hashes1 = 0,
        const CTR::HashArray* add_hashes2 = 0,
        const CTR::HashArray* add_hashes3 = 0)
        const
        /*throw(Overflow)*/;

      RevenueDecimal
      vanga_eval_ctr_(
        const Model& model,
        const CTR::HashArray& hashes,
        const CTR::HashArray* add_hashes1 = 0,
        const CTR::HashArray* add_hashes2 = 0)
        const
        /*throw(Overflow)*/;

      RevenueDecimal
      trivial_eval_ctr_(
        const Model& model,
        const Creative* creative) const;

      static RevenueDecimal
      adapt_ctr_(double ctr)
        /*throw(Overflow)*/;

    protected:
      typedef ReferenceCounting::SmartPtr<FastFeatureSetHolder>
        FastFeatureSetHolder_var;

      struct FeatureSetLevelHashAdapter
      {
      public:
        FeatureSetLevelHashAdapter(
          uint64_t feature_set_hash,
          FeatureType feature_type)
          : feature_set_hash_(feature_set_hash),
            feature_type_(feature_type)
        {
          Generics::Murmur64Hash hasher(hash_);
          hash_add(hasher, feature_set_hash);
          hash_add(hasher, static_cast<unsigned int>(feature_type));
        }

        bool
        operator==(const FeatureSetLevelHashAdapter& right) const
        {
          return feature_set_hash_ == right.feature_set_hash_ &&
            feature_type_ == right.feature_type_;
        }

        size_t hash() const
        {
          return hash_;
        }

      protected:
        uint64_t feature_set_hash_;
        FeatureType feature_type_;
        size_t hash_;
      };

    protected:
      const unsigned long rand_;
      ReferenceCounting::ConstPtr<CTRProvider> ctr_provider_;
      CCampaignSelectParams_var request_params_;

      //AlgorithmValueArray algorithm_values_;

      // XGBoost
      mutable CTR::XGBoostPredictorPool::Predictor_var xgboost_predictor_;
      mutable ModelXGBoostPredictorMap model_xgboost_predictors_;

      // XGBoost & Vanga
      mutable FeatureSetLevelEvalHashMap feature_set_level_eval_hashes_;

      // FTRL
      mutable FeatureSetLevelEvalWeightMap feature_set_level_eval_weights_;

      // cached result ctr's
      mutable ModelCTRMap model_ctrs_;
    };

    typedef ReferenceCounting::SmartPtr<Calculation>
      Calculation_var;

  public:
    CTRProvider(
      const String::SubString& directory,
      const Generics::Time& config_timestamp,
      Generics::TaskRunner* task_runner)
      /*throw(Exception)*/;

    // return null if must be used old CTR calculation logic
    Calculation_var
    create_calculation(
      const CampaignSelectParams* request_params)
      const
      noexcept;

    Generics::Time
    config_timestamp() const noexcept
    {
      return config_timestamp_;
    }

    static
    Generics::Time
    check_config_appearance(
      std::string& config_root,
      const String::SubString& check_root)
      /*throw(Exception)*/;

    void
    remove_config_files_at_destroy(bool val)
      const noexcept;

  protected:
    class ConfigParser;

    typedef ReferenceCounting::SmartPtr<ConfigParser>
      ConfigParser_var;

    typedef Generics::Singleton<ConfigParser, ConfigParser_var>
      ConfigParserFactory;

    typedef std::list<std::string> FileList;

    struct FeatureArrayLess
    {
      bool
      operator()(const FeatureArray& left, const FeatureArray& right)
        const
      {
        return std::lexicographical_compare(
          left.begin(),
          left.end(),
          right.begin(),
          right.end());
      }
    };

    typedef std::map<FeatureArray, unsigned long, FeatureArrayLess>
      FeatureSetIndexMap;

  protected:
    virtual
    ~CTRProvider() noexcept;

    void
    load_(
      const String::SubString& directory,
      const String::SubString& file)
      /*throw(InvalidConfig, Exception)*/;

    static FeatureType
    feature_type_(const Feature& feature) noexcept;

    std::size_t
    eval_feature_hash_seed_(
      const Feature& feature) noexcept;

    // util methods
    static void
    normalize_algs_ref_(AlgsRef& args_ref)
      noexcept;

    void
    load_campaign_list_(
      CampaignIdSet& campaigns,
      const String::SubString& file_name)
      /*throw(InvalidConfig)*/;

    std::unique_ptr<CTR::HashMap>
    load_hash_mapping_(
      const String::SubString& file_name)
      /*throw(InvalidConfig)*/;

    void
    clear_config_files_() /*throw(Exception)*/;

    uint64_t
    eval_feature_set_index_(const FeatureArray& features)
      noexcept;

  protected:
    const CTR::HashArray empty_hash_array_;
    const Generics::Time config_timestamp_;
    Generics::TaskRunner_var task_runner_;
    Logging::Logger_var logger_;

    std::unique_ptr<CTR::HashMap> hash_mapping_;

    // alg id(index) => algorithm
    AlgorithmArray ctr_algorithms_;
    // algorithms for concrete campaigns (no cross with non_campaign_algs_)
    AlgsRefByCampaignIdMap campaign_algs_;
    // algorithms
    AlgsRef non_campaign_algs_;

    Calculation_var default_calculation_;
    FileList config_files_;
    FileList config_directories_;

    mutable RemoveFilesSyncPolicy::Mutex remove_config_files_lock_;
    mutable bool remove_config_files_at_destroy_;

    FeatureSetIndexMap feature_set_indexes_;
  };

  typedef ReferenceCounting::SmartPtr<CTRProvider>
    CTRProvider_var;
  typedef ReferenceCounting::SmartPtr<const CTRProvider>
    ConstCTRProvider_var;
}
}

namespace AdServer
{
namespace CampaignSvcs
{
  inline
  CTRProvider::ModelValue::ModelValue(CTRProvider::ModelValue&& init)
  {
    ctr = std::move(init.ctr);
    request_features_weight = init.request_features_weight;
    xgboost_predictor.swap(init.xgboost_predictor);
    //hashes.swap(init.hashes);
  }
}
}

#endif /*CAMPAIGNMANAGER_CTRPROVIDER_HPP*/
