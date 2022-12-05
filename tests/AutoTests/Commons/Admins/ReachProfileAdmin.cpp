
#include "ReachProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<ReachProfileAdmin, 6>::names_type const  
  BaseAdminCmd<ReachProfileAdmin, 6>::field_names = {  
    "last_request_day",    
    "total_appear_campaigns",    
    "total_appear_ccgs",    
    "total_appear_creatives",    
    "total_appear_advs",    
    "total_appear_display_advs"    
  };  
  bool  
  ReachProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  ReachProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
