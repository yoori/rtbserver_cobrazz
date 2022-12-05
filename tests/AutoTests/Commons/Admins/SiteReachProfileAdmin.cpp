
#include "SiteReachProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<SiteReachProfileAdmin, 3>::names_type const  
  BaseAdminCmd<SiteReachProfileAdmin, 3>::field_names = {  
    "last_request_day",    
    "daily_appear_lists",    
    "monthly_appear_lists"    
  };  
  bool  
  SiteReachProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  SiteReachProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
