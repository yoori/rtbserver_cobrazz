
#include "UserTriggerMatchProfileAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<UserTriggerMatchProfileAdmin, 5>::names_type const  
  BaseAdminCmd<UserTriggerMatchProfileAdmin, 5>::field_names = {  
    "user_id",    
    "page_matches",    
    "search_matches",    
    "url_matches",    
    "requests"    
  };  
  bool  
  UserTriggerMatchProfileAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  UserTriggerMatchProfileAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
