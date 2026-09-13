#ifndef CAMPAIGNSVCS_CAMPAIGNMANAGER_CTRCONFIGSELECTOR_HPP
#define CAMPAIGNSVCS_CAMPAIGNMANAGER_CTRCONFIGSELECTOR_HPP

#include <string>

#include <Generics/Time.hpp>
#include <String/SubString.hpp>

namespace AdServer::CampaignSvcs::CTR
{
  Generics::Time
  select_latest_config(std::string& config_root, const String::SubString& check_root);
}

#endif
