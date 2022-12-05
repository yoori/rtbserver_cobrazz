
#include "AccountAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<AccountAdmin, 20>::names_type const  
  BaseAdminCmd<AccountAdmin, 20>::field_names = {  
    "account_id",    
    "agency_account_id",    
    "internal_account_id",    
    "status",    
    "eval_status",    
    "flags",    
    "at_flags",    
    "text_adserving",    
    "currency_id",    
    "country",    
    "commision",    
    "budget",    
    "paid_amount",    
    "time_offset",    
    "walled_garden_accounts",    
    "auction_rate",    
    "use_pub_pixels",    
    "pub_pixel_optin",    
    "pub_pixel_optout",    
    "timestamp"    
  };  
  bool  
  AccountAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  AccountAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
