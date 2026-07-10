#pragma once

#include "CTRProvider.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  class CTRProviderImpl final: public CTRProvider
  {
  public:
    using Calculation_var = CTRProvider::Calculation_var;
    using CalculationContext_var = CTRProvider::CalculationContext_var;
    using CTRProvider_var = CTRProvider::CTRProvider_var;
    using ConstCTRProvider_var = CTRProvider::ConstCTRProvider_var;

    class Calculation;
    class CalculationContext;

  public:
    using AlgorithmArray = std::vector<Algorithm_var>;
    using AlgIdMap = std::map<unsigned long, long>;

    struct AlgsRef
    {
      void
      print(std::ostream& out) const noexcept;

      AlgIdMap algs;
    };

    using AlgsRefByCampaignIdMap = std::map<unsigned long, AlgsRef>;

    struct ModelValue
    {
      ModelValue() = default;
      ModelValue(ModelValue&& init);

      Commons::Optional<RevenueDecimal> ctr;
      float request_features_weight;
    };

    using ModelValueList = std::list<ModelValue>;

    struct AlgorithmValue
    {
      ModelValueList model_values;
    };

    using AlgorithmValueArray = std::vector<AlgorithmValue>;
    using RemoveFilesSyncPolicy = Sync::Policy::PosixThreadRW;
    using CampaignIdSet = std::set<unsigned long>;

    struct HashArrayHolder:
      public HashArray,
      public ReferenceCounting::DefaultImpl<>
    {
    protected:
      ~HashArrayHolder() noexcept override = default;
    };

    using HashArrayHolder_var = ReferenceCounting::SmartPtr<HashArrayHolder>;

    using FeatureSetLevelEvalHashMap = Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>, HashArrayHolder_var>;

    using FeatureSetLevelEvalWeightMap = Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>, float>;

    using ModelCTRMap = Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>, RevenueDecimal>;

    using Priority = float;

    class CalculationContext final: public CTRProvider::CalculationContext
    {
    public:
      using CTRList = CTRProvider::CalculationContext::CTRList;

      CalculationContext(
        const Calculation* calculation,
        const Tag::Size* tag_size)
        noexcept;

      RevenueDecimal
      get_ctr(const Creative* creative) const;

      bool
      check_rate(
        const Creative* creative,
        RevenueDecimal* rate = 0,
        bool* creative_dependent = 0) const;

      void
      get_ctr_details(
        CTRList& ctrs,
        const Creative* creative) const;

      void
      get_xgb_hashes_i(
        HashArray& res,
        const Creative* creative) const
        noexcept;

    protected:
      ~CalculationContext() noexcept override;

      std::pair<RevenueDecimal, const Algorithm*>
      get_ctr_(
        const Creative* creative,
        bool* creative_dependent) const;

      RevenueDecimal
      get_model_ctr_(
        const Algorithm& algorithm,
        const Model& model,
        const Creative* creative) const;

      HashArrayHolder_var
      get_features_hashes_(
        const Algorithm& algorithm,
        const Model& model,
        FeatureType feature_type,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative) const
        noexcept;

      float
      get_features_weight_(
        const Algorithm& algorithm,
        const Model& model,
        FeatureType feature_type,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative) const
        noexcept;

      HashArrayHolder_var
      check_feature_set_level_eval_hashes_(
        FeatureType feature_type,
        unsigned long feature_set_index) const
        noexcept;

      void
      insert_feature_set_level_eval_hashes_(
        FeatureType feature_type,
        unsigned long feature_set_index,
        HashArrayHolder* hashes) const
        noexcept;

      bool
      check_feature_set_level_eval_weights_(
        FeatureType feature_type,
        float& weight) const
        noexcept;

      void
      insert_feature_set_level_eval_weights_(
        FeatureType feature_type,
        float weight) const
        noexcept;

    private:
      const ReferenceCounting::ConstPtr<Calculation> calculation_;
      const ReferenceCounting::ConstPtr<Tag::Size> tag_size_;
      mutable HashArray opt_hashes_;
      mutable FeatureSetLevelEvalHashMap feature_set_level_eval_hashes_;
      mutable FeatureSetLevelEvalWeightMap feature_set_level_eval_weights_;
      mutable ModelCTRMap model_ctrs_;
    };

    using CalculationContextImpl_var =
      ReferenceCounting::SmartPtr<CalculationContext>;

    class Calculation final: public CTRProvider::Calculation
    {
      friend class CTRProviderImpl;
      friend class CalculationContext;

    public:
      CalculationContext_var
      create_context(const Tag::Size* tag_size) const
        noexcept override;

      std::string
      algorithm_id(const Creative* creative) const
        noexcept override;

    protected:
      Calculation(
        const CTRProviderImpl* ctr_provider,
        const CampaignSelectParams* request_params)
        noexcept;

      ~Calculation() noexcept override;

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
        HashArray& hashes,
        const Algorithm& algorithm,
        const Model& model,
        FeatureType feature_type,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative) const
        noexcept;

      static RevenueDecimal
      eval_ctr_(float weight);

      static RevenueDecimal
      adapt_ctr_(double ctr);

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
      ReferenceCounting::ConstPtr<CTRProviderImpl> ctr_provider_;
      CCampaignSelectParams_var request_params_;
      mutable FeatureSetLevelEvalHashMap feature_set_level_eval_hashes_;
      mutable FeatureSetLevelEvalWeightMap feature_set_level_eval_weights_;
      mutable ModelCTRMap model_ctrs_;
    };

    using CalculationImpl_var = ReferenceCounting::SmartPtr<Calculation>;

  public:
    CTRProviderImpl(
      const String::SubString& directory,
      const Generics::Time& config_timestamp,
      Generics::TaskRunner* task_runner);

    Calculation_var
    create_calculation(
      const CampaignSelectParams* request_params) const
      noexcept override;

    Generics::Time
    config_timestamp() const noexcept
    {
      return config_timestamp_;
    }

    static Generics::Time
    check_config_appearance(
      std::string& config_root,
      const String::SubString& check_root);

    void
    remove_config_files_at_destroy(bool val) const noexcept;

  protected:
    class ConfigParser;

    using ConfigParser_var = ReferenceCounting::SmartPtr<ConfigParser>;
    using ConfigParserFactory =
      Generics::Singleton<ConfigParser, ConfigParser_var>;
    using FileList = std::list<std::string>;

    struct FeatureArrayLess
    {
      bool
      operator()(const FeatureArray& left, const FeatureArray& right) const
      {
        return std::lexicographical_compare(
          left.begin(),
          left.end(),
          right.begin(),
          right.end());
      }
    };

    using FeatureSetIndexMap =
      std::map<FeatureArray, unsigned long, FeatureArrayLess>;

  protected:
    ~CTRProviderImpl() noexcept override;

    void
    load_(
      const String::SubString& directory,
      const String::SubString& file);

    static FeatureType
    feature_type_(const Feature& feature) noexcept;

    std::size_t
    eval_feature_hash_seed_(const Feature& feature) noexcept;

    static void
    normalize_algs_ref_(AlgsRef& args_ref) noexcept;

    void
    load_campaign_list_(
      CampaignIdSet& campaigns,
      const String::SubString& file_name);

    std::unique_ptr<HashMap>
    load_hash_mapping_(const String::SubString& file_name);

    void
    clear_config_files_();

    uint64_t
    eval_feature_set_index_(const FeatureArray& features) noexcept;

  protected:
    const HashArray empty_hash_array_;
    const Generics::Time config_timestamp_;
    Generics::TaskRunner_var task_runner_;
    std::unique_ptr<HashMap> hash_mapping_;
    AlgorithmArray ctr_algorithms_;
    AlgsRefByCampaignIdMap campaign_algs_;
    AlgsRef non_campaign_algs_;
    Calculation_var default_calculation_;
    FileList config_files_;
    FileList config_directories_;
    mutable RemoveFilesSyncPolicy::Mutex remove_config_files_lock_;
    mutable bool remove_config_files_at_destroy_;
    FeatureSetIndexMap feature_set_indexes_;
  };

  using CTRProviderImpl_var = ReferenceCounting::SmartPtr<CTRProviderImpl>;
  using ConstCTRProviderImpl_var =
    ReferenceCounting::SmartPtr<const CTRProviderImpl>;

  inline
  CTRProviderImpl::ModelValue::ModelValue(CTRProviderImpl::ModelValue&& init)
  {
    ctr = std::move(init.ctr);
    request_features_weight = init.request_features_weight;
  }
}
