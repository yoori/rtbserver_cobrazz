#include <sstream>
#include "CTRGenerator.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  // Index calculations implementation
  // for independence in impl errors,
  // consciously, duplicated some functionality from CTRProvider
  // because we implement other side of protocol
  //

  namespace
  {
    template<typename Type>
    std::string
    value_to_string(const Type& val)
    {
      std::ostringstream ostr;
      ostr << val;
      return ostr.str();
    }

    std::string
    value_to_string(unsigned char val)
    {
      std::ostringstream ostr;
      ostr << static_cast<unsigned long>(val);
      return ostr.str();
    }

    // hash calculators
#   define DEFINE_FEATURE_HASH_FUN(NAME) \
    void NAME(CTR::Murmur32v3Adapter& hash, \
      const CTRGenerator::CalculateParams& calc_params)

    DEFINE_FEATURE_HASH_FUN(add_hash_publisher_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.publisher_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_site_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.site_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_tag_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.tag_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_etag_id_)
    {
      hash.add(calc_params.etag);
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_domain_)
    {
      hash.add(calc_params.domain);
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_url_)
    {
      hash.add(static_cast<uint32_t>(calc_params.referer_hash));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_advertiser_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.advertiser_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_campaign_id_)
    {
      // campaign_group_id = campaign_id in OIX termines
      hash.add(static_cast<uint32_t>(calc_params.campaign_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_ccg_id_)
    {
      // campaign_group_id = campaign_id in OIX termines
      hash.add(static_cast<uint32_t>(calc_params.ccg_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_size_type_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.size_type_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_size_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.size_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_hour_)
    {
      hash.add(static_cast<unsigned char>(calc_params.hour));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_week_day_)
    {
      hash.add(static_cast<unsigned char>(calc_params.wd));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_isp_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.isp_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_colo_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.colo_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_device_channel_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.device_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_creative_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.creative_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_cc_id_)
    {
      hash.add(static_cast<uint32_t>(calc_params.cc_id));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_campaign_freq_)
    {
      hash.add(static_cast<uint32_t>(calc_params.campaign_freq));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_campaign_freq_log_)
    {
      hash.add(static_cast<uint32_t>(calc_params.campaign_freq_log));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_tag_visibility_)
    {
      hash.add(static_cast<int32_t>(calc_params.tag_visibility));
    }

    DEFINE_FEATURE_HASH_FUN(add_hash_tag_predicted_viewability_)
    {
      hash.add(static_cast<int32_t>(calc_params.tag_predicted_viewability));
    }

    // value getters
#   define DEFINE_FEATURE_VALUE_FUN(NAME) \
    std::string NAME(const CTRGenerator::CalculateParams& calc_params)

    DEFINE_FEATURE_VALUE_FUN(get_value_publisher_id_)
    {
      return value_to_string(calc_params.publisher_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_site_id_)
    {
      return value_to_string(calc_params.site_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_tag_id_)
    {
      return value_to_string(calc_params.tag_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_etag_id_)
    {
      return calc_params.etag;
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_domain_)
    {
      return calc_params.domain;
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_url_)
    {
      return value_to_string(calc_params.referer_hash);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_advertiser_id_)
    {
      return value_to_string(calc_params.advertiser_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_campaign_id_)
    {
      return value_to_string(calc_params.campaign_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_ccg_id_)
    {
      return value_to_string(calc_params.ccg_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_size_type_id_)
    {
      return value_to_string(calc_params.size_type_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_size_id_)
    {
      return value_to_string(calc_params.size_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_hour_)
    {
      return value_to_string(calc_params.hour);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_week_day_)
    {
      return value_to_string(calc_params.wd);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_isp_id_)
    {
      return value_to_string(calc_params.isp_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_colo_id_)
    {
      return value_to_string(calc_params.colo_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_device_channel_id_)
    {
      return value_to_string(calc_params.device_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_creative_id_)
    {
      return value_to_string(calc_params.creative_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_cc_id_)
    {
      return value_to_string(calc_params.cc_id);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_campaign_freq_)
    {
      return value_to_string(calc_params.campaign_freq);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_campaign_freq_log_)
    {
      return value_to_string(calc_params.campaign_freq_log);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_tag_visibility_)
    {
      return value_to_string(calc_params.tag_visibility);
    }

    DEFINE_FEATURE_VALUE_FUN(get_value_tag_predicted_viewability_)
    {
      return value_to_string(calc_params.tag_predicted_viewability);
    }

    //
    // FeatureHashCalculator implementations
    template<
      void (*add_hash_fun)(
        CTR::Murmur32v3Adapter& hash,
        const CTRGenerator::CalculateParams& calc_params),
      std::string (*get_value_fun)(const CTRGenerator::CalculateParams& calc_params)>
    class FeatureHashCalculatorFinalImpl: public CTRGenerator::FeatureHashCalculator
    {
    public:
      FeatureHashCalculatorFinalImpl(const char* name)
        noexcept
        : name_(name)
      {}

      virtual void
      eval(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::Calculation& index_calculation,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        add_hash_fun(hash_adapter, calc_params);
        index_calculation.hashes.insert(
          index_calculation.hashes.end(),
          std::make_pair(hash_adapter.finalize(), 1));
      }

      virtual void
      fill_dictionary(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::FeatureDictionary& dictionary,
        const CTRGenerator::FeatureDictionary& global_dictionary,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        add_hash_fun(hash_adapter, calc_params);
        unsigned long hash_val = hash_adapter.finalize();
        if(global_dictionary.find(hash_val) == global_dictionary.end())
        {
          dictionary.insert(
            std::make_pair(hash_val, name_ + ":" + get_value_fun(calc_params)));
        }
      }

    protected:
      virtual ~FeatureHashCalculatorFinalImpl() noexcept
      {}

    protected:
      const std::string name_;
    };

    // FeatureHashCalculatorDelegateImpl
    template<void (*add_hash_fun)(
      CTR::Murmur32v3Adapter& hash,
      const CTRGenerator::CalculateParams& calc_params),
      std::string (*get_value_fun)(const CTRGenerator::CalculateParams& calc_params)>
    class FeatureHashCalculatorDelegateImpl:
      public CTRGenerator::FeatureHashCalculator
    {
    public:
      FeatureHashCalculatorDelegateImpl(
        FeatureHashCalculator* next_calculator,
        const char* name)
        noexcept
        : next_calculator_(ReferenceCounting::add_ref(next_calculator)),
          name_(name)
      {}

      virtual void
      eval(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::Calculation& index_calculation,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        add_hash_fun(hash_adapter, calc_params);
        next_calculator_->eval(
          hash_adapter,
          index_calculation,
          calc_params);
      }

      virtual void
      fill_dictionary(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::FeatureDictionary& dictionary,
        const CTRGenerator::FeatureDictionary& global_dictionary,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        CTRGenerator::FeatureDictionary local_dictionary;

        next_calculator_->fill_dictionary(
          hash_adapter,
          local_dictionary,
          global_dictionary,
          calc_params);

        if(!local_dictionary.empty())
        {
          for(auto dict_it = local_dictionary.begin();
            dict_it != local_dictionary.end(); ++dict_it)
          {
            dictionary.insert(
              std::make_pair(
                dict_it->first,
                name_ + ":" + get_value_fun(calc_params) + "," + dict_it->second));
          }
        }
      }

    protected:
      virtual ~FeatureHashCalculatorDelegateImpl() noexcept
      {}

    protected:
      CTRGenerator::FeatureHashCalculator_var next_calculator_;
      const std::string name_;
    };

    // FeatureWeightCalculatorChannelIdSetFinalImpl
    class FeatureHashCalculatorIdSetFinalImpl:
      public CTRGenerator::FeatureHashCalculator
    {
    public:
      FeatureHashCalculatorIdSetFinalImpl(
        CTRGenerator::IdSet CTRGenerator::CalculateParams::* field,
        const char* name)
        noexcept
        : field_(field),
          name_(name)
      {}

      virtual void
      eval(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::Calculation& index_calculation,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        if(!(calc_params.*field_).empty())
        {
          for(CTRGenerator::IdSet::const_iterator it = (calc_params.*field_).begin();
              it != (calc_params.*field_).end(); ++it)
          {
            // need local hasher
            CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(static_cast<uint32_t>(*it));
            unsigned long hash_val = hash_adapter_copy.finalize();
            index_calculation.hashes.insert(
              index_calculation.hashes.end(),
              std::make_pair(hash_val, 1));
          }
        }
        else
        {
          hash_adapter.add(static_cast<uint32_t>(0));
          unsigned long hash_val = hash_adapter.finalize();
          index_calculation.hashes.insert(
            index_calculation.hashes.end(),
            std::make_pair(hash_val, 1));
        }
      }

      virtual void
      fill_dictionary(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::FeatureDictionary& dictionary,
        const CTRGenerator::FeatureDictionary& global_dictionary,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        if(!(calc_params.*field_).empty())
        {
          for(CTRGenerator::IdSet::const_iterator it = (calc_params.*field_).begin();
              it != (calc_params.*field_).end(); ++it)
          {
            // need local hasher
            CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(static_cast<uint32_t>(*it));
            unsigned long hash_val = hash_adapter_copy.finalize();
            if(global_dictionary.find(hash_val) == global_dictionary.end())
            {
              dictionary.insert(
                std::make_pair(hash_val, name_ + ":" + value_to_string(*it)));
            }
          }
        }
        else
        {
          hash_adapter.add(static_cast<uint32_t>(0));
          unsigned long hash_val = hash_adapter.finalize();
          if(global_dictionary.find(hash_val) == global_dictionary.end())
          {
            dictionary.insert(std::make_pair(hash_val, name_ + ":"));
          }
        }
      }

    protected:
      virtual ~FeatureHashCalculatorIdSetFinalImpl() noexcept
      {}

    protected:
      CTRGenerator::IdSet CTRGenerator::CalculateParams::* field_;
      const std::string name_;
    };

    // FeatureHashCalculatorIdSetDelegateImpl
    class FeatureHashCalculatorIdSetDelegateImpl:
      public CTRGenerator::FeatureHashCalculator
    {
    public:
      FeatureHashCalculatorIdSetDelegateImpl(
        FeatureHashCalculator* next_calculator,
        CTRGenerator::IdSet CTRGenerator::CalculateParams::* field,
        const char* name)
        noexcept
        : next_calculator_(ReferenceCounting::add_ref(next_calculator)),
          field_(field),
          name_(name)
      {}

      virtual void
      eval(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::Calculation& index_calculation,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        if(!(calc_params.*field_).empty())
        {
          for(CTRGenerator::IdSet::const_iterator it =
                (calc_params.*field_).begin();
              it != (calc_params.*field_).end(); ++it)
          {
            CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(static_cast<uint32_t>(*it));
            next_calculator_->eval(
              hash_adapter_copy,
              index_calculation,
              calc_params);
          }
        }
        else
        {
          hash_adapter.add(static_cast<uint32_t>(0));
          next_calculator_->eval(
            hash_adapter,
            index_calculation,
            calc_params);
        }
      }

      virtual void
      fill_dictionary(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::FeatureDictionary& dictionary,
        const CTRGenerator::FeatureDictionary& global_dictionary,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        if(!(calc_params.*field_).empty())
        {
          for(CTRGenerator::IdSet::const_iterator it =
                (calc_params.*field_).begin();
              it != (calc_params.*field_).end(); ++it)
          {
            CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(static_cast<uint32_t>(*it));

            CTRGenerator::FeatureDictionary local_dictionary;

            next_calculator_->fill_dictionary(
              hash_adapter,
              local_dictionary,
              global_dictionary,
              calc_params);

            if(!local_dictionary.empty())
            {
              for(auto dict_it = local_dictionary.begin();
                dict_it != local_dictionary.end(); ++dict_it)
              {
                dictionary.insert(
                  std::make_pair(
                    dict_it->first,
                    name_ + ":" + value_to_string(*it) + "," + dict_it->second));
              }
            }
          }
        }
        else
        {
          hash_adapter.add(static_cast<uint32_t>(0));

          CTRGenerator::FeatureDictionary local_dictionary;

          next_calculator_->fill_dictionary(
            hash_adapter,
            local_dictionary,
            global_dictionary,
            calc_params);

          if(!local_dictionary.empty())
          {
            for(auto dict_it = local_dictionary.begin();
              dict_it != local_dictionary.end(); ++dict_it)
            {
              dictionary.insert(
                std::make_pair(
                  dict_it->first,
                  name_ + ":," + dict_it->second)); // empty value
            }
          }
        }
      }

    protected:
      virtual ~FeatureHashCalculatorIdSetDelegateImpl() noexcept
      {}

    protected:
      CTRGenerator::FeatureHashCalculator_var next_calculator_;
      CTRGenerator::IdSet CTRGenerator::CalculateParams::* field_;
      const std::string name_;
    };

    // FeatureWeightCalculatorAfterIntFinalImpl
    template<typename IntType>
    class FeatureHashCalculatorAfterIntFinalImpl:
      public CTRGenerator::FeatureHashCalculator
    {
    public:
      FeatureHashCalculatorAfterIntFinalImpl(
        IntType CTRGenerator::CalculateParams::* field,
        const char* name)
        noexcept
        : field_(field),
          name_(name)
      {}

      virtual void
      eval(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::Calculation& index_calculation,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        for(uint32_t hour_i = 0; hour_i < calc_params.*field_; ++hour_i)
        {
          // need local hasher
          CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
          hash_adapter_copy.add(hour_i);
          unsigned long hash_val = hash_adapter_copy.finalize();
          index_calculation.hashes.insert(
            index_calculation.hashes.end(),
            std::make_pair(hash_val, 1));
        }
      }

      virtual void
      fill_dictionary(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::FeatureDictionary& dictionary,
        const CTRGenerator::FeatureDictionary& global_dictionary,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        for(uint32_t hour_i = 0; hour_i < calc_params.*field_; ++hour_i)
        {
          // need local hasher
          CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
          hash_adapter_copy.add(hour_i);
          unsigned long hash_val = hash_adapter_copy.finalize();
          if(global_dictionary.find(hash_val) == global_dictionary.end())
          {
            dictionary.insert(
              std::make_pair(hash_val, name_ + ":" + value_to_string(hour_i)));
          }
        }
      }

    protected:
      virtual ~FeatureHashCalculatorAfterIntFinalImpl() noexcept
      {}

    protected:
      IntType CTRGenerator::CalculateParams::* field_;
      const std::string name_;
    };

    // FeatureHashCalculatorAfterIntDelegateImpl
    template<typename IntType>
    class FeatureHashCalculatorAfterIntDelegateImpl:
      public CTRGenerator::FeatureHashCalculator
    {
    public:
      FeatureHashCalculatorAfterIntDelegateImpl(
        FeatureHashCalculator* next_calculator,
        IntType CTRGenerator::CalculateParams::* field,
        const char* name)
        noexcept
        : next_calculator_(ReferenceCounting::add_ref(next_calculator)),
          field_(field),
          name_(name)
      {}

      virtual void
      eval(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::Calculation& index_calculation,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        for(uint32_t hour_i = 0; hour_i < calc_params.*field_; ++hour_i)
        {
          CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
          hash_adapter_copy.add(hour_i);
          next_calculator_->eval(
            hash_adapter_copy,
            index_calculation,
            calc_params);
        }
      }

      virtual void
      fill_dictionary(
        CTR::Murmur32v3Adapter& hash_adapter,
        CTRGenerator::FeatureDictionary& dictionary,
        const CTRGenerator::FeatureDictionary& global_dictionary,
        const CTRGenerator::CalculateParams& calc_params)
        noexcept
      {
        for(uint32_t hour_i = 0; hour_i < calc_params.*field_; ++hour_i)
        {
          CTR::Murmur32v3Adapter hash_adapter_copy(hash_adapter);
          hash_adapter_copy.add(hour_i);

          CTRGenerator::FeatureDictionary local_dictionary;

          next_calculator_->fill_dictionary(
            hash_adapter,
            local_dictionary,
            global_dictionary,
            calc_params);

          if(!local_dictionary.empty())
          {
            for(auto dict_it = local_dictionary.begin();
              dict_it != local_dictionary.end(); ++dict_it)
            {
              dictionary.insert(
                std::make_pair(
                  dict_it->first,
                  name_ + ":" + value_to_string(hour_i) + "," + dict_it->second));
            }
          }
        }
      }

    protected:
      virtual ~FeatureHashCalculatorAfterIntDelegateImpl() noexcept
      {}

    protected:
      CTRGenerator::FeatureHashCalculator_var next_calculator_;
      IntType CTRGenerator::CalculateParams::* field_;
      const std::string name_;
    };
  }

  // CTRGenerator::CalculateParams
  CTRGenerator::CalculateParams::CalculateParams()
    : publisher_id(0),
      site_id(0),
      tag_id(0),
      referer_hash(0),
      advertiser_id(0),
      campaign_id(0),
      ccg_id(0),
      hour(0),
      wd(0),
      isp_id(0),
      colo_id(0),
      device_id(0),
      creative_id(0),
      cc_id(0),
      campaign_freq(0),
      campaign_freq_log(0),
      tag_visibility(-1),
      tag_predicted_viewability(-1)
  {}

  // CTRGenerator
  CTRGenerator::CTRGenerator(const FeatureList& features, bool xgb_model)
    : push_hour_(false),
      push_week_day_(false),
      push_campaign_freq_(false),
      push_campaign_freq_log_(false)
  {
    // create calculator
    for(FeatureList::const_iterator fit = features.begin();
        fit != features.end(); ++fit)
    {
      if(!fit->basic_features.empty())
      {
        if(xgb_model && fit->basic_features.size() == 1)
        {
          auto feature_id = *fit->basic_features.begin();

          if(feature_id == CTR::BF_HOUR)
          {
            push_hour_ = true;
          }

          if(feature_id == CTR::BF_WEEK_DAY)
          {
            push_week_day_ = true;
          }

          if(feature_id == CTR::BF_CAMPAIGN_FREQ_ID)
          {
            push_campaign_freq_ = true;
          }

          if(feature_id == CTR::BF_CAMPAIGN_FREQ_LOG_ID)
          {
            push_campaign_freq_log_ = true;
          }
        }

        FeatureHolder feature_holder;
        feature_holder.hash_seed = eval_feature_hash_seed_(*fit);

        FeatureHashCalculator_var feature_hash_calculator =
          create_final_feature_hash_calculator_(
            *(fit->basic_features.rbegin()));

        for(CTR::BasicFeatureSet::const_reverse_iterator basic_fit =
              ++(fit->basic_features.rbegin());
            basic_fit != fit->basic_features.rend(); ++basic_fit)
        {
          feature_hash_calculator = create_delegate_feature_hash_calculator_(
            *basic_fit,
            feature_hash_calculator);
        }

        feature_holder.feature_calculator = feature_hash_calculator;

        feature_holders_.push_back(feature_holder);
      }
    }
  }

  void
  CTRGenerator::calculate(
    Calculation& calculation,
    const CalculateParams& calc_params)
  {
    for(FeatureHolderList::const_iterator feature_holder_it =
          feature_holders_.begin();
        feature_holder_it != feature_holders_.end();
        ++feature_holder_it)
    {
      CTR::Murmur32v3Adapter hash_adapter(feature_holder_it->hash_seed);

      feature_holder_it->feature_calculator->eval(
        hash_adapter,
        calculation,
        calc_params);
    }

    if(push_hour_)
    {
      calculation.hashes.insert(std::make_pair(CTR::BF_HOUR, calc_params.hour));
    }

    if(push_week_day_)
    {
      calculation.hashes.insert(std::make_pair(CTR::BF_WEEK_DAY, calc_params.wd));
    }

    if(push_campaign_freq_)
    {
      calculation.hashes.insert(std::make_pair(CTR::BF_CAMPAIGN_FREQ_ID, calc_params.campaign_freq));
    }

    if(push_campaign_freq_log_)
    {
      calculation.hashes.insert(std::make_pair(CTR::BF_CAMPAIGN_FREQ_LOG_ID, calc_params.campaign_freq_log));
    }
  }

  void
  CTRGenerator::fill_dictionary(
    FeatureDictionary& global_dictionary,
    const CalculateParams& calc_params)
  {
    for(FeatureHolderList::const_iterator feature_holder_it =
          feature_holders_.begin();
        feature_holder_it != feature_holders_.end();
        ++feature_holder_it)
    {
      CTR::Murmur32v3Adapter hash_adapter(feature_holder_it->hash_seed);

      feature_holder_it->feature_calculator->fill_dictionary(
        hash_adapter,
        global_dictionary,
        global_dictionary,
        calc_params);
    }
  }

  CTRGenerator::FeatureHashCalculator_var
  CTRGenerator::create_final_feature_hash_calculator_(
    CTR::BasicFeature basic_feature)
  {
    switch(basic_feature)
    {
    case CTR::BF_PUBLISHER_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_publisher_id_, get_value_publisher_id_>("publisher");
    case CTR::BF_SITE_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_site_id_, get_value_site_id_>("site");
    case CTR::BF_TAG_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_tag_id_, get_value_tag_id_>("tag");
    case CTR::BF_ETAG_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_etag_id_, get_value_etag_id_>("etag");
    case CTR::BF_DOMAIN:
      return new FeatureHashCalculatorFinalImpl<add_hash_domain_, get_value_domain_>("domain");
    case CTR::BF_URL:
      return new FeatureHashCalculatorFinalImpl<add_hash_url_, get_value_url_>("url");
    case CTR::BF_ADVERTISER_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_advertiser_id_, get_value_advertiser_id_>("advertiser");
    case CTR::BF_CAMPAIGN_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_campaign_id_, get_value_campaign_id_>("campaign");
    case CTR::BF_CCG_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_ccg_id_, get_value_ccg_id_>("ccg");
    case CTR::BF_SIZE_TYPE_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_size_type_id_, get_value_size_type_id_>("sizetype");
    case CTR::BF_SIZE_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_size_id_, get_value_size_id_>("size");
    case CTR::BF_HOUR:
      return new FeatureHashCalculatorFinalImpl<add_hash_hour_, get_value_hour_>("hour");
    case CTR::BF_WEEK_DAY:
      return new FeatureHashCalculatorFinalImpl<add_hash_week_day_, get_value_week_day_>("wd");
    case CTR::BF_ISP_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_isp_id_, get_value_isp_id_>("isp");
    case CTR::BF_COLO_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_colo_id_, get_value_colo_id_>("colo");
    case CTR::BF_DEVICE_CHANNEL_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_device_channel_id_, get_value_device_channel_id_>("device");
    case CTR::BF_AFTER_HOUR:
      return new FeatureHashCalculatorAfterIntFinalImpl<unsigned char>(&CalculateParams::hour, "afterhour");

    case CTR::BF_CREATIVE_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_creative_id_, get_value_creative_id_>("creative");
    case CTR::BF_CC_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_cc_id_, get_value_cc_id_>("ccid");
    case CTR::BF_CAMPAIGN_FREQ_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_campaign_freq_, get_value_campaign_freq_>("campaignfreq");
    case CTR::BF_CAMPAIGN_FREQ_LOG_ID:
      return new FeatureHashCalculatorFinalImpl<add_hash_campaign_freq_log_, get_value_campaign_freq_log_>("campaignfreqlog");
    case CTR::BF_VISIBILITY:
      return new FeatureHashCalculatorFinalImpl<add_hash_tag_visibility_, get_value_tag_visibility_>("tagvisibility");
    case CTR::BF_PREDICTED_VIEWABILITY:
      return new FeatureHashCalculatorFinalImpl<add_hash_tag_predicted_viewability_, get_value_tag_predicted_viewability_>("tagviewability");
    case CTR::BF_HISTORY_CHANNELS:
      return new FeatureHashCalculatorIdSetFinalImpl(
        &CalculateParams::channels, "channel");
    case CTR::BF_GEO_CHANNELS:
      return new FeatureHashCalculatorIdSetFinalImpl(
        &CalculateParams::geo_channels, "geochannel");
    case CTR::BF_CONTENT_CATEGORIES:
      return new FeatureHashCalculatorIdSetFinalImpl(
        &CalculateParams::content_categories, "contcat");
    case CTR::BF_VISUAL_CATEGORIES:
      return new FeatureHashCalculatorIdSetFinalImpl(
        &CalculateParams::visual_categories, "viscat");
    };

    Stream::Error ostr;
    ostr << "not supported feature #" << static_cast<unsigned long>(basic_feature);
    throw InvalidConfig(ostr);
  }

  CTRGenerator::FeatureHashCalculator_var
  CTRGenerator::create_delegate_feature_hash_calculator_(
    CTR::BasicFeature basic_feature,
    FeatureHashCalculator* next_calculator)
  {
    switch(basic_feature)
    {
    case CTR::BF_PUBLISHER_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_publisher_id_, get_value_publisher_id_>(
        next_calculator,
        "publisher");
    case CTR::BF_SITE_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_site_id_, get_value_site_id_>(
        next_calculator,
        "site");
    case CTR::BF_TAG_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_tag_id_, get_value_tag_id_>(
        next_calculator,
        "tag");
    case CTR::BF_ETAG_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_etag_id_, get_value_etag_id_>(
        next_calculator,
        "etag");
    case CTR::BF_DOMAIN:
      return new FeatureHashCalculatorDelegateImpl<add_hash_domain_, get_value_domain_>(
        next_calculator, "domain");
    case CTR::BF_URL:
      return new FeatureHashCalculatorDelegateImpl<add_hash_url_, get_value_url_>(
        next_calculator, "url");
    case CTR::BF_ADVERTISER_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_advertiser_id_, get_value_advertiser_id_>(
        next_calculator, "advertiser");
    case CTR::BF_CAMPAIGN_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_campaign_id_, get_value_campaign_id_>(
        next_calculator, "campaign");
    case CTR::BF_CCG_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_ccg_id_, get_value_ccg_id_>(
        next_calculator, "ccg");
    case CTR::BF_SIZE_TYPE_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_size_type_id_, get_value_size_type_id_>(
        next_calculator, "sizetype");
    case CTR::BF_SIZE_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_size_id_, get_value_size_id_>(
        next_calculator, "size");
    case CTR::BF_HOUR:
      return new FeatureHashCalculatorDelegateImpl<add_hash_hour_, get_value_hour_>(
        next_calculator, "hour");
    case CTR::BF_WEEK_DAY:
      return new FeatureHashCalculatorDelegateImpl<add_hash_week_day_, get_value_week_day_>(
        next_calculator, "wd");
    case CTR::BF_ISP_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_isp_id_, get_value_isp_id_>(
        next_calculator, "isp");
    case CTR::BF_COLO_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_colo_id_, get_value_colo_id_>(
        next_calculator, "colo");
    case CTR::BF_DEVICE_CHANNEL_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_device_channel_id_, get_value_device_channel_id_>(
        next_calculator, "device");
    case CTR::BF_AFTER_HOUR:
      return new FeatureHashCalculatorAfterIntDelegateImpl<unsigned char>(
        next_calculator, &CalculateParams::hour, "afterhour");

    case CTR::BF_CREATIVE_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_creative_id_, get_value_creative_id_>(
        next_calculator, "creative");
    case CTR::BF_CC_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_cc_id_, get_value_cc_id_>(
        next_calculator, "ccid");
    case CTR::BF_CAMPAIGN_FREQ_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_campaign_freq_, get_value_campaign_freq_>(
        next_calculator, "campaignfreq");
    case CTR::BF_CAMPAIGN_FREQ_LOG_ID:
      return new FeatureHashCalculatorDelegateImpl<add_hash_campaign_freq_log_, get_value_campaign_freq_log_>(
        next_calculator, "campaignfreqlog");
    case CTR::BF_VISIBILITY:
      return new FeatureHashCalculatorDelegateImpl<add_hash_tag_visibility_, get_value_tag_visibility_>(
        next_calculator, "tagvisibility");
    case CTR::BF_PREDICTED_VIEWABILITY:
      return new FeatureHashCalculatorDelegateImpl<add_hash_tag_predicted_viewability_, get_value_tag_predicted_viewability_>(
        next_calculator, "tagviewability");
    case CTR::BF_HISTORY_CHANNELS:
      return new FeatureHashCalculatorIdSetDelegateImpl(
        next_calculator, &CalculateParams::channels, "channel");
    case CTR::BF_GEO_CHANNELS:
      return new FeatureHashCalculatorIdSetDelegateImpl(
        next_calculator, &CalculateParams::geo_channels, "geochannel");
    case CTR::BF_CONTENT_CATEGORIES:
      return new FeatureHashCalculatorIdSetDelegateImpl(
        next_calculator, &CalculateParams::content_categories, "contcat");
    case CTR::BF_VISUAL_CATEGORIES:
      return new FeatureHashCalculatorIdSetDelegateImpl(
        next_calculator, &CalculateParams::visual_categories, "viscat");
    };

    Stream::Error ostr;
    ostr << "not supported feature #" << static_cast<unsigned long>(basic_feature);
    throw InvalidConfig(ostr);
  }

  std::size_t
  CTRGenerator::eval_feature_hash_seed_(
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
}
}
