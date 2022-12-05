#include <HTTP/UrlAddress.hpp>

#include "CampaignConfig.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  namespace AdInstances
  {
    const Tag::TagPricing Tag::TagPricing::DEFAULT;
    //const RevenueDecimal Campaign::CTR_MULTIPLIER(false, 100000, 0);

    inline
    const Tag::TagPricing*
    Tag::select_tag_pricing_(
      const char* country_code,
      CCGType ccg_type,
      CCGRateType ccg_rate_type) const noexcept
    {
      TagPricings::const_iterator tp_it;

      if((tp_it = tag_pricings.find(
        Tag::TagPricingKey(country_code, ccg_type, ccg_rate_type))) !=
        tag_pricings.end())
      {
        return &(tp_it->second);
      }

      if((tp_it = tag_pricings.find(
        Tag::TagPricingKey(country_code, ccg_type, CR_ALL))) !=
        tag_pricings.end())
      {
        return &(tp_it->second);
      }

      if((tp_it = tag_pricings.find(
        Tag::TagPricingKey(country_code, CT_ALL, ccg_rate_type))) !=
        tag_pricings.end())
      {
        return &(tp_it->second);
      }

      if((tp_it = tag_pricings.find(
        Tag::TagPricingKey(country_code, CT_ALL, CR_ALL))) !=
        tag_pricings.end())
      {
        return &(tp_it->second);
      }

      return 0;
    }

    const Tag::TagPricing*
    Tag::select_tag_pricing(
      const char* country_code,
      CCGType ccg_type,
      CCGRateType ccg_rate_type) const noexcept
    {
      // reduce ccg rate for tag pricing selection (MAXBID => CPM)
      if(ccg_rate_type == CR_MAXBID)
      {
        ccg_rate_type = CR_CPM;
      }

      const Tag::TagPricing* res = 0;

      if((res = select_tag_pricing_(country_code, ccg_type, ccg_rate_type)) != 0)
      {
        return res;
      }

      if((res = select_tag_pricing_("", ccg_type, ccg_rate_type)) != 0)
      {
        return res;
      }

      return &Tag::TagPricing::DEFAULT;
    }

    const Tag::TagPricing*
    Tag::select_country_tag_pricing(
      const char* country_code) const noexcept
    {
      CountryTagPricingMap::const_iterator tp_it;

      if((tp_it = country_tag_pricings.find(country_code)) !=
        country_tag_pricings.end())
      {
        return &(tp_it->second);
      }

      if((tp_it = country_tag_pricings.find("")) !=
        country_tag_pricings.end())
      {
        return &(tp_it->second);
      }

      return &Tag::TagPricing::DEFAULT;
    }

    const Tag::TagPricing*
    Tag::select_no_impression_tag_pricing(
      const char* country_code) const noexcept
    {
      if(no_imp_tag_pricings.empty())
      {
        return 0;
      }

      CountryTagPricingPtrMap::const_iterator tp_it =
        no_imp_tag_pricings.find(country_code);
      if(tp_it != no_imp_tag_pricings.end())
      {
        return tp_it->second;
      }

      if(no_imp_tag_pricings.begin()->first.empty())
      {
        return no_imp_tag_pricings.begin()->second;
      }

      return 0;
    }

    Creative::Creative(
      const Campaign* campaign_val,
      unsigned long ccid_val,
      unsigned long creative_id_val,
      unsigned long fc_id_val,
      unsigned long weight_val,
      const char* creative_format_val,
      const char* version_id_val,
      const OptionValue& click_url_val,
      const char* click_url_domain_val,
      const char* short_click_url_val,
      const CategorySet& categories_val)
      /*throw(eh::Exception)*/
      : campaign(campaign_val),
        ccid(ccid_val),
        creative_id(creative_id_val),
        fc_id(fc_id_val),
        weight(weight_val),
        creative_format(creative_format_val ? creative_format_val : ""),
        version_id(version_id_val),
        status('A'),
        click_url_domain(click_url_domain_val),
        short_click_url(short_click_url_val),
        click_url(click_url_val),
        categories(categories_val),
        defined_content_category(false),
        video_duration(0),
        https_safe_flag(false)
    {}
  }
}
}

