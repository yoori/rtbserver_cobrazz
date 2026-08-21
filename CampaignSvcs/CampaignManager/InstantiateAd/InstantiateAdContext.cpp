#include "InstantiateAdContext.hpp"

#include "../CreativeInstantiator.hpp"

#include <cassert>
#include <utility>

namespace AdServer::CampaignSvcs::InstantiateAd
{
  void
  InstantiateAdContext::CreativeArgsData::init_click_urls()
  {
    if (click_urls_initialized)
    {
      return;
    }

    click_urls_initialized = true;
    if (click_url_initializer)
    {
      click_url_initializer(*this);
    }
  }

  AccountIdList&
  InstantiateAdContext::consider_pub_pixel_accounts()
  {
    if (!consider_pub_pixel_accounts_cache)
    {
      if ((inst_params && inst_params->generate_pubpixel_accounts) ||
        force_generate_pubpixel_accounts)
      {
        assert(creative_instantiator);
        assert(exclude_pubpixel_accounts);
        assert(request_params);

        AccountIdList accounts;
        creative_instantiator->get_inst_optin_pub_pixel_account_ids_(
          accounts,
          campaign_config,
          tag,
          *request_params,
          *exclude_pubpixel_accounts);
        consider_pub_pixel_accounts_cache = std::move(accounts);
      }
      else
      {
        consider_pub_pixel_accounts_cache =
          inst_params ? inst_params->pubpixel_accounts : AccountIdList();
      }
    }

    return *consider_pub_pixel_accounts_cache;
  }

  bool
  InstantiateAdContext::pub_pixels_optout()
  {
    if (!pub_pixels_optout_cache)
    {
      if (creative_instantiator && campaign_config && request_params)
      {
        const char* const country = !request_params->location.empty() ?
          request_params->location[0].country.c_str() : "";
        pub_pixels_optout_cache =
          creative_instantiator->find_pub_pixel_accounts_(
            campaign_config,
            country,
            US_OPTOUT) != campaign_config->pub_pixel_accounts.end();
      }
      else
      {
        pub_pixels_optout_cache = false;
      }
    }

    return *pub_pixels_optout_cache;
  }
}
