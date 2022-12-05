
#include "CreativeAdmin.hpp"

namespace AutoTest
{
  template<>  
  BaseAdminCmd<CreativeAdmin, 7>::names_type const  
  BaseAdminCmd<CreativeAdmin, 7>::field_names = {  
    "ccid",    
    "campaign_id",    
    "creative_format",    
    "sizes",    
    "categories",    
    "weight",    
    "status"    
  };  
  bool  
  CreativeAdmin::check(const Expected& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected.values_):  
      !check_(expected.values_);  
  }  
  bool  
  CreativeAdmin::check(const std::string& expected, bool exist)  
    /*throw(eh::Exception)*/  
  {  
    return exist? check_(expected):  
      !check_(expected);  
  }  
}
