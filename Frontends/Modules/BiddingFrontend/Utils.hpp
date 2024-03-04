#ifndef BIDDINGFRONTEND_UTILS_HPP_
#define BIDDINGFRONTEND_UTILS_HPP_

#include <string>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

namespace AdServer
{
namespace Bidding
{
  void add_token(
    AdServer::CampaignSvcs::CampaignManager::TokenSeq& tokens,
    const char* token_name,
    const std::string& token_value);
}
}

#endif /*BIDDINGFRONTEND_UTILS_HPP_*/
