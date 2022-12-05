
#include "CampaignAdminStatCcg.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<CampaignAdminStatCcg, 11>::names_type const  
  BaseAdminCmd<CampaignAdminStatCcg, 11>::field_names = {  
    "ccg_id",    
    "impressions",    
    "clicks",    
    "actions",    
    "amount",    
    "comm_amount",    
    "daily_amount",    
    "daily_comm_amount",    
    "creatives",    
    "publishers",    
    "tags"    
  };  
  bool  
  CampaignAdminStatCcg::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  CampaignAdminStatCcg::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
