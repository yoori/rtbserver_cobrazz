
#include "SiteAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<SiteAdmin, 7>::names_type const  
  BaseAdminCmd<SiteAdmin, 7>::field_names = {  
    "site_id",    
    "status",    
    "approved_creative_categories",    
    "rejected_creative_categories",    
    "approved_creatives",    
    "account_id",    
    "noads_timeout"    
  };  
  bool  
  SiteAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  SiteAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
