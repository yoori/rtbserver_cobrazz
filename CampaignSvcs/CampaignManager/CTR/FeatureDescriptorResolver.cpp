#include <Generics/BitAlgs.hpp>

#include "FeatureDescriptorResolver.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  namespace
  {
    // trivial feature hash functions
#   define DEFINE_REQUEST_FEATURE_HASH_FUN(NAME) \
    void NAME(Murmur32v3Adapter& hash, \
      const CampaignSelectParams& request_params, \
      const Tag::Size*, \
      const Creative*)

#   define DEFINE_AUCTION_FEATURE_HASH_FUN(NAME) \
    void NAME(Murmur32v3Adapter& hash, \
      const CampaignSelectParams&, \
      const Tag::Size* tag_size, \
      const Creative*)

#   define DEFINE_CANDIDATE_FEATURE_HASH_FUN(NAME) \
    void NAME(Murmur32v3Adapter& hash, \
      const CampaignSelectParams&, \
      const Tag::Size*, \
      const Creative* creative)

#   define DEFINE_CANDIDATE_FEATURE_HASH_WITH_RP_FUN(NAME) \
    void NAME(Murmur32v3Adapter& hash, \
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

    struct AfterHourFeatureCalculatorCreator: public FeatureCalculatorCreator
    {
    public:
      virtual FeatureCalculator_var
      create_final()
      {
        return new AfterHourFeatureCalculatorFinalImpl();
      }

      virtual FeatureCalculator_var
      create_delegate(
        FeatureCalculator* next_calculator)
      {
        return new AfterHourFeatureCalculatorDelegateImpl(next_calculator);
      }

    protected:
      class AfterHourFeatureCalculatorFinalImpl:
        public FeatureCalculator,
        public FeatureCalculatorFinalImplHelper
      {
      public:
        AfterHourFeatureCalculatorFinalImpl()
          noexcept
          : FeatureCalculatorFinalImplHelper()
        {}

        virtual void
        eval_hashes(
          HashArray& result_hashes,
          Murmur32v3Adapter& hash_adapter,
          const HashMap* hash_mapping,
          const CampaignSelectParams& request_params,
          const Tag::Size*,
          const Creative*)
          noexcept
        {
          for(uint32_t time_hour_i = 0; time_hour_i < request_params.time_hour; ++time_hour_i)
          {
            // need local hasher
            Murmur32v3Adapter hash_adapter_copy(hash_adapter);
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
        public FeatureCalculator
      {
      public:
        AfterHourFeatureCalculatorDelegateImpl(
          FeatureCalculator* next_calculator)
          noexcept
          : next_calculator_(ReferenceCounting::add_ref(next_calculator))
        {}

        virtual void
        eval_hashes(
          HashArray& result_hashes,
          Murmur32v3Adapter& hash_adapter,
          const HashMap* hash_mapping,
          const CampaignSelectParams& request_params,
          const Tag::Size* tag_size,
          const Creative* creative)
          noexcept
        {
          for(uint32_t time_hour_i = 0; time_hour_i < request_params.time_hour; ++time_hour_i)
          {
            Murmur32v3Adapter hash_adapter_copy(hash_adapter);
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
        FeatureCalculator_var next_calculator_;
      };
    };
  }

  FeatureDescriptorResolver_::FeatureDescriptorResolver_()
  {
#   define F2C(NAME) new TrivialFeatureCalculatorCreator<NAME>()

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
        new ArrayParamFeatureCalculatorCreator<ChannelIdHashSet>(
          &CampaignSelectParams::channels)
      },
      {
        BF_GEO_CHANNELS,
        "geoch",
        new ArrayParamFeatureCalculatorCreator<ChannelIdSet>(
          &CampaignSelectParams::geo_channels)
      },
      {
        BF_CONTENT_CATEGORIES,
        "crcatcont",
        new ArrayCreativeFeatureCalculatorCreator<
          Creative::CategoryIdArray>(
            &Creative::content_categories)
      },
      {
        BF_VISUAL_CATEGORIES,
        "crcatvis",
        new ArrayCreativeFeatureCalculatorCreator<
          Creative::CategoryIdArray>(
            &Creative::visual_categories)
      },
    };

#   undef F2C

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
  FeatureDescriptorResolver_::resolve(BasicFeature basic_feature) const
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
  FeatureDescriptorResolver_::resolve_by_name(const String::SubString& feature_name) const
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

  typedef Generics::Singleton<FeatureDescriptorResolver_>
    FeatureDescriptorResolver;
}
