
#include "SimpleChannelAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<SimpleChannelAdmin, 7>::names_type const  
  BaseAdminCmd<SimpleChannelAdmin, 7>::field_names = {  
    "channel_id",    
    "country_code",    
    "status",    
    "behav_param_list_id",    
    "categories",    
    "threshold",    
    "timestamp"    
  };  
  bool  
  SimpleChannelAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  SimpleChannelAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
