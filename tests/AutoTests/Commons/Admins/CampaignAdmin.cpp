
#include "CampaignAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<CampaignAdmin, 23>::names_type const  
  BaseAdminCmd<CampaignAdmin, 23>::field_names = {  
    "id",    
    "account_id",    
    "account_currency_id",    
    "campaign_group_name",    
    "channels",    
    "ecpm",    
    "delivery_threshold",    
    "status",    
    "eval_status",    
    "timestamp",    
    "exclude_pub_accounts",    
    "exclude_tags",    
    "date_start",    
    "date_end",    
    "budget",    
    "daily_budget",    
    "cmp_date_start",    
    "cmp_date_end",    
    "max_pub_share",    
    "channel",    
    "flags",    
    "country",    
    "sites"    
  };  
  bool  
  CampaignAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  CampaignAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
  const char*  
  CampaignAdmin::modificator_names_[1] = {  
    "--expand"    
  };  
}
