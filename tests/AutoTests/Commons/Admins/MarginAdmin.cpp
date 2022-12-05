
#include "MarginAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<MarginAdmin, 9>::names_type const  
  BaseAdminCmd<MarginAdmin, 9>::field_names = {  
    "margin_rule_id",    
    "account_id",    
    "type",    
    "sort_order",    
    "fixed_margin",    
    "relative_margin",    
    "isp_accounts",    
    "publisher_accounts",    
    "advertiser_accounts"    
  };  
  bool  
  MarginAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  MarginAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
