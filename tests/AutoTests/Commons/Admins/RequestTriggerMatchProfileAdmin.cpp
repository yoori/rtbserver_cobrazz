
#include "RequestTriggerMatchProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<RequestTriggerMatchProfileAdmin, 6>::names_type const  
  BaseAdminCmd<RequestTriggerMatchProfileAdmin, 6>::field_names = {  
    "request_id",    
    "time",    
    "page_matches",    
    "search_matches",    
    "url_matches",    
    "click_done"    
  };  
  bool  
  RequestTriggerMatchProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  RequestTriggerMatchProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
